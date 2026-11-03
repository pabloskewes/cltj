#ifndef CLTJ_COMPACT_METATRIE_HASH_H
#define CLTJ_COMPACT_METATRIE_HASH_H

#include <cds/succ_support_v.hpp>
#include <cltj_config.hpp>
#include <iostream>
#include <map>
#include <sstream>
#include <sdsl/select_support_mcl.hpp>
#include <sdsl/vectors.hpp>
#include <string>
#include <utility>
#include <vector>
#include <hashing/key_dumper.hpp>
#include <hashing/mphf_bdz.hpp>
#include <hashing/mphf_build_tracer.hpp>
#include <hashing/storage/glgh.hpp>
#include <metatrie/overlay/sep_bitmap.hpp>
#include <metatrie/overlay/step_table.hpp>
#include <metatrie/overlay/step_table_rb.hpp>
#include <sdsl/rank_support_v.hpp>
#include <stdexcept>
#include <util/instrument.hpp>

namespace cltj {

#ifdef CLTJ_OVERLAY_STEP_TABLE
using default_overlay = overlay::StepTableOverlay;
#else
using default_overlay = overlay::SepBitmapOverlay;
#endif

template <class Overlay = default_overlay>
class compact_metatrie_hash {
  public:
    typedef uint64_t size_type;
    typedef uint32_t value_type;
    typedef Overlay overlay_type;

    // Every serialized trie starts with this tag; its low byte is the format version:
    //   (no tag)  pre-overlay format: the stream starts with the bit size of B, which
    //             never reaches the high bytes of the tag, so load() rejects it
    //   2         scheme A overlay (SepBitmapOverlay)
    //   3         scheme C overlay (StepTableOverlay)
    //   4         cltj_index_metatrie_rb (X-CLTJ with root bitmap), one tag per index
    //   5         scheme C overlay with the root block out of L_plain (StepTableRbOverlay)
    static constexpr uint64_t format_magic = 0x48434C544A000000ULL;
    static constexpr uint64_t format_tag = format_magic | Overlay::format_version;

    // The children of the root of a full trie live in a root_bitmap: its block is not in L_plain
    // and the root must not be hashed.
    static constexpr bool drops_root_labels = Overlay::drops_root_labels;

  private:
    using mphf_type = hashing::MPHF<hashing::GlGhStorage, hashing::policies::QuotientKey>;

    sdsl::bit_vector m_bv;  // LOUDS bitvector (B)
    sdsl::int_vector<> m_seq_plain;  // L_plain: L without the blocks of hashed nodes
    // sdsl::rank_support_v<1> m_rank1;
    cds::succ_support_v<0> m_succ0;
    sdsl::select_support_mcl<0> m_select0;

    size_type m_root_degree;

    // Hash overlay
    // TODO: Empty until Batch 2 builds MPHFs
    std::vector<mphf_type> m_mphfs;
    Overlay m_overlay;
    uint32_t m_threshold = 0;

    // Build scratch, in the layout before reorder_louds_by_mphf(), which releases both:
    // the hashed nodes, and L with its trailing mock label.
    sdsl::bit_vector m_build_has_hash;
    sdsl::int_vector<> m_build_seq;

    // Copy is disabled: std::vector<mphf_type> is not copyable (MPHF copy is deleted by design).
    // Might change in the future if we find a good reason to copy it.

    /*inline size_type rank0(const size_type i) const {
      return i - m_rank1(i);
  }*/

    /**
     * @brief O(n) count of bits equal to @p bit in @p bv.
     *        Use when a rank support structure is not available or not worth building.
     */
    static size_type count_bits(const sdsl::bit_vector& bv, bool bit) {
        size_type n = 0;
        for (size_type i = 0; i < bv.size(); i++)
            if (static_cast<bool>(bv[i]) == bit)
                n++;
        return n;
    }

    /**
     * @brief Builds L_plain and the overlay of a trie without MPHFs.
     *
     * The layout stays as built: every node is non-hashed and L_plain is L
     * without its mock (and without the root block, with drops_root_labels).
     * Only reached on a full trie: a partial trie always gets a root_perm.
     */
    void build_plain_overlay() {
        const size_type dropped = drops_root_labels ? children(0) : 0;
        const size_type plain_len = m_build_seq.size() - 1 - dropped;
        m_seq_plain = sdsl::int_vector<>(plain_len, 0, m_build_seq.width());
        for (size_type i = 0; i < plain_len; i++)
            m_seq_plain[i] = m_build_seq[dropped + i];
        sdsl::util::bit_compress(m_seq_plain);

        Overlay new_overlay;
        new_overlay.begin(m_bv.size(), plain_len);
        for (size_type v = 0; v + 1 < m_bv.size(); v += children(v)) {
            if (drops_root_labels && v == 0)
                new_overlay.on_dropped_root(children(v));
            else
                new_overlay.on_plain_node(v, children(v));
        }
        m_overlay = std::move(new_overlay);
        m_overlay.finalize(m_bv, m_threshold);
        release_build_scratch();
    }

    void release_build_scratch() {
        sdsl::bit_vector().swap(m_build_has_hash);
        sdsl::int_vector<>().swap(m_build_seq);
    }

  public:
    compact_metatrie_hash() = default;

    compact_metatrie_hash(sdsl::bit_vector& _bv, sdsl::int_vector<>& _seq) {
        m_bv = _bv;
        m_build_seq = _seq;
        sdsl::util::bit_compress(m_build_seq);
        sdsl::util::init_support(m_succ0, &m_bv);
        sdsl::util::init_support(m_select0, &m_bv);
        m_root_degree = m_succ0(1);
    }

    compact_metatrie_hash(const compact_metatrie_hash&) = delete;
    compact_metatrie_hash& operator=(const compact_metatrie_hash&) = delete;

    //! Move constructor
    compact_metatrie_hash(compact_metatrie_hash&& o) { *this = std::move(o); }

    //! Move Operator=
    compact_metatrie_hash& operator=(compact_metatrie_hash&& o) {
        if (this != &o) {
            m_bv = std::move(o.m_bv);
            // m_rank1 = std::move(o.m_rank1);
            m_succ0 = std::move(o.m_succ0);
            // m_rank1.set_vector(&m_bv);
            m_succ0.set_vector(&m_bv);
            m_select0 = std::move(o.m_select0);
            m_select0.set_vector(&m_bv);
            m_root_degree = o.m_root_degree;
            m_seq_plain = std::move(o.m_seq_plain);
            m_mphfs = std::move(o.m_mphfs);
            m_overlay = std::move(o.m_overlay);
            m_overlay.bind(m_bv);
            m_threshold = o.m_threshold;
            m_build_has_hash = std::move(o.m_build_has_hash);
            m_build_seq = std::move(o.m_build_seq);
        }
        return *this;
    }

    void swap(compact_metatrie_hash& o) {
        // m_bp.swap(bp_support.m_bp); use set_vector to set the supported
        // bit_vector
        std::swap(m_bv, o.m_bv);
        sdsl::util::swap_support(m_succ0, o.m_succ0, &m_bv, &o.m_bv);
        sdsl::util::swap_support(m_select0, o.m_select0, &m_bv, &o.m_bv);
        std::swap(m_root_degree, o.m_root_degree);
        std::swap(m_seq_plain, o.m_seq_plain);
        std::swap(m_mphfs, o.m_mphfs);
        m_overlay.swap(o.m_overlay);
        m_overlay.bind(m_bv);
        o.m_overlay.bind(o.m_bv);
        std::swap(m_threshold, o.m_threshold);
        std::swap(m_build_has_hash, o.m_build_has_hash);
        std::swap(m_build_seq, o.m_build_seq);
    }

    /*
      Degree of the trie root
  */

    size_type root_degree() const { return m_root_degree; }

    size_type louds_size() const { return m_bv.size(); }

    size_type mphf_count() const { return m_mphfs.size(); }

    uint32_t threshold() const { return m_threshold; }

    const sdsl::bit_vector& bv() const { return m_bv; }

    const sdsl::int_vector<>& seq_plain() const { return m_seq_plain; }

    const Overlay& overlay() const { return m_overlay; }

    /** 
     * @brief True iff the node at LOUDS position @p node_pos has an MPHF overlay.
     * @param node_pos LOUDS position of the node to check
     * @return True if the node has an MPHF overlay, false otherwise.
     */
    bool node_has_hash(size_type node_pos) const { return m_overlay.has_dict(node_pos); }

    /** 
     * @brief O(1) membership check: is @p key a child of the hashed node at @p node_pos?
     * @param node_pos LOUDS position of the node to check
     * @param key The key to check
     * @return True if @p key is a child, false otherwise.
     */
    bool hash_contains(size_type node_pos, value_type key) const {
        size_type mphf_idx = m_overlay.dict_idx(node_pos);
        return m_mphfs[mphf_idx].contains(key);
    }

    /** 
     * @brief Membership + slot in one pass: 
     * @param node_pos LOUDS position of the node to check
     * @param key The key to check
     * @return {true, slot} if @p key is a child, {false, 0} otherwise.
     */
    std::pair<bool, uint32_t> hash_locate(size_type node_pos, value_type key) const {
        size_type mphf_idx = m_overlay.dict_idx(node_pos);
        return m_mphfs[mphf_idx].locate(key);
    }

    /**
     * @brief Enumerates the children of the hashed node at @p node_pos, in slot order.
     * @param node_pos LOUDS position of the node to scan
     * @return Cursor over the node's keys, slot 0 first.
     */
    mphf_type::key_cursor hash_keys(size_type node_pos) const {
        size_type mphf_idx = m_overlay.dict_idx(node_pos);
        return m_mphfs[mphf_idx].keys();
    }

    //! r of the node at @p node_pos: the number of hashed nodes that start before it.
    size_type dict_idx(size_type node_pos) const { return m_overlay.dict_idx(node_pos); }

    // Overloads for a caller that already knows r = dict_idx(node_pos). In Debug they
    // check that r is the one of node_pos.

    bool node_has_hash(size_type node_pos, size_type r) const {
        assert(r == m_overlay.dict_idx(node_pos) && "r belongs to another node");
        return m_overlay.has_dict(node_pos, r);
    }

    bool hash_contains([[maybe_unused]] size_type node_pos, size_type r, value_type key) const {
        assert(r == m_overlay.dict_idx(node_pos) && "r belongs to another node");
        return m_mphfs[r].contains(key);
    }

    std::pair<bool, uint32_t> hash_locate([[maybe_unused]] size_type node_pos, size_type r, value_type key) const {
        assert(r == m_overlay.dict_idx(node_pos) && "r belongs to another node");
        return m_mphfs[r].locate(key);
    }

    mphf_type::key_cursor hash_keys([[maybe_unused]] size_type node_pos, size_type r) const {
        assert(r == m_overlay.dict_idx(node_pos) && "r belongs to another node");
        return m_mphfs[r].keys();
    }

    //! Number of keys of the MPHF of the hashed node at @p node_pos.
    uint32_t hash_size(size_type node_pos) const { return m_mphfs[m_overlay.dict_idx(node_pos)].n(); }

    //! True iff the MPHF of the hashed node at @p node_pos keeps part of its keys in the fallback.
    bool hash_has_fallback(size_type node_pos) const {
        return m_mphfs[m_overlay.dict_idx(node_pos)].has_fallback();
    }

    /**
     * @brief Extracts the MPHF-induced permutation for the root's children.
     *
     * Must be called before reorder_louds_by_mphf() (which changes the layout).
     * Returns perm such that perm[slot] = old_child_index.
     * If the root is not hashed, returns the identity: the result is never empty,
     * so the partial trie always takes the first-level path of reorder_louds_by_mphf().
     */
    std::vector<size_type> extract_root_permutation() const {
        assert(!m_build_has_hash.empty() && "call between build_hash_overlay() and reorder_louds_by_mphf()");
        size_type d = children(0);
        std::vector<size_type> perm(d);
        if (!m_build_has_hash[0]) {
            for (size_type i = 0; i < d; i++)
                perm[i] = i;
            return perm;
        }
        // build_hash_overlay() visits the root first, so its MPHF is the first one.
        size_type mphf_idx = 0;
        for (size_type i = 0; i < d; i++) {
            value_type key = m_build_seq[i];
            auto [found, slot] = m_mphfs[mphf_idx].locate(key);
            perm[slot] = i;
        }
        return perm;
    }

    /*
      Receives index of current node and the child that is required
      Returns index of the nth child of current node
  */
    inline size_type child(uint32_t it, uint32_t n) const { return m_select0(it + 1 + n); }

    /*
      Receives index of node whos children we want to count
      Returns how many children said node has
  */
    size_type children(size_type i) const { return m_succ0(i + 1) - i; }

    size_type first_child(size_type i) const { return i; }

    /**
     * @brief Computes a histogram of node out-degrees in the metatrie.
     * @return A map where key = number of children and value = node count.
     */
    std::map<size_type, size_type> children_histogram() const {
        std::map<size_type, size_type> hist;

        size_type num_zeros = count_bits(m_bv, 0);

        std::vector<size_type> current_level = {0};
        while (!current_level.empty()) {
            std::vector<size_type> next_level;
            for (auto node : current_level) {
                size_type d = children(node);
                hist[d]++;
                for (size_type n = 1; n <= d; n++) {
                    if (node + 1 + n > num_zeros)
                        break;
                    size_type child_node = child(node, n);
                    if (child_node + 1 < m_bv.size())
                        next_level.push_back(child_node);
                }
            }
            current_level = std::move(next_level);
        }

        return hist;
    }

    struct NodeInfo {
        int64_t parent;  // row index of parent in the output vector; -1 for the root
        uint32_t depth;
        uint32_t key;  // 0 for the virtual root
        size_type n_children;
        bool is_leaf;
    };

    /**
     * @brief Builds MPHF for every node with >= threshold children.
     *
     * Walks the LOUDS in BFS order
     * For each node whose out-degree meets the threshold, extracts the keys
     * from L and builds an MPHF.  m_build_has_hash[node_pos] is set to 1
     * for each such node; the overlay itself is built by reorder_louds_by_mphf().
     *
     * @param skip_root  Leave node 0 unhashed. Only meaningful on a full trie,
     *                   where node 0 is the root.
     */
    // TODO: move tracer metadata (e.g., trie_id) out of this functional API.
    void build_hash_overlay(uint32_t threshold, uint32_t trie_id = UINT32_MAX, bool skip_root = false) {
        m_threshold = threshold;
        m_build_has_hash = sdsl::bit_vector(m_bv.size(), 0);
        m_mphfs.clear();

        hashing::MphfBuildTracer<COLLECT_MPHF_BUILD_TRACE> tracer("mphf_trace.jsonl");
        hashing::KeyDumper<DUMP_MPHF_KEYS> dumper(".");

        size_type num_zeros = count_bits(m_bv, 0);

        std::vector<size_type> current_level = {0};
        while (!current_level.empty()) {
            std::vector<size_type> next_level;
            for (auto node : current_level) {
                size_type n_children = children(node);
                if (n_children >= threshold && !(skip_root && node == 0)) {
                    std::vector<uint32_t> keys;
                    keys.reserve(n_children);
                    for (size_type k = 0; k < n_children; k++)
                        keys.push_back(m_build_seq[node + k]);

                    tracer.on_node_start(trie_id, node, n_children, threshold);
                    mphf_type mphf;
                    bool ok = mphf.build(keys, tracer);
                    tracer.on_node_end(ok);

                    if (!ok)
                        dumper.dump(keys, trie_id, node);

                    if (ok) {
                        m_build_has_hash[node] = 1;
                        m_mphfs.push_back(std::move(mphf));
                    }
                }
                for (size_type n = 1; n <= n_children; n++) {
                    if (node + 1 + n > num_zeros)
                        break;
                    size_type child_node = child(node, n);
                    if (child_node + 1 < m_bv.size())
                        next_level.push_back(child_node);
                }
            }
            current_level = std::move(next_level);
        }
    }

    /**
     * @brief Rebuilds m_bv so that children of hashed nodes appear in MPHF slot
     *        order instead of sorted order, and emits L_plain and the overlay.
     *
     * Must be called after build_hash_overlay().  The MPHFs are not modified;
     * the layout (m_bv), L_plain, the overlay and the supports are rebuilt, and
     * the build scratch (L included) is released.
     *
     * For non-hashed nodes children remain in their original (sorted) order.
     * With drops_root_labels, the root block of a full trie is not copied to L_plain.
     *
     * @param root_perm  If non-empty, use this permutation for the root's
     *                   children instead of the root's own MPHF.  This is
     *                   needed for partial tries that must share the same
     *                   first-level order as their paired full trie.
     *                   perm[slot] = old_child_index.
     */
    void reorder_louds_by_mphf(const std::vector<size_type>& root_perm = {}) {
        if (m_mphfs.empty() && root_perm.empty()) {
            build_plain_overlay();
            return;
        }

        // 1. Init variables
        const bool is_partial_trie = !root_perm.empty();
        const bool drop_root = drops_root_labels && !is_partial_trie;
        if (drop_root && m_build_has_hash[0])
            throw std::runtime_error("reorder_louds_by_mphf: the root block leaves L_plain, so the root must not be hashed (skip_root)");
        const size_type bv_len = m_bv.size();
        const size_type seq_len = m_build_seq.size();
        size_type hashed_labels = 0;
        for (const auto& mphf : m_mphfs)
            hashed_labels += mphf.n();
        const size_type dropped_labels = drop_root ? children(0) : 0;
        const size_type plain_len = seq_len - 1 - hashed_labels - dropped_labels;  // the -1 is the mock of L

        sdsl::bit_vector new_bv(bv_len, 1);
        sdsl::int_vector<> new_plain(plain_len, 0, m_build_seq.width());
        Overlay new_overlay;
        new_overlay.begin(bv_len, plain_len);
        std::vector<mphf_type> new_mphfs;
        new_mphfs.reserve(m_mphfs.size());
        sdsl::rank_support_v<1> build_hash_rank(&m_build_has_hash);

        size_type seq_cursor = 0;
        size_type plain_cursor = 0;

        size_type num_zeros = count_bits(m_bv, 0);

        // 2. Initialize "current_level" queue for BFS
        std::vector<size_type> current_level;
        if (is_partial_trie) {
            // Partial trie: must use the same permutation as its full trie counterpart
            std::vector<size_type> top_level_nodes;
            top_level_nodes.reserve(num_zeros - 1);
            top_level_nodes.push_back(0);  // first implicit first-level node
            for (size_type i = 0; i + 1 < num_zeros - 1; i++)
                top_level_nodes.push_back(nodeselect(i));

            current_level.reserve(root_perm.size());
            for (size_type slot = 0; slot < root_perm.size(); slot++)
                current_level.push_back(top_level_nodes[root_perm[slot]]);
        } else {
            // Full trie: start BFS from the root
            current_level = {0};
        }

        // 3. Reconstruct the LOUDS layout in BFS order
        bool in_partial_trie_first_level = is_partial_trie;
        while (!current_level.empty()) {
            std::vector<size_type> next_level;
            for (auto old_pos : current_level) {
                size_type new_pos = seq_cursor;
                size_type n_children = children(old_pos);
                const bool is_dropped_root = drop_root && old_pos == 0;

                // 3.1. Build permutation
                std::vector<size_type> perm(n_children);
                bool is_hashed = m_build_has_hash[old_pos];
                if (is_hashed) {
                    // Hashed node: build permutation from MPHF
                    size_type mphf_idx = build_hash_rank(old_pos);
                    if (m_mphfs[mphf_idx].n() != n_children)
                        throw std::runtime_error("reorder_louds_by_mphf: MPHF size differs from the node degree");
                    assert(n_children >= m_threshold && "has_dict(v) implies deg(v) >= threshold");
                    for (size_type i = 0; i < n_children; i++) {
                        value_type key = m_build_seq[old_pos + i];
                        auto [found, slot] = m_mphfs[mphf_idx].locate(key);
                        perm[slot] = i;
                    }
                    new_mphfs.push_back(std::move(m_mphfs[mphf_idx]));
                    new_overlay.on_hashed_node(new_pos, n_children);
                } else {
                    // When not hashed, perm is the identity
                    for (size_type i = 0; i < n_children; i++)
                        perm[i] = i;
                    if (is_dropped_root)
                        new_overlay.on_dropped_root(n_children);
                    else
                        new_overlay.on_plain_node(new_pos, n_children);
                }

                // 3.2. Emit the block; the labels of non-hashed nodes go to L_plain in sorted order
                new_bv[new_pos] = 0;
                seq_cursor += n_children;
                if (!is_hashed && !is_dropped_root) {
                    if (plain_cursor + n_children > plain_len)
                        throw std::runtime_error("reorder_louds_by_mphf: non-hashed blocks overflow L_plain");
                    for (size_type i = 0; i < n_children; i++)
                        new_plain[plain_cursor + i] = m_build_seq[old_pos + i];
                    plain_cursor += n_children;
                }

                // In a partial trie the first-level nodes have no subtrees in the
                // BFS sense (their children are leaves encoded only in L).
                // So we skip enqueuing during the first iteration.
                if (in_partial_trie_first_level)
                    continue;

                // 3.3. Enqueue internal children in the chosen (permuted) order.
                for (size_type i = 0; i < n_children; i++) {
                    size_type old_child_num = perm[i] + 1;  // 1-based original child index
                    if (old_pos + 1 + old_child_num > num_zeros)
                        continue;  // select0(old_pos + 1 + old_child_num) would be out of bounds
                    size_type child_pos = child(old_pos, old_child_num);
                    if (child_pos + 1 < m_bv.size())
                        next_level.push_back(child_pos);
                }
            }
            in_partial_trie_first_level = false;
            current_level = std::move(next_level);
        }

        assert(seq_cursor + 1 == seq_len);
        if (plain_cursor != plain_len)
            throw std::runtime_error("reorder_louds_by_mphf: non-hashed blocks do not fill L_plain");
        new_bv[seq_cursor] = 0;  // Restore the trailing LOUDS sentinel after the last explicit node.

        // 4. Install new arrays and rebuild supports
        m_bv.swap(new_bv);
        sdsl::util::init_support(m_succ0, &m_bv);
        sdsl::util::init_support(m_select0, &m_bv);

        m_seq_plain.swap(new_plain);
        sdsl::util::bit_compress(m_seq_plain);

        m_mphfs.swap(new_mphfs);
        m_overlay = std::move(new_overlay);
        m_overlay.finalize(m_bv, m_threshold);
        release_build_scratch();
        m_root_degree = m_succ0(1);
    }

    /**
     * @brief Returns one NodeInfo per node in BFS order, including leaf nodes.
     *
     * Each entry's parent field is the 0-based row index of its parent in
     * the returned vector.  The root has parent = -1.
     *
     * In this LOUDS encoding, a node at position p stores its *children's*
     * keys at L[p .. p + children(p) - 1].  The node's own key lives
     * in its parent's range of L.  The key is passed from parent to child
     * during BFS, and the virtual root gets key = 0.
     *
     * The last-level nodes (e.g. O-values in full tries) are not encoded in
     * the BV.  Their keys are emitted as leaf entries (is_leaf = true,
     * n_children = 0).  Keys come from L_plain, or from the MPHF cursor for
     * hashed nodes.
     *
     * For partial tries (SOP, PSO, OPS) the first-level keys are not stored
     * in the trie (they come from the full trie via trie switching).
     * First-level nodes therefore appear with key = 0.
     */
    std::vector<NodeInfo> dump_nodes() const {
        std::vector<NodeInfo> nodes;

        size_type num_zeros = count_bits(m_bv, 0);

        struct BFSEntry {
            size_type louds_pos;
            int64_t parent_idx;
            uint32_t depth;
            uint32_t key;
        };

        std::vector<BFSEntry> current_level = {
            {0, -1, 0, 0}
        };
        while (!current_level.empty()) {
            std::vector<BFSEntry> next_level;
            for (auto& e : current_level) {
                size_type d = children(e.louds_pos);
                int64_t my_idx = static_cast<int64_t>(nodes.size());
                nodes.push_back({e.parent_idx, e.depth, e.key, d, false});

                std::vector<uint32_t> child_keys(d);
                if (node_has_hash(e.louds_pos)) {
                    auto cur = hash_keys(e.louds_pos);
                    while (cur.next())
                        child_keys[cur.slot()] = cur.key();
                } else {
                    const size_type off = deleted_before(e.louds_pos);
                    for (size_type k = 0; k < d; k++)
                        child_keys[k] = static_cast<uint32_t>(plain_label(e.louds_pos + k, off));
                }

                for (size_type n = 1; n <= d; n++) {
                    uint32_t child_key = child_keys[n - 1];
                    if (e.louds_pos + 1 + n > num_zeros) {
                        nodes.push_back({my_idx, e.depth + 1, child_key, 0, true});
                    } else {
                        size_type child_pos = child(e.louds_pos, n);
                        if (child_pos + 1 < m_bv.size()) {
                            next_level.push_back({child_pos, my_idx, e.depth + 1, child_key});
                        } else {
                            nodes.push_back({my_idx, e.depth + 1, child_key, 0, true});
                        }
                    }
                }
            }
            current_level = std::move(next_level);
        }

        return nodes;
    }

    inline size_type nodeselect(size_type i) const { return m_select0(i + 2); }

    /**
     * @brief Labels of hashed nodes that come before the node at @p node_pos.
     *
     * For a non-hashed node v, L_plain[pos - deleted_before(v)] is the label at LOUDS
     * position pos of its block [v, v + deg(v)).
     */
    size_type deleted_before(size_type node_pos) const { return node_pos - m_overlay.plain_start(node_pos); }

    //! deleted_before(node_pos) when r = dict_idx(node_pos) is already known.
    size_type deleted_before(size_type node_pos, size_type r) const {
        assert(r == m_overlay.dict_idx(node_pos) && "r belongs to another node");
        return node_pos - m_overlay.plain_start(node_pos, r);
    }

    //! Labels of the root block that are not in L_plain: deg(root) in a full trie with drops_root_labels, else 0.
    size_type dropped_root_labels() const {
        if constexpr (drops_root_labels)
            return m_overlay.root_deleted();
        else
            return 0;
    }

    //! Label at LOUDS position @p pos of a non-hashed node with deleted_before() == @p off.
    value_type plain_label(size_type pos, size_type off) const {
        assert(pos >= off && pos - off < m_seq_plain.size() && "position outside L_plain");
        return m_seq_plain[pos - off];
    }

    /**
     * @brief Binary search in the block of a non-hashed node.
     * @param off deleted_before() of the node; [i, f] and the result are LOUDS positions
     * @return {first label >= val, its position}, or {0, f + 1} if there is none.
     */
    pair<uint32_t, uint32_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f, size_type off) const {
        if (plain_label(f, off) < val)
            return make_pair(0, f + 1);
        uint32_t mid;
        while (i < f) {
            mid = (i + f) / 2;
            if (plain_label(mid, off) < val) {
                i = mid + 1;
            } else {
                f = mid;
            }
        }
        return make_pair(plain_label(i, off), i);
    }

    void print() const {
        for (size_type i = 0; i < m_bv.size(); ++i) {
            std::cout << (uint)m_bv[i];
        }
        std::cout << std::endl;
    }

    //! Serializes the data structure into the given ostream
    size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "")
        const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += sdsl::write_member(format_tag, out, child, "format_tag");
        written_bytes += m_bv.serialize(out, child, "bv");
        written_bytes += m_seq_plain.serialize(out, child, "seq_plain");
        written_bytes += m_succ0.serialize(out, child, "succ0");
        written_bytes += m_select0.serialize(out, child, "select0");
        written_bytes += sdsl::write_member(m_threshold, out, child, "threshold");
        written_bytes += m_overlay.serialize(out, child, "overlay");
        uint32_t n = m_mphfs.size();
        written_bytes += sdsl::write_member(n, out, child, "n_mphfs");
        for (auto& mphf : m_mphfs)
            written_bytes += mphf.serialize(out, child, "mphf");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    /**
     * @brief Serialized bytes of every member, in serialization order; they add up to
     *        sdsl::size_in_bytes() of the trie. Overlay entries are prefixed "overlay.".
     */
    overlay::OverlaySizeBreakdown get_size_breakdown() const {
        overlay::OverlaySizeBreakdown b;
        size_t mphf_bytes = 0;
        for (const auto& mphf : m_mphfs)
            mphf_bytes += sdsl::size_in_bytes(mphf);
        b.components = {
            {"format_tag", sizeof(format_tag)              },
            {"bv",         sdsl::size_in_bytes(m_bv)       },
            {"seq_plain",  sdsl::size_in_bytes(m_seq_plain)},
            {"succ0",      sdsl::size_in_bytes(m_succ0)    },
            {"select0",    sdsl::size_in_bytes(m_select0)  },
            {"threshold",  sizeof(m_threshold)             },
        };
        for (const auto& c : m_overlay.get_size_breakdown().components)
            b.components.push_back({"overlay." + c.first, c.second});
        b.components.push_back({"n_mphfs", sizeof(uint32_t)});
        b.components.push_back({"mphfs", mphf_bytes});
        return b;
    }

    void load(std::istream& in) {
        uint64_t tag = 0;
        sdsl::read_member(tag, in);
        if (!in || tag != format_tag) {
            std::ostringstream msg;
            msg << "compact_metatrie_hash::load: ";
            if (in && (tag & ~0xFFULL) == format_magic)
                msg << "index format version " << (tag & 0xFF) << ", this binary reads version "
                    << int(Overlay::format_version);
            else
                msg << "not an index with a format version (tag 0x" << std::hex << tag << ")";
            msg << "; rebuild the index with this binary";
            throw std::runtime_error(msg.str());
        }
        m_bv.load(in);
        m_seq_plain.load(in);
        m_succ0.load(in, &m_bv);
        m_select0.load(in, &m_bv);
        if (!in)
            throw std::runtime_error("compact_metatrie_hash::load: truncated stream");
        m_root_degree = m_succ0(1);
        sdsl::read_member(m_threshold, in);
        m_overlay.load(in);
        m_overlay.bind(m_bv);
        uint32_t n = 0;
        sdsl::read_member(n, in);
        if (!in)
            throw std::runtime_error("compact_metatrie_hash::load: truncated stream");
        m_mphfs.resize(n);
        for (auto& mphf : m_mphfs)
            mphf.load(in);
        if (!in)
            throw std::runtime_error("compact_metatrie_hash::load: truncated stream");
    }
};
}  // namespace cltj
#endif

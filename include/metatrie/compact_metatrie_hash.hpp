#ifndef CLTJ_COMPACT_METATRIE_HASH_H
#define CLTJ_COMPACT_METATRIE_HASH_H

#include <cds/succ_support_v.hpp>
#include <cltj_config.hpp>
#include <iostream>
#include <map>
#include <sdsl/select_support_mcl.hpp>
#include <sdsl/vectors.hpp>
#include <string>
#include <utility>
#include <vector>
#include <hashing/key_dumper.hpp>
#include <hashing/mphf_bdz.hpp>
#include <hashing/mphf_build_tracer.hpp>
#include <hashing/storage/glgh.hpp>
#include <util/instrument.hpp>

namespace cltj {

class compact_metatrie_hash {
  public:
    typedef uint64_t size_type;
    typedef uint32_t value_type;

  private:
    using mphf_type = hashing::MPHF<hashing::GlGhStorage, hashing::policies::QuotientKey>;

    sdsl::bit_vector m_bv;  // LOUDS bitvector (B)
    sdsl::int_vector<> m_seq;  // Trie labels (L)
    // sdsl::rank_support_v<1> m_rank1;
    cds::succ_support_v<0> m_succ0;
    sdsl::select_support_mcl<0> m_select0;

    size_type m_root_degree;

    // Hash overlay
    // TODO: Empty until Batch 2 builds MPHFs
    std::vector<mphf_type> m_mphfs;
    sdsl::bit_vector m_has_hash;
    sdsl::rank_support_v<1> m_hash_rank;
    uint32_t m_threshold = 0;

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

  public:
    const sdsl::int_vector<>& seq = m_seq;

    compact_metatrie_hash() = default;

    compact_metatrie_hash(sdsl::bit_vector& _bv, sdsl::int_vector<>& _seq) {
        m_bv = _bv;
        m_seq = _seq;
        sdsl::util::bit_compress(m_seq);
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
            m_seq = std::move(o.m_seq);
            // m_rank1 = std::move(o.m_rank1);
            m_succ0 = std::move(o.m_succ0);
            // m_rank1.set_vector(&m_bv);
            m_succ0.set_vector(&m_bv);
            m_select0 = std::move(o.m_select0);
            m_select0.set_vector(&m_bv);
            m_root_degree = o.m_root_degree;
            m_mphfs = std::move(o.m_mphfs);
            m_has_hash = std::move(o.m_has_hash);
            m_hash_rank = std::move(o.m_hash_rank);
            m_hash_rank.set_vector(&m_has_hash);
            m_threshold = o.m_threshold;
        }
        return *this;
    }

    void swap(compact_metatrie_hash& o) {
        // m_bp.swap(bp_support.m_bp); use set_vector to set the supported
        // bit_vector
        std::swap(m_bv, o.m_bv);
        std::swap(m_seq, o.m_seq);
        sdsl::util::swap_support(m_succ0, o.m_succ0, &m_bv, &o.m_bv);
        sdsl::util::swap_support(m_select0, o.m_select0, &m_bv, &o.m_bv);
        std::swap(m_root_degree, o.m_root_degree);
        std::swap(m_mphfs, o.m_mphfs);
        std::swap(m_has_hash, o.m_has_hash);
        sdsl::util::swap_support(m_hash_rank, o.m_hash_rank, &m_has_hash, &o.m_has_hash);
        std::swap(m_threshold, o.m_threshold);
    }

    /*
      Degree of the trie root
  */

    size_type root_degree() const { return m_root_degree; }

    size_type louds_size() const { return m_bv.size(); }

    size_type mphf_count() const { return m_mphfs.size(); }

    /** 
     * @brief True iff the node at LOUDS position @p node_pos has an MPHF overlay.
     * @param node_pos LOUDS position of the node to check
     * @return True if the node has an MPHF overlay, false otherwise.
     */
    bool node_has_hash(size_type node_pos) const { return m_has_hash[node_pos]; }

    /** 
     * @brief O(1) membership check: is @p key a child of the hashed node at @p node_pos?
     * @param node_pos LOUDS position of the node to check
     * @param key The key to check
     * @return True if @p key is a child, false otherwise.
     */
    bool hash_contains(size_type node_pos, value_type key) const {
        size_type mphf_idx = m_hash_rank(node_pos);
        return m_mphfs[mphf_idx].contains(key);
    }

    /** 
     * @brief Membership + slot in one pass: 
     * @param node_pos LOUDS position of the node to check
     * @param key The key to check
     * @return {true, slot} if @p key is a child, {false, 0} otherwise.
     */
    std::pair<bool, uint32_t> hash_locate(size_type node_pos, value_type key) const {
        size_type mphf_idx = m_hash_rank(node_pos);
        return m_mphfs[mphf_idx].locate(key);
    }

    /**
     * @brief Enumerates the children of the hashed node at @p node_pos, in slot order.
     * @param node_pos LOUDS position of the node to scan
     * @return Cursor over the node's keys, slot 0 first.
     */
    mphf_type::key_cursor hash_keys(size_type node_pos) const {
        size_type mphf_idx = m_hash_rank(node_pos);
        return m_mphfs[mphf_idx].keys();
    }

    /**
     * @brief Extracts the MPHF-induced permutation for the root's children.
     *
     * Must be called before reorder_louds_by_mphf() (which changes m_seq layout).
     * Returns perm such that perm[slot] = old_child_index.
     * If the root is not hashed, returns an empty vector (identity is implied).
     */
    std::vector<size_type> extract_root_permutation() const {
        if (!m_has_hash[0])
            return {};
        size_type d = children(0);
        size_type mphf_idx = m_hash_rank(0);
        std::vector<size_type> perm(d);
        for (size_type i = 0; i < d; i++) {
            value_type key = m_seq[i];
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
     * from m_seq and builds an MPHF.  m_has_hash[node_pos] is set to 1 for
     * each such node, and m_hash_rank is rebuilt over the updated bitvector.
     */
    // TODO: move tracer metadata (e.g., trie_id) out of this functional API.
    void build_hash_overlay(uint32_t threshold, uint32_t trie_id = UINT32_MAX) {
        m_threshold = threshold;
        m_has_hash = sdsl::bit_vector(m_bv.size(), 0);
        m_mphfs.clear();

        hashing::MphfBuildTracer<COLLECT_MPHF_BUILD_TRACE> tracer("mphf_trace.jsonl");
        hashing::KeyDumper<DUMP_MPHF_KEYS> dumper(".");

        size_type num_zeros = count_bits(m_bv, 0);

        std::vector<size_type> current_level = {0};
        while (!current_level.empty()) {
            std::vector<size_type> next_level;
            for (auto node : current_level) {
                size_type n_children = children(node);
                if (n_children >= threshold) {
                    std::vector<uint32_t> keys;
                    keys.reserve(n_children);
                    for (size_type k = 0; k < n_children; k++)
                        keys.push_back(m_seq[node + k]);

                    tracer.on_node_start(trie_id, node, n_children, threshold);
                    mphf_type mphf;
                    bool ok = mphf.build(keys, tracer);
                    tracer.on_node_end(ok);

                    if (!ok)
                        dumper.dump(keys, trie_id, node);

                    if (ok) {
                        m_has_hash[node] = 1;
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
        sdsl::util::init_support(m_hash_rank, &m_has_hash);
    }

    /**
     * @brief Rebuilds m_bv and m_seq so that children of hashed nodes appear
     *        in MPHF slot order instead of sorted order.
     *
     * Must be called after build_hash_overlay().  The MPHFs and m_mphfs vector
     * are not modified; only the physical layout (m_bv, m_seq) and the
     * positional metadata (m_has_hash, m_hash_rank, supports) are rebuilt.
     *
     * For non-hashed nodes children remain in their original (sorted) order.
     *
     * @param root_perm  If non-empty, use this permutation for the root's
     *                   children instead of the root's own MPHF.  This is
     *                   needed for partial tries that must share the same
     *                   first-level order as their paired full trie.
     *                   perm[slot] = old_child_index.
     */
    void reorder_louds_by_mphf(const std::vector<size_type>& root_perm = {}) {
        if (m_mphfs.empty() && root_perm.empty())
            return;

        // 1. Init variables
        const size_type bv_len = m_bv.size();
        const size_type seq_len = m_seq.size();

        sdsl::bit_vector new_bv(bv_len, 1);
        sdsl::int_vector<> new_seq(seq_len);
        sdsl::bit_vector new_has_hash(bv_len, 0);
        std::vector<mphf_type> new_mphfs;
        new_mphfs.reserve(m_mphfs.size());

        size_type seq_cursor = 0;

        size_type num_zeros = count_bits(m_bv, 0);

        // 2. Initialize "current_level" queue for BFS
        const bool is_partial_trie = !root_perm.empty();
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

                // 3.1. Build permutation
                std::vector<size_type> perm(n_children);
                bool is_hashed = m_has_hash[old_pos];
                if (is_hashed) {
                    // Hashed node: build permutation from MPHF
                    size_type mphf_idx = m_hash_rank(old_pos);
                    for (size_type i = 0; i < n_children; i++) {
                        value_type key = m_seq[old_pos + i];
                        auto [found, slot] = m_mphfs[mphf_idx].locate(key);
                        perm[slot] = i;
                    }
                    new_has_hash[new_pos] = 1;
                    new_mphfs.push_back(std::move(m_mphfs[mphf_idx]));
                } else {
                    // When not hashed, perm is the identity
                    for (size_type i = 0; i < n_children; i++)
                        perm[i] = i;
                }

                // 3.2. Emit children in the chosen ("permuted") order
                new_bv[new_pos] = 0;
                for (size_type i = 0; i < n_children; i++) {
                    new_seq[new_pos + i] = m_seq[old_pos + perm[i]];
                }
                seq_cursor += n_children;

                // In a partial trie the first-level nodes have no subtrees in the
                // BFS sense (their children are leaves encoded only in m_seq).
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
        new_bv[seq_cursor] = 0;  // Restore the trailing LOUDS sentinel after the last explicit node.

        // 4. Install new arrays and rebuild supports
        m_bv.swap(new_bv);
        m_seq.swap(new_seq);
        sdsl::util::bit_compress(m_seq);
        sdsl::util::init_support(m_succ0, &m_bv);
        sdsl::util::init_support(m_select0, &m_bv);

        m_mphfs.swap(new_mphfs);
        m_has_hash.swap(new_has_hash);
        sdsl::util::init_support(m_hash_rank, &m_has_hash);
        m_root_degree = m_succ0(1);
    }

    /**
     * @brief Returns one NodeInfo per node in BFS order, including leaf nodes.
     *
     * Each entry's parent field is the 0-based row index of its parent in
     * the returned vector.  The root has parent = -1.
     *
     * In this LOUDS encoding, a node at position p stores its *children's*
     * keys at m_seq[p .. p + children(p) - 1].  The node's own key lives
     * in its parent's m_seq range.  The key is passed from parent to child
     * during BFS, and the virtual root gets key = 0.
     *
     * The last-level nodes (e.g. O-values in full tries) are not encoded in
     * the BV.  Their keys are read from m_seq and emitted as leaf entries
     * (is_leaf = true, n_children = 0).
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

                for (size_type n = 1; n <= d; n++) {
                    uint32_t child_key = static_cast<uint32_t>(m_seq[e.louds_pos + n - 1]);
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

    pair<uint32_t, uint32_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f) const {
        if (m_seq[f] < val)
            return make_pair(0, f + 1);
        uint32_t mid;
        while (i < f) {
            mid = (i + f) / 2;
            if (m_seq[mid] < val) {
                i = mid + 1;
            } else {
                f = mid;
            }
        }
        return make_pair(m_seq[i], i);
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
        written_bytes += m_bv.serialize(out, child, "bv");
        written_bytes += m_seq.serialize(out, child, "seq");
        written_bytes += m_succ0.serialize(out, child, "succ0");
        written_bytes += m_select0.serialize(out, child, "select0");
        written_bytes += sdsl::write_member(m_threshold, out, child, "threshold");
        written_bytes += m_has_hash.serialize(out, child, "has_hash");
        written_bytes += m_hash_rank.serialize(out, child, "hash_rank");
        uint32_t n = m_mphfs.size();
        written_bytes += sdsl::write_member(n, out, child, "n_mphfs");
        for (auto& mphf : m_mphfs)
            written_bytes += mphf.serialize(out, child, "mphf");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        m_bv.load(in);
        m_seq.load(in);
        m_succ0.load(in, &m_bv);
        m_select0.load(in, &m_bv);
        m_root_degree = m_succ0(1);
        sdsl::read_member(m_threshold, in);
        m_has_hash.load(in);
        m_hash_rank.load(in, &m_has_hash);
        uint32_t n;
        sdsl::read_member(n, in);
        m_mphfs.resize(n);
        for (auto& mphf : m_mphfs)
            mphf.load(in);
    }
};
}  // namespace cltj
#endif

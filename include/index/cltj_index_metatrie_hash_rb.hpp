#ifndef CLTJ_INDEX_METATRIE_HASH_RB_HPP
#define CLTJ_INDEX_METATRIE_HASH_RB_HPP

#include <cltj_config.hpp>
#include <index/cltj_index_metatrie.hpp>
#include <index/cltj_index_metatrie_rb.hpp>
#include <index/root_bitmap.hpp>
#include <metatrie/compact_metatrie_hash.hpp>
#include <metatrie/overlay/step_table_rb.hpp>

#include <array>
#include <stdexcept>

namespace cltj {

// H-CLTJ (scheme C) with the children of the roots of SPO, POS and OSP in a root_bitmap instead of
// L_plain or an MPHF. The roots stay in B as node 0, unhashed, with their children in id order, so
// the partial tries get the identity permutation. Every trie carries its own format tag (version 5).
// The hash overlay is built afterwards, pair by pair, with skip_root on the full tries.
class cltj_index_metatrie_hash_rb {
  public:
    typedef uint64_t size_type;
    typedef uint32_t value_type;
    typedef compact_metatrie_hash<overlay::StepTableRbOverlay> trie_type;

  private:
    std::array<trie_type, 6> m_tries;
    root_bitmap m_root;

  public:
    cltj_index_metatrie_hash_rb() = default;

    cltj_index_metatrie_hash_rb(vector<spo_triple>& D) {
        cltj_index_metatrie<detail::raw_louds_trie> raw(D);
        for (size_type t = 0; t < 6; ++t) {
            auto* r = raw.get_trie(t);
            m_tries[t] = trie_type(r->bv, r->seq);
            if (t % 2 == 0) {
                const size_type d = m_tries[t].root_degree();
                m_root.set(static_cast<state_type>(t / 2), r->seq.begin(), r->seq.begin() + d);
            }
            *r = detail::raw_louds_trie();
        }
    }

    cltj_index_metatrie_hash_rb(const cltj_index_metatrie_hash_rb&) = delete;
    cltj_index_metatrie_hash_rb& operator=(const cltj_index_metatrie_hash_rb&) = delete;

    inline trie_type* get_trie(size_type i) { return &m_tries[i]; }

    inline const trie_type* get_trie(size_type i) const { return &m_tries[i]; }

    inline const root_bitmap& root() const { return m_root; }

    size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const {
        sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        for (const auto& trie : m_tries)
            written_bytes += trie.serialize(out, child, "tries");
        written_bytes += m_root.serialize(out, child, "root");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        for (auto& trie : m_tries)
            trie.load(in);
        m_root.load(in);
        if (!in)
            throw std::runtime_error("cltj_index_metatrie_hash_rb::load: truncated stream");
    }
};

typedef cltj::cltj_index_metatrie_hash_rb compact_ltj_metatrie_hash_rb;

} // namespace cltj

#endif

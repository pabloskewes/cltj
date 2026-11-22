#ifndef CLTJ_COMPACT_METATRIE_RB_HPP
#define CLTJ_COMPACT_METATRIE_RB_HPP

#include <metatrie/cltj_compact_metatrie.hpp>
#include <metatrie/plain_labels.hpp>
#include <sdsl/io.hpp>

#include <cassert>
#include <utility>

namespace cltj {

// compact_metatrie whose L drops its first `offset` labels: the block of the root in a full trie
// (its children live in a root_bitmap), nothing in a partial one. B is unchanged, so positions are
// still positions in B and the label at position i is L[i - offset]. Positions inside the root
// block have no label. L is kept in a Labels policy (plain_labels.hpp) and the compact_metatrie
// keeps B with an empty L.
template <class Labels = single_width_labels>
class compact_metatrie_rb {
  public:
    typedef uint64_t size_type;
    typedef uint32_t value_type;
    typedef Labels labels_type;

  private:
    compact_metatrie m_trie;
    Labels m_labels;
    size_type m_offset = 0;

  public:
    compact_metatrie_rb() = default;

    compact_metatrie_rb(sdsl::bit_vector& bv, const sdsl::int_vector<>& seq, size_type offset) : m_offset(offset) {
        sdsl::int_vector<> rest(seq.size() - offset, 0, seq.width());
        for (size_type i = 0; i < rest.size(); ++i)
            rest[i] = seq[i + offset];
        sdsl::int_vector<> no_labels;
        m_trie = compact_metatrie(bv, no_labels);
        // Level 2 is the blocks of the level-1 nodes: in a full trie offset = deg(root), and level 3
        // starts at the block of node `offset`; a partial trie has only level 2.
        const size_type n2 = offset ? m_trie.nodeselect(offset) - offset : rest.size();
        m_labels = Labels(std::move(rest), n2);
    }

    const compact_metatrie& base() const { return m_trie; }

    const Labels& labels() const { return m_labels; }

    size_type offset() const { return m_offset; }

    size_type root_degree() const { return m_trie.root_degree(); }

    inline size_type child(uint32_t it, uint32_t n) const { return m_trie.child(it, n); }

    size_type children(size_type i) const { return m_trie.children(i); }

    size_type first_child(size_type i) const { return m_trie.first_child(i); }

    inline size_type nodeselect(size_type i) const { return m_trie.nodeselect(i); }

    inline value_type label(size_type i) const {
        assert(i >= m_offset);
        return m_labels.access(i - m_offset);
    }

    pair<uint32_t, uint32_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f) const {
        assert(i >= m_offset);
        return m_labels.seek(val, i, f, m_offset);
    }

    //! L goes where compact_metatrie writes its seq, so a single-width trie keeps the bytes of X-rb.
    size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const {
        sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        sdsl::structure_tree_node* trie =
            sdsl::structure_tree::add_child(child, "trie", sdsl::util::class_name(m_trie));
        size_type trie_bytes = 0;
        trie_bytes += m_trie.m_bv.serialize(out, trie, "bv");
        trie_bytes += m_labels.serialize(out, trie, "seq");
        trie_bytes += m_trie.m_succ0.serialize(out, trie, "succ0");
        trie_bytes += m_trie.m_select0.serialize(out, trie, "select0");
        sdsl::structure_tree::add_size(trie, trie_bytes);
        size_type written_bytes = trie_bytes;
        written_bytes += sdsl::write_member(m_offset, out, child, "offset");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        m_trie.m_bv.load(in);
        m_labels.load(in);
        m_trie.m_succ0.load(in, &m_trie.m_bv);
        m_trie.m_select0.load(in, &m_trie.m_bv);
        m_trie.m_root_degree = m_trie.m_succ0(1);
        sdsl::read_member(m_offset, in);
    }
};

} // namespace cltj

#endif

#ifndef CLTJ_COMPACT_METATRIE_RB_HPP
#define CLTJ_COMPACT_METATRIE_RB_HPP

#include <metatrie/cltj_compact_metatrie.hpp>
#include <sdsl/io.hpp>

#include <cassert>
#include <utility>

namespace cltj {

// compact_metatrie whose L drops its first `offset` labels: the block of the root in a full trie
// (its children live in a root_bitmap), nothing in a partial one. B is unchanged, so positions are
// still positions in B and the label at position i is L[i - offset]. Positions inside the root
// block have no label.
class compact_metatrie_rb {
  public:
    typedef uint64_t size_type;
    typedef uint32_t value_type;

  private:
    compact_metatrie m_trie;
    size_type m_offset = 0;

  public:
    compact_metatrie_rb() = default;

    compact_metatrie_rb(sdsl::bit_vector& bv, const sdsl::int_vector<>& seq, size_type offset) : m_offset(offset) {
        sdsl::int_vector<> rest(seq.size() - offset, 0, seq.width());
        for (size_type i = 0; i < rest.size(); ++i)
            rest[i] = seq[i + offset];
        m_trie = compact_metatrie(bv, rest);
    }

    const compact_metatrie& base() const { return m_trie; }

    size_type offset() const { return m_offset; }

    size_type root_degree() const { return m_trie.root_degree(); }

    inline size_type child(uint32_t it, uint32_t n) const { return m_trie.child(it, n); }

    size_type children(size_type i) const { return m_trie.children(i); }

    size_type first_child(size_type i) const { return m_trie.first_child(i); }

    inline size_type nodeselect(size_type i) const { return m_trie.nodeselect(i); }

    inline value_type label(size_type i) const {
        assert(i >= m_offset);
        return m_trie.seq[i - m_offset];
    }

    pair<uint32_t, uint32_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f) const {
        assert(i >= m_offset);
        auto p = m_trie.binary_search_seek(val, i - m_offset, f - m_offset);
        p.second += m_offset;
        return p;
    }

    size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const {
        sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += m_trie.serialize(out, child, "trie");
        written_bytes += sdsl::write_member(m_offset, out, child, "offset");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        m_trie.load(in);
        sdsl::read_member(m_offset, in);
    }
};

} // namespace cltj

#endif

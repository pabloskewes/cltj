#ifndef CLTJ_INDEX_METATRIE_RB_HPP
#define CLTJ_INDEX_METATRIE_RB_HPP

#include <cltj_config.hpp>
#include <index/cltj_index_metatrie.hpp>
#include <index/root_bitmap.hpp>
#include <metatrie/compact_metatrie_rb.hpp>
#include <sdsl/io.hpp>

#include <array>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace cltj {

namespace detail {

// B and L exactly as cltj_index_metatrie builds them, kept unsupported until they are split.
struct raw_louds_trie {
    sdsl::bit_vector bv;
    sdsl::int_vector<> seq;

    raw_louds_trie() = default;

    raw_louds_trie(sdsl::bit_vector& _bv, sdsl::int_vector<>& _seq) : bv(std::move(_bv)), seq(std::move(_seq)) {
        sdsl::util::bit_compress(seq);
    }
};

} // namespace detail

// X-CLTJ with the children of the roots of SPO, POS and OSP in a root_bitmap instead of L. The
// tries keep the root in B, so every position in B is the same as in X-CLTJ.
class cltj_index_metatrie_rb {
  public:
    typedef uint64_t size_type;
    typedef uint32_t value_type;
    typedef compact_metatrie_rb trie_type;

    // Same tag scheme as compact_metatrie_hash (versions table next to its format_magic), written
    // once at the start of the index.
    static constexpr uint8_t format_version = 4;
    static constexpr uint64_t format_tag = compact_metatrie_hash<>::format_magic | format_version;

  private:
    std::array<trie_type, 6> m_tries;
    root_bitmap m_root;

  public:
    cltj_index_metatrie_rb() = default;

    cltj_index_metatrie_rb(vector<spo_triple>& D) {
        cltj_index_metatrie<detail::raw_louds_trie> raw(D);
        for (size_type t = 0; t < 6; ++t) {
            auto* r = raw.get_trie(t);
            size_type offset = 0;
            if (t % 2 == 0) {
                // The root block is bv[1..d]: d - 1 ones and the 0 that closes it.
                while (r->bv[offset + 1])
                    ++offset;
                ++offset;
                m_root.set(static_cast<state_type>(t / 2), r->seq.begin(), r->seq.begin() + offset);
            }
            m_tries[t] = trie_type(r->bv, r->seq, offset);
            *r = detail::raw_louds_trie();
        }
    }

    inline const trie_type* get_trie(size_type i) const { return &m_tries[i]; }

    inline const root_bitmap& root() const { return m_root; }

    size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const {
        sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += sdsl::write_member(format_tag, out, child, "format_tag");
        for (const auto& trie : m_tries)
            written_bytes += trie.serialize(out, child, "tries");
        written_bytes += m_root.serialize(out, child, "root");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        uint64_t tag = 0;
        sdsl::read_member(tag, in);
        if (!in || tag != format_tag) {
            std::ostringstream msg;
            msg << "cltj_index_metatrie_rb::load: ";
            if (in && (tag & ~0xFFULL) == compact_metatrie_hash<>::format_magic)
                msg << "index format version " << (tag & 0xFF) << ", this binary reads version "
                    << int(format_version);
            else
                msg << "not an index with a format version (tag 0x" << std::hex << tag << ")";
            msg << "; rebuild the index with this binary";
            throw std::runtime_error(msg.str());
        }
        for (auto& trie : m_tries)
            trie.load(in);
        m_root.load(in);
        if (!in)
            throw std::runtime_error("cltj_index_metatrie_rb::load: truncated stream");
    }
};

typedef cltj::cltj_index_metatrie_rb compact_ltj_metatrie_rb;

} // namespace cltj

#endif

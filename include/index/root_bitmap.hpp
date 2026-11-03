#ifndef CLTJ_ROOT_BITMAP_HPP
#define CLTJ_ROOT_BITMAP_HPP

#include <cds/succ_support_v.hpp>
#include <cltj_config.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/io.hpp>
#include <sdsl/rank_support_v.hpp>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace cltj {

// Children of the roots of the three full tries (SPO, POS, OSP), one bitmap per role (s, p, o)
// with a 1 at every id that is a child of that root. The i-th child of a root is the i-th 1, so
// its position in the root block is rank(id). Ids start at 1: 0 is the "no value" of leap().
class root_bitmap {
  public:
    typedef uint64_t size_type;
    typedef uint32_t value_type;

  private:
    std::array<sdsl::bit_vector, 3> m_bv;
    std::array<sdsl::rank_support_v<1>, 3> m_rank;
    std::array<cds::succ_support_v<1>, 3> m_succ;

    void bind() {
        for (size_type i = 0; i < 3; ++i) {
            m_rank[i].set_vector(&m_bv[i]);
            m_succ[i].set_vector(&m_bv[i]);
        }
    }

  public:
    root_bitmap() = default;

    root_bitmap(const root_bitmap& o) : m_bv(o.m_bv), m_rank(o.m_rank), m_succ(o.m_succ) { bind(); }

    root_bitmap(root_bitmap&& o) noexcept
        : m_bv(std::move(o.m_bv)), m_rank(std::move(o.m_rank)), m_succ(std::move(o.m_succ)) {
        bind();
    }

    root_bitmap& operator=(const root_bitmap& o) {
        if (this != &o) {
            m_bv = o.m_bv;
            m_rank = o.m_rank;
            m_succ = o.m_succ;
            bind();
        }
        return *this;
    }

    root_bitmap& operator=(root_bitmap&& o) noexcept {
        if (this != &o) {
            m_bv = std::move(o.m_bv);
            m_rank = std::move(o.m_rank);
            m_succ = std::move(o.m_succ);
            bind();
        }
        return *this;
    }

    // [first, last) are the labels of the root's children, in the order of the root block.
    template <class Iter> void set(state_type st, Iter first, Iter last) {
        static const char* roles = "spo";
        if (first == last)
            throw std::runtime_error(std::string("root_bitmap: empty root for ") + roles[st]);
        uint64_t prev = 0;
        for (Iter it = first; it != last; ++it) {
            const uint64_t id = *it;
            if (id <= prev)
                throw std::runtime_error(
                    std::string("root_bitmap: the children of the root of ") + roles[st] +
                    (id == 0 ? " include id 0; ids must start at 1" : " are not strictly increasing")
                );
            prev = id;
        }
        m_bv[st] = sdsl::bit_vector(prev + 1, 0);
        for (Iter it = first; it != last; ++it)
            m_bv[st][*it] = 1;
        m_rank[st] = sdsl::rank_support_v<1>(&m_bv[st]);
        m_succ[st] = cds::succ_support_v<1>(&m_bv[st]);
        bind();
    }

    inline bool contains(state_type st, uint64_t c) const { return c < m_bv[st].size() && m_bv[st][c]; }

    // Number of children with label < c; for a child, its position in the root block.
    inline size_type position(state_type st, uint64_t c) const { return m_rank[st](c); }

    // Smallest child label >= c, or 0 if there is none.
    inline value_type next(state_type st, uint64_t c) const {
        const size_type n = m_bv[st].size();
        if (c >= n)
            return 0;
        const uint64_t v = m_succ[st](c);
        return v == n ? 0 : static_cast<value_type>(v);
    }

    inline size_type count(state_type st) const { return m_rank[st](m_bv[st].size()); }

    inline size_type universe(state_type st) const { return m_bv[st].size(); }

    // Serialized bytes of every member, in serialization order.
    std::vector<std::pair<std::string, uint64_t>> size_components() const {
        static const char* roles[] = {"s", "p", "o"};
        std::vector<std::pair<std::string, uint64_t>> c;
        for (size_type i = 0; i < 3; ++i) {
            c.push_back({std::string(roles[i]) + ".bv", sdsl::size_in_bytes(m_bv[i])});
            c.push_back({std::string(roles[i]) + ".rank", sdsl::size_in_bytes(m_rank[i])});
            c.push_back({std::string(roles[i]) + ".succ", sdsl::size_in_bytes(m_succ[i])});
        }
        return c;
    }

    size_type serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const {
        static const char* roles[] = {"s", "p", "o"};
        sdsl::structure_tree_node* child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        for (size_type i = 0; i < 3; ++i) {
            const std::string role(roles[i]);
            written_bytes += m_bv[i].serialize(out, child, role + ".bv");
            written_bytes += m_rank[i].serialize(out, child, role + ".rank");
            written_bytes += m_succ[i].serialize(out, child, role + ".succ");
        }
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        for (size_type i = 0; i < 3; ++i) {
            m_bv[i].load(in);
            m_rank[i].load(in, &m_bv[i]);
            m_succ[i].load(in, &m_bv[i]);
        }
        bind();
    }
};

} // namespace cltj

#endif

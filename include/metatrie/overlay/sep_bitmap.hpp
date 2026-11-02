#pragma once
#include "strategy.hpp"
#include <cassert>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/select_support_mcl.hpp>
#include <sdsl/util.hpp>
#include <stdexcept>

namespace cltj {
namespace overlay {

/**
 * @brief Scheme A: separator bitmap over L_plain
 *
 * B_h marks the hashed nodes and its rank gives r. L_sep has a 1 at the start of every
 * non-hashed block of L_plain plus a trailing 1 at |L_plain|, so L_sep has |L_plain| + 1
 * bits and the last block is closed like any other. With
 *   i = rank0(B, v) - rank1(B_h, v)    (non-hashed nodes before v),
 * plain_start(v) = select1(L_sep, i + 1). sdsl rank counts [0, v) and select is 1-based.
 */
class SepBitmapOverlay : public OverlayPolicy<SepBitmapOverlay> {
  public:
    typedef uint64_t size_type;

    static constexpr uint8_t format_version = 2;

  private:
    sdsl::bit_vector m_has_hash;  // B_h
    sdsl::rank_support_v<1> m_hash_rank;
    sdsl::rank_support_v<0> m_bv_rank0;  // over the trie's B, set by finalize() and bind()
    sdsl::bit_vector m_lsep;  // L_sep
    sdsl::select_support_mcl<1> m_lsep_select;
    size_type m_lsep_cursor = 0;

  public:
    SepBitmapOverlay() = default;
    SepBitmapOverlay(const SepBitmapOverlay&) = delete;
    SepBitmapOverlay& operator=(const SepBitmapOverlay&) = delete;

    SepBitmapOverlay(SepBitmapOverlay&& o) { *this = std::move(o); }

    SepBitmapOverlay& operator=(SepBitmapOverlay&& o) {
        if (this != &o) {
            m_has_hash = std::move(o.m_has_hash);
            m_hash_rank = std::move(o.m_hash_rank);
            m_hash_rank.set_vector(&m_has_hash);
            m_bv_rank0 = std::move(o.m_bv_rank0);
            m_lsep = std::move(o.m_lsep);
            m_lsep_select = std::move(o.m_lsep_select);
            m_lsep_select.set_vector(&m_lsep);
            m_lsep_cursor = o.m_lsep_cursor;
        }
        return *this;
    }

    void swap(SepBitmapOverlay& o) {
        std::swap(m_has_hash, o.m_has_hash);
        sdsl::util::swap_support(m_hash_rank, o.m_hash_rank, &m_has_hash, &o.m_has_hash);
        m_bv_rank0.swap(o.m_bv_rank0);
        std::swap(m_lsep, o.m_lsep);
        sdsl::util::swap_support(m_lsep_select, o.m_lsep_select, &m_lsep, &o.m_lsep);
        std::swap(m_lsep_cursor, o.m_lsep_cursor);
    }

    bool has_dict(size_type v) const { return m_has_hash[v]; }

    size_type dict_idx(size_type v) const { return m_hash_rank(v); }

    size_type plain_start(size_type v) const { return m_lsep_select(m_bv_rank0(v) - m_hash_rank(v) + 1); }

    void begin(size_type louds_len, size_type plain_len) {
        m_has_hash = sdsl::bit_vector(louds_len, 0);
        m_lsep = sdsl::bit_vector(plain_len + 1, 0);
        m_lsep_cursor = 0;
    }

    void on_hashed_node(size_type new_pos, size_type) { m_has_hash[new_pos] = 1; }

    void on_plain_node(size_type, size_type d) {
        assert(d > 0 && m_lsep_cursor + d < m_lsep.size());
        m_lsep[m_lsep_cursor] = 1;
        m_lsep_cursor += d;
    }

    void finalize(const sdsl::bit_vector& bv, uint32_t) {
        assert(m_lsep_cursor + 1 == m_lsep.size() && "non-hashed blocks must fill L_plain exactly");
        m_lsep[m_lsep_cursor] = 1;
        sdsl::util::init_support(m_hash_rank, &m_has_hash);
        sdsl::util::init_support(m_bv_rank0, &bv);
        sdsl::util::init_support(m_lsep_select, &m_lsep);
    }

    void bind(const sdsl::bit_vector& bv) { m_bv_rank0.set_vector(&bv); }

    size_type lsep_size() const { return m_lsep.size(); }

    size_type lsep_ones() const { return sdsl::util::cnt_one_bits(m_lsep); }

    //! select1(L_sep, k), 1-based; k must not exceed lsep_ones()
    size_type lsep_select(size_type k) const { return m_lsep_select(k); }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, const std::string& name = "")
        const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written_bytes = 0;
        written_bytes += m_has_hash.serialize(out, child, "has_hash");
        written_bytes += m_hash_rank.serialize(out, child, "hash_rank");
        written_bytes += m_bv_rank0.serialize(out, child, "bv_rank0");
        written_bytes += m_lsep.serialize(out, child, "lsep");
        written_bytes += m_lsep_select.serialize(out, child, "lsep_select");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        m_has_hash.load(in);
        m_hash_rank.load(in, &m_has_hash);
        m_bv_rank0.load(in);
        m_lsep.load(in);
        m_lsep_select.load(in, &m_lsep);
        if (!in)
            throw std::runtime_error("SepBitmapOverlay::load: truncated stream");
    }

    OverlaySizeBreakdown get_size_breakdown() const {
        OverlaySizeBreakdown b;
        b.components = {
            {"has_hash",    sdsl::size_in_bytes(m_has_hash)   },
            {"hash_rank",   sdsl::size_in_bytes(m_hash_rank)  },
            {"bv_rank0",    sdsl::size_in_bytes(m_bv_rank0)   },
            {"lsep",        sdsl::size_in_bytes(m_lsep)       },
            {"lsep_select", sdsl::size_in_bytes(m_lsep_select)},
        };
        return b;
    }
};

}  // namespace overlay
}  // namespace cltj

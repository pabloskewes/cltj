#pragma once
#include "strategy.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace cltj {
namespace overlay {

/**
 * @brief Scheme C: one step per hashed node of the staircase deleted_before(v)
 *
 * hashed_start[r] is the position in B of the r-th hashed node (increasing) and
 * cum_deleted[r] the labels of hashed nodes up to the end of its block. With
 *   r = lower_bound(hashed_start, v)    (hashed nodes that start before v),
 * v is hashed iff hashed_start[r] == v, its MPHF is the r-th, and
 * plain_start(v) = v - cum_deleted[r - 1] (v if r == 0).
 */
class StepTableOverlay : public OverlayPolicy<StepTableOverlay> {
  public:
    typedef uint64_t size_type;

    static constexpr uint8_t format_version = 4;

  private:
    std::vector<uint32_t> m_hashed_start;
    std::vector<uint32_t> m_cum_deleted;

    size_type rank(size_type v) const {
        return std::lower_bound(m_hashed_start.begin(), m_hashed_start.end(), v) - m_hashed_start.begin();
    }

    static size_t serialize_u32s(const std::vector<uint32_t>& vec, std::ostream& out) {
        uint64_t n = vec.size();
        sdsl::write_member(n, out);
        out.write(reinterpret_cast<const char*>(vec.data()), n * sizeof(uint32_t));
        return sizeof(n) + n * sizeof(uint32_t);
    }

    static void load_u32s(std::vector<uint32_t>& vec, std::istream& in) {
        uint64_t n = 0;
        sdsl::read_member(n, in);
        vec.resize(n);
        in.read(reinterpret_cast<char*>(vec.data()), n * sizeof(uint32_t));
    }

  public:
    void swap(StepTableOverlay& o) {
        m_hashed_start.swap(o.m_hashed_start);
        m_cum_deleted.swap(o.m_cum_deleted);
    }

    bool has_dict(size_type v) const {
        size_type r = rank(v);
        return r < m_hashed_start.size() && m_hashed_start[r] == v;
    }

    size_type dict_idx(size_type v) const { return rank(v); }

    size_type plain_start(size_type v) const {
        size_type r = rank(v);
        return v - (r ? m_cum_deleted[r - 1] : 0);
    }

    void begin(size_type, size_type) {
        m_hashed_start.clear();
        m_cum_deleted.clear();
    }

    void on_hashed_node(size_type new_pos, size_type d) {
        const size_type cum = (m_cum_deleted.empty() ? 0 : m_cum_deleted.back()) + d;
        if (new_pos > std::numeric_limits<uint32_t>::max() || cum > std::numeric_limits<uint32_t>::max())
            throw std::runtime_error("StepTableOverlay: position or deleted count does not fit in 32 bits");
        if (!m_hashed_start.empty() && new_pos <= m_hashed_start.back())
            throw std::runtime_error("StepTableOverlay: hashed nodes must arrive in increasing position");
        m_hashed_start.push_back(static_cast<uint32_t>(new_pos));
        m_cum_deleted.push_back(static_cast<uint32_t>(cum));
    }

    void on_plain_node(size_type, size_type) {}

    void finalize(const sdsl::bit_vector&, uint32_t) {}

    void bind(const sdsl::bit_vector&) {}

    size_type hashed_count() const { return m_hashed_start.size(); }

    //! hashed_start[r] and cum_deleted[r], for r < hashed_count()
    size_type hashed_start(size_type r) const { return m_hashed_start[r]; }

    size_type cum_deleted(size_type r) const { return m_cum_deleted[r]; }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, const std::string& name = "")
        const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        sdsl::structure_tree_node* hs = sdsl::structure_tree::add_child(child, "hashed_start", "std::vector<uint32_t>");
        size_t hs_bytes = serialize_u32s(m_hashed_start, out);
        sdsl::structure_tree::add_size(hs, hs_bytes);
        sdsl::structure_tree_node* cd = sdsl::structure_tree::add_child(child, "cum_deleted", "std::vector<uint32_t>");
        size_t cd_bytes = serialize_u32s(m_cum_deleted, out);
        sdsl::structure_tree::add_size(cd, cd_bytes);
        sdsl::structure_tree::add_size(child, hs_bytes + cd_bytes);
        return hs_bytes + cd_bytes;
    }

    void load(std::istream& in) {
        load_u32s(m_hashed_start, in);
        load_u32s(m_cum_deleted, in);
    }

    OverlaySizeBreakdown get_size_breakdown() const {
        OverlaySizeBreakdown b;
        b.components = {
            {"hashed_start", sizeof(uint64_t) + m_hashed_start.size() * sizeof(uint32_t)},
            {"cum_deleted",  sizeof(uint64_t) + m_cum_deleted.size() * sizeof(uint32_t) },
        };
        return b;
    }
};

}  // namespace overlay
}  // namespace cltj

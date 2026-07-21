#pragma once
#include "strategy.hpp"
#include "rank_support_packed_glgh.hpp"
#include <sdsl/int_vector.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>

namespace cltj {
namespace hashing {

/**
 * @brief Packed GlGh storage strategy.
 *
 * Stores G as a single int_vector<2> where each vertex takes 2 contiguous bits.
 * G[v] = 3 is the unassigned sentinel.
 * B is implicit: B[v] = (G[v] != 3).
 *
 * Space: 2.81 bits/key (same as GlGhStorage, but 1 access per probe instead of 2).
 */
class PackedGlGhStorage : public StorageStrategy<PackedGlGhStorage> {
  private:
    sdsl::int_vector<2> G_;  // G[v] in {0,1,2,3}, 2 bits per vertex
    rank_support_packed_glgh<> rank_B_;  // Rank for B = (G[v] != 3), on-the-fly
    uint32_t m_ = 0;

  public:
    PackedGlGhStorage() = default;

    PackedGlGhStorage(PackedGlGhStorage&& o) noexcept
        : G_(std::move(o.G_)), rank_B_(std::move(o.rank_B_)), m_(o.m_) {
        rank_B_.set_vector(&G_);
    }

    PackedGlGhStorage& operator=(PackedGlGhStorage&& o) noexcept {
        if (this != &o) {
            G_ = std::move(o.G_);
            rank_B_ = std::move(o.rank_B_);
            m_ = o.m_;
            rank_B_.set_vector(&G_);
        }
        return *this;
    }

    uint32_t g_get(uint32_t vertex) const {
        if (vertex >= m_)
            return 3;
        return G_[vertex];
    }

    void g_set(uint32_t vertex, uint32_t value) {
        if (vertex < m_)
            G_[vertex] = value;
    }

    bool is_vertex_occupied(uint32_t vertex) const {
        if (vertex >= m_)
            return false;
        return G_[vertex] != 3;
    }

    void initialize(uint32_t m) {
        m_ = m;
        G_ = sdsl::int_vector<2>(m, 3);
    }

    uint32_t m() const { return m_; }

    void build_rank() { rank_B_ = rank_support_packed_glgh<>(&G_); }

    uint32_t rank(uint32_t position) const {
        if (position > m_)
            position = m_;
        return static_cast<uint32_t>(rank_B_.rank(position));
    }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written_bytes = 0;
        written_bytes += G_.serialize(out, child, "G_");
        written_bytes += rank_B_.serialize(out, child, "rank_B_");
        written_bytes += sdsl::write_member(m_, out, child, "m_");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        G_.load(in);
        rank_B_.load(in, &G_);
        sdsl::read_member(m_, in);
    }

    size_t size_in_bytes() const { return sdsl::size_in_bytes(G_) + rank_B_.size_in_bytes(); }

    StorageSizeBreakdown get_size_breakdown() const {
        StorageSizeBreakdown breakdown;
        breakdown.g_bytes = sdsl::size_in_bytes(G_);
        breakdown.used_pos_bytes = 0;
        breakdown.rank_bytes = rank_B_.size_in_bytes();
        return breakdown;
    }
};

}  // namespace hashing
}  // namespace cltj

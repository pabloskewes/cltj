#pragma once
#include "strategy.hpp"
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>

namespace cltj {
namespace hashing {

/**
 * @brief Baseline storage strategy
 *
 * Stores complete G[0..m-1] as sdsl::int_vector<2>, explicit bitvector B,
 * and rank support. This is the baseline implementation.
 */
class BaselineStorage : public StorageStrategy<BaselineStorage> {
  private:
    sdsl::int_vector<2> G_;  // G array: assignment values {0,1,2,3} per vertex
    sdsl::bit_vector used_positions_;  // Bitvector B: marks positions where G[v] != 3
    sdsl::rank_support_v<1> rank_support_;  // Rank support for O(1) compactification queries

  public:
    BaselineStorage() {}

    uint32_t g_get(uint32_t vertex) const {
        if (vertex >= G_.size())
            return 3;
        return G_[vertex];
    }

    void g_set(uint32_t vertex, uint32_t value) {
        if (vertex < G_.size()) {
            G_[vertex] = static_cast<uint32_t>(value);
        }
    }

    void initialize(uint32_t m) { G_ = sdsl::int_vector<2>(m, 3); }

    uint32_t m() const { return static_cast<uint32_t>(G_.size()); }

    /**
     * @brief Build bitvector B and rank support
     *
     * Constructs B marking all positions where G[v] != 3 and initializes rank support.
     */
    void build_rank(const std::vector<Triple>& triples) {
        // B[v] = 0 iff G[v] = 3
        used_positions_ = sdsl::bit_vector(m(), 0);
        for (uint32_t v = 0; v < m(); v++) {
            if (g_get(v) != 3) {
                used_positions_[v] = 1;
            }
        }
        sdsl::util::init_support(rank_support_, &used_positions_);
    }

    /**
     * @brief Compute rank query for compactification
     *
     * Converts position from [0, m) to compact position in [0, n) using
     * rank support on bitvector B.
     */
    uint32_t rank(uint32_t position) const { return static_cast<uint32_t>(rank_support_(position)); }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written_bytes = 0;
        written_bytes += sdsl::write_member(G_, out, child, "G_");
        written_bytes += used_positions_.serialize(out, child, "used_positions_");
        written_bytes += rank_support_.serialize(out, child, "rank_support_");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        sdsl::read_member(G_, in);
        used_positions_.load(in);
        rank_support_.load(in);
        rank_support_.set_vector(&used_positions_);
    }

    size_t size_in_bytes() const {
        return sdsl::size_in_bytes(G_) + sdsl::size_in_bytes(used_positions_) +
            sdsl::size_in_bytes(rank_support_);
    }

    StorageSizeBreakdown get_size_breakdown() const {
        StorageSizeBreakdown breakdown;
        breakdown.g_bytes = sdsl::size_in_bytes(G_);
        breakdown.used_pos_bytes = sdsl::size_in_bytes(used_positions_);
        breakdown.rank_bytes = sdsl::size_in_bytes(rank_support_);
        return breakdown;
    }
};

}  // namespace hashing
}  // namespace cltj

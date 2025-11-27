#pragma once
#include "strategy.hpp"
#include "trit_packed_array.hpp"
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>
#include <vector>

namespace cltj {
namespace hashing {

/**
 * @brief Packed trit storage strategy
 *
 * Stores only non-3 values from G as packed trits in G'[0..n-1] instead of
 * full G[0..m-1]. Uses explicit bitvector B and rank support for indexing.
 *
 * Theoretically reduces G storage from 2.5n to 1.6n bits.
 */
class PackedTritStorage : public StorageStrategy<PackedTritStorage> {
  private:
    TritPackedArray G_prime_;  // G' array: packed trits storing only non-3 values [0..n-1]
    std::vector<uint32_t> temp_G_;  // Temporary storage during construction [0..m-1]
    sdsl::bit_vector used_positions_;  // Bitvector B: marks positions where G[v] != 3
    sdsl::rank_support_v<1> rank_support_;  // Rank support for O(1) compactification queries
    uint32_t m_;  // Size of G array (number of vertices, approximately 1.23n)
    uint32_t n_;  // Number of non-3 values (size of G')

  public:
    PackedTritStorage() : m_(0), n_(0) {}

    /**
     * @brief Get G value for a vertex
     * 
     * If B[vertex] == 0, returns 3 (unused). Otherwise, retrieves the value
     * from G' using rank(vertex) to map from [0, m) to [0, n).
     * 
     * @param vertex Vertex index in [0, m-1]
     * @return G[vertex] value in {0, 1, 2, 3}
     */
    uint32_t g_get(uint32_t vertex) const {
        if (vertex >= m_) {
            return 3;
        }

        if (used_positions_[vertex] == 0) {
            return 3;
        }

        return G_prime_.get(rank(vertex) - 1);
    }

    void g_set(uint32_t vertex, uint32_t value) {
        if (vertex < temp_G_.size()) {
            temp_G_[vertex] = value;
        }
    }

    void initialize(uint32_t m) {
        m_ = m;
        temp_G_.resize(m, 3);  // Initialize temporary G array with all 3s (unassigned)
    }

    uint32_t m() const { return m_; }

    /**
     * @brief Build bitvector B and pack G' from triples
     *
     * Constructs B marking used positions, then packs only non-3 values
     * into G' using trit packing.
     */
    void build_rank(const std::vector<Triple>& triples) {
        // Build B marking used positions (same as baseline) and initialize rank support
        used_positions_ = sdsl::bit_vector(m_, 0);
        for (const auto& t : triples) {
            uint32_t j = static_cast<uint32_t>((temp_G_[t.v0] + temp_G_[t.v1] + temp_G_[t.v2]) % 3);
            uint32_t pos = (j == 0 ? t.v0 : (j == 1 ? t.v1 : t.v2));
            used_positions_[pos] = 1;
        }

        sdsl::util::init_support(rank_support_, &used_positions_);

        // Count how many non-3 values we have (this is n_)
        n_ = 0;
        for (uint32_t v = 0; v < m_; v++) {
            if (temp_G_[v] != 3) {
                n_++;
            }
        }

        // Pack only non-3 values into G_prime_
        G_prime_.initialize(n_);
        uint32_t g_prime_idx = 0;
        for (uint32_t v = 0; v < m_; v++) {
            if (temp_G_[v] != 3) {
                G_prime_.set(g_prime_idx, static_cast<uint8_t>(temp_G_[v]));
                g_prime_idx++;
            }
        }

        // Clean up: temp_G_ no longer needed
        temp_G_.clear();
        temp_G_.shrink_to_fit();
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
        written_bytes += G_prime_.serialize(out, child, "G_prime_");
        written_bytes += used_positions_.serialize(out, child, "used_positions_");
        written_bytes += rank_support_.serialize(out, child, "rank_support_");
        written_bytes += sdsl::write_member(m_, out, child, "m_");
        written_bytes += sdsl::write_member(n_, out, child, "n_");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        G_prime_.load(in);
        used_positions_.load(in);
        rank_support_.load(in);
        sdsl::read_member(m_, in);
        sdsl::read_member(n_, in);
        rank_support_.set_vector(&used_positions_);
    }

    size_t size_in_bytes() const {
        return G_prime_.size_in_bytes() + sdsl::size_in_bytes(used_positions_) +
            sdsl::size_in_bytes(rank_support_);
    }

    StorageSizeBreakdown get_size_breakdown() const {
        StorageSizeBreakdown breakdown;
        breakdown.g_bytes = G_prime_.size_in_bytes();
        breakdown.used_pos_bytes = sdsl::size_in_bytes(used_positions_);
        breakdown.rank_bytes = sdsl::size_in_bytes(rank_support_);
        return breakdown;
    }
};

}  // namespace hashing
}  // namespace cltj

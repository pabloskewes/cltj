#pragma once
#include "strategy.hpp"
#include "trit_packed_array.hpp"
#include "b_strategy.hpp"
#include <sdsl/bit_vectors.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>
#include <vector>

namespace cltj {
namespace hashing {

/**
 * @brief Packed trit storage strategy
 *
 * Stores only non-3 values from G as packed trits in G'[0..n-1] instead of
 * full G[0..m-1]. Uses configurable bitvector B strategy (explicit or compressed)
 * and rank support for indexing.
 *
 * Theoretically reduces G storage from 2.5n to 1.6n bits.
 *
 * @tparam BStrategy Bitvector strategy (ExplicitBitvector or CompressedBitvector)
 */
template <typename BStrategy = ExplicitBitvector>
class PackedTritStorage : public StorageStrategy<PackedTritStorage<BStrategy>> {
  private:
    TritPackedArray G_prime_;  // G' array: packed trits storing only non-3 values [0..n-1]
    std::vector<uint32_t> temp_G_;  // Temporary storage during construction [0..m-1]
    BStrategy B_;  // Bitvector B strategy: marks positions where G[v] != 3
    uint32_t m_;  // Size of G array (number of vertices, approximately 1.23n)
    uint32_t n_;  // Number of non-3 values (size of G')
    bool construction_complete_;  // Flag: true after build_rank(), false during construction

  public:
    PackedTritStorage() : m_(0), n_(0), construction_complete_(false) {}

    /**
     * @brief Get G value for a vertex
     * 
     * During construction: reads from temp_G_.
     * After build_rank(): reads from G' using B and rank.
     * 
     * @param vertex Vertex index in [0, m-1]
     * @return G[vertex] value in {0, 1, 2, 3}
     */
    uint32_t g_get(uint32_t vertex) const {
        if (vertex >= m_) {
            return 3;
        }

        // During construction: read from temporary array
        if (!construction_complete_) {
            return temp_G_[vertex];
        }

        // After construction: read from packed G' using B and rank
        if (B_[vertex] == 0) {
            // B[v] = 0 => G[v] = 3
            return 3;
        }

        // rank(i) counts 1s in [0, i), so rank(vertex+1) counts 1s in [0, vertex] (inclusive)
        // This gives us the index in G' (1-based), convert to 0-based
        uint32_t idx = B_.rank(vertex + 1);
        return G_prime_.get(idx - 1);
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
     * @brief Build bitvector B and pack G'
     *
     * Steps:
     * 1. Mark all positions where G[v] != 3 in temporary bitvector
     * 2. Build B strategy (explicit or compressed) from temporary bitvector
     * 3. Pack only non-3 values into G' (dense array of n trits)
     * 4. Clean up temporary G array
     *
     * After this, g_get(v) uses: B[v] ? G'[rank(v)-1] : 3
     */
    void build_rank() {
        // B[v] = 0 iff G[v] = 3
        // Build temporary bitvector first
        sdsl::bit_vector temp_bv(m_, 0);
        for (uint32_t v = 0; v < m_; v++) {
            if (temp_G_[v] != 3) {
                temp_bv[v] = 1;
            }
        }

        // Build B strategy from temporary bitvector
        B_.build(temp_bv);

        // Count total number of 1s in B (this is n_)
        n_ = sdsl::util::cnt_one_bits(temp_bv);

        // Pack only non-3 values into G_prime_
        G_prime_.initialize(n_);
        uint32_t g_prime_idx = 0;
        for (uint32_t v = 0; v < m_; v++) {
            if (temp_G_[v] != 3) {
                G_prime_.set(g_prime_idx, static_cast<uint8_t>(temp_G_[v]));
                g_prime_idx++;
            }
        }

        // Sanity check: g_prime_idx should equal n_
        assert(g_prime_idx == n_ && "Mismatch between counted and packed trits");

        // Mark construction as complete and clean up temp_G_
        construction_complete_ = true;
        temp_G_.clear();
        temp_G_.shrink_to_fit();
    }

    /**
     * @brief Compute rank query for compactification
     *
     * Converts position from [0, m) to compact position in [0, n) using
     * rank support on bitvector B.
     */
    uint32_t rank(uint32_t position) const { return B_.rank(position); }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written_bytes = 0;
        written_bytes += G_prime_.serialize(out, child, "G_prime_");
        written_bytes += B_.serialize(out, child, "B_");
        written_bytes += sdsl::write_member(m_, out, child, "m_");
        written_bytes += sdsl::write_member(n_, out, child, "n_");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        G_prime_.load(in);
        B_.load(in);
        sdsl::read_member(m_, in);
        sdsl::read_member(n_, in);
    }

    size_t size_in_bytes() const {
        return G_prime_.size_in_bytes() + B_.size_in_bytes();
    }

    StorageSizeBreakdown get_size_breakdown() const {
        StorageSizeBreakdown breakdown;
        breakdown.g_bytes = G_prime_.size_in_bytes();
        // B strategy includes both bitvector and rank support
        size_t b_total = B_.size_in_bytes();
        // Approximate split: bitvector ~75%, rank ~25% (rough estimate)
        breakdown.used_pos_bytes = (b_total * 3) / 4;
        breakdown.rank_bytes = b_total / 4;
        return breakdown;
    }
};

}  // namespace hashing
}  // namespace cltj

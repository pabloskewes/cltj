#pragma once
#include "strategy.hpp"
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
    // TODO: G' array (packed trits), will be implemented later
    // TODO: B bitvector, explicit for now (will be template later)
    // TODO: Rank support
    uint32_t m_;  // Size of G array (number of vertices)

  public:
    PackedTritStorage() : m_(0) {}

    uint32_t g_get(uint32_t vertex) const {
        // TODO: Implement: return B[v] ? G'[rank(v)] : 3
        return 3;
    }

    void g_set(uint32_t vertex, uint32_t value) {
        // TODO: Implement: store temporarily during construction
    }

    void initialize(uint32_t m) {
        // TODO: Implement: prepare storage for m vertices
        m_ = m;
    }

    uint32_t m() const { return m_; }

    /**
     * @brief Build bitvector B and pack G' from triples
     *
     * Constructs B marking used positions, then packs only non-3 values
     * into G' using trit packing.
     */
    void build_rank(const std::vector<Triple>& triples) {
        // TODO: Implement
        // 1. Build B marking used positions
        // 2. Pack only non-3 values into G'
        // 3. Initialize rank support
    }

    /**
     * @brief Compute rank query for compactification
     *
     * Converts position from [0, m) to compact position in [0, n) using
     * rank support on bitvector B.
     */
    uint32_t rank(uint32_t position) const {
        // TODO: Implement
        return 0;
    }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        // TODO: Implement
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written_bytes = 0;
        // TODO: Serialize G', B, rank support
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        // TODO: Implement
        // Load G', B, rank support
    }

    size_t size_in_bytes() const {
        // TODO: Implement
        return 0;
    }

    StorageSizeBreakdown get_size_breakdown() const {
        // TODO: Implement
        StorageSizeBreakdown breakdown;
        breakdown.g_bytes = 0;
        breakdown.used_pos_bytes = 0;
        breakdown.rank_bytes = 0;
        return breakdown;
    }
};

}  // namespace hashing
}  // namespace cltj

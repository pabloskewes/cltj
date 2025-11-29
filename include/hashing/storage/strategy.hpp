#pragma once
#include "../mphf_types.hpp"
#include <sdsl/bit_vectors.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>
#include <iostream>
#include <vector>
#include <string>

namespace cltj {
namespace hashing {

/**
 * @brief Size breakdown structure for storage components
 */
struct StorageSizeBreakdown {
    size_t g_bytes = 0;  // G array bytes
    size_t used_pos_bytes = 0;  // Bitvector B bytes
    size_t rank_bytes = 0;  // Rank support bytes

    size_t total_bytes() const { return g_bytes + used_pos_bytes + rank_bytes; }
};

/**
 * @brief CRTP base class for storage strategies
 *
 * Provides unified interface for G array storage and rank/compactification.
 *
 * G array: stores assignment values {0, 1, 2, 3} per vertex in [0, m-1].
 * Value 3 indicates unassigned vertices.
 *
 * Rank/compactification: converts positions from [0, m) to [0, n) using
 * a bitvector B marking used positions and rank support for O(1) queries.
 *
 * @tparam Derived The concrete storage implementation (e.g., BaselineStorage, PackedTritStorage)
 */
template <typename Derived>
class StorageStrategy {
  protected:
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }

  public:
    /**
     * @brief Get the G value for a vertex
     * @param vertex Vertex index in [0, m-1]
     * @return G[vertex] value in {0, 1, 2, 3}, where 3 means unassigned/unused
     */
    uint32_t g_get(uint32_t vertex) const { return derived().g_get(vertex); }

    /**
     * @brief Set the G value for a vertex
     * @param vertex Vertex index in [0, m-1]
     * @param value Value in {0, 1, 2, 3} to assign
     */
    void g_set(uint32_t vertex, uint32_t value) { derived().g_set(vertex, value); }

    /**
     * @brief Initialize storage for m vertices
     *
     * Prepares storage structures for m vertices. Specific initialization
     * behavior depends on the storage strategy implementation.
     *
     * @param m Size of the G array (number of vertices, approximately 1.23n)
     */
    void initialize(uint32_t m) { derived().initialize(m); }

    /**
     * @brief Get the size of the G array (number of vertices, m)
     * @return Number of vertices in [0, m-1]
     */
    uint32_t m() const { return derived().m(); }

    /**
     * @brief Build rank support
     *
     * Prepares rank/compactification structures after G values have been assigned.
     * Constructs bitvector B marking all positions where G[v] != 3, and initializes
     * rank support for O(1) compactification queries.
     *
     * Specific behavior depends on the storage strategy: may construct explicit
     * or compressed bitvectors, rank metadata, or additional structures like G'.
     */
    void build_rank() { derived().build_rank(); }

    /**
     * @brief Compute rank query for compactification
     *
     * Converts position from [0, m) to compact position in [0, n).
     *
     * @param position Position in [0, m)
     * @return Compact position in [0, n)
     */
    uint32_t rank(uint32_t position) const { return derived().rank(position); }

    /**
     * @brief Serialize storage to output stream
     * @param out Output stream
     * @param v Structure tree node for visualization
     * @param name Name for the structure tree node
     * @return Number of bytes written
     */
    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        return derived().serialize(out, v, name);
    }

    /**
     * @brief Load storage from input stream
     * @param in Input stream
     */
    void load(std::istream& in) { derived().load(in); }

    /**
     * @brief Get the total size in bytes of the storage
     * @return Size in bytes (used for bits-per-key calculations)
     */
    size_t size_in_bytes() const { return derived().size_in_bytes(); }

    /**
     * @brief Get detailed size breakdown of storage components
     * @return StorageSizeBreakdown with individual component sizes
     */
    StorageSizeBreakdown get_size_breakdown() const { return derived().get_size_breakdown(); }
};

}  // namespace hashing
}  // namespace cltj

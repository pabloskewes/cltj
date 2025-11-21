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
     * @brief Build rank support from triples
     *
     * Prepares rank/compactification structures after G values have been assigned.
     * For each triple (v0, v1, v2), computes j = (G[v0] + G[v1] + G[v2]) mod 3
     * to determine the winning vertex position.
     *
     * Specific behavior depends on the storage strategy: may construct explicit
     * or compressed bitvectors, rank metadata, or additional structures like G'.
     *
     * @param triples Vector of triples representing hyperedges
     */
    void build_rank(const std::vector<Triple>& triples) { derived().build_rank(triples); }

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
};

}  // namespace hashing
}  // namespace cltj

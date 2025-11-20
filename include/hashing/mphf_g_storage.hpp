#pragma once
#include "mphf_types.hpp"
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>
#include <iostream>
#include <vector>
#include <string>

namespace cltj {
namespace hashing {

/**
 * @brief CRTP base class for G array storage strategies
 *
 * G is the assignment array storing values in {0, 1, 2, 3} for each vertex in [0, m-1].
 * Value 3 indicates unassigned/unused vertices.
 *
 * During construction, triples are processed in reverse peeling order, assigning
 * G[v_j] = (j - G[v0] - G[v1] - G[v2]) mod 3 for the first unvisited vertex.
 *
 * During hashing, j = (G[v0] + G[v1] + G[v2]) mod 3 selects the winning vertex
 * h(x) = v_j, ensuring each key maps uniquely.
 *
 * @tparam Derived The concrete storage implementation (e.g., FullArrayStorage, PackedTritStorage)
 */
template <typename Derived>
class GStorage {
  protected:
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }

  public:
    /**
     * @brief Get the G value for a vertex
     * @param vertex Vertex index in [0, m-1]
     * @return G[vertex] value in {0, 1, 2, 3}, where 3 means unassigned/unused
     */
    uint32_t get(uint32_t vertex) const { return derived().get(vertex); }

    /**
     * @brief Set the G value for a vertex
     * @param vertex Vertex index in [0, m-1]
     * @param value Value in {0, 1, 2, 3} to assign
     */
    void set(uint32_t vertex, uint32_t value) { derived().set(vertex, value); }

    /**
     * @brief Build G array from peeling order
     *
     * Processes triples in reverse peeling order, assigning G[v_j] for the first unvisited
     * vertex in each triple.
     *
     * @param peeling_order Triples sorted in peeling order (topological sort)
     * @param m Size of the G array (number of vertices, approximately 1.23n)
     */
    void build(const std::vector<Triple>& peeling_order, uint32_t m) { derived().build(peeling_order, m); }

    /**
     * @brief Serialize G array to output stream
     * @param out Output stream
     * @param v Structure tree node for visualization
     * @param name Name for the structure tree node
     * @return Number of bytes written
     */
    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        return derived().serialize(out, v, name);
    }

    /**
     * @brief Load G array from input stream
     * @param in Input stream
     */
    void load(std::istream& in) { derived().load(in); }

    /**
     * @brief Get the size in bytes of the G array storage
     * @return Size in bytes (used for bits-per-key calculations)
     */
    size_t size_in_bytes() const { return derived().size_in_bytes(); }
};

}  // namespace hashing
}  // namespace cltj

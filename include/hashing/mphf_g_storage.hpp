#pragma once
#include "mphf_types.hpp"
#include <sdsl/bit_vectors.hpp>
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
     * @brief Initialize G array storage for m vertices
     *
     * Allocates storage and initializes all positions to 3 (unassigned).
     *
     * @param m Size of the G array (number of vertices, approximately 1.23n)
     */
    void initialize(uint32_t m) { derived().initialize(m); }

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

/**
 * @brief Full array storage for G
 *
 * Stores complete G[0..m-1] as sdsl::int_vector<2>, using 2 bits per value.
 * This is the baseline implementation that stores all m positions explicitly.
 */
class FullArrayStorage : public GStorage<FullArrayStorage> {
  private:
    sdsl::int_vector<2> G_;
    uint32_t m_;

  public:
    FullArrayStorage() : m_(0) {}

    uint32_t get(uint32_t vertex) const {
        if (vertex >= m_)
            return 3;
        return G_[vertex];
    }

    void set(uint32_t vertex, uint32_t value) {
        if (vertex < m_) {
            G_[vertex] = static_cast<uint32_t>(value);
        }
    }

    void initialize(uint32_t m) {
        m_ = m;
        G_ = sdsl::int_vector<2>(m_, 3);
    }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written_bytes = 0;
        written_bytes += sdsl::write_member(G_, out, child, "G_");
        written_bytes += sdsl::write_member(m_, out, child, "m_");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        sdsl::read_member(G_, in);
        sdsl::read_member(m_, in);
    }

    size_t size_in_bytes() const { return sdsl::size_in_bytes(G_) + sizeof(m_); }

    const sdsl::int_vector<2>& get_g() const { return G_; }
    uint32_t m() const { return m_; }
};

}  // namespace hashing
}  // namespace cltj

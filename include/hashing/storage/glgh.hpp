#pragma once
#include "strategy.hpp"
#include "rank_support_glgh.hpp"
#include <sdsl/bit_vectors.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>

namespace cltj {
namespace hashing {

/**
 * @brief GlGh storage strategy (alternative method from book)
 *
 * Stores G as two bitvectors Gl and Gh, where G[v] = 2*Gh[v] + Gl[v].
 * Computes B on-the-fly: B[v] = ~(Gl[v] & Gh[v]) (since G[v]=3 when both bits are 1).
 * Only stores rank metadata (superblocks/blocks), not B explicitly.
 *
 * Expected space: ~2.57 bits/key
 * - Gl + Gh: 2m ≈ 2.5n bits
 * - Rank metadata: o(m) ≈ 0.07n bits
 */
class GlGhStorage : public StorageStrategy<GlGhStorage> {
  private:
    sdsl::bit_vector Gl_;  // Lower bit of G: G[v] mod 2
    sdsl::bit_vector Gh_;  // Higher bit of G: (G[v] >> 1) & 1
    rank_support_glgh<> rank_B_;  // Rank support for B, computed on-the-fly from Gl_ and Gh_
    uint32_t m_;  // Size of G array (number of vertices, approximately 1.23n)

  public:
    GlGhStorage() : m_(0) {}

    /**
     * @brief Get G value for a vertex
     * 
     * G[v] = 2*Gh[v] + Gl[v]
     * 
     * @param vertex Vertex index in [0, m-1]
     * @return G[vertex] value in {0, 1, 2, 3}
     */
    uint32_t g_get(uint32_t vertex) const {
        if (vertex >= m_) {
            return 3;
        }
        return 2 * Gh_[vertex] + Gl_[vertex];
    }

    /**
     * @brief Set G value for a vertex
     * 
     * Sets the value of G[vertex] to the given value.
     * 
     * @param vertex Vertex index in [0, m-1]
     * @param value Value in {0, 1, 2, 3} to assign
     */
    void g_set(uint32_t vertex, uint32_t value) {
        if (vertex < m_) {
            Gl_[vertex] = value & 1;
            Gh_[vertex] = (value >> 1) & 1;
        }
    }

    bool is_vertex_occupied(uint32_t vertex) const {
        if (vertex >= m_) {
            return false;
        }
        // B[v] = ~(Gl[v] & Gh[v]); occupied iff B[v] = 1.
        return !(Gl_[vertex] & Gh_[vertex]);
    }

    /**
     * @brief Initialize storage for m vertices
     * 
     * Initializes Gl_ and Gh_ with all bits set to 1 (representing G[v] = 3).
     * 
     * @param m Size of G array (number of vertices)
     */
    void initialize(uint32_t m) {
        m_ = m;
        Gl_ = sdsl::bit_vector(m, 1);  // All 1s = G[v] = 3 (unassigned)
        Gh_ = sdsl::bit_vector(m, 1);  // All 1s = G[v] = 3 (unassigned)
    }

    uint32_t m() const { return m_; }

    /**
     * @brief Build rank metadata
     *
     * Computes rank metadata (superblocks/blocks) for B computed on-the-fly.
     * B[v] = ~(Gl[v] & Gh[v]) is computed dynamically during rank queries.
     * 
     */
    void build_rank() {
        rank_B_ = rank_support_glgh<>(&Gl_, &Gh_);
    }

    /**
     * @brief Compute rank query for compactification
     *
     * Computes rank on B which is computed on-the-fly from Gl and Gh.
     * B[v] = ~(Gl[v] & Gh[v])
     * 
     * @param position Position in [0, m)
     * @return Compact position in [0, n)
     * 
     */
    uint32_t rank(uint32_t position) const {
        if (position > m_) {
            position = m_;
        }
        return static_cast<uint32_t>(rank_B_.rank(position));
    }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written_bytes = 0;
        written_bytes += Gl_.serialize(out, child, "Gl_");
        written_bytes += Gh_.serialize(out, child, "Gh_");
        written_bytes += rank_B_.serialize(out, child, "rank_B_");
        written_bytes += sdsl::write_member(m_, out, child, "m_");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        Gl_.load(in);
        Gh_.load(in);
        rank_B_.load(in, &Gl_);
        rank_B_.set_gh(&Gh_);
        sdsl::read_member(m_, in);
    }

    size_t size_in_bytes() const {
        return sdsl::size_in_bytes(Gl_) + sdsl::size_in_bytes(Gh_) + rank_B_.size_in_bytes();
    }

    StorageSizeBreakdown get_size_breakdown() const {
        StorageSizeBreakdown breakdown;
        breakdown.g_bytes = sdsl::size_in_bytes(Gl_) + sdsl::size_in_bytes(Gh_);
        breakdown.used_pos_bytes = 0;  // B is not stored explicitly
        breakdown.rank_bytes = rank_B_.size_in_bytes();
        return breakdown;
    }
};

}  // namespace hashing
}  // namespace cltj

#pragma once
#include <sdsl/bit_vectors.hpp>
#include <sdsl/structure_tree.hpp>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace cltj {
namespace overlay {

/**
 * @brief Size breakdown of a translation policy, one entry per structure it stores
 */
struct OverlaySizeBreakdown {
    std::vector<std::pair<std::string, size_t>> components;

    size_t total_bytes() const {
        size_t total = 0;
        for (const auto& c : components)
            total += c.second;
        return total;
    }
};

/**
 * @brief CRTP base class for the translation policies of compact_metatrie_hash
 *
 * Once the labels of hashed nodes leave L, B and L stop being parallel. A policy
 * answers, for the node at LOUDS position v:
 *   - has_dict(v): whether v has an MPHF (B_h in the notation),
 *   - dict_idx(v): r, the index of its MPHF among the hashed nodes,
 *   - plain_start(v): v - deleted_before(v), where its block starts in L_plain.
 *
 * plain_start(v) is defined for every node, hashed or not: for a hashed node it is
 * where the next non-hashed block starts (|L_plain| if there is none).
 *
 * Build protocol: begin(), then exactly one on_hashed_node() or on_plain_node() per
 * node in increasing position order, then finalize() with the final B. bind() has to
 * be called whenever B moves (move, swap, load), because a policy may keep supports
 * over it.
 *
 * @tparam Derived The concrete policy (e.g., SepBitmapOverlay)
 */
template <typename Derived>
class OverlayPolicy {
  protected:
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }

  public:
    typedef uint64_t size_type;

    bool has_dict(size_type v) const { return derived().has_dict(v); }

    size_type dict_idx(size_type v) const { return derived().dict_idx(v); }

    size_type plain_start(size_type v) const { return derived().plain_start(v); }

    /**
     * @brief Start a build
     * @param louds_len |B|, sentinel included
     * @param plain_len |L_plain|
     */
    void begin(size_type louds_len, size_type plain_len) { derived().begin(louds_len, plain_len); }

    void on_hashed_node(size_type new_pos, size_type d) { derived().on_hashed_node(new_pos, d); }

    void on_plain_node(size_type new_pos, size_type d) { derived().on_plain_node(new_pos, d); }

    void finalize(const sdsl::bit_vector& bv, uint32_t threshold) { derived().finalize(bv, threshold); }

    void bind(const sdsl::bit_vector& bv) { derived().bind(bv); }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        return derived().serialize(out, v, name);
    }

    void load(std::istream& in) { derived().load(in); }

    OverlaySizeBreakdown get_size_breakdown() const { return derived().get_size_breakdown(); }
};

}  // namespace overlay
}  // namespace cltj

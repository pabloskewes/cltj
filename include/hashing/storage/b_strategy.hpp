#pragma once
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/rrr_vector.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>
#include <iostream>

namespace cltj {
namespace hashing {

/**
 * @brief CRTP base class for bitvector strategies
 *
 * Implements common methods that work the same for all strategies.
 * Derived classes only need to implement build().
 */
template <typename Derived>
struct BitvectorStrategyBase {
    uint32_t operator[](uint32_t i) const {
        const auto& self = static_cast<const Derived&>(*this);
        return self.bv[i];
    }

    uint32_t rank(uint32_t pos) const {
        const auto& self = static_cast<const Derived&>(*this);
        return static_cast<uint32_t>(self.rank_support(pos));
    }

    size_t size() const {
        const auto& self = static_cast<const Derived&>(*this);
        return self.bv.size();
    }

    size_t size_in_bytes() const {
        const auto& self = static_cast<const Derived&>(*this);
        return sdsl::size_in_bytes(self.bv) + sdsl::size_in_bytes(self.rank_support);
    }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        const auto& self = static_cast<const Derived&>(*this);
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(self));
        size_t written_bytes = 0;
        written_bytes += self.bv.serialize(out, child, "bv");
        written_bytes += self.rank_support.serialize(out, child, "rank_support");
        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream& in) {
        auto& self = static_cast<Derived&>(*this);
        self.bv.load(in);
        self.rank_support.load(in);
        self.rank_support.set_vector(&self.bv);
    }
};

/**
 * @brief Explicit bitvector strategy (uncompressed)
 *
 * Uses sdsl::bit_vector with rank_support_v<1>.
 * Fast queries, larger space: ~1.25n bits.
 */
struct ExplicitBitvector : public BitvectorStrategyBase<ExplicitBitvector> {
    using bitvector_type = sdsl::bit_vector;
    using rank_support_type = sdsl::rank_support_v<1>;

    bitvector_type bv;
    rank_support_type rank_support;

    void build(const sdsl::bit_vector& temp_bv) {
        bv = temp_bv;
        sdsl::util::init_support(rank_support, &bv);
    }
};

/**
 * @brief Compressed bitvector strategy (RRR compression)
 *
 * Uses sdsl::rrr_vector<> with rank_1_type.
 * Slower queries, smaller space: ~0.86n bits.
 */
struct CompressedBitvector : public BitvectorStrategyBase<CompressedBitvector> {
    using bitvector_type = sdsl::rrr_vector<>;
    using rank_support_type = sdsl::rrr_vector<>::rank_1_type;

    bitvector_type bv;
    rank_support_type rank_support;

    void build(const sdsl::bit_vector& temp_bv) {
        bv = bitvector_type(temp_bv);
        sdsl::util::init_support(rank_support, &bv);
    }
};

}  // namespace hashing
}  // namespace cltj

#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <limits>

#include <sdsl/bits.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>

#include "mphf_types.hpp"

namespace cltj {
namespace hashing {
namespace policies {

struct KeyInitContext {
    size_t n;
    const std::array<uint64_t, 3>& primes;
    const std::array<uint64_t, 3>& multipliers;
    const std::array<uint64_t, 3>& biases;
    const std::array<uint64_t, 3>& segment_starts;
    uint32_t max_mixed_key;  // upper bound for the post-mixer key domain used by the policy
};

struct NoKey {
    static constexpr bool supports_contains = false;
    static constexpr bool needs_input_stats = false;

    void init(const KeyInitContext&) {}

    void store(size_t, uint32_t, const Triple&, int) {}

    size_t size_in_bytes() const { return 0; }

    // Serialization interface: NoKey has no payload to persist.
    size_t serialize(std::ostream&, sdsl::structure_tree_node*, const std::string&) const { return 0; }
    void load(std::istream&) {}

    // Context binding: No-op, NoKey does not depend on hash parameters.
    void bind_context(const KeyInitContext&) {}
};

struct FullKey {
    static constexpr bool supports_contains = true;
    static constexpr bool needs_input_stats = false;

    std::vector<uint32_t> keys_;

    void init(const KeyInitContext& ctx) { keys_.assign(ctx.n, 0); }

    void store(size_t idx, uint32_t key, const Triple&, int) { keys_[idx] = key; }

    bool verify(size_t idx, uint32_t key, int) const { return keys_[idx] == key; }

    size_t size_in_bytes() const { return sizeof(uint32_t) * keys_.size(); }

    // Context binding: FullKey does not depend on hash parameters.
    void bind_context(const KeyInitContext&) {}

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written = 0;

        uint64_t sz = static_cast<uint64_t>(keys_.size());
        written += sdsl::write_member(sz, out, child, "size");
        if (sz > 0) {
            out.write(
                reinterpret_cast<const char*>(keys_.data()),
                static_cast<std::streamsize>(sz * sizeof(uint32_t))
            );
            written += sz * sizeof(uint32_t);
        }

        sdsl::structure_tree::add_size(child, written);
        return written;
    }

    void load(std::istream& in) {
        uint64_t sz = 0;
        sdsl::read_member(sz, in);
        keys_.resize(static_cast<size_t>(sz));
        if (sz > 0) {
            in.read(
                reinterpret_cast<char*>(keys_.data()), static_cast<std::streamsize>(sz * sizeof(uint32_t))
            );
        }
    }
};

struct QuotientKey {
    static constexpr bool supports_contains = true;
    static constexpr bool needs_input_stats = true;  // Needs max_mixed_key to compute optimal width

    // Persisted values
    sdsl::int_vector<> quotients_;  // q_j(y) = floor(y / p_j) for each mixed key y

    // Cached parameters (copied from KeyInitContext in init()).
    std::array<uint64_t, 3> primes_{};
    std::array<uint64_t, 3> multipliers_{};
    std::array<uint64_t, 3> biases_{};
    std::array<uint64_t, 3> segment_starts_{};

    void bind_context(const KeyInitContext& ctx) {
        // Cache parameters; the quotienting logic defines how they are used.
        primes_ = ctx.primes;
        multipliers_ = ctx.multipliers;
        biases_ = ctx.biases;
        segment_starts_ = ctx.segment_starts;
    }

    void init(const KeyInitContext& ctx) {
        bind_context(ctx);

        // Quotients are computed over the post-mixer domain y, so the width bound
        // must use the largest mixed key that can reach this policy.
        uint64_t p_min = std::min({primes_[0], primes_[1], primes_[2]});
        uint64_t q_max;
        if (ctx.max_mixed_key > 0) {
            q_max = ctx.max_mixed_key / p_min;
        } else {
            q_max = std::numeric_limits<uint32_t>::max() / p_min;
        }
        uint8_t quotient_width = (q_max == 0) ? 1 : static_cast<uint8_t>(sdsl::bits::hi(q_max) + 1);
        quotients_ = sdsl::int_vector<>(ctx.n, 0, quotient_width);
    }

    void store(size_t idx, uint32_t mixed_key, const Triple&, int which_h) {
        assert(idx < quotients_.size());
        assert(which_h >= 0 && which_h <= 2);

        const size_t j = static_cast<size_t>(which_h);
        const uint64_t p = primes_[j];
        const uint64_t q = mixed_key / p;  // q_j(y) = floor(y / p_j)

        quotients_[idx] = q;
    }

    bool verify(size_t idx, uint32_t mixed_key, int which_h) const {
        if (idx >= quotients_.size())
            return false;
        if (which_h < 0 || which_h > 2)
            return false;

        const size_t j = static_cast<size_t>(which_h);
        const uint64_t p = primes_[j];

        // Check quotient: by biyectivity, B[v]=1 filter already guarantees rest matches.
        const uint64_t q_stored = quotients_[idx];
        const uint64_t q_query = mixed_key / p;

        return q_query == q_stored;
    }

    size_t size_in_bytes() const { return sdsl::size_in_bytes(quotients_); }

    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v, const std::string& name) const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written = 0;
        written += quotients_.serialize(out, child, "quotients_");
        sdsl::structure_tree::add_size(child, written);
        return written;
    }

    void load(std::istream& in) { quotients_.load(in); }
};

}  // namespace policies
}  // namespace hashing
}  // namespace cltj

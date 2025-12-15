#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <sdsl/int_vector.hpp>

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
};

struct NoKey {
    static constexpr bool supports_contains = false;

    void init(const KeyInitContext&) {}

    void store(size_t, uint64_t, const Triple&, int) {}

    size_t size_in_bytes() const { return 0; }
};

struct FullKey {
    static constexpr bool supports_contains = true;

    std::vector<uint64_t> keys_;

    void init(const KeyInitContext& ctx) { keys_.assign(ctx.n, 0); }

    void store(size_t idx, uint64_t key, const Triple&, int) { keys_[idx] = key; }

    bool verify(size_t idx, uint64_t key, const Triple&, int) const { return keys_[idx] == key; }

    size_t size_in_bytes() const { return sizeof(uint64_t) * keys_.size(); }
};

struct QuotientKey {
    static constexpr bool supports_contains = true;

    sdsl::int_vector<> quotients_;

    // Cached parameters (copied from KeyInitContext in init()).
    std::array<uint64_t, 3> primes_{};
    std::array<uint64_t, 3> multipliers_{};
    std::array<uint64_t, 3> biases_{};
    std::array<uint64_t, 3> a_inv_{};

    void init(const KeyInitContext& ctx) {
        // Cache parameters; the quotienting logic defines how they are used.
        primes_ = ctx.primes;
        multipliers_ = ctx.multipliers;
        biases_ = ctx.biases;
        // Allocate quotients_ with a placeholder bit width for now.
        if (ctx.n > 0) {
            quotients_ = sdsl::int_vector<>(ctx.n, 0, 16);  // TODO: refine bit width once m_j is fixed.
        } else {
            quotients_ = sdsl::int_vector<>();
        }
    }

    void store(size_t, uint64_t, const Triple&, int) {
        // TODO: implement quotient computation and storage.
    }

    bool verify(size_t, uint64_t, const Triple&, int) const {
        // TODO: implement quotient-based membership check.
        return true;
    }

    size_t size_in_bytes() const { return sdsl::size_in_bytes(quotients_); }
};

}  // namespace policies
}  // namespace hashing
}  // namespace cltj

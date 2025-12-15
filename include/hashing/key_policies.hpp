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
        // We allocate enough bits to store the full quotient
        // q_j(x) = floor(H_j(x) / p_j), where H_j behaves like a 64-bit value and
        // p_j is around 2^25, so ~39–40 bits are sufficient.
        constexpr uint8_t quotient_width = 40;
        quotients_ = (ctx.n > 0) ? sdsl::int_vector<>(ctx.n, 0, quotient_width) : sdsl::int_vector<>();
    }

    void store(size_t idx, uint64_t key, const Triple&, int which_h) {
        assert(idx < quotients_.size());
        assert(which_h >= 0 && which_h <= 2);

        const size_t j = static_cast<size_t>(which_h);
        const uint64_t p = primes_[j];
        const uint64_t a = multipliers_[j];
        const uint64_t b = biases_[j];

        // Compute H_j(x) = a_j * key + b_j in extended precision (conceptually 64-bit hash).
        const __uint128_t H =
            static_cast<__uint128_t>(a) * static_cast<__uint128_t>(key) + static_cast<__uint128_t>(b);
        const uint64_t q = static_cast<uint64_t>(H / p);

        quotients_[idx] = q;
    }

    bool verify(size_t idx, uint64_t key, const Triple&, int which_h) const {
        if (idx >= quotients_.size())
            return false;
        if (which_h < 0 || which_h > 2)
            return false;

        const size_t j = static_cast<size_t>(which_h);
        const uint64_t p = primes_[j];
        const uint64_t a = multipliers_[j];
        const uint64_t b = biases_[j];

        const __uint128_t H =
            static_cast<__uint128_t>(a) * static_cast<__uint128_t>(key) + static_cast<__uint128_t>(b);
        const uint64_t expected_q = static_cast<uint64_t>(H / p);

        return quotients_[idx] == expected_q;
    }

    size_t size_in_bytes() const { return sdsl::size_in_bytes(quotients_); }
};

}  // namespace policies
}  // namespace hashing
}  // namespace cltj

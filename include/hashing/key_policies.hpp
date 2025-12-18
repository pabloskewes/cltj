#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <limits>

#include <sdsl/bits.hpp>
#include <sdsl/int_vector.hpp>

#include "mphf_types.hpp"
#include "mphf_utils.hpp"

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

    sdsl::int_vector<> quotients_;  // q_j(x) = floor(H_j(x) / p_j) for each key x
    std::array<uint64_t, 3> inv_multipliers_{};  // (a_j^{-1} mod p_j) for each hash family j

    // Cached parameters (copied from KeyInitContext in init()).
    std::array<uint64_t, 3> primes_{};
    std::array<uint64_t, 3> multipliers_{};
    std::array<uint64_t, 3> biases_{};
    std::array<uint64_t, 3> segment_starts_{};

    void init(const KeyInitContext& ctx) {
        // Cache parameters; the quotienting logic defines how they are used.
        primes_ = ctx.primes;
        multipliers_ = ctx.multipliers;
        biases_ = ctx.biases;
        segment_starts_ = ctx.segment_starts;

        // Precompute modular inverses a_j^{-1} modulo p_j for each hash family j.
        for (size_t j = 0; j < 3; ++j) {
            const uint64_t p = primes_[j];
            const uint64_t a = multipliers_[j] % p;
            inv_multipliers_[j] = mod_inverse(a, p);
        }

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
        const uint64_t q = key / p;  // q_j(x) = floor(x / p_j)

        quotients_[idx] = q;
    }

    bool verify(size_t idx, uint64_t key, const Triple& triple, int which_h) const {
        if (idx >= quotients_.size())
            return false;
        if (which_h < 0 || which_h > 2)
            return false;

        const size_t j = static_cast<size_t>(which_h);
        const uint64_t p = primes_[j];
        const uint64_t a_inv = inv_multipliers_[j];
        const uint64_t b = biases_[j];

        // First filter: quotient must match exactly.
        const uint64_t q_stored = quotients_[idx];
        const uint64_t q_query = key / p;
        if (q_query != q_stored)
            return false;

        // Second filter: reconstructed remainder r_rec must match key % p.
        const uint64_t v_global = triple.v(which_h);
        const uint64_t d = segment_starts_[j];
        assert(v_global >= d);
        const uint64_t v_local = v_global - d;

        // Compute r_rec = a_j^{-1} * (v_local - b_j) mod p_j.
        uint64_t diff;
        if (v_local >= b) {
            diff = v_local - b;
        } else {
            diff = v_local + p - b;
        }
        const uint64_t r_rec = mod_mul(diff % p, a_inv, p);
        const uint64_t r_query = key % p;

        return r_query == r_rec;
    }

    size_t size_in_bytes() const { return sdsl::size_in_bytes(quotients_); }
};

}  // namespace policies
}  // namespace hashing
}  // namespace cltj

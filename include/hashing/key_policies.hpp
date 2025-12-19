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

    // Serialization interface: NoKey has no payload to persist.
    size_t serialize(std::ostream&, sdsl::structure_tree_node*, const std::string&) const { return 0; }
    void load(std::istream&) {}

    // Context binding: No-op, NoKey does not depend on hash parameters.
    void bind_context(const KeyInitContext&) {}
};

struct FullKey {
    static constexpr bool supports_contains = true;

    std::vector<uint64_t> keys_;

    void init(const KeyInitContext& ctx) { keys_.assign(ctx.n, 0); }

    void store(size_t idx, uint64_t key, const Triple&, int) { keys_[idx] = key; }

    bool verify(size_t idx, uint64_t key, const Triple&, int) const { return keys_[idx] == key; }

    size_t size_in_bytes() const { return sizeof(uint64_t) * keys_.size(); }

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
                static_cast<std::streamsize>(sz * sizeof(uint64_t))
            );
            written += sz * sizeof(uint64_t);
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
                reinterpret_cast<char*>(keys_.data()), static_cast<std::streamsize>(sz * sizeof(uint64_t))
            );
        }
    }
};

struct QuotientKey {
    static constexpr bool supports_contains = true;

    sdsl::int_vector<> quotients_;  // q_j(x) = floor(x / p_j) for each key x
    std::array<uint64_t, 3> inv_multipliers_{};  // (a_j^{-1} mod p_j) for each hash family j

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

        // Precompute modular inverses a_j^{-1} modulo p_j for each hash family j.
        for (size_t j = 0; j < 3; ++j) {
            const uint64_t p = primes_[j];
            const uint64_t a = multipliers_[j] % p;
            inv_multipliers_[j] = mod_inverse(a, p);
        }
    }

    void init(const KeyInitContext& ctx) {
        bind_context(ctx);

        // p_min it's the worst case scenario for the quotient, so we use it to compute the width.
        uint64_t p_min = std::min({primes_[0], primes_[1], primes_[2]});
        uint64_t q_max = std::numeric_limits<uint64_t>::max() / p_min;  // 2^64 / p_min
        uint8_t quotient_width = static_cast<uint8_t>(sdsl::bits::hi(q_max) + 1);
        quotients_ = sdsl::int_vector<>(ctx.n, 0, quotient_width);
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

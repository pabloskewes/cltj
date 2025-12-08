#pragma once

#include <array>
#include <cstdint>
#include <vector>

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
};

struct FullKey {
    static constexpr bool supports_contains = true;

    std::vector<uint64_t> keys_;

    void init(const KeyInitContext& ctx) { keys_.assign(ctx.n, 0); }

    void store(size_t idx, uint64_t key, const Triple&, int) { keys_[idx] = key; }

    bool verify(size_t idx, uint64_t key, const Triple&, int) const { return keys_[idx] == key; }
};

}  // namespace policies
}  // namespace hashing
}  // namespace cltj

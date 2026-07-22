#pragma once

#include <array>
#include <cstdint>

#include <fastmod.h>

namespace cltj {
namespace hashing {
namespace policies {

struct NativeMod {
    void bind(const std::array<uint64_t, 3>&) {}

    uint64_t mod(uint64_t v, int, uint64_t p) const { return v % p; }
};

struct FastMod {
    std::array<__uint128_t, 3> M_{};

    void bind(const std::array<uint64_t, 3>& primes) {
        for (int k = 0; k < 3; ++k) {
            M_[k] = fastmod::computeM_u64(primes[k]);
        }
    }

    uint64_t mod(uint64_t v, int k, uint64_t p) const {
        return fastmod::fastmod_u64(v, M_[static_cast<size_t>(k)], p);
    }
};

}  // namespace policies
}  // namespace hashing
}  // namespace cltj

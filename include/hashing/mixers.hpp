#pragma once

#include <cstdint>

namespace cltj {
namespace hashing {

/**
 * @brief SplitMix64 hash mixer for seed diversification.
 *
 * Ensures that numerically close seeds produce completely different outputs
 * when initializing PRNGs.
 *
 * Written in 2015 by Sebastiano Vigna (vigna@acm.org), public domain.
 * Reference: https://github.com/svaarala/duktape/blob/master/misc/splitmix64.c
 */
inline constexpr uint64_t splitmix64(uint64_t x) noexcept {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

/**
 * @brief Bijective 32-bit integer mixer used before the MPHF's linear hashes.
 *
 * Reversible 32-bit integer hash by Thomas Mueller.
 * Reference: https://stackoverflow.com/a/12996028
 */
inline constexpr uint32_t premix32(uint32_t x) noexcept {
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = ((x >> 16) ^ x) * 0x45d9f3bU;
    x = (x >> 16) ^ x;
    return x;
}

/**
 * @brief Inverse of premix32.
 *
 * Uses the modular inverse of 0x45d9f3b modulo 2^32.
 */
inline constexpr uint32_t unpremix32(uint32_t x) noexcept {
    x = (x >> 16) ^ x;
    x *= 0x119de1f3U;
    x = (x >> 16) ^ x;
    x *= 0x119de1f3U;
    x = (x >> 16) ^ x;
    return x;
}

}  // namespace hashing
}  // namespace cltj

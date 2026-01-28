#pragma once
#include <cstdint>
#include <vector>
#include <cassert>

namespace cltj {
namespace hashing {

/**
 * @brief Efficient modular multiplication. Complexity: O(1)
 * @param a First operand
 * @param b Second operand
 * @param mod Modulus
 * @return a * b % mod
 */
inline uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t mod) {
    return (static_cast<__uint128_t>(a) * b) % mod;
}

/**
 * @brief Modular exponentiation. Complexity: O(log(exp))
 * @param base Base of the exponentiation
 * @param exp Exponent of the exponentiation
 * @param mod Modulus
 * @return base^exp % mod
 */
inline uint64_t mod_pow(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1)
            result = mod_mul(result, base, mod);
        base = mod_mul(base, base, mod);
        exp >>= 1;
    }
    return result;
}

/**
 * @brief Deterministic Miller-Rabin primality test for 64-bit integers.
 * Complexity: O(k * log n), where k = number of witnesses (constant here).
 * Uses the known minimal witness set that guarantees correctness for 64-bit
 * inputs: {2, 325, 9375, 28178, 450775, 9780504, 1795265022}.
 * @param n Number to test for primality
 * @return true if n is prime, false otherwise
 */
inline bool is_prime(uint64_t n) {
    if (n < 2)
        return false;
    if (n == 2 || n == 3)
        return true;
    if (n % 2 == 0)
        return false;

    // Factor n-1 as d * 2^r using count-trailing-zeros
    uint64_t r = __builtin_ctzll(n - 1);
    uint64_t d = (n - 1) >> r;

    // Deterministic witness set for 64-bit integers
    const uint64_t witnesses[] = {2, 325, 9375, 28178, 450775, 9780504, 1795265022};

    for (uint64_t a : witnesses) {
        if (a % n == 0)
            continue;
        uint64_t x = mod_pow(a, d, n);
        if (x == 1 || x == n - 1)
            continue;

        bool composite = true;
        for (uint64_t i = 1; i < r; ++i) {
            x = mod_mul(x, x, n);
            if (x == n - 1) {
                composite = false;
                break;
            }
        }
        if (composite)
            return false;
    }
    return true;
}

/**
 * @brief Find next prime >= start
 * @param start Starting number
 * @return Next prime number
 */
inline uint64_t next_prime(uint64_t start) {
    if (start <= 2)
        return 2;
    if (start % 2 == 0)
        ++start;
    while (!is_prime(start)) {
        start += 2;
    }
    return start;
}

/**
 * @brief Check if all keys are coprime to modulus
 * @param keys Keys to check
 * @param mod Modulus
 * @return true if all keys are coprime to modulus, false otherwise
 */
inline bool all_coprime(const std::vector<uint64_t>& keys, uint64_t mod) {
    for (auto x : keys) {
        if (x != 0 && x % mod == 0) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Modular multiplicative inverse using Fermat's little theorem.
 *
 * For a prime modulus p and a not divisible by p, Fermat's little theorem states
 * that a^(p-1) ≡ 1 (mod p). Therefore a * a^(p-2) ≡ 1 (mod p), so a^(p-2) is the
 * modular inverse of a modulo p.
 *
 * Preconditions:
 *  - p is prime
 *  - a and p are coprime (a % p != 0)
 */
inline uint64_t mod_inverse(uint64_t a, uint64_t p) {
    a %= p;
    assert(is_prime(p));
    assert(a != 0);
    return mod_pow(a, p - 2, p);
}

}  // namespace hashing
}  // namespace cltj
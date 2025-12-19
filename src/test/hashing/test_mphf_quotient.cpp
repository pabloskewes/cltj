#include <hashing/mphf_bdz.hpp>
#include <hashing/storage/glgh.hpp>
#include <sdsl/bits.hpp>
#include <iostream>
#include <limits>
#include <random>
#include <unordered_set>
#include <string>

using cltj::hashing::GlGhStorage;
using cltj::hashing::MPHF;
using cltj::hashing::policies::QuotientKey;

int main() {
    constexpr size_t n = 2000;
    std::unordered_set<uint64_t> seeds;
    seeds.reserve(n);
    std::mt19937_64 rng_keys(987654321ULL);
    while (seeds.size() < n) {
        seeds.insert(rng_keys());
    }
    std::vector<uint64_t> keys(seeds.begin(), seeds.end());

    MPHF<GlGhStorage, QuotientKey> mphf;
    if (!mphf.build(keys)) {
        std::cerr << "QuotientKey test failure: MPHF build failed." << std::endl;
        return 1;
    }

    std::unordered_set<uint64_t> key_set(keys.begin(), keys.end());

    for (auto key : keys) {
        if (!mphf.contains(key)) {
            std::cerr << "QuotientKey test failure: contains() rejected stored key " << key << "."
                      << std::endl;
            return 1;
        }
    }

    std::mt19937_64 rng(123456789ULL);
    for (size_t i = 0; i < 5000; ++i) {
        uint64_t candidate;
        do {
            candidate = rng();
        } while (key_set.count(candidate));
        if (mphf.contains(candidate)) {
            std::cerr << "QuotientKey test failure: contains() accepted random non-key." << std::endl;
            return 1;
        }
    }

    auto primes = mphf.get_primes();
    __uint128_t jump128 = static_cast<__uint128_t>(primes[0]) * primes[1] * primes[2];
    if (jump128 >= (static_cast<__uint128_t>(1) << 64)) {
        std::cerr << "QuotientKey test failure: jump overflowed 64 bits." << std::endl;
        return 1;
    }
    uint64_t jump = static_cast<uint64_t>(jump128);

    for (auto key : keys) {
        __uint128_t fake128 = static_cast<__uint128_t>(key) + jump128;
        if (fake128 >= (static_cast<__uint128_t>(1) << 64)) {
            std::cerr << "QuotientKey test failure: fake key overflowed 64 bits." << std::endl;
            return 1;
        }
        uint64_t fake = static_cast<uint64_t>(fake128);
        if (mphf.contains(fake)) {
            std::cerr << "QuotientKey test failure: contains() accepted key+product (same vertex)."
                      << std::endl;
            return 1;
        }
    }

    auto breakdown = mphf.get_size_breakdown();
    double q_bits_per_key = (breakdown.q_bytes * 8.0) / static_cast<double>(keys.size());
    uint64_t p_min = std::min({primes[0], primes[1], primes[2]});
    uint64_t q_max = std::numeric_limits<uint64_t>::max() / p_min;
    double expected_width = static_cast<double>(sdsl::bits::hi(q_max) + 1);
    if (q_bits_per_key < expected_width || q_bits_per_key > expected_width + 0.5) {
        std::cerr << "QuotientKey test failure: q_bits_per_key=" << q_bits_per_key
                  << " expected~=" << expected_width << std::endl;
        return 1;
    }

    std::cout << "QuotientKey tests passed." << std::endl;
    return 0;
}

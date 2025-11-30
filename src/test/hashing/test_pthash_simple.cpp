#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <random>
#include <algorithm>

#include "pthash.hpp"

// TODO: Delete this file after integrating PTHash into the project.

namespace pthash {
template <typename Uint>
std::vector<Uint> distinct_uints(const uint64_t num_keys, const uint64_t seed) {
    assert(num_keys > 0);
    auto gen = std::mt19937_64(seed);
    std::vector<Uint> keys(num_keys * 1.05);
    std::generate(keys.begin(), keys.end(), gen);
    std::sort(keys.begin(), keys.end());
    auto it = std::unique(keys.begin(), keys.end());
    uint64_t size = std::distance(keys.begin(), it);
    if (size < num_keys) {
        uint64_t end = size;
        assert(end > 0);
        for (uint64_t i = 0; i != end - 1 and size != num_keys; ++i) {
            uint64_t first = keys[i];
            uint64_t second = keys[i + 1];
            for (uint64_t j = first + 1; j != second and size != num_keys; ++j) {
                keys[size++] = j;
            }
        }
        while (size != num_keys) {
            keys[size++] = keys[end - 1] + (size - end + 1);
        }
    }
    keys.resize(num_keys);
    return keys;
}

template <typename Iterator, typename Function>
bool check(Iterator keys, Function const& f) {
    std::vector<bool> seen(f.num_keys(), false);
    for (auto it = keys; it != keys + f.num_keys(); ++it) {
        auto hash_val = f(*it);
        if (hash_val >= f.num_keys() || seen[hash_val]) {
            return false;
        }
        seen[hash_val] = true;
    }
    return true;
}
}  // namespace pthash

int main() {
    using namespace pthash;

    std::cout << "=== PTHash Simple Test ===" << std::endl;

    const uint64_t num_keys = 1000;
    const uint64_t seed = 42;

    std::cout << "Generating " << num_keys << " random keys..." << std::endl;
    auto keys = distinct_uints<uint64_t>(num_keys, seed);
    assert(keys.size() == num_keys);
    std::cout << "Generated " << keys.size() << " distinct keys" << std::endl;

    build_configuration config;
    config.seed = seed;
    config.lambda = 4.0;
    config.alpha = 0.99;
    config.verbose = true;
    config.num_threads = 1;

    typedef single_phf<xxhash_128, skew_bucketer, dictionary_dictionary, true> pthash_type;
    pthash_type f;

    std::cout << "Building MPHF..." << std::endl;
    f.build_in_internal_memory(keys.begin(), keys.size(), config);
    std::cout << "Build completed!" << std::endl;

    double bits_per_key = static_cast<double>(f.num_bits()) / f.num_keys();
    std::cout << "Bits per key: " << bits_per_key << std::endl;
    std::cout << "Total bits: " << f.num_bits() << std::endl;
    std::cout << "Number of keys: " << f.num_keys() << std::endl;

    std::cout << "Checking correctness..." << std::endl;
    if (check(keys.begin(), f)) {
        std::cout << "✓ Correctness check PASSED" << std::endl;
    } else {
        std::cout << "✗ Correctness check FAILED" << std::endl;
        return 1;
    }

    std::cout << "Testing queries on first 10 keys:" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(10, keys.size()); ++i) {
        uint64_t hash_val = f(keys[i]);
        std::cout << "  f(" << keys[i] << ") = " << hash_val << std::endl;
        assert(hash_val < num_keys);
    }

    std::cout << "\n=== All tests passed! ===" << std::endl;
    return 0;
}

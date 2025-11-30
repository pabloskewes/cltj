#include <iostream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <random>
#include <algorithm>
#include <chrono>
#include <unordered_set>

#include "pthash.hpp"

// TODO: Delete this file after integrating PTHash into the project.

// Generate unique keys similar to our benchmark
static std::vector<uint64_t> generate_unique_keys(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::unordered_set<uint64_t> unique_keys;
    unique_keys.reserve(n * 2);
    std::uniform_int_distribution<uint64_t> dist(1, n * 100);
    while (unique_keys.size() < n) {
        unique_keys.insert(dist(rng));
    }
    return std::vector<uint64_t>(unique_keys.begin(), unique_keys.end());
}

// Simple correctness check: verify all keys map to unique values in [0, n)
template <typename Iterator, typename Function>
bool check_correctness(Iterator keys, size_t n, Function const& f) {
    std::vector<bool> seen(n, false);
    for (size_t i = 0; i < n; ++i) {
        auto hash_val = f(keys[i]);
        if (hash_val >= n || seen[hash_val]) {
            return false;
        }
        seen[hash_val] = true;
    }
    return true;
}

int main(int argc, char* argv[]) {
    using namespace pthash;

    size_t n = 1000;
    uint64_t seed = 42;
    size_t query_reps = 5;

    if (argc > 1) {
        n = std::stoull(argv[1]);
    }
    if (argc > 2) {
        seed = std::stoull(argv[2]);
    }
    if (argc > 3) {
        query_reps = std::stoull(argv[3]);
    }

    std::cout << "=== PTHash Benchmark (n=" << n << ", seed=" << seed << ") ===" << std::endl;

    std::cout << "Generating " << n << " unique keys..." << std::endl;
    auto keys = generate_unique_keys(n, seed);
    assert(keys.size() == n);
    std::cout << "Generated " << keys.size() << " distinct keys" << std::endl;

    build_configuration config;
    config.seed = seed;
    config.lambda = 4.0;
    config.alpha = 0.99;
    config.verbose = false;
    config.num_threads = 1;

    typedef single_phf<xxhash_128, skew_bucketer, dictionary_dictionary, true> pthash_type;
    pthash_type f;

    std::cout << "Building MPHF..." << std::endl;
    auto start_build = std::chrono::high_resolution_clock::now();
    f.build_in_internal_memory(keys.begin(), keys.size(), config);
    auto end_build = std::chrono::high_resolution_clock::now();
    auto build_time_us =
        std::chrono::duration_cast<std::chrono::microseconds>(end_build - start_build).count();
    std::cout << "Build completed in " << (build_time_us / 1000.0) << " ms" << std::endl;

    size_t size_bytes = (f.num_bits() + 7) / 8;
    double bits_per_key = static_cast<double>(f.num_bits()) / f.num_keys();
    std::cout << "Size: " << size_bytes << " bytes (" << (size_bytes / 1024.0) << " KB)" << std::endl;
    std::cout << "Bits per key: " << bits_per_key << std::endl;
    std::cout << "Number of keys: " << f.num_keys() << std::endl;

    std::cout << "Checking correctness..." << std::endl;
    if (check_correctness(keys.begin(), n, f)) {
        std::cout << "✓ Correctness check PASSED" << std::endl;
    } else {
        std::cout << "✗ Correctness check FAILED" << std::endl;
        return 1;
    }

    std::cout << "Benchmarking queries (" << query_reps << " reps)..." << std::endl;
    volatile uint64_t sink = 0;
    auto start_query = std::chrono::high_resolution_clock::now();
    for (size_t rep = 0; rep < query_reps; ++rep) {
        for (uint64_t k : keys) {
            uint64_t h = f(k);
            sink ^= h;
        }
    }
    auto end_query = std::chrono::high_resolution_clock::now();
    auto query_time_us =
        std::chrono::duration_cast<std::chrono::microseconds>(end_query - start_query).count();
    double query_time_ns_per_key = (query_time_us * 1000.0) / (n * query_reps);
    std::cout << "Query time: " << (query_time_us / 1000.0) << " ms total" << std::endl;
    std::cout << "Query time: " << query_time_ns_per_key << " ns/key" << std::endl;

    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "n: " << n << std::endl;
    std::cout << "Build time: " << build_time_us << " us (" << (build_time_us / 1000.0) << " ms)"
              << std::endl;
    std::cout << "Query time: " << query_time_ns_per_key << " ns/key" << std::endl;
    std::cout << "Size: " << size_bytes << " bytes" << std::endl;
    std::cout << "Bits per key: " << bits_per_key << std::endl;
    std::cout << "(sink=" << sink << " to prevent optimization)" << std::endl;

    return 0;
}

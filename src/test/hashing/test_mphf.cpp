// A unified and robust test suite for MPHF correctness, performance, and size.
#include <hashing/mphf_bdz.hpp>
#include <util/logger.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <numeric>
#include <random>
#include <unordered_set>
#include <vector>
#include <iostream>

using cltj::hashing::MPHF;

// Helper to generate a vector of unique random keys in a reasonable range.
static std::vector<uint64_t> generate_reasonable_keys(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::unordered_set<uint64_t> unique_keys;
    // Generate keys in a range up to 100*n to reduce collisions during generation.
    std::uniform_int_distribution<uint64_t> dist(1, n * 100);
    while (unique_keys.size() < n) {
        unique_keys.insert(dist(rng));
    }
    return std::vector<uint64_t>(unique_keys.begin(), unique_keys.end());
}

// A struct to hold all the results from a single test run.
struct TestResult {
    size_t n = 0;
    uint32_t m = 0;
    int retries = 0;
    size_t size_bytes = 0;
    double bits_per_key = 0.0;
    double overhead = 0.0;
    long long build_time_us = 0;
    bool build_success = false;
    bool is_permutation = false;
    double false_positive_rate = 0.0;
};

// Runs a full test case for a given n, checking correctness, performance, and size.
TestResult run_test_case(size_t n) {
    TestResult result;
    result.n = n;

    if (n > 100000) {
        std::cout << "Testing n=" << n << ", generating keys..." << std::endl;
    }
    auto keys = generate_reasonable_keys(n, 42 + n);

    MPHF mphf;

    // 1. Measure Build Time
    auto start = std::chrono::high_resolution_clock::now();
    result.build_success = mphf.build(keys);
    auto end = std::chrono::high_resolution_clock::now();
    result.build_time_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    if (!result.build_success) {
        result.retries = mphf.retry_count();
        return result;
    }

    // 2. Check for Correct Permutation & `contains` for true positives
    std::unordered_set<uint32_t> seen;
    bool distinct = true;
    bool in_range = true;
    bool contains_all_positives = true;
    for (auto k : keys) {
        uint32_t h = mphf.query(k);
        if (h >= n) {
            in_range = false;
        }
        if (!seen.insert(h).second) {
            distinct = false;
        }
        if (!mphf.contains(k)) {
            contains_all_positives = false;
        }
    }
    result.is_permutation = distinct && in_range && contains_all_positives;

    // 3. Check for False Positives
    if (result.is_permutation) {
        size_t negative_sample_size = std::min((size_t)100000, n);  // Limit sample size
        std::unordered_set<uint64_t> key_set(keys.begin(), keys.end());
        std::mt19937_64 rng(12345 + n);  // Different seed from keygen
        size_t false_positives = 0;
        for (size_t i = 0; i < negative_sample_size; ++i) {
            uint64_t non_key;
            do {
                non_key = rng();
            } while (key_set.count(non_key));  // Ensure it's not actually a key

            if (mphf.contains(non_key)) {
                false_positives++;
            }
        }
        result.false_positive_rate = (double)false_positives / negative_sample_size;
    }

    // 4. Manually calculate the true size by summing the components.
    size_t g_bytes = sdsl::size_in_bytes(mphf.get_g());
    size_t used_pos_bytes = sdsl::size_in_bytes(mphf.get_used_positions());
    size_t rank_bytes = sdsl::size_in_bytes(mphf.get_rank_support());
    size_t q_bytes = sdsl::size_in_bytes(mphf.get_q());
    size_t other_bytes = sizeof(mphf.m()) + sizeof(mphf.n()) + sizeof(mphf.get_primes()) +
        sizeof(mphf.get_multipliers()) + sizeof(mphf.get_biases()) + sizeof(mphf.get_segment_starts());
    result.size_bytes = g_bytes + used_pos_bytes + rank_bytes + q_bytes + other_bytes;

    result.bits_per_key = (result.size_bytes * 8.0) / n;
    result.m = mphf.m();
    result.retries = mphf.retry_count();
    result.overhead = (double)result.m / n;

    return result;
}

void print_results(const TestResult& result) {
    std::cout << "--- Test Case: n = " << result.n << " ---" << std::endl;
    if (!result.build_success) {
        std::cout << "  STATUS: BUILD FAILED after " << result.retries << " retries." << std::endl;
        return;
    }

    std::cout << "  Correctness:" << std::endl;
    std::cout << "    Is permutation & contains all keys? " << (result.is_permutation ? "YES" : "NO")
              << std::endl;
    if (result.is_permutation) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4) << (result.false_positive_rate * 100.0);
        std::cout << "    False positive rate: " << oss.str() << "%" << std::endl;
    }
    std::cout << "  Performance:" << std::endl;
    std::cout << "    Build time: " << result.build_time_us << " us (" << result.build_time_us / 1000.0
              << " ms)" << std::endl;
    std::cout << "  Size & Overhead:" << std::endl;
    std::cout << "    Size: " << result.size_bytes << " bytes (" << result.size_bytes / 1024.0 << " KB)"
              << std::endl;
    std::ostringstream oss_bits, oss_overhead;
    oss_bits << std::fixed << std::setprecision(2) << result.bits_per_key;
    oss_overhead << std::fixed << std::setprecision(3) << result.overhead;
    std::cout << "    Bits per key: " << oss_bits.str() << std::endl;
    std::cout << "    m/n overhead: " << oss_overhead.str() << std::endl;
    std::cout << "    Retries needed: " << result.retries << std::endl;
}

int main() {
    std::cout << "========== Unified MPHF Test Suite ==========" << std::endl;
    std::cout << "Correctness, Performance, and Size Analysis" << std::endl;

    std::vector<size_t> test_sizes = {100, 1000, 10000, 100000, 1000000, 2000000, 5000000, 10000000};
    int failures = 0;

    for (size_t n : test_sizes) {
        TestResult result = run_test_case(n);
        print_results(result);
        if (!result.build_success || !result.is_permutation) {
            failures++;
        }
    }

    std::cout << "========== Test Summary ==========" << std::endl;
    std::cout << "Total test cases: " << test_sizes.size() << std::endl;
    std::cout << "Failures: " << failures << std::endl;
    std::cout << "Final Result: " << (failures == 0 ? "ALL PASSED" : "SOME FAILED") << std::endl;

    return failures > 0 ? 1 : 0;
}

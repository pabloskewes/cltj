// A unified and robust test suite for MPHF correctness, performance, and size.
#include <include/hashing/mphf_bdz.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <unordered_set>
#include <vector>

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
};

// Runs a full test case for a given n, checking correctness, performance, and size.
TestResult run_test_case(size_t n) {
    TestResult result;
    result.n = n;

    if (n > 100000) {
        std::cout << "Testing n=" << n << ", generating keys...\n";
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

    // 2. Check for Correct Permutation
    std::unordered_set<uint32_t> seen;
    bool distinct = true;
    bool in_range = true;
    for (auto k : keys) {
        uint32_t h = mphf.query(k);
        if (h >= n) {
            in_range = false;
        }
        if (!seen.insert(h).second) {
            distinct = false;
        }
    }
    result.is_permutation = distinct && in_range;

    // 3. Collect Metrics
    result.m = mphf.m();
    result.retries = mphf.retry_count();
    result.size_bytes = sdsl::size_in_bytes(mphf);
    result.bits_per_key = (result.size_bytes * 8.0) / n;
    result.overhead = (double)result.m / n;

    return result;
}

void print_results(const TestResult& result) {
    std::cout << "--- Test Case: n = " << result.n << " ---\n";
    if (!result.build_success) {
        std::cout << "  STATUS: BUILD FAILED after " << result.retries << " retries.\n\n";
        return;
    }

    std::cout << "  Correctness:\n";
    std::cout << "    Is permutation? " << (result.is_permutation ? "YES" : "NO") << "\n";
    std::cout << "  Performance:\n";
    std::cout << "    Build time: " << result.build_time_us << " us (" << result.build_time_us / 1000.0
              << " ms)\n";
    std::cout << "  Size & Overhead:\n";
    std::cout << "    Size: " << result.size_bytes << " bytes (" << result.size_bytes / 1024.0 << " KB)\n";
    std::cout << "    Bits per key: " << std::fixed << std::setprecision(2) << result.bits_per_key << "\n";
    std::cout << "    m/n overhead: " << std::fixed << std::setprecision(3) << result.overhead << "\n";
    std::cout << "    Retries needed: " << result.retries << "\n\n";
}

int main() {
    std::cout << "========== Unified MPHF Test Suite ==========\n";
    std::cout << "Correctness, Performance, and Size Analysis\n\n";

    std::vector<size_t> test_sizes = {100, 1000, 10000, 100000, 1000000, 2000000, 5000000, 10000000};
    int failures = 0;

    for (size_t n : test_sizes) {
        TestResult result = run_test_case(n);
        print_results(result);
        if (!result.build_success || !result.is_permutation) {
            failures++;
        }
    }

    std::cout << "========== Test Summary ==========\n";
    std::cout << "Total test cases: " << test_sizes.size() << "\n";
    std::cout << "Failures: " << failures << "\n";
    std::cout << "Final Result: " << (failures == 0 ? "ALL PASSED" : "SOME FAILED") << "\n";

    return failures > 0 ? 1 : 0;
}

// A unified and robust test suite for MPHF correctness, performance, and size.
#include <hashing/mphf_bdz.hpp>
#include <hashing/storage/packed_trit.hpp>
#include <hashing/storage/glgh.hpp>
#include <util/logger.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>
#include <unordered_set>
#include <vector>
#include <iostream>

using cltj::hashing::BaselineStorage;
using cltj::hashing::CompressedBitvector;
using cltj::hashing::GlGhStorage;
using cltj::hashing::MPHF;
using cltj::hashing::PackedTritStorage;
using cltj::hashing::policies::WithFingerprints;

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
template <typename StorageStrategy>
TestResult run_test_case(size_t n) {
    TestResult result;
    result.n = n;

    if (n > 100000) {
        std::cout << "Testing n=" << n << ", generating keys..." << std::endl;
    }
    auto keys = generate_reasonable_keys(n, 42 + n);

    MPHF<StorageStrategy, WithFingerprints> mphf;

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
    uint32_t min_val = UINT32_MAX;
    uint32_t max_val = 0;
    for (auto k : keys) {
        uint32_t h = mphf.query(k);
        if (h >= n) {
            in_range = false;
            std::cerr << "ERROR: query returned " << h << " >= n=" << n << " for key " << k << std::endl;
        }
        if (h < min_val)
            min_val = h;
        if (h > max_val)
            max_val = h;
        if (!seen.insert(h).second) {
            distinct = false;
            std::cerr << "ERROR: duplicate hash value " << h << " for key " << k << std::endl;
        }
        if (!mphf.contains(k)) {
            contains_all_positives = false;
        }
    }
    // Verify it's a perfect permutation: covers exactly [0, n-1]
    bool perfect_permutation =
        distinct && in_range && (min_val == 0) && (max_val == n - 1) && (seen.size() == n);
    result.is_permutation = perfect_permutation && contains_all_positives;

    if (!perfect_permutation && distinct && in_range) {
        std::cerr << "WARNING: Not a perfect permutation - min=" << min_val << " max=" << max_val
                  << " size=" << seen.size() << " expected n=" << n << std::endl;
    }

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

    // 4. Calculate size using breakdown
    auto breakdown = mphf.get_size_breakdown();
    result.size_bytes = breakdown.total_bytes();

    result.bits_per_key = (result.size_bytes * 8.0) / n;
    result.m = mphf.m();
    result.retries = mphf.retry_count();
    result.overhead = (double)result.m / n;

    return result;
}

void print_results(const TestResult& result, const std::string& strategy_name = "") {
    std::cout << "--- Test Case: n = " << result.n;
    if (!strategy_name.empty()) {
        std::cout << " [" << strategy_name << "]";
    }
    std::cout << " ---" << std::endl;
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
    std::cout << "Comparing BaselineStorage vs PackedTritStorage vs GlGhStorage" << std::endl;

    std::vector<size_t> test_sizes = {10000, 100000, 1000000, 2000000, 5000000, 10000000};
    int failures = 0;

    for (size_t n : test_sizes) {
        std::cout << "\n========== Analysis for n = " << n << " ==========" << std::endl;

        // Test BaselineStorage
        TestResult baseline = run_test_case<BaselineStorage>(n);
        print_results(baseline, "BaselineStorage");
        if (!baseline.build_success || !baseline.is_permutation) {
            failures++;
        }

        // Test PackedTritStorage with compressed B
        TestResult packed = run_test_case<PackedTritStorage<CompressedBitvector>>(n);
        print_results(packed, "PackedTritStorage (Compressed B)");
        if (!packed.build_success || !packed.is_permutation) {
            failures++;
        }

        // Test GlGhStorage (on-the-fly B via Gl/Gh)
        TestResult glgh = run_test_case<GlGhStorage>(n);
        print_results(glgh, "GlGhStorage (Gl/Gh on-the-fly B)");
        if (!glgh.build_success || !glgh.is_permutation) {
            failures++;
        }

        // Comparison
        if (baseline.build_success && packed.build_success && glgh.build_success && baseline.is_permutation &&
            packed.is_permutation && glgh.is_permutation) {
            std::cout << "\n  Comparison:" << std::endl;
            double speedup = (double)baseline.build_time_us / packed.build_time_us;
            double space_saving = baseline.bits_per_key - packed.bits_per_key;
            std::ostringstream oss_speedup, oss_saving;
            oss_speedup << std::fixed << std::setprecision(2) << speedup;
            oss_saving << std::fixed << std::setprecision(2) << space_saving;
            std::cout << "    Build time ratio (baseline/packed): " << oss_speedup.str() << "x" << std::endl;
            std::cout << "    Space saving: " << oss_saving.str() << " bits/key" << std::endl;
        }
    }

    std::cout << "\n========== Test Summary ==========" << std::endl;
    std::cout << "Total test cases: " << (test_sizes.size() * 3) << " (3 strategies × " << test_sizes.size()
              << " sizes)" << std::endl;
    std::cout << "Failures: " << failures << std::endl;
    std::cout << "Final Result: " << (failures == 0 ? "ALL PASSED" : "SOME FAILED") << std::endl;

    return failures > 0 ? 1 : 0;
}

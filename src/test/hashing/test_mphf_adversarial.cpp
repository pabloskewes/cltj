/**
 * @file test_mphf_adversarial.cpp
 * @brief Adversarial test suite for MPHF QuotientKey exactness validation
 * 
 * This suite implements surgical tests designed to validate the mathematical
 * properties of the Key-Quotienting scheme after redundancy elimination.
 * 
 * Key theoretical property being tested:
 * - Biyectivity (remainder <-> position): B[v]=1 filter guarantees r_query == r_rec
 * - Quotient disambiguation: q_query == q_stored is sufficient for exactness
 * 
 */

#include <hashing/mphf_bdz.hpp>
#include <hashing/storage/glgh.hpp>
#include <hashing/key_policies.hpp>
#include <CLI11.hpp>

#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>
#include <cassert>
#include <unordered_set>

using namespace cltj::hashing;

// ============================================================================
// Test Result Tracking
// ============================================================================

struct TestResult {
    std::string test_name;
    bool passed;
    size_t checks_run;
    size_t false_positives;
    size_t false_negatives;
    std::string error_message;
    double execution_time_ms;

    TestResult(const std::string& name)
        : test_name(name),
          passed(false),
          checks_run(0),
          false_positives(0),
          false_negatives(0),
          error_message(""),
          execution_time_ms(0.0) {}
};

class TestReport {
  private:
    std::vector<TestResult> results_;

  public:
    void add_result(const TestResult& result) { results_.push_back(result); }

    void print_summary() const {
        std::cout << "\n";
        std::cout << "════════════════════════════════════════════════════════════════\n";
        std::cout << "                    TEST SUITE SUMMARY                          \n";
        std::cout << "════════════════════════════════════════════════════════════════\n\n";

        size_t passed = 0;
        size_t failed = 0;
        size_t total_checks = 0;
        size_t total_fp = 0;
        size_t total_fn = 0;

        for (const auto& result : results_) {
            std::string status = result.passed ? "✓ PASS" : "✗ FAIL";
            std::cout << std::left << std::setw(40) << result.test_name << " " << status;

            if (!result.passed) {
                std::cout << "\n  └─ " << result.error_message;
            }
            std::cout << "\n";
            std::cout << "     Checks: " << result.checks_run << " | FP: " << result.false_positives
                      << " | FN: " << result.false_negatives << " | Time: " << std::fixed
                      << std::setprecision(2) << result.execution_time_ms << " ms\n";

            if (result.passed)
                passed++;
            else
                failed++;

            total_checks += result.checks_run;
            total_fp += result.false_positives;
            total_fn += result.false_negatives;
        }

        std::cout << "\n────────────────────────────────────────────────────────────────\n";
        std::cout << "Total: " << results_.size() << " tests | "
                  << "Passed: " << passed << " | Failed: " << failed << "\n";
        std::cout << "Total checks: " << total_checks << " | "
                  << "FP: " << total_fp << " | FN: " << total_fn << "\n";
        std::cout << "════════════════════════════════════════════════════════════════\n";

        if (failed == 0) {
            std::cout << "✓ ALL TESTS PASSED - QuotientKey is mathematically exact\n";
        } else {
            std::cout << "✗ SOME TESTS FAILED - See details above\n";
        }
        std::cout << "════════════════════════════════════════════════════════════════\n\n";
    }

    bool all_passed() const {
        return std::all_of(results_.begin(), results_.end(), [](const TestResult& r) { return r.passed; });
    }
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Generate random keys with controlled distribution
 * @param n Number of keys to generate
 * @param max_value Maximum value for keys (default: UINT64_MAX)
 * @param seed Random seed for reproducibility
 */
std::vector<uint64_t> generate_random_keys(size_t n, uint64_t seed = 42, uint64_t max_value = UINT64_MAX) {
    std::vector<uint64_t> keys;
    keys.reserve(n);

    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<uint64_t> dist(0, max_value);

    // Use a set to ensure uniqueness
    std::set<uint64_t> unique_keys;
    while (unique_keys.size() < n) {
        unique_keys.insert(dist(rng));
    }

    keys.assign(unique_keys.begin(), unique_keys.end());
    return keys;
}

/**
 * @brief Simple timer for measuring test execution time
 */
class Timer {
  private:
    std::chrono::high_resolution_clock::time_point start_;

  public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
};

// ============================================================================
// TIER 1: CRITICAL TESTS
// ============================================================================

/**
 * TEST 1: Dopplergänger Attack (Remainder Collision)
 * 
 * Validates that quotient check (q) disambiguates when remainder (r) is identical.
 * 
 * For each stored key x, generates impostor x' = x ± p_j:
 * - Mathematically guaranteed: r' = r (same remainder mod p_j)
 * - Mathematically guaranteed: q' = q ± 1 (quotient differs by 1)
 * 
 * If x' falls in the same segment as x, this FORCES the quotient check to work.
 * 
 * This test directly attacks the justification for removing the remainder check.
 * If quotients don't disambiguate correctly, we'll see false positives.
 */
TestResult test_dopplerganger_attack() {
    TestResult result("Tier1.1: Dopplergänger Attack");
    Timer timer;

    try {
        std::cout << "\nRunning: " << result.test_name << "...\n";

        // Setup: Build MPHF with random keys
        const size_t N = 10000;
        std::vector<uint64_t> keys = generate_random_keys(N);

        MPHF<GlGhStorage, policies::QuotientKey> mphf;
        if (!mphf.build(keys)) {
            result.error_message = "Failed to build MPHF";
            return result;
        }

        auto primes = mphf.get_primes();

        // Attack: For each key, generate impostors with same remainder
        for (uint64_t x : keys) {
            for (uint64_t p : primes) {
                // Impostor up: x' = x + p (same r, q' = q+1)
                if (x <= UINT64_MAX - p) {
                    uint64_t impostor = x + p;
                    bool contains_result = mphf.contains(impostor);

                    result.checks_run++;
                    if (contains_result) {
                        result.false_positives++;
                    }
                }

                // Impostor down: x' = x - p (same r, q' = q-1)
                if (x >= p) {
                    uint64_t impostor = x - p;
                    bool contains_result = mphf.contains(impostor);

                    result.checks_run++;
                    if (contains_result) {
                        result.false_positives++;
                    }
                }
            }
        }

        // Verify: All stored keys should still be found
        for (uint64_t x : keys) {
            result.checks_run++;
            if (!mphf.contains(x)) {
                result.false_negatives++;
            }
        }

        result.passed = (result.false_positives == 0 && result.false_negatives == 0);

        if (!result.passed) {
            result.error_message = "FP=" + std::to_string(result.false_positives) +
                ", FN=" + std::to_string(result.false_negatives);
        }

    } catch (const std::exception& e) {
        result.error_message = std::string("Exception: ") + e.what();
        result.passed = false;
    }

    result.execution_time_ms = timer.elapsed_ms();
    return result;
}

/**
 * TEST 1.2: Zero-Quotient Edge Case
 *
 * Validates that the system correctly handles q = 0 (small keys).
 *
 * When x < p_j, then q = floor(x / p_j) = 0. This edge case could expose bugs:
 * - Confusing q=0 with "uninitialized"
 * - Off-by-one in int_vector with dynamic width
 * - Strange behavior in comparisons with zero
 *
 * Setup: Insert keys deliberately chosen to have q=0 (x < min(p_j))
 * Attack: Verify that both stored keys and non-keys with q=0 behave correctly
 */
TestResult test_zero_quotient_edge_case() {
    TestResult result("Tier1.2: Zero-Quotient Edge Case");
    Timer timer;

    try {
        std::cout << "\nRunning: " << result.test_name << "...\n";

        // We'll use typical primes from a build to determine p_min
        // Build a small MPHF first to get actual primes
        std::vector<uint64_t> probe_keys = {1, 2, 3};
        MPHF<GlGhStorage, policies::QuotientKey> probe_mphf;
        if (!probe_mphf.build(probe_keys)) {
            result.error_message = "Failed to build probe MPHF";
            return result;
        }
        auto primes = probe_mphf.get_primes();
        uint64_t p_min = std::min({primes[0], primes[1], primes[2]});

        // Generate keys guaranteed to have q=0
        std::vector<uint64_t> small_keys;
        const size_t N = std::min(10000ULL, p_min);
        for (uint64_t i = 0; i < N; ++i) {
            small_keys.push_back(i);
        }

        MPHF<GlGhStorage, policies::QuotientKey> mphf;
        if (!mphf.build(small_keys)) {
            result.error_message = "Failed to build MPHF with small keys";
            return result;
        }

        // Verify all keys with q=0 are present
        for (uint64_t k : small_keys) {
            result.checks_run++;
            if (!mphf.contains(k)) {
                result.false_negatives++;
            }
        }

        // Verify non-keys with q=0 are rejected
        uint64_t test_limit = std::min(static_cast<uint64_t>(N) + 1000, p_min);
        for (uint64_t i = N; i < test_limit; ++i) {
            result.checks_run++;
            if (mphf.contains(i)) {
                result.false_positives++;
            }
        }

        result.passed = (result.false_positives == 0 && result.false_negatives == 0);

        if (!result.passed) {
            result.error_message = "FP=" + std::to_string(result.false_positives) +
                ", FN=" + std::to_string(result.false_negatives);
        }

    } catch (const std::exception& e) {
        result.error_message = std::string("Exception: ") + e.what();
        result.passed = false;
    }

    result.execution_time_ms = timer.elapsed_ms();
    return result;
}

/**
 * TEST 1.3: Dense Cluster Attack
 *
 * Stress test of rank and GlGhStorage with consecutive keys.
 *
 * Consecutive keys are worst-case for:
 * - Rank: Many consecutive 1s in B[v] stress accumulators
 * - GlGh: Dense patterns may expose encoding bugs
 * - Off-by-one: Boundaries between consecutive keys are error-prone
 *
 * Note: BDZ construction can fail with very dense consecutive keys (known limitation).
 * We use N=20K as a compromise: large enough to stress rank, small enough to usually build.
 *
 * Setup: Insert 20K consecutive keys [0, 19999]
 * Attack: Verify all present, boundaries rejected, random gaps rejected
 */
TestResult test_dense_cluster_attack() {
    TestResult result("Tier1.3: Dense Cluster Attack");
    Timer timer;

    try {
        std::cout << "\nRunning: " << result.test_name << "...\n";

        const size_t N = 20000;  // Reduced to 20K for BDZ to be able to construct
        std::vector<uint64_t> dense_keys;
        dense_keys.reserve(N);

        // Insert consecutive keys
        for (uint64_t i = 0; i < N; ++i) {
            dense_keys.push_back(i);
        }

        MPHF<GlGhStorage, policies::QuotientKey> mphf;
        if (!mphf.build(dense_keys)) {
            result.error_message = "Failed to build MPHF with dense cluster (BDZ limitation)";
            return result;
        }

        // Verify all present
        for (uint64_t k : dense_keys) {
            result.checks_run++;
            if (!mphf.contains(k)) {
                result.false_negatives++;
            }
        }

        // Verify boundary (off-by-one critical)
        result.checks_run++;
        if (mphf.contains(N)) {
            result.false_positives++;
        }

        result.checks_run++;
        if (mphf.contains(N + 100)) {
            result.false_positives++;
        }

        // Verify random gaps beyond cluster
        std::mt19937_64 rng(42);
        std::uniform_int_distribution<uint64_t> dist(N + 1, N + 100000);
        for (int i = 0; i < 1000; ++i) {
            result.checks_run++;
            if (mphf.contains(dist(rng))) {
                result.false_positives++;
            }
        }

        result.passed = (result.false_positives == 0 && result.false_negatives == 0);

        if (!result.passed) {
            result.error_message = "FP=" + std::to_string(result.false_positives) +
                ", FN=" + std::to_string(result.false_negatives);
        }

    } catch (const std::exception& e) {
        result.error_message = std::string("Exception: ") + e.what();
        result.passed = false;
    }

    result.execution_time_ms = timer.elapsed_ms();
    return result;
}

/**
 * TEST 1.4: 64-bit Boundary (Arithmetic Overflow)
 *
 * Validates correct behavior with overflow of uint64_t in hash function.
 *
 * Hash function is h_j(x) = (a_j * x + b_j) mod p_j.
 * If x is very large and a_j is also large, a_j * x overflows 64 bits.
 *
 * While C++ has defined behavior (modulo 2^64), this could:
 * - Break biyectivity if code doesn't handle overflow correctly
 * - Cause bugs in quotient calculation q = x / p_j near UINT64_MAX
 * - Expose problems in comparisons with extreme values
 *
 * Setup: Use exact boundary values (not randoms) to expose overflow bugs
 */
TestResult test_64bit_boundary() {
    TestResult result("Tier1.4: 64-bit Boundary");
    Timer timer;

    try {
        std::cout << "\nRunning: " << result.test_name << "...\n";

        std::vector<uint64_t> edge_keys = {
            UINT64_MAX,  // Maximum absolute value
            UINT64_MAX - 1,  // Off-by-one from maximum
            UINT64_MAX - 12345,  // Near maximum
            (1ULL << 63),  // 2^63 (MSB change)
            (1ULL << 63) + 1,  // Just after
            (1ULL << 63) - 1,  // Just before
            (1ULL << 32),  // 2^32 (uint32 boundary)
            (1ULL << 32) - 1  // Maximum uint32
        };

        MPHF<GlGhStorage, policies::QuotientKey> mphf;
        if (!mphf.build(edge_keys)) {
            result.error_message = "Failed to build MPHF with boundary keys";
            return result;
        }

        // Verify all present
        for (uint64_t k : edge_keys) {
            result.checks_run++;
            if (!mphf.contains(k)) {
                result.false_negatives++;
            }
        }

        // Verify neighbors not inserted
        std::vector<uint64_t> non_keys = {
            UINT64_MAX - 2,
            UINT64_MAX - 3,
            (1ULL << 63) + 2,
            (1ULL << 63) - 2,
            (1ULL << 32) + 1,
            (1ULL << 32) - 2,
            0  // Zero is always a good edge case
        };

        for (uint64_t nk : non_keys) {
            // Check if it's actually a non-key
            if (std::find(edge_keys.begin(), edge_keys.end(), nk) == edge_keys.end()) {
                result.checks_run++;
                if (mphf.contains(nk)) {
                    result.false_positives++;
                }
            }
        }

        result.passed = (result.false_positives == 0 && result.false_negatives == 0);

        if (!result.passed) {
            result.error_message = "FP=" + std::to_string(result.false_positives) +
                ", FN=" + std::to_string(result.false_negatives);
        }

    } catch (const std::exception& e) {
        result.error_message = std::string("Exception: ") + e.what();
        result.passed = false;
    }

    result.execution_time_ms = timer.elapsed_ms();
    return result;
}

// ============================================================================
// Tier 2: Important Tests
// ============================================================================

/**
 * TEST 2.1: Remainder-Zero Edge Case
 *
 * Validates correct behavior when r = 0 (keys that are exact multiples of p_j).
 *
 * Complement to Zero-Quotient. When x = k * p_j, then r = 0. This edge case could expose:
 * - Bugs in position calculation: v_loc = (a_j * 0 + b_j) mod p_j = b_j
 * - Comparisons with r=0
 * - Interaction between large q and r=0
 *
 * Setup: Insert keys x = k * p_min for k in [1, 1000]
 * Attack: Verify stored keys present, non-stored multiples rejected
 */
TestResult test_remainder_zero_edge_case() {
    TestResult result("Tier2.1: Remainder-Zero Edge Case");
    Timer timer;

    try {
        std::cout << "\nRunning: " << result.test_name << "...\n";

        // Build a small MPHF to get primes
        std::vector<uint64_t> probe_keys = {1, 2, 3};
        MPHF<GlGhStorage, policies::QuotientKey> probe_mphf;
        if (!probe_mphf.build(probe_keys)) {
            result.error_message = "Failed to build probe MPHF";
            return result;
        }
        auto primes = probe_mphf.get_primes();
        uint64_t p_min = std::min({primes[0], primes[1], primes[2]});

        // Generate keys guaranteed to have r=0 for at least one hash function
        std::vector<uint64_t> remainder_zero_keys;
        const size_t N = 1000;
        for (uint64_t k = 1; k <= N; ++k) {
            remainder_zero_keys.push_back(k * p_min);  // r=0, q=k
        }

        MPHF<GlGhStorage, policies::QuotientKey> mphf;
        if (!mphf.build(remainder_zero_keys)) {
            result.error_message = "Failed to build MPHF with remainder-zero keys";
            return result;
        }

        // Verify all present
        for (uint64_t k : remainder_zero_keys) {
            result.checks_run++;
            if (!mphf.contains(k)) {
                result.false_negatives++;
            }
        }

        // Verify non-stored multiples are rejected
        for (uint64_t k = N + 1; k <= N + 100; ++k) {
            result.checks_run++;
            if (mphf.contains(k * p_min)) {
                result.false_positives++;
            }
        }

        result.passed = (result.false_positives == 0 && result.false_negatives == 0);

        if (!result.passed) {
            result.error_message = "FP=" + std::to_string(result.false_positives) +
                ", FN=" + std::to_string(result.false_negatives);
        }

    } catch (const std::exception& e) {
        result.error_message = std::string("Exception: ") + e.what();
        result.passed = false;
    }

    result.execution_time_ms = timer.elapsed_ms();
    return result;
}

/**
 * TEST 2.2: QuotientKey vs FullKey (Gold Standard)
 *
 * Validates that QuotientKey gives EXACTLY the same answers as FullKey.
 *
 * FullKey stores the complete 64-bit key, so it's the "ground truth". If QuotientKey
 * gives different answers, there's a bug.
 *
 * This is the ULTIMATE validation test: if QuotientKey agrees 100% with FullKey on
 * a large random dataset, then the mathematical simplification is empirically exact.
 *
 * Setup: 50K random keys, build both MPHF<QuotientKey> and MPHF<FullKey>
 * Attack: Verify all keys + 100K non-keys, both must agree on every single query
 */
TestResult test_quotient_vs_fullkey() {
    TestResult result("Tier2.2: QuotientKey vs FullKey");
    Timer timer;

    try {
        std::cout << "\nRunning: " << result.test_name << "...\n";

        const size_t N = 50000;
        auto test_keys = generate_random_keys(N, 42);
        std::cout << "[DEBUG] Generated " << N << " keys\n";

        // Build both MPHFs (they will get different random hash params)
        MPHF<GlGhStorage, policies::QuotientKey> mphf_quotient;
        MPHF<GlGhStorage, policies::FullKey> mphf_full;

        std::cout << "[DEBUG] Building QuotientKey MPHF...\n";
        if (!mphf_quotient.build(test_keys)) {
            result.error_message = "Failed to build QuotientKey MPHF";
            return result;
        }
        std::cout << "[DEBUG] QuotientKey MPHF built successfully\n";

        std::cout << "[DEBUG] Building FullKey MPHF...\n";
        if (!mphf_full.build(test_keys)) {
            result.error_message = "Failed to build FullKey MPHF";
            return result;
        }
        std::cout << "[DEBUG] FullKey MPHF built successfully\n";

        // Create unordered_set for O(1) lookup instead of O(n) linear search
        std::unordered_set<uint64_t> key_set(test_keys.begin(), test_keys.end());
        std::cout << "[DEBUG] Created key_set with " << key_set.size() << " entries\n";

        // Verify all positives
        std::cout << "[DEBUG] Verifying " << test_keys.size() << " positives...\n";
        for (uint64_t k : test_keys) {
            result.checks_run++;
            bool q = mphf_quotient.contains(k);
            bool f = mphf_full.contains(k);

            if (!q)
                result.false_negatives++;  // QuotientKey should say true
            if (q != f) {
                // Disagreement detected
                result.false_positives++;  // Count as error
                if (result.error_message.empty()) {
                    result.error_message = "Disagreement on key " + std::to_string(k);
                }
            }
        }
        std::cout << "[DEBUG] Positives verified. FN=" << result.false_negatives << "\n";

        // Verify random non-keys
        std::cout << "[DEBUG] Verifying 100K non-keys...\n";
        std::mt19937_64 rng(999);
        std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

        size_t non_key_tests = 0;
        while (non_key_tests < 100000) {
            uint64_t non_key = dist(rng);

            // Ensure it's actually not a key (O(1) with unordered_set)
            if (key_set.find(non_key) != key_set.end()) {
                continue;
            }

            result.checks_run++;
            non_key_tests++;

            if (non_key_tests % 10000 == 0) {
                std::cout << "[DEBUG] Non-keys tested: " << non_key_tests << "/100000\n";
            }

            bool q = mphf_quotient.contains(non_key);
            bool f = mphf_full.contains(non_key);

            if (q != f) {
                // Disagreement on non-key
                result.false_positives++;
                if (result.error_message.empty()) {
                    result.error_message = "Disagreement on non-key " + std::to_string(non_key) +
                        " (Q=" + std::to_string(q) + ", F=" + std::to_string(f) + ")";
                }
            }
        }
        std::cout << "[DEBUG] Non-keys verified. Disagreements=" << result.false_positives << "\n";

        result.passed = (result.false_positives == 0 && result.false_negatives == 0);

        if (!result.passed && result.error_message.empty()) {
            result.error_message = "FP=" + std::to_string(result.false_positives) +
                ", FN=" + std::to_string(result.false_negatives);
        }

    } catch (const std::exception& e) {
        result.error_message = std::string("Exception: ") + e.what();
        result.passed = false;
    }

    result.execution_time_ms = timer.elapsed_ms();
    return result;
}

// ============================================================================
// Main Function with CLI
// ============================================================================

int main(int argc, char** argv) {
    CLI::App app{"MPHF Adversarial Test Suite - Validates QuotientKey exactness"};

    // Test selection flags
    bool run_all = false;
    bool run_tier1 = false;
    bool run_tier2 = false;
    bool run_tier3 = false;
    std::vector<std::string> specific_tests;

    app.add_flag("--all", run_all, "Run all tests (default if no flags specified)");
    app.add_flag("--tier1", run_tier1, "Run Tier 1 tests (Critical)");
    app.add_flag("--tier2", run_tier2, "Run Tier 2 tests (Important)");
    app.add_flag("--tier3", run_tier3, "Run Tier 3 tests (Validation)");
    app.add_option("--test", specific_tests, "Run specific test(s) by name");

    CLI11_PARSE(app, argc, argv);

    // Default to all if nothing specified
    if (!run_tier1 && !run_tier2 && !run_tier3 && specific_tests.empty()) {
        run_all = true;
    }

    // Print header
    std::cout << "\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";
    std::cout << "       MPHF ADVERSARIAL TEST SUITE - QuotientKey Validation     \n";
    std::cout << "════════════════════════════════════════════════════════════════\n";
    std::cout << "\nTheoretical basis: X-Quotient with biyectivity (remainder ↔ position)\n";
    std::cout << "After redundancy elimination, only quotient check remains.\n";
    std::cout << "These tests validate that the simplification is mathematically exact.\n";

    TestReport report;

    // Run selected tests
    if (run_all || run_tier1) {
        std::cout << "\n┌────────────────────────────────────────────────────────────┐\n";
        std::cout << "│ TIER 1: CRITICAL TESTS (Must Pass)                        │\n";
        std::cout << "└────────────────────────────────────────────────────────────┘\n";

        report.add_result(test_dopplerganger_attack());
        report.add_result(test_zero_quotient_edge_case());
        report.add_result(test_dense_cluster_attack());
        report.add_result(test_64bit_boundary());
    }

    if (run_all || run_tier2) {
        std::cout << "\n┌────────────────────────────────────────────────────────────┐\n";
        std::cout << "│ TIER 2: IMPORTANT TESTS (High Priority)                   │\n";
        std::cout << "└────────────────────────────────────────────────────────────┘\n";

        report.add_result(test_remainder_zero_edge_case());
        report.add_result(test_quotient_vs_fullkey());
    }

    if (run_all || run_tier3) {
        std::cout << "\n┌────────────────────────────────────────────────────────────┐\n";
        std::cout << "│ TIER 3: VALIDATION TESTS (Optional)                       │\n";
        std::cout << "└────────────────────────────────────────────────────────────┘\n";
        std::cout << "[Note: Tier 3 tests not yet implemented]\n";
    }

    // Print summary
    report.print_summary();

    return report.all_passed() ? 0 : 1;
}

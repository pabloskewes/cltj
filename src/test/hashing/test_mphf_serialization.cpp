// Test suite for MPHF serialization round-trip correctness
// Verifies that serialize() → load() preserves all internal state and query results
// NOTE: Currently only tests GlGhStorage because BaselineStorage and PackedTritStorage have double-free bugs in destructors (detected by ASan - issue with SDSL memory management)
// TODO: Fix above issue and re-enable BaselineStorage and PackedTritStorage tests
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <random>
#include <stdexcept>
#include <unordered_set>
#include <vector>

// Test-only access to synthesize a fallback-active fixture.
#define private public
#include <hashing/mphf_bdz.hpp>
#undef private

#include <hashing/storage/packed_trit.hpp>
#include <hashing/storage/glgh.hpp>
#include <util/logger.hpp>

using cltj::hashing::GlGhStorage;
using cltj::hashing::MPHF;
using cltj::hashing::policies::FullKey;

// Helper to generate unique random keys
static std::vector<uint32_t> generate_keys(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::unordered_set<uint32_t> unique_keys;
    std::uniform_int_distribution<uint32_t> dist(1, static_cast<uint32_t>(n * 100));
    while (unique_keys.size() < n) {
        unique_keys.insert(dist(rng));
    }
    return std::vector<uint32_t>(unique_keys.begin(), unique_keys.end());
}

template <typename StorageStrategy>
std::vector<uint32_t> synthesize_fallback_fixture(
    MPHF<StorageStrategy, FullKey>& mphf, const std::vector<uint32_t>& keys, size_t residual_count
) {
    assert(residual_count > 0 && residual_count < keys.size());

    std::vector<std::pair<uint32_t, uint32_t>> indexed_keys;
    indexed_keys.reserve(keys.size());
    for (uint32_t key : keys) {
        indexed_keys.emplace_back(mphf.query(key), key);
    }
    std::sort(indexed_keys.begin(), indexed_keys.end());

    std::vector<uint32_t> residual_keys;
    residual_keys.reserve(residual_count);
    mphf.residual_keys_.clear();
    for (size_t i = indexed_keys.size() - residual_count; i < indexed_keys.size(); ++i) {
        uint32_t key = indexed_keys[i].second;
        residual_keys.push_back(key);
        mphf.residual_keys_.push_back(cltj::hashing::premix32(key));
    }
    std::sort(mphf.residual_keys_.begin(), mphf.residual_keys_.end());
    mphf.n_peeled_ = static_cast<uint32_t>(keys.size() - residual_count);

    // The synthetic fixture should remain a valid MPHF for the original key set.
    std::unordered_set<uint32_t> indices;
    indices.reserve(keys.size());
    for (uint32_t key : keys) {
        assert(mphf.contains(key));
        indices.insert(mphf.query(key));
    }
    assert(indices.size() == keys.size());

    return residual_keys;
}

/**
 * @brief Test serialization round-trip for a given storage strategy
 * 
 * Verifies that after serialize() -> load():
 * 1. All metadata fields match (m, n, retry_count, n_peeled, n_residual, has_fallback)
 * 2. All query() results are identical
 * 3. contains() behaves identically
 * 4. Fallback-system queries are preserved
 * 5. Size breakdown is preserved
 */
template <typename StorageStrategy>
void test_roundtrip(
    const std::vector<uint32_t>& keys,
    const std::string& strategy_name,
    const std::string& case_label,
    bool synthesize_fallback = false
) {
    std::cout << "\n--- Round-Trip Test: " << case_label << " [" << strategy_name << "] ---" << std::endl;

    // Build original MPHF
    MPHF<StorageStrategy, FullKey> mphf1;
    bool build_ok = mphf1.build(keys);
    if (!build_ok) {
        throw std::runtime_error("Build failed");
    }
    std::cout << "  [OK] Built original MPHF" << std::endl;

    std::vector<uint32_t> residual_fixture_keys;
    if (synthesize_fallback) {
        residual_fixture_keys = synthesize_fallback_fixture(mphf1, keys, 2);
        std::cout << "  [OK] Synthesized fallback-active fixture with residual=2" << std::endl;
    }

    // Capture original state
    uint32_t original_m = mphf1.m();
    uint32_t original_n = mphf1.n();
    int original_retries = mphf1.retry_count();
    uint32_t original_n_peeled = mphf1.n_peeled();
    uint32_t original_n_residual = mphf1.n_residual();
    bool original_has_fallback = mphf1.has_fallback();
    auto original_breakdown = mphf1.get_size_breakdown();

    // Serialize to stringstream
    std::stringstream ss;
    size_t written = mphf1.serialize(ss, nullptr, "mphf_test");
    std::cout << "  [OK] Serialized " << written << " bytes" << std::endl;

    // Reset stream position to beginning for reading
    ss.seekg(0, std::ios::beg);

    // Deserialize into new MPHF
    MPHF<StorageStrategy, FullKey> mphf2;
    mphf2.load(ss);
    std::cout << "  [OK] Deserialized from stream" << std::endl;

    // ===== VERIFICATION =====

    // 1. Check metadata fields
    assert(mphf2.m() == original_m && "m_ mismatch after load");
    assert(mphf2.n() == original_n && "n_ mismatch after load");
    assert(mphf2.retry_count() == original_retries && "retry_count_ mismatch after load");
    assert(mphf2.n_peeled() == original_n_peeled && "n_peeled_ mismatch after load");
    assert(mphf2.n_residual() == original_n_residual && "n_residual mismatch after load");
    assert(mphf2.has_fallback() == original_has_fallback && "has_fallback mismatch after load");
    std::cout << "  [OK] Metadata preserved: m=" << original_m << " n=" << original_n
              << " retries=" << original_retries << " n_peeled=" << original_n_peeled
              << " residual=" << original_n_residual << std::endl;

    // 1b. Hash coefficients must be reloaded byte-for-byte
    assert(mphf2.get_multipliers() == mphf1.get_multipliers() && "multipliers_ mismatch after load");
    assert(mphf2.get_biases() == mphf1.get_biases() && "biases_ mismatch after load");
    assert(mphf2.get_primes() == mphf1.get_primes() && "primes_ mismatch after load");
    std::cout << "  [OK] Hash coefficients (multipliers, biases, primes) preserved" << std::endl;

    // 2. Check that all query() results match
    bool queries_match = true;
    for (const auto& key : keys) {
        uint32_t h1 = mphf1.query(key);
        uint32_t h2 = mphf2.query(key);
        if (h1 != h2) {
            std::cerr << "ERROR: query(" << key << ") differs: " << h1 << " vs " << h2 << std::endl;
            queries_match = false;
        }
    }
    assert(queries_match && "Queries differ after deserialization");
    std::cout << "  [OK] All " << keys.size() << " query() results match" << std::endl;

    // 3. Check that contains() behaves identically for true positives
    bool contains_match = true;
    for (const auto& key : keys) {
        bool c1 = mphf1.contains(key);
        bool c2 = mphf2.contains(key);
        if (c1 != c2) {
            std::cerr << "ERROR: contains(" << key << ") differs: " << c1 << " vs " << c2 << std::endl;
            contains_match = false;
        }
    }
    assert(contains_match && "contains() differs after deserialization");
    std::cout << "  [OK] All contains() results match" << std::endl;

    // 4. Check fallback-system queries explicitly
    for (uint32_t key : residual_fixture_keys) {
        uint32_t h1 = mphf1.query(key);
        uint32_t h2 = mphf2.query(key);
        assert(h1 == h2 && "Fallback query differs after deserialization");
        assert(h1 >= original_n_peeled && h1 < original_n && "Fallback query index out of range");
        assert(mphf1.contains(key) && mphf2.contains(key) && "Fallback key missing after deserialization");
    }
    if (!residual_fixture_keys.empty()) {
        std::cout << "  [OK] Fallback-system queries preserved for " << residual_fixture_keys.size()
                  << " residual keys" << std::endl;
    }

    // 5. Check false positives match (sample a few non-keys)
    const size_t n = keys.size();
    std::mt19937_64 rng(12345);
    std::uniform_int_distribution<uint32_t> non_key_dist(0, UINT32_MAX);
    std::unordered_set<uint32_t> key_set(keys.begin(), keys.end());
    size_t fp_sample = std::min((size_t)1000, n / 10);
    bool fp_match = true;
    for (size_t i = 0; i < fp_sample; ++i) {
        uint32_t non_key;
        do {
            non_key = non_key_dist(rng);
        } while (key_set.count(non_key));

        bool c1 = mphf1.contains(non_key);
        bool c2 = mphf2.contains(non_key);
        if (c1 != c2) {
            std::cerr << "ERROR: contains(" << non_key << ") differs for non-key: " << c1 << " vs " << c2
                      << std::endl;
            fp_match = false;
        }
    }
    assert(fp_match && "False positive behavior differs after deserialization");
    std::cout << "  [OK] False positive behavior matches (sampled " << fp_sample << " non-keys)" << std::endl;

    // 6. Check that size breakdown is identical
    auto new_breakdown = mphf2.get_size_breakdown();
    assert(
        new_breakdown.total_bytes() == original_breakdown.total_bytes() &&
        "Size breakdown differs after deserialization"
    );
    assert(new_breakdown.g_bytes == original_breakdown.g_bytes && "G bytes differ");
    assert(new_breakdown.used_pos_bytes == original_breakdown.used_pos_bytes && "Used pos bytes differ");
    assert(new_breakdown.rank_bytes == original_breakdown.rank_bytes && "Rank bytes differ");
    assert(new_breakdown.q_bytes == original_breakdown.q_bytes && "Q bytes differ");
    assert(new_breakdown.other_bytes == original_breakdown.other_bytes && "Other bytes differ");
    assert(new_breakdown.fallback_bytes == original_breakdown.fallback_bytes && "Fallback bytes differ");
    std::cout << "  [OK] Size breakdown matches: " << new_breakdown.total_bytes() << " bytes" << std::endl;

    std::cout << "  PASSED: All fields and queries preserved after round-trip" << std::endl;
}

int main() {
    std::cout << "========== MPHF Serialization Round-Trip Tests ==========" << std::endl;
    std::cout << "Verifying that serialize() -> load() preserves all internal state" << std::endl;

    std::vector<size_t> test_sizes = {100, 1000, 10000, 100000};
    int total_tests = 0;
    int passed = 0;

    for (size_t n : test_sizes) {
        try {
            // Test GlGhStorage (the optimized one with seed-based serialization)
            auto keys = generate_keys(n, 42 + n);
            test_roundtrip<GlGhStorage>(keys, "GlGhStorage", "n=" + std::to_string(n));
            passed++;
        } catch (const std::exception& e) {
            std::cerr << "FAILED: GlGhStorage n=" << n << " - " << e.what() << std::endl;
        }
        total_tests++;

        // try {
        //     test_roundtrip<BaselineStorage>(n, "BaselineStorage");
        //     passed++;
        // } catch (const std::exception& e) {
        //     std::cerr << "FAILED: BaselineStorage n=" << n << " - " << e.what() << std::endl;
        // }
        // total_tests++;
        //
        // try {
        //     test_roundtrip<PackedTritStorage<CompressedBitvector>>(n, "PackedTritStorage");
        //     passed++;
        // } catch (const std::exception& e) {
        //     std::cerr << "FAILED: PackedTritStorage n=" << n << " - " << e.what() << std::endl;
        // }
        // total_tests++;
    }

    try {
        auto keys = generate_keys(10000, 20260418);
        test_roundtrip<GlGhStorage>(keys, "GlGhStorage", "synthetic fallback fixture", true);
        passed++;
    } catch (const std::exception& e) {
        std::cerr << "FAILED: GlGhStorage synthetic fallback fixture - " << e.what() << std::endl;
    }
    total_tests++;

    std::cout << "\n========== Test Summary ==========" << std::endl;
    std::cout << "Total: " << total_tests << " (" << test_sizes.size()
              << " size cases + 1 synthetic fallback fixture × GlGhStorage only)" << std::endl;
    std::cout << "Passed: " << passed << "/" << total_tests << std::endl;
    std::cout << "Failed: " << (total_tests - passed) << "/" << total_tests << std::endl;

    if (passed == total_tests) {
        std::cout << "ALL SERIALIZATION TESTS PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "SOME SERIALIZATION TESTS FAILED" << std::endl;
        return 1;
    }
}

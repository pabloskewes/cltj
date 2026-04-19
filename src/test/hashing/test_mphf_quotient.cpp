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

namespace {

struct TestDataset {
    std::string name;
    std::vector<uint64_t> keys;
    std::vector<uint64_t> guaranteed_non_keys;
};

// All key generators use uint32 range because premix32 maps keys to [0, 2^32)
// before hashing. Using 64-bit keys would risk silent 32-bit collisions between
// keys and non-keys, causing spurious test failures.

TestDataset make_random_dataset(size_t n, uint64_t seed) {
    TestDataset dataset;
    dataset.name = "random-u32";
    std::unordered_set<uint64_t> key_set;
    key_set.reserve(n);
    std::mt19937 rng(static_cast<uint32_t>(seed));
    while (key_set.size() < n) {
        key_set.insert(static_cast<uint64_t>(rng()));
    }
    dataset.keys.assign(key_set.begin(), key_set.end());

    std::mt19937 rng_nk(static_cast<uint32_t>(seed ^ 0xABCDEF12ULL));
    for (size_t i = 0; i < n; ++i) {
        uint64_t candidate;
        do {
            candidate = static_cast<uint64_t>(rng_nk());
        } while (key_set.count(candidate));
        dataset.guaranteed_non_keys.push_back(candidate);
    }
    return dataset;
}

TestDataset make_even_odd_dataset(size_t n) {
    TestDataset dataset;
    dataset.name = "even-odd-u32";
    std::unordered_set<uint64_t> unique;
    unique.reserve(n);
    std::mt19937 rng(static_cast<uint32_t>(0xEADDA11CEULL));
    while (unique.size() < n) {
        uint64_t value = static_cast<uint64_t>(rng()) & ~uint64_t(1);
        unique.insert(value);
    }
    for (auto v : unique) {
        dataset.keys.push_back(v);
        dataset.guaranteed_non_keys.push_back(v | 1ULL);
    }
    return dataset;
}

// Adversarial: consecutive keys [base, base+n). This is the primary case premix32
// is designed to solve — dense arithmetic structure that causes peeling failure
// with linear hash families.
TestDataset make_consecutive_dataset(size_t n, uint64_t base = 0) {
    TestDataset dataset;
    dataset.name = "consecutive-" + std::to_string(base) + "+" + std::to_string(n);
    for (size_t i = 0; i < n; ++i) {
        dataset.keys.push_back(base + i);
    }
    for (size_t i = 1; i <= n; ++i) {
        dataset.guaranteed_non_keys.push_back(base + n + i);
    }
    return dataset;
}

// Adversarial: arithmetic progression {base, base+step, base+2*step, ...}.
// Exercises the remainder-zero failure mode (all keys share residue structure).
TestDataset make_arithmetic_progression_dataset(size_t n, uint64_t base, uint64_t step) {
    TestDataset dataset;
    dataset.name = "arith-step" + std::to_string(step);
    for (size_t i = 0; i < n; ++i) {
        dataset.keys.push_back(base + i * step);
    }
    for (size_t i = 0; i < n; ++i) {
        dataset.guaranteed_non_keys.push_back(base + i * step + 1);
    }
    return dataset;
}

bool run_dataset_test(const TestDataset& dataset,
                      uint64_t max_key_domain = std::numeric_limits<uint64_t>::max()) {
    MPHF<GlGhStorage, QuotientKey> mphf;
    if (!mphf.build(dataset.keys)) {
        std::cerr << "QuotientKey test failure: build failed for dataset " << dataset.name << "."
                  << std::endl;
        return false;
    }

    for (auto key : dataset.keys) {
        if (!mphf.contains(key)) {
            std::cerr << "QuotientKey test failure: contains rejected " << key << " in dataset "
                      << dataset.name << "." << std::endl;
            return false;
        }
    }

    for (auto non_key : dataset.guaranteed_non_keys) {
        if (mphf.contains(non_key)) {
            std::cerr << "QuotientKey test failure: contains accepted " << non_key << " (dataset "
                      << dataset.name << ")." << std::endl;
            return false;
        }
    }

    auto primes = mphf.get_primes();
    __uint128_t jump128 = static_cast<__uint128_t>(primes[0]) * primes[1] * primes[2];
    if (jump128 >= (static_cast<__uint128_t>(1) << 64)) {
        std::cerr << "QuotientKey test failure: jump overflowed for dataset " << dataset.name
                  << "." << std::endl;
        return false;
    }

    // Under premix32, key+product no longer targets the algebraic collision it was designed
    // for (premix destroys the linear relationship). It effectively tests that an arbitrary
    // non-key in a different region of the uint64 space is rejected.
    for (auto key : dataset.keys) {
        __uint128_t fake128 = static_cast<__uint128_t>(key) + jump128;
        if (fake128 >= (static_cast<__uint128_t>(1) << 64)) {
            continue;
        }
        uint64_t fake = static_cast<uint64_t>(fake128);
        if (mphf.contains(fake)) {
            std::cerr << "QuotientKey test failure: key+product accepted (dataset " << dataset.name
                      << ")." << std::endl;
            return false;
        }
    }

    auto breakdown = mphf.get_size_breakdown();
    double q_bits_per_key = (breakdown.q_bytes * 8.0) / static_cast<double>(dataset.keys.size());
    uint64_t p_min = std::min({primes[0], primes[1], primes[2]});
    // q_max reflects the highest possible premixed key value divided by the smallest prime.
    // For 64-bit keys: max_key_domain = UINT64_MAX.
    // For 32-bit keys (e.g. Wikidata IDs): max_key_domain = UINT32_MAX.
    uint64_t q_max = max_key_domain / p_min;
    double expected_width = static_cast<double>(sdsl::bits::hi(q_max) + 1);
    if (q_bits_per_key < expected_width || q_bits_per_key > expected_width + 0.5) {
        std::cerr << "QuotientKey test failure: q_bits_per_key=" << q_bits_per_key
                  << " expected~=" << expected_width << " (dataset " << dataset.name << ")."
                  << std::endl;
        return false;
    }

    return true;
}

}  // namespace

int main() {
    // premix32 maps all keys to [0, 2^32) before hashing, so the effective quotient
    // domain is bounded by UINT32_MAX regardless of input key width.
    constexpr uint64_t premix_domain = std::numeric_limits<uint32_t>::max();
    int n_passed = 0;

    // --- Group 1: Random keys (uint32 range) ---
    std::vector<TestDataset> random_datasets;
    random_datasets.push_back(make_random_dataset(4000, 0xDEADBEEF12345678ULL));
    random_datasets.push_back(make_random_dataset(8000, 0xBADCAFE987654321ULL));
    random_datasets.push_back(make_even_odd_dataset(2048));

    for (const auto& dataset : random_datasets) {
        if (!run_dataset_test(dataset, premix_domain)) {
            return 1;
        }
        std::cout << "  PASS: " << dataset.name << " (n=" << dataset.keys.size() << ")" << std::endl;
        ++n_passed;
    }

    // --- Group 2: Adversarial — consecutive and arithmetic keys ---
    // These are the cases that caused peeling failure before premix32.
    std::vector<TestDataset> adversarial_datasets;
    adversarial_datasets.push_back(make_consecutive_dataset(1000));
    adversarial_datasets.push_back(make_consecutive_dataset(5000));
    adversarial_datasets.push_back(make_consecutive_dataset(5000, 100000));
    adversarial_datasets.push_back(make_arithmetic_progression_dataset(1000, 3, 3));
    adversarial_datasets.push_back(make_arithmetic_progression_dataset(2000, 0, 7));

    for (const auto& dataset : adversarial_datasets) {
        if (!run_dataset_test(dataset, premix_domain)) {
            return 1;
        }
        std::cout << "  PASS: " << dataset.name << " (n=" << dataset.keys.size() << ")" << std::endl;
        ++n_passed;
    }

    std::cout << "QuotientKey tests: all " << n_passed << " datasets passed." << std::endl;
    return 0;
}

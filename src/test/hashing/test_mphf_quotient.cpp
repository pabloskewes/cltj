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

std::vector<uint64_t> make_random_keys(size_t n, uint64_t seed) {
    std::unordered_set<uint64_t> unique;
    unique.reserve(n);
    std::mt19937_64 rng(seed);
    while (unique.size() < n) {
        unique.insert(rng());
    }
    return std::vector<uint64_t>(unique.begin(), unique.end());
}

TestDataset make_random_dataset(size_t n, uint64_t seed) {
    TestDataset dataset;
    dataset.name = "random";
    dataset.keys = make_random_keys(n, seed);
    std::mt19937_64 rng(seed ^ 0xABCDEF1234567890ULL);
    for (size_t i = 0; i < n; ++i) {
        uint64_t candidate;
        do {
            candidate = rng();
        } while (std::find(dataset.keys.begin(), dataset.keys.end(), candidate) != dataset.keys.end());
        dataset.guaranteed_non_keys.push_back(candidate);
    }
    return dataset;
}

TestDataset make_even_odd_dataset(size_t n) {
    TestDataset dataset;
    dataset.name = "even";
    std::unordered_set<uint64_t> unique;
    unique.reserve(n);
    std::mt19937_64 rng(0xEADDA11CEULL);
    while (unique.size() < n) {
        uint64_t value = rng() & ~uint64_t(1);  // force even
        unique.insert(value);
    }
    for (auto v : unique) {
        dataset.keys.push_back(v);
        dataset.guaranteed_non_keys.push_back(v | 1ULL);
    }
    return dataset;
}

bool run_dataset_test(const TestDataset& dataset) {
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

    for (auto key : dataset.keys) {
        __uint128_t fake128 = static_cast<__uint128_t>(key) + jump128;
        if (fake128 >= (static_cast<__uint128_t>(1) << 64)) {
            continue;  // Skip overflow; extremely unlikely but safe.
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
    uint64_t q_max = std::numeric_limits<uint64_t>::max() / p_min;
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
    std::vector<TestDataset> datasets;
    datasets.push_back(make_random_dataset(4000, 0xDEADBEEF12345678ULL));
    datasets.push_back(make_random_dataset(8000, 0xBADCAFE987654321ULL));
    datasets.push_back(make_even_odd_dataset(2048));

    for (const auto& dataset : datasets) {
        if (!run_dataset_test(dataset)) {
            return 1;
        }
    }

    std::cout << "QuotientKey tests passed." << std::endl;
    return 0;
}

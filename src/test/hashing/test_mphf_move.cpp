// Tests that MPHF<GlGhStorage, QuotientKey> is movable and remains correct
// after being stored in a std::vector (which requires move during reallocation).
#include <hashing/mphf_bdz.hpp>
#include <hashing/storage/glgh.hpp>
#include <hashing/key_policies.hpp>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

using MPHF_type = cltj::hashing::MPHF<cltj::hashing::GlGhStorage, cltj::hashing::policies::QuotientKey>;

static std::vector<uint32_t> make_keys(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::unordered_set<uint32_t> seen;
    std::uniform_int_distribution<uint32_t> dist(1, static_cast<uint32_t>(n * 100));
    while (seen.size() < n)
        seen.insert(dist(rng));
    return std::vector<uint32_t>(seen.begin(), seen.end());
}

int main() {
    const size_t N = 5000;
    auto keys = make_keys(N, 42);

    // Build N/1000 small MPHFs and push them into a vector.
    // The vector will reallocate and move elements internally.
    std::vector<MPHF_type> mphfs;
    std::vector<std::vector<uint32_t>> all_keys;

    const size_t num_mphfs = 5;
    const size_t keys_per_mphf = N / num_mphfs;

    for (size_t i = 0; i < num_mphfs; ++i) {
        std::vector<uint32_t> chunk(keys.begin() + i * keys_per_mphf, keys.begin() + (i + 1) * keys_per_mphf);
        all_keys.push_back(chunk);

        MPHF_type mphf;
        bool ok = mphf.build(chunk);
        if (!ok) {
            std::cerr << "FAIL: build failed for chunk " << i << std::endl;
            return 1;
        }
        mphfs.push_back(std::move(mphf));
    }

    // Verify each MPHF still works correctly after potential reallocation.
    int failures = 0;
    for (size_t i = 0; i < num_mphfs; ++i) {
        for (uint32_t key : all_keys[i]) {
            if (!mphfs[i].contains(key)) {
                std::cerr << "FAIL: contains() returned false for key " << key << " in mphf " << i
                          << " (after move into vector)" << std::endl;
                ++failures;
            }
        }
    }

    if (failures == 0) {
        std::cout << "PASS: all " << num_mphfs << " MPHFs correct after move into vector." << std::endl;
        return 0;
    }
    return 1;
}

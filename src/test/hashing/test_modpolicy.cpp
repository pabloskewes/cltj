#include <hashing/mphf_bdz.hpp>
#include <hashing/storage/glgh.hpp>
#include <util/logger.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <unordered_set>
#include <vector>

using cltj::hashing::GlGhStorage;
using cltj::hashing::MPHF;
using cltj::hashing::next_prime;
using cltj::hashing::premix32;
using cltj::hashing::policies::FastMod;
using cltj::hashing::policies::NativeMod;
using cltj::hashing::policies::QuotientKey;

namespace {

using MPHFNative = MPHF<GlGhStorage, QuotientKey, NativeMod>;
using MPHFFast = MPHF<GlGhStorage, QuotientKey, FastMod>;

uint64_t regime_prime(uint64_t n, int which) {
    uint64_t target = std::max<uint64_t>(3, ((uint64_t)std::ceil(1.25 * (double)n) + 2) / 3);
    uint64_t p = next_prime(target);
    for (int i = 0; i < which; ++i) {
        p = next_prime(p + 1);
    }
    return p;
}

std::vector<uint32_t> make_random_keys(size_t n, uint32_t seed) {
    std::unordered_set<uint32_t> key_set;
    key_set.reserve(n);
    std::mt19937 rng(seed);
    while (key_set.size() < n) {
        key_set.insert(rng());
    }
    return std::vector<uint32_t>(key_set.begin(), key_set.end());
}

std::vector<uint32_t> make_non_keys(const std::vector<uint32_t>& keys, size_t n, uint32_t seed) {
    std::unordered_set<uint32_t> key_set(keys.begin(), keys.end());
    std::vector<uint32_t> non_keys;
    std::mt19937 rng(seed);
    while (non_keys.size() < n) {
        uint32_t candidate = rng();
        if (!key_set.count(candidate)) {
            non_keys.push_back(candidate);
        }
    }
    return non_keys;
}

bool test_fastmod_equivalence() {
    LOG_INFO("Testing fastmod_u64 vs native % ...");
    size_t checked = 0;
    for (uint64_t n : {1000UL, 100000UL, 10000000UL}) {
        for (int k = 0; k < 3; ++k) {
            uint64_t p = regime_prime(n, k);
            __uint128_t M = fastmod::computeM_u64(p);
            std::mt19937_64 rng(n * 10 + k);
            std::uniform_int_distribution<uint64_t> distA(1, p - 1);
            uint64_t a = distA(rng);

            std::vector<uint64_t> xs = {0, 1, 2, p - 1, p, p + 1, 0xFFFFFFFFull};
            std::uniform_int_distribution<uint32_t> distX;
            for (int i = 0; i < 10000; ++i) {
                xs.push_back(premix32(distX(rng)));
            }
            for (uint64_t x : xs) {
                if (fastmod::fastmod_u64(x * a, M, p) != (x * a) % p) return false;
                ++checked;
            }
        }
    }
    LOG_INFO("fastmod_u64 == native % on " << checked << " cases");
    return true;
}

bool test_mphf_equivalence(const std::vector<uint32_t>& keys, const std::vector<uint32_t>& non_keys) {
    MPHFNative mphf_native;
    MPHFFast mphf_fast;
    if (!mphf_native.build(keys)) return false;
    if (!mphf_fast.build(keys)) return false;

    for (uint32_t k : keys) {
        if (mphf_native.query(k) != mphf_fast.query(k)) return false;
        auto ln = mphf_native.locate(k);
        auto lf = mphf_fast.locate(k);
        if (!(ln.first && lf.first)) return false;
        if (ln.second != lf.second) return false;
    }
    for (uint32_t nk : non_keys) {
        if (mphf_native.locate(nk) != mphf_fast.locate(nk)) return false;
    }
    return true;
}

bool test_round_trip(const std::vector<uint32_t>& keys) {
    MPHFFast mphf;
    if (!mphf.build(keys)) return false;

    std::stringstream ss;
    mphf.serialize(ss);
    MPHFFast loaded;
    loaded.load(ss);

    for (uint32_t k : keys) {
        if (mphf.query(k) != loaded.query(k)) return false;
        auto lf = loaded.locate(k);
        if (!(lf.first && lf.second == mphf.locate(k).second)) return false;
    }
    return true;
}

}  // namespace

int main() {
    if (!test_fastmod_equivalence()) return 1;

    auto keys = make_random_keys(20000, 12345);
    auto non_keys = make_non_keys(keys, 20000, 999);
    LOG_INFO("Testing MPHF equivalence (random keys) ...");
    if (!test_mphf_equivalence(keys, non_keys)) return 1;

    std::vector<uint32_t> consecutive(20000);
    for (uint32_t i = 0; i < consecutive.size(); ++i) {
        consecutive[i] = 1000000 + i;
    }
    auto cons_non_keys = make_non_keys(consecutive, 20000, 555);
    LOG_INFO("Testing MPHF equivalence (consecutive keys) ...");
    if (!test_mphf_equivalence(consecutive, cons_non_keys)) return 1;

    LOG_INFO("Testing FastMod serialize/load round-trip ...");
    if (!test_round_trip(keys)) return 1;
    if (!test_round_trip(consecutive)) return 1;

    LOG_INFO("All mod policy tests passed successfully!");
    return 0;
}

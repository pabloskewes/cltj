#include <hashing/mphf_bdz.hpp>
#include <hashing/storage/glgh.hpp>
#include <hashing/storage/packed_glgh.hpp>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <sstream>
#include <unordered_set>
#include <vector>

using cltj::hashing::GlGhStorage;
using cltj::hashing::MPHF;
using cltj::hashing::PackedGlGhStorage;
using cltj::hashing::policies::NoKey;
using cltj::hashing::policies::QuotientKey;

std::vector<uint32_t> consecutive_keys(size_t n) {
    std::vector<uint32_t> keys(n);
    for (size_t i = 0; i < n; ++i) {
        keys[i] = static_cast<uint32_t>(i + 1);
    }
    return keys;
}

std::vector<uint32_t> random_keys(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::unordered_set<uint32_t> seen;
    seen.reserve(n * 2);
    std::uniform_int_distribution<uint32_t> dist(1, static_cast<uint32_t>(n * 100));

    while (seen.size() < n) {
        seen.insert(dist(rng));
    }
    return std::vector<uint32_t>(seen.begin(), seen.end());
}

std::vector<uint32_t> non_keys(const std::vector<uint32_t>& keys, size_t count) {
    std::unordered_set<uint32_t> key_set(keys.begin(), keys.end());
    std::vector<uint32_t> result;
    result.reserve(count);

    uint32_t candidate = 1;
    while (result.size() < count) {
        if (!key_set.count(candidate)) {
            result.push_back(candidate);
        }
        ++candidate;
    }
    return result;
}

template <typename Mphf>
bool check_permutation(const Mphf& mphf, const std::vector<uint32_t>& keys) {
    std::vector<uint8_t> seen(keys.size(), 0);
    for (uint32_t key : keys) {
        uint32_t h = mphf.query(key);
        if (h >= keys.size())
            return false;
        if (seen[h])
            return false;
        seen[h] = 1;
    }
    return std::all_of(seen.begin(), seen.end(), [](uint8_t v) { return v != 0; });
}

bool check_nokey_equivalence(const std::vector<uint32_t>& keys) {
    MPHF<GlGhStorage, NoKey> glgh;
    MPHF<PackedGlGhStorage, NoKey> packed;

    if (!glgh.build(keys))
        return false;
    if (!packed.build(keys))
        return false;

    if (glgh.n() != packed.n())
        return false;
    if (glgh.m() != packed.m())
        return false;
    if (glgh.retry_count() != packed.retry_count())
        return false;
    if (glgh.n_peeled() != packed.n_peeled())
        return false;
    if (glgh.n_residual() != packed.n_residual())
        return false;

    if (!check_permutation(glgh, keys))
        return false;
    if (!check_permutation(packed, keys))
        return false;

    for (uint32_t key : keys) {
        if (packed.query(key) != glgh.query(key))
            return false;
    }
    return true;
}

bool check_quotient_equivalence(const std::vector<uint32_t>& keys) {
    MPHF<GlGhStorage, QuotientKey> glgh;
    MPHF<PackedGlGhStorage, QuotientKey> packed;

    if (!glgh.build(keys))
        return false;
    if (!packed.build(keys))
        return false;

    if (glgh.n() != packed.n())
        return false;
    if (glgh.m() != packed.m())
        return false;
    if (glgh.retry_count() != packed.retry_count())
        return false;
    if (glgh.n_peeled() != packed.n_peeled())
        return false;
    if (glgh.n_residual() != packed.n_residual())
        return false;

    for (uint32_t key : keys) {
        auto a = glgh.locate(key);
        auto b = packed.locate(key);
        if (!a.first || !b.first)
            return false;
        if (a.second != b.second)
            return false;
        if (glgh.query(key) != packed.query(key))
            return false;
    }

    for (uint32_t key : non_keys(keys, 1000)) {
        if (glgh.contains(key))
            return false;
        if (packed.contains(key))
            return false;
    }
    return true;
}

bool check_round_trip(const std::vector<uint32_t>& keys) {
    MPHF<PackedGlGhStorage, QuotientKey> packed;
    if (!packed.build(keys))
        return false;

    std::stringstream ss;
    packed.serialize(ss);

    MPHF<PackedGlGhStorage, QuotientKey> loaded;
    loaded.load(ss);

    if (loaded.n() != packed.n())
        return false;
    if (loaded.m() != packed.m())
        return false;
    if (loaded.n_peeled() != packed.n_peeled())
        return false;
    if (loaded.n_residual() != packed.n_residual())
        return false;

    for (uint32_t key : keys) {
        if (loaded.query(key) != packed.query(key))
            return false;
        if (loaded.locate(key) != packed.locate(key))
            return false;
    }

    for (uint32_t key : non_keys(keys, 1000)) {
        if (loaded.contains(key))
            return false;
    }
    return true;
}

int main() {
    std::cout << "Testing PackedGlGhStorage inside MPHF ...\n";
    int failures = 0;

    std::vector<std::vector<uint32_t>> datasets;
    datasets.push_back(consecutive_keys(1000));
    datasets.push_back(consecutive_keys(10000));
    datasets.push_back(random_keys(1000, 42));
    datasets.push_back(random_keys(10000, 43));

    {
        bool ok = true;
        for (const auto& keys : datasets) {
            if (!check_nokey_equivalence(keys)) {
                ok = false;
                break;
            }
        }
        if (!ok)
            ++failures;
        std::cout << "  NoKey equivalence vs GlGhStorage: " << (ok ? "OK" : "FAIL") << "\n";
    }

    {
        bool ok = true;
        for (const auto& keys : datasets) {
            if (!check_quotient_equivalence(keys)) {
                ok = false;
                break;
            }
        }
        if (!ok)
            ++failures;
        std::cout << "  QuotientKey equivalence vs GlGhStorage: " << (ok ? "OK" : "FAIL") << "\n";
    }

    {
        bool ok = check_round_trip(random_keys(10000, 7));
        if (!ok)
            ++failures;
        std::cout << "  Packed MPHF serialize/load round-trip: " << (ok ? "OK" : "FAIL") << "\n";
    }

    if (failures == 0) {
        std::cout << "All PackedGlGhStorage MPHF tests passed.\n";
    }
    return failures;
}

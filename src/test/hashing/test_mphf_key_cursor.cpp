// Tests for MPHF::key_cursor: enumerating the key set in slot order by walking
// the occupied vertices of B, with no select support.
// Covers reconstruction from the quotient, the FullKey payload, the generic
// occupancy_word default, the fallback residual chain, the empty MPHF, and the
// serialization round-trip (where the modular inverses are recomputed).

#include <hashing/storage/baseline.hpp>
#include <hashing/storage/glgh.hpp>

// Test-only access to synthesize a fallback-active fixture.
#define private public
#include <hashing/mphf_bdz.hpp>
#undef private

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using cltj::hashing::BaselineStorage;
using cltj::hashing::GlGhStorage;
using cltj::hashing::MPHF;
using cltj::hashing::policies::FullKey;
using cltj::hashing::policies::NoKey;
using cltj::hashing::policies::QuotientKey;

namespace {

std::vector<uint32_t> make_random_keys(size_t n, uint64_t seed) {
    std::unordered_set<uint32_t> unique;
    unique.reserve(n);
    std::mt19937_64 rng(seed);
    while (unique.size() < n) {
        unique.insert(static_cast<uint32_t>(rng()));
    }
    return std::vector<uint32_t>(unique.begin(), unique.end());
}

std::vector<uint32_t> make_consecutive_keys(size_t n, uint32_t base) {
    std::vector<uint32_t> keys;
    keys.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        keys.push_back(base + static_cast<uint32_t>(i));
    }
    return keys;
}

/**
 * @brief The core check: the cursor must reproduce the key set in slot order.
 *
 * Slots must come out as 0..n-1, the emitted keys must be exactly the input set,
 * and every emitted key must locate() back to the slot it was emitted at. The
 * round-trip needs no external ground truth, so it also runs on real indices.
 */
template <typename Mphf>
bool check_cursor(const Mphf& mphf, const std::vector<uint32_t>& keys, const std::string& label) {
    std::unordered_set<uint32_t> pending(keys.begin(), keys.end());
    uint32_t expected_slot = 0;

    for (auto cur = mphf.keys(); cur.next();) {
        if (cur.slot() != expected_slot) {
            std::cerr << "key_cursor failure [" << label << "]: slot " << cur.slot() << ", expected "
                      << expected_slot << "." << std::endl;
            return false;
        }
        if (pending.erase(cur.key()) == 0) {
            std::cerr << "key_cursor failure [" << label << "]: key " << cur.key()
                      << " is not in the input set (or came out twice)." << std::endl;
            return false;
        }
        auto [found, slot] = mphf.locate(cur.key());
        if (!found || slot != cur.slot()) {
            std::cerr << "key_cursor failure [" << label << "]: locate(" << cur.key() << ") = {" << found
                      << ", " << slot << "}, cursor said slot " << cur.slot() << "." << std::endl;
            return false;
        }
        ++expected_slot;
    }

    if (expected_slot != keys.size()) {
        std::cerr << "key_cursor failure [" << label << "]: emitted " << expected_slot << " keys, expected "
                  << keys.size() << "." << std::endl;
        return false;
    }
    if (!pending.empty()) {
        std::cerr << "key_cursor failure [" << label << "]: " << pending.size()
                  << " input keys were never emitted." << std::endl;
        return false;
    }
    return true;
}

template <typename Mphf>
bool build_and_check(const std::vector<uint32_t>& keys, const std::string& label) {
    Mphf mphf;
    if (!mphf.build(keys)) {
        std::cerr << "key_cursor failure [" << label << "]: build failed." << std::endl;
        return false;
    }
    if (!check_cursor(mphf, keys, label))
        return false;
    std::cout << "  PASS: " << label << " (n=" << keys.size() << ")" << std::endl;
    return true;
}

/**
 * @brief Move the highest-slot keys into the residual array to force a fallback.
 *
 * Those keys keep their vertices occupied, but n_peeled_ shrinks so the cursor
 * stops before them and must pick them up from residual_keys_ instead.
 */
template <typename Mphf>
void synthesize_fallback_fixture(Mphf& mphf, const std::vector<uint32_t>& keys, size_t residual_count) {
    assert(residual_count > 0 && residual_count <= keys.size());

    std::vector<std::pair<uint32_t, uint32_t>> indexed_keys;
    indexed_keys.reserve(keys.size());
    for (uint32_t key : keys) {
        indexed_keys.emplace_back(mphf.query(key), key);
    }
    std::sort(indexed_keys.begin(), indexed_keys.end());

    mphf.residual_keys_.clear();
    for (size_t i = indexed_keys.size() - residual_count; i < indexed_keys.size(); ++i) {
        mphf.residual_keys_.push_back(cltj::hashing::premix32(indexed_keys[i].second));
    }
    std::sort(mphf.residual_keys_.begin(), mphf.residual_keys_.end());
    mphf.n_peeled_ = static_cast<uint32_t>(keys.size() - residual_count);
}

bool test_fallback_chain(const std::vector<uint32_t>& keys, size_t residual_count) {
    const std::string label = "fallback residual=" + std::to_string(residual_count);
    MPHF<GlGhStorage, QuotientKey> mphf;
    if (!mphf.build(keys)) {
        std::cerr << "key_cursor failure [" << label << "]: build failed." << std::endl;
        return false;
    }
    synthesize_fallback_fixture(mphf, keys, residual_count);
    if (mphf.n_residual() != residual_count) {
        std::cerr << "key_cursor failure [" << label << "]: fixture did not take." << std::endl;
        return false;
    }
    if (!check_cursor(mphf, keys, label))
        return false;
    std::cout << "  PASS: " << label << " (n=" << keys.size() << ", peeled=" << mphf.n_peeled() << ")"
              << std::endl;
    return true;
}

// An unbuilt MPHF is a valid empty state, the same one locate() answers with
// {false, 0}: the cursor must yield no keys rather than abort.
bool test_empty_mphf() {
    MPHF<GlGhStorage, QuotientKey> mphf;
    if (mphf.keys().next()) {
        std::cerr << "key_cursor failure [unbuilt MPHF]: emitted a key." << std::endl;
        return false;
    }
    std::cout << "  PASS: unbuilt MPHF (n=0)" << std::endl;
    return true;
}

bool test_serialization_round_trip(const std::vector<uint32_t>& keys) {
    const std::string label = "serialization round-trip";
    MPHF<GlGhStorage, QuotientKey> original;
    if (!original.build(keys)) {
        std::cerr << "key_cursor failure [" << label << "]: build failed." << std::endl;
        return false;
    }

    std::stringstream buffer;
    original.serialize(buffer);
    buffer.seekg(0);

    MPHF<GlGhStorage, QuotientKey> restored;
    restored.load(buffer);

    if (!check_cursor(restored, keys, label))
        return false;
    std::cout << "  PASS: " << label << " (n=" << keys.size() << ")" << std::endl;
    return true;
}

}  // namespace

int main() {
    static_assert(!NoKey::supports_reconstruction, "NoKey must not expose a key cursor");
    static_assert(FullKey::supports_reconstruction, "FullKey holds the mixed keys verbatim");
    static_assert(QuotientKey::supports_reconstruction, "QuotientKey rebuilds keys from the quotient");

    const std::vector<uint32_t> random_keys = make_random_keys(4000, 0xDEADBEEF12345678ULL);
    const std::vector<uint32_t> small_keys = make_random_keys(37, 0xBADCAFE987654321ULL);
    const std::vector<uint32_t> consecutive_keys = make_consecutive_keys(5000, 100000);

    // QuotientKey: the reconstruction path this ticket is about.
    if (!build_and_check<MPHF<GlGhStorage, QuotientKey>>(random_keys, "GlGh + QuotientKey, random"))
        return 1;
    if (!build_and_check<MPHF<GlGhStorage, QuotientKey>>(small_keys, "GlGh + QuotientKey, n=37"))
        return 1;
    if (!build_and_check<MPHF<GlGhStorage, QuotientKey>>(consecutive_keys, "GlGh + QuotientKey, consecutive"))
        return 1;

    // FullKey: the payload is already the mixed key, so the cursor only unmixes it.
    if (!build_and_check<MPHF<GlGhStorage, FullKey>>(random_keys, "GlGh + FullKey, random"))
        return 1;

    // BaselineStorage exercises the generic occupancy_word default of the CRTP base.
    if (!build_and_check<MPHF<BaselineStorage, QuotientKey>>(random_keys, "Baseline + QuotientKey, random"))
        return 1;

    if (!test_fallback_chain(random_keys, 1))
        return 1;
    if (!test_fallback_chain(random_keys, 17))
        return 1;
    // Nothing peeled: reachable for real, since a node at the threshold that peels
    // nothing leaves a residual below MAX_FALLBACK_RESIDUAL.
    if (!test_fallback_chain(small_keys, small_keys.size()))
        return 1;

    if (!test_empty_mphf())
        return 1;

    if (!test_serialization_round_trip(random_keys))
        return 1;

    std::cout << "key_cursor tests: all datasets passed." << std::endl;
    return 0;
}

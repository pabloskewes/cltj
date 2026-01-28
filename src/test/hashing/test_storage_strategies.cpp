/*!
// Low-level regression tests for storage strategies. Every implementation is
// fed both sparse and fully dense patterns, toggles assignments before build(),
// and validates g_set/g_get, is_vertex_occupied, and rank invariants before and
// after compactification. The goal is to catch mismatches between G, B, and
// rank without running the full MPHF pipeline.
*/

#include <hashing/storage/baseline.hpp>
#include <hashing/storage/glgh.hpp>
#include <hashing/storage/packed_trit.hpp>
#include <cassert>
#include <array>
#include <iostream>

using cltj::hashing::BaselineStorage;
using cltj::hashing::CompressedBitvector;
using cltj::hashing::ExplicitBitvector;
using cltj::hashing::GlGhStorage;
using cltj::hashing::PackedTritStorage;

namespace {

constexpr uint32_t kNumVertices = 12;
constexpr std::array<uint32_t, kNumVertices> kSparsePattern = {
    3,
    0,
    3,
    1,
    3,
    3,
    2,
    1,
    3,
    0,
    3,
    3,
};
constexpr std::array<uint32_t, kNumVertices> kDensePattern = {
    0,
    1,
    2,
    0,
    1,
    2,
    0,
    1,
    2,
    0,
    1,
    2,
};

template <typename Storage>
void run_storage_checks(const char* name, const std::array<uint32_t, kNumVertices>& pattern) {
    Storage storage;
    storage.initialize(kNumVertices);
    assert(storage.m() == kNumVertices);

    for (uint32_t v = 0; v < kNumVertices; ++v) {
        if (pattern[v] != 3) {
            storage.g_set(v, pattern[v]);
        }
    }

    // Toggle vertices pre-build to ensure transitions between used/un-used states behave.
    int toggle_free = -1;
    for (uint32_t v = 0; v < kNumVertices; ++v) {
        if (pattern[v] == 3) {
            toggle_free = static_cast<int>(v);
            break;
        }
    }
    if (toggle_free >= 0) {
        const uint32_t tv = static_cast<uint32_t>(toggle_free);
        storage.g_set(tv, 2);
        assert(storage.g_get(tv) == 2);
        assert(storage.is_vertex_occupied(tv));
        storage.g_set(tv, 3);
        assert(storage.g_get(tv) == 3);
        assert(!storage.is_vertex_occupied(tv));
    } else {
        // All vertices used: temporarily clear vertex 0 and restore.
        const uint32_t saved = storage.g_get(0);
        storage.g_set(0, 3);
        assert(storage.g_get(0) == 3);
        assert(!storage.is_vertex_occupied(0));
        storage.g_set(0, saved);
        assert(storage.is_vertex_occupied(0));
    }

    for (uint32_t v = 0; v < kNumVertices; ++v) {
        assert(storage.g_get(v) == pattern[v]);
        const bool expected = pattern[v] != 3;
        assert(storage.is_vertex_occupied(v) == expected);
    }
    storage.build_rank();

    auto expected_rank = [&pattern](uint32_t position) {
        uint32_t count = 0;
        for (uint32_t i = 0; i < position; ++i) {
            if (pattern[i] != 3) {
                ++count;
            }
        }
        return count;
    };

    const uint32_t total_used = expected_rank(kNumVertices);

    for (uint32_t v = 0; v < kNumVertices; ++v) {
        assert(storage.g_get(v) == pattern[v]);
        const bool expected = pattern[v] != 3;
        assert(storage.is_vertex_occupied(v) == expected);
    }
    assert(storage.g_get(kNumVertices + 5) == 3);
    assert(!storage.is_vertex_occupied(kNumVertices + 5));

    // Rank should increase exactly when a vertex is marked as used.
    uint32_t prev_rank = storage.rank(0);
    assert(prev_rank == 0);
    for (uint32_t v = 0; v < kNumVertices; ++v) {
        uint32_t next_rank = storage.rank(v + 1);
        const bool expected_used = pattern[v] != 3;
        const uint32_t diff = next_rank - prev_rank;
        assert(diff == (expected_used ? 1u : 0u));
        prev_rank = next_rank;
    }
    assert(storage.rank(kNumVertices) == total_used);

    for (uint32_t pos = 0; pos <= kNumVertices; ++pos) {
        assert(storage.rank(pos) == expected_rank(pos));
    }

    // Query rank at m() explicitly to ensure it returns the total number of used vertices.
    assert(storage.rank(storage.m()) == total_used);

    std::cout << name << " passed storage checks.\n";
}

}  // namespace

int main() {
    run_storage_checks<BaselineStorage>("BaselineStorage (sparse)", kSparsePattern);
    run_storage_checks<BaselineStorage>("BaselineStorage (dense)", kDensePattern);
    run_storage_checks<PackedTritStorage<ExplicitBitvector>>(
        "PackedTritStorage<ExplicitBitvector> (sparse)", kSparsePattern
    );
    run_storage_checks<PackedTritStorage<ExplicitBitvector>>(
        "PackedTritStorage<ExplicitBitvector> (dense)", kDensePattern
    );
    run_storage_checks<PackedTritStorage<CompressedBitvector>>(
        "PackedTritStorage<CompressedBitvector> (sparse)", kSparsePattern
    );
    run_storage_checks<PackedTritStorage<CompressedBitvector>>(
        "PackedTritStorage<CompressedBitvector> (dense)", kDensePattern
    );
    run_storage_checks<GlGhStorage>("GlGhStorage (sparse)", kSparsePattern);
    run_storage_checks<GlGhStorage>("GlGhStorage (dense)", kDensePattern);
    std::cout << "All storage strategies validated successfully.\n";
    return 0;
}

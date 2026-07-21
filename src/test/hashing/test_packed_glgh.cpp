#include <hashing/storage/glgh.hpp>
#include <hashing/storage/packed_glgh.hpp>

#include <iostream>
#include <random>
#include <sstream>
#include <vector>

using cltj::hashing::GlGhStorage;
using cltj::hashing::PackedGlGhStorage;

std::vector<uint32_t> random_values(size_t m, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<uint32_t> dist(0, 3);

    std::vector<uint32_t> values(m);
    for (size_t i = 0; i < m; ++i) {
        values[i] = dist(rng);
    }
    return values;
}

std::vector<uint32_t> filled_values(size_t m, uint32_t value) {
    return std::vector<uint32_t>(m, value);
}

void fill_storage(GlGhStorage& glgh, PackedGlGhStorage& packed, const std::vector<uint32_t>& values) {
    const uint32_t m = static_cast<uint32_t>(values.size());
    glgh.initialize(m);
    packed.initialize(m);

    for (uint32_t v = 0; v < m; ++v) {
        glgh.g_set(v, values[v]);
        packed.g_set(v, values[v]);
    }
}

bool check_against_glgh(const std::vector<uint32_t>& values) {
    GlGhStorage glgh;
    PackedGlGhStorage packed;
    fill_storage(glgh, packed, values);

    const uint32_t m = static_cast<uint32_t>(values.size());
    if (glgh.m() != packed.m())
        return false;

    for (uint32_t v = 0; v < m; ++v) {
        if (packed.g_get(v) != glgh.g_get(v))
            return false;
        if (packed.is_vertex_occupied(v) != glgh.is_vertex_occupied(v))
            return false;
    }

    if (packed.g_get(m + 10) != 3)
        return false;
    if (packed.is_vertex_occupied(m + 10))
        return false;

    glgh.build_rank();
    packed.build_rank();

    for (uint32_t v = 0; v <= m; ++v) {
        if (packed.rank(v) != glgh.rank(v))
            return false;
    }
    if (packed.rank(m + 10) != glgh.rank(m))
        return false;

    auto breakdown = packed.get_size_breakdown();
    if (breakdown.used_pos_bytes != 0)
        return false;
    if (breakdown.g_bytes == 0 && m > 0)
        return false;
    if (breakdown.rank_bytes == 0 && m > 0)
        return false;

    return true;
}

bool check_round_trip(const std::vector<uint32_t>& values) {
    PackedGlGhStorage packed;
    packed.initialize(static_cast<uint32_t>(values.size()));
    for (uint32_t v = 0; v < values.size(); ++v) {
        packed.g_set(v, values[v]);
    }
    packed.build_rank();

    std::stringstream ss;
    packed.serialize(ss, nullptr, "packed");

    PackedGlGhStorage loaded;
    loaded.load(ss);

    if (loaded.m() != packed.m())
        return false;

    for (uint32_t v = 0; v < packed.m(); ++v) {
        if (loaded.g_get(v) != packed.g_get(v))
            return false;
        if (loaded.is_vertex_occupied(v) != packed.is_vertex_occupied(v))
            return false;
    }

    for (uint32_t v = 0; v <= packed.m(); ++v) {
        if (loaded.rank(v) != packed.rank(v))
            return false;
    }

    return true;
}

int main() {
    std::cout << "Testing PackedGlGhStorage ...\n";
    int failures = 0;

    {
        bool ok = true;
        for (size_t m : {511ul, 50000ul, 65536ul, 49999ul}) {
            if (!check_against_glgh(random_values(m, 42))) {
                ok = false;
                break;
            }
        }
        if (!ok)
            ++failures;
        std::cout << "  random equivalence vs GlGhStorage: " << (ok ? "OK" : "FAIL") << "\n";
    }

    {
        const size_t m = 10000;
        bool ok = check_against_glgh(filled_values(m, 0));
        if (!ok)
            ++failures;
        std::cout << "  all occupied: " << (ok ? "OK" : "FAIL") << "\n";
    }

    {
        const size_t m = 10000;
        bool ok = check_against_glgh(filled_values(m, 3));
        if (!ok)
            ++failures;
        std::cout << "  all unassigned: " << (ok ? "OK" : "FAIL") << "\n";
    }

    {
        bool ok = check_round_trip(random_values(50000, 7));
        if (!ok)
            ++failures;
        std::cout << "  serialize/load round-trip: " << (ok ? "OK" : "FAIL") << "\n";
    }

    if (failures == 0) {
        std::cout << "All PackedGlGhStorage tests passed.\n";
    }
    return failures;
}

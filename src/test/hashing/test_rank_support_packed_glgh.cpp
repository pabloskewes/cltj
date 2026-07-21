#include <hashing/storage/rank_support_glgh.hpp>
#include <hashing/storage/rank_support_packed_glgh.hpp>

#include <cassert>
#include <iostream>
#include <random>
#include <sstream>

/**
 * @brief Build a pair (Gl, Gh, G) encoding the same random G values.
 *
 * G[v] in {0,1,2,3}. Each vertex: Gl[v] = G[v] & 1, Gh[v] = (G[v] >> 1) & 1.
 * The int_vector<2> stores the same values packed as 2 contiguous bits.
 */
struct DualVectors {
    sdsl::bit_vector Gl;
    sdsl::bit_vector Gh;
    sdsl::int_vector<2> G;

    DualVectors(size_t m, uint64_t seed) : Gl(m), Gh(m), G(m) {
        std::mt19937_64 rng(seed);
        std::uniform_int_distribution<uint32_t> dist(0, 3);
        for (size_t v = 0; v < m; ++v) {
            uint32_t val = dist(rng);
            Gl[v] = val & 1;
            Gh[v] = (val >> 1) & 1;
            G[v] = val;
        }
    }
};

int main() {
    std::cout << "Testing rank_support_packed_glgh vs rank_support_glgh ...\n";
    int failures = 0;

    // Random case, several sizes: a plain size, a superblock boundary (m%256==0),
    // an odd size, and a size below one superblock.
    {
        bool ok = true;
        for (size_t m : {50000ul, 65536ul, 49999ul, 511ul}) {
            DualVectors dv(m, 42);

            cltj::hashing::rank_support_glgh<> rank_glgh(&dv.Gl, &dv.Gh);
            cltj::hashing::rank_support_packed_glgh rank_packed(&dv.G);

            for (size_t v = 0; v <= m; ++v) {
                if (rank_packed.rank(v) != rank_glgh.rank(v)) {
                    ok = false;
                    break;
                }
            }
            if (!ok)
                break;
        }
        if (!ok)
            ++failures;
        std::cout << "  random (multiple sizes): " << (ok ? "OK" : "FAIL") << "\n";
    }

    // Edge: all occupied (no 3's, like a full MPHF). Here rank(v) must equal v.
    {
        bool ok = true;
        const size_t m = 10000;
        sdsl::bit_vector Gl(m, 0);
        sdsl::bit_vector Gh(m, 0);
        sdsl::int_vector<2> G(m, 0);

        cltj::hashing::rank_support_glgh<> rank_glgh(&Gl, &Gh);
        cltj::hashing::rank_support_packed_glgh rank_packed(&G);

        for (size_t v = 0; v <= m; ++v) {
            if (rank_packed.rank(v) != rank_glgh.rank(v) || rank_packed.rank(v) != v) {
                ok = false;
                break;
            }
        }

        if (!ok)
            ++failures;
        std::cout << "  all occupied (m=" << m << "): " << (ok ? "OK" : "FAIL") << "\n";
    }

    // Edge: all unassigned (all 3's). Here rank(v) must be 0.
    {
        bool ok = true;
        const size_t m = 10000;
        sdsl::bit_vector Gl(m, 1);
        sdsl::bit_vector Gh(m, 1);
        sdsl::int_vector<2> G(m, 3);

        cltj::hashing::rank_support_glgh<> rank_glgh(&Gl, &Gh);
        cltj::hashing::rank_support_packed_glgh rank_packed(&G);

        for (size_t v = 0; v <= m; ++v) {
            if (rank_packed.rank(v) != rank_glgh.rank(v) || rank_packed.rank(v) != 0) {
                ok = false;
                break;
            }
        }

        if (!ok)
            ++failures;
        std::cout << "  all unassigned (m=" << m << "): " << (ok ? "OK" : "FAIL") << "\n";
    }

    // Serialize/load round-trip: a loaded rank must answer like the original.
    {
        bool ok = true;
        const size_t m = 50000;
        DualVectors dv(m, 7);
        cltj::hashing::rank_support_packed_glgh rank_packed(&dv.G);

        std::stringstream ss;
        rank_packed.serialize(ss);
        cltj::hashing::rank_support_packed_glgh<> loaded;
        loaded.load(ss, &dv.G);

        for (size_t v = 0; v <= m; ++v) {
            if (loaded.rank(v) != rank_packed.rank(v)) {
                ok = false;
                break;
            }
        }

        if (!ok)
            ++failures;
        std::cout << "  serialize/load round-trip (m=" << m << "): " << (ok ? "OK" : "FAIL") << "\n";
    }

    if (failures == 0)
        std::cout << "All rank_support_packed_glgh tests passed.\n";
    return failures;
}

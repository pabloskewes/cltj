#pragma once
#include "mphf_utils.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <list>
#include <queue>
#include <random>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/util.hpp>
#include <stack>
#include <vector>

namespace cltj {
namespace hashing {

/**
 * @brief Triple structure representing a hyperedge in the 3-uniform hypergraph
 */
struct Triple {
    uint64_t key;  // Original key value
    uint32_t v0, v1, v2;  // Three vertices in the hypergraph

    Triple() : key(0), v0(0), v1(0), v2(0) {}

    Triple(uint64_t k, uint32_t vertex0, uint32_t vertex1, uint32_t vertex2)
        : key(k), v0(vertex0), v1(vertex1), v2(vertex2) {}
};

/**
 * @brief Minimal Perfect Hash Function (MPHF) Builder using MWHC/BDZ algorithm
 *
 * Algorithm overview:
 * 1. Map each key to a triple (v0, v1, v2) using 3 hash functions
 * 2. Build a 3-uniform hypergraph and perform "peeling" to get topological
 * order
 * 3. Assign values to array G such that each triple has a unique "winner"
 * vertex
 * 4. Use a bitvector to compact the hash function to minimal range [0,n)
 */
class MPHF {
  private:
    // Core data structures
    std::vector<uint8_t> G_;  // Assignment array, values in {0,1,2,3}
    sdsl::bit_vector used_positions_;  // Bitvector marking used positions
    sdsl::rank_support_v<1> rank_support_;  // For O(1) rank queries
    uint32_t m_;  // Size of G array (~1.23 * n)
    uint32_t n_;  // Number of keys

    // Hash function parameters
    std::array<uint64_t, 3> primes_;  // r[k] = prime for hash function k
    std::array<uint64_t, 3> offsets_;  // d[k] = offset for hash function k
    std::array<uint64_t, 3> multipliers_;  // a[k] = multiplier for hash function k

    // For retry logic
    static constexpr int MAX_RETRIES = 10;

  public:
    MPHF() : m_(0), n_(0), primes_{0, 0, 0}, offsets_{0, 0, 0}, multipliers_{0, 0, 0} {}

    /**
     * @brief Build MPHF for given keys
     * @param keys Vector of keys to hash
     * @return true if successful, false if failed after all retries
     */
    bool build(const std::vector<uint64_t>& keys) {
        n_ = keys.size();
        if (n_ == 0) {
            return false;
        }

        // Retry logic
        for (int retry = 0; retry < MAX_RETRIES; ++retry) {
            if (try_build(keys, retry)) {
                return true;
            }
            // If failed, the next retry will use different hash parameters
        }

        return false;  // Failed after all retries
    }

    /**
     * @brief Single attempt to build MPHF
     * @param keys Vector of keys to hash
     * @param retry_count Number of previous failed attempts (affects hash
     * parameters)
     * @return true if successful, false if need to retry
     */
    bool try_build(const std::vector<uint64_t>& keys, int retry_count) {
        // Step 1 - Initialize hash functions and arrays
        if (!initialize_hash_functions(keys, retry_count)) {
            return false;
        }

        // TODO: Step 2 - Generate triples for all keys
        std::vector<Triple> triples = generate_triples(keys);

        // TODO: Step 3 - Perform peeling to get topological ordering
        std::vector<Triple> peeling_order;
        if (!perform_peeling(triples, peeling_order)) {
            return false;  // Graph not peelable, need to retry
        }

        // TODO: Step 4 - Assign G array values in reverse order
        assign_g_values(peeling_order);

        // TODO: Step 5 - Build compactification structures
        build_compactification();

        return true;
    }

    /**
     * @brief Query the MPHF for a key
     * @param key Key to query
     * @return Hash value in range [0, n)
     */
    uint32_t query(uint64_t key) const {
        if (m_ == 0 || G_.empty()) {
            return 0;
        }
        // TODO: Step 1 - Compute triple (v0, v1, v2)
        auto triple = compute_triple(key);

        // TODO: Step 2 - Compute j = (G[v0] + G[v1] + G[v2]) mod 3
        uint32_t j = (G_[triple.v0] + G_[triple.v1] + G_[triple.v2]) % 3;

        // TODO: Step 3 - Select vertex based on j
        uint32_t selected_vertex;
        switch (j) {
            case 0:
                selected_vertex = triple.v0;
                break;
            case 1:
                selected_vertex = triple.v1;
                break;
            case 2:
                selected_vertex = triple.v2;
                break;
            default:
                selected_vertex = triple.v0;  // Should never happen
        }

        // TODO: Step 4 - Apply rank operation for compactification
        return compact_position(selected_vertex);
    }

  private:
    // ========== STEP 1: Hash Function Initialization ==========
    /**
     * @brief Initialize the three hash functions h0, h1, h2
     * Uses separate primes for each hash function
     */
    bool initialize_hash_functions(const std::vector<uint64_t>& keys, int retry_count) {
        // Determine target sizes
        const uint64_t target_m = static_cast<uint64_t>(std::ceil(1.25 * static_cast<double>(n_)));
        const uint64_t target_segment = std::max<uint64_t>(3, (target_m + 2) / 3);  // ceil(target_m/3)

        // Pick three primes near target_segment; advance based on retry_count
        // We stagger starts slightly across k to diversify
        for (int k = 0; k < 3; ++k) {
            uint64_t start = target_segment + static_cast<uint64_t>(retry_count) + static_cast<uint64_t>(k);
            uint64_t p = next_prime(start);
            // Ensure no key (non-zero) is divisible by p; if so, advance to next
            // prime
            while (!all_coprime(keys, p)) {
                p = next_prime(p + 1);
            }
            primes_[static_cast<size_t>(k)] = p;
        }

        // Compute segment offsets and total m
        offsets_[0] = 0;
        offsets_[1] = primes_[0];
        offsets_[2] = primes_[0] + primes_[1];
        m_ = static_cast<uint32_t>(primes_[0] + primes_[1] + primes_[2]);

        // Deterministic RNG for multipliers, varied by retry_count
        // 0xC1A0 is an arbitrary hex seed constant; offset by retry_count for
        // variety
        std::mt19937_64 rng(0xC1A0ULL + static_cast<uint64_t>(retry_count));
        for (int k = 0; k < 3; ++k) {
            uint64_t p = primes_[static_cast<size_t>(k)];
            std::uniform_int_distribution<uint64_t> dist(1, p - 1);
            multipliers_[static_cast<size_t>(k)] = dist(rng);
        }

        // Initialize G with sentinel value 3 (acts as 0 mod 3 but marks unassigned)
        G_.assign(m_, static_cast<uint8_t>(3));

        // Initialize compactification structures to empty; will be rebuilt later
        used_positions_ = sdsl::bit_vector(m_, 0);
        sdsl::util::init_support(rank_support_, &used_positions_);

        // Debug prints (will be replaced by logging later)
        std::cout << "[MPHF::initialize] n=" << n_ << " target_m=" << target_m << " primes(r): {"
                  << primes_[0] << ", " << primes_[1] << ", " << primes_[2] << "}"
                  << " offsets(d): {" << offsets_[0] << ", " << offsets_[1] << ", " << offsets_[2] << "}"
                  << " multipliers(a): {" << multipliers_[0] << ", " << multipliers_[1] << ", "
                  << multipliers_[2] << "}"
                  << " m=" << m_ << "\n";

        return true;
    }

    // ========== STEP 2: Triple Generation ==========
    /**
     * @brief Generate triples for all keys using the three hash functions
     */
    std::vector<Triple> generate_triples(const std::vector<uint64_t>& keys) {
        std::vector<Triple> triples;
        // TODO:
        // For each key:
        // 1. Compute h0(key), h1(key), h2(key)
        // 2. Map to vertices: v0 = h0(key), v1 = m/3 + h1(key), v2 = 2*m/3 +
        // h2(key)
        // 3. Create Triple(key, v0, v1, v2) and add to vector
        return triples;
    }

    // ========== STEP 3: Peeling Algorithm ==========
    /**
     * @brief Perform peeling algorithm to find processing order
     * @param triples Input triples
     * @param peeling_order Output order (topological sort)
     * @return true if peeling successful (graph is peelable)
     */
    bool perform_peeling(const std::vector<Triple>& triples, std::vector<Triple>& peeling_order) {
        // TODO: Implement the peeling algorithm from the pseudocode
        // 1. Build adjacency lists L[0..m-1] and degree counters N[0..m-1]
        // 2. Use priority queue Q to always extract minimum degree vertex
        // 3. While queue not empty:
        //    - Extract vertex v with degree 1
        //    - If degree > 1, return false (not peelable)
        //    - Push corresponding triple to stack
        //    - Remove triple and update degrees of other vertices
        // 4. peeling_order should contain all triples in the order they were
        // processed
        return false;  // Placeholder
    }

    // ========== STEP 4: G Array Assignment ==========
    /**
     * @brief Assign values to G array based on peeling order
     */
    void assign_g_values(const std::vector<Triple>& peeling_order) {
        // TODO: Process triples in REVERSE order of peeling
        // For each triple (v0, v1, v2):
        // 1. Find first unvisited vertex vj (j ∈ {0,1,2})
        // 2. Set G[vj] = (j - G[v0] - G[v1] - G[v2]) mod 3
        // 3. Mark all three vertices as visited
    }

    // ========== STEP 5: Compactification ==========
    /**
     * @brief Build structures for compacting hash function to [0,n)
     */
    void build_compactification() {
        // TODO:
        // 1. Identify which positions in [0,m) are actually used by the PHF
        // 2. Build bitvector marking used positions
        // 3. Build rank support structure for O(1) rank queries
        // 4. Store used_positions_ and initialize SDSL structures
    }

    // ========== HELPER FUNCTIONS ==========
    /**
     * @brief Compute hash function h_k(key) for k ∈ {0,1,2}
     * Uses the improved approach: h_k(x) = offset_k + (key % prime_k) *
     * multiplier_k % prime_k
     */
    uint32_t hash_function(uint64_t key, int function_index) const {
        // TODO: Implement the colleague's hash approach
        // return offsets_[function_index] + mod_mul(key % primes_[function_index],
        //                                          multipliers_[function_index],
        //                                          primes_[function_index]);
        return 0;  // Placeholder
    }

    /**
     * @brief Compute triple (v0, v1, v2) for a given key
     * Each hash function maps to its own segment of the vertex space
     */
    Triple compute_triple(uint64_t key) const {
        // TODO: Each hash function has its own range
        // v0 = hash_function(key, 0)  // in range [0, r[0])
        // v1 = hash_function(key, 1)  // in range [r[0], r[0] + r[1])
        // v2 = hash_function(key, 2)  // in range [r[0] + r[1], r[0] + r[1] + r[2])
        return Triple(key, 0, 0, 0);  // Placeholder
    }

    /**
     * @brief Convert position in [0,m) to compact position in [0,n)
     */
    uint32_t compact_position(uint32_t position) const {
        // TODO: Use SDSL rank operation on bitvector
        // return rank_support_(position);
        return 0;  // Placeholder
    }
};

}  // namespace hashing
}  // namespace cltj
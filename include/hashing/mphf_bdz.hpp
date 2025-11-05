#pragma once
#include "mphf_utils.hpp"
#include <util/logger.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <list>
#include <queue>
#include <random>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>
#include <stack>
#include <vector>

namespace cltj {
namespace hashing {

// A constant for the number of bits in our fingerprints. 8 bits gives a 1/256
// chance of a random collision (false positive).
static constexpr int FINGERPRINT_BITS = 8;

/**
 * @brief Triple structure representing a hyperedge in the 3-uniform hypergraph
 */
struct Triple {
    uint64_t key;  // Original key value
    uint32_t v0, v1, v2;  // Three vertices in the hypergraph

    Triple() : key(0), v0(0), v1(0), v2(0) {}

    Triple(uint64_t k, uint32_t vertex0, uint32_t vertex1, uint32_t vertex2)
        : key(k), v0(vertex0), v1(vertex1), v2(vertex2) {}

    uint32_t v(int idx) const {
        switch (idx) {
            case 0:
                return v0;
            case 1:
                return v1;
            case 2:
                return v2;
            default:
                assert(false && "Triple index out of range");
                return 0u;
        }
    }
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
    sdsl::int_vector<2> G_;  // Assignment array, values in {0,1,2,3}
    sdsl::bit_vector used_positions_;  // Bitvector marking used positions
    sdsl::rank_support_v<1> rank_support_;  // For O(1) rank queries
    sdsl::int_vector<> Q_;  // Fingerprints for membership queries
    uint32_t m_;  // Size of G array (~1.23 * n)
    uint32_t n_;  // Number of keys

    // Hash function parameters
    std::array<uint64_t, 3> primes_;  // r[k] = modulus (prime) per hash function
    std::array<uint64_t, 3> multipliers_;  // a[k] = multiplier inside modulo
    std::array<uint64_t, 3> biases_;  // b[k] = additive bias inside modulo
    std::array<uint64_t, 3> segment_starts_;  // d[k] = global segment start

    // For retry logic
    static constexpr int MAX_RETRIES = 10;
    static constexpr uint64_t SEED = 0xC1A0ULL;
    int retry_count_;  // Number of retries used in last build

  public:
    using size_type = size_t;  // Required for sdsl::size_in_bytes

    MPHF()
        : m_(0),
          n_(0),
          retry_count_(0),
          primes_{0, 0, 0},
          multipliers_{0, 0, 0},
          biases_{0, 0, 0},
          segment_starts_{0, 0, 0} {}

    // Disabling copy and move because this class has a complex resource
    // management like the pointer relationship between rank_support_ and its vector.
    // TODO: Understand why this is necessary and try to make it cleaner
    MPHF(const MPHF&) = delete;
    MPHF& operator=(const MPHF&) = delete;
    MPHF(MPHF&&) = delete;
    MPHF& operator=(MPHF&&) = delete;

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
            retry_count_ = retry;
            if (try_build(keys, retry)) {
                return true;
            }
            // If failed, the next retry will use different hash parameters
        }

        retry_count_ = MAX_RETRIES;  // Failed after all retries
        return false;
    }

    uint32_t n() const { return n_; }
    uint32_t m() const { return m_; }
    int retry_count() const { return retry_count_; }

    // --- Getters for manual size calculation ---
    const sdsl::int_vector<2>& get_g() const { return G_; }
    const sdsl::bit_vector& get_used_positions() const { return used_positions_; }
    const sdsl::rank_support_v<1>& get_rank_support() const { return rank_support_; }
    const sdsl::int_vector<>& get_q() const { return Q_; }
    const std::array<uint64_t, 3>& get_primes() const { return primes_; }
    const std::array<uint64_t, 3>& get_multipliers() const { return multipliers_; }
    const std::array<uint64_t, 3>& get_biases() const { return biases_; }
    const std::array<uint64_t, 3>& get_segment_starts() const { return segment_starts_; }

    /**
     * @brief Serialize the MPHF to an output stream.
     * Conforms to the SDSL serialization interface.
     * @param out The output stream.
     * @param v The structure tree node (for visualization).
     * @param name The name for the structure tree node.
     * @return The number of bytes written.
     */
    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written_bytes = 0;

        // Core data structures (essential for queries)
        written_bytes += sdsl::write_member(G_, out, child, "G_");
        written_bytes += used_positions_.serialize(out, child, "used_positions_");
        written_bytes += rank_support_.serialize(out, child, "rank_support_");
        written_bytes += sdsl::write_member(Q_, out, child, "Q_");
        written_bytes += sdsl::write_member(m_, out, child, "m_");
        written_bytes += sdsl::write_member(n_, out, child, "n_");

        // Hash function parameters (essential for queries)
        written_bytes += sdsl::write_member(primes_, out, child, "primes_");
        written_bytes += sdsl::write_member(multipliers_, out, child, "multipliers_");
        written_bytes += sdsl::write_member(biases_, out, child, "biases_");
        written_bytes += sdsl::write_member(segment_starts_, out, child, "segment_starts_");

        // Note: retry_count_ is NOT serialized as it's only needed during construction

        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    /**
     * @brief Load the MPHF from an input stream.
     * @param in The input stream.
     */
    void load(std::istream& in) {
        // Core data structures
        sdsl::read_member(G_, in);
        used_positions_.load(in);
        rank_support_.load(in);
        rank_support_.set_vector(&used_positions_);  // Re-link rank support
        sdsl::read_member(Q_, in);
        sdsl::read_member(m_, in);
        sdsl::read_member(n_, in);

        // Hash function parameters
        sdsl::read_member(primes_, in);
        sdsl::read_member(multipliers_, in);
        sdsl::read_member(biases_, in);
        sdsl::read_member(segment_starts_, in);

        // Reset retry_count since it's not serialized
        retry_count_ = 0;
    }

    /**
     * @brief Check if a key is in the set.
     * This query can have false positives, but no false negatives.
     * @param key The key to check.
     * @return true if the key is likely in the set, false otherwise.
     */
    bool contains(uint64_t key) const {
        if (n_ == 0)
            return false;
        uint32_t idx = query(key);
        return Q_[idx] == fingerprint(key);
    }

    /**
     * @brief Single attempt to build MPHF
     * Algorithm:
     * 1. Initialize hash functions and arrays
     * 2. Generate triples for all keys
     * 3. Perform peeling to get topological ordering
     * 4. Assign G array values in reverse order
     * 5. Build compactification structures
     * @param keys Vector of keys to hash
     * @param retry_count Number of previous failed attempts (affects hash
     * parameters)
     * @return true if successful, false if need to retry
     */
    bool try_build(const std::vector<uint64_t>& keys, int retry_count) {
        if (!initialize_hash_functions(keys, retry_count)) {
            return false;
        }

        std::vector<Triple> triples = generate_triples(keys);

        std::vector<Triple> peeling_order;
        if (!perform_peeling(triples, peeling_order)) {
            return false;  // Graph not peelable, need to retry
        }

        assign_g_values(peeling_order);

        build_compactification(triples);

        build_fingerprints(keys);

        return true;
    }

    /**
     * @brief Query the MPHF for a key
     * Algorithm:
     * 1. Compute triple (v0, v1, v2)
     * 2. Compute j = (G[v0] + G[v1] + G[v2]) mod 3
     * 3. Select vertex based on j
     * 4. Apply rank operation for compactification
     * @param key Key to query
     * @return Hash value in range [0, n)
     */
    uint32_t query(uint64_t key) const {
        if (m_ == 0 || G_.empty()) {
            return 0;
        }

        auto triple = compute_triple(key);

        // Compute j = (G[v0] + G[v1] + G[v2]) mod 3
        uint32_t j = (G_[triple.v0] + G_[triple.v1] + G_[triple.v2]) % 3;

        // Select v_j
        uint32_t selected_vertex = triple.v(static_cast<int>(j));

        // Apply rank operation for compactification
        uint32_t res = compact_position(selected_vertex);

        // std::cout << "[MPHF::query] key=" << key << " triple=(" << triple.v0 << ", " << triple.v1 << ", "
        //           << triple.v2 << ") j=" << j << " sel=" << selected_vertex
        //           << " used=" << (used_positions_.size() ? (int)used_positions_[selected_vertex] : -1)
        //           << " rank=" << (used_positions_.size() ? (uint64_t)rank_support_(selected_vertex) : 0)
        //           << " -> res=" << res << "\n";
        return res;
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

        // On retries, increase m by a moderate linear factor.
        // A 10% stride is a balance between guaranteeing success and keeping overhead low.
        uint64_t stride = std::max<uint64_t>(3, target_segment / 10);  // 10% stride
        uint64_t base = target_segment + static_cast<uint64_t>(retry_count) * stride;
        uint64_t p0 = next_prime(base);
        uint64_t p1 = next_prime(p0 + 1);
        uint64_t p2 = next_prime(p1 + 1);
        primes_[0] = p0;
        primes_[1] = p1;
        primes_[2] = p2;

        // Compute segment starts and total m
        segment_starts_[0] = 0;
        segment_starts_[1] = primes_[0];
        segment_starts_[2] = primes_[0] + primes_[1];
        m_ = static_cast<uint32_t>(primes_[0] + primes_[1] + primes_[2]);

        // Deterministic RNG for multipliers with retry_count offset
        std::mt19937_64 rng(SEED + static_cast<uint64_t>(retry_count));
        for (int k = 0; k < 3; ++k) {
            uint64_t p = primes_[static_cast<size_t>(k)];
            std::uniform_int_distribution<uint64_t> distA(1, p - 1);
            multipliers_[static_cast<size_t>(k)] = distA(rng);  // a[k] ∈ [1, p-1]
        }

        // Sample biases b[k] ∈ [0, p-1]
        for (int k = 0; k < 3; ++k) {
            uint64_t p = primes_[static_cast<size_t>(k)];
            std::uniform_int_distribution<uint64_t> distB(0, p - 1);
            biases_[static_cast<size_t>(k)] = distB(rng);
        }

        // Initialize G with sentinel value 3 (acts as 0 mod 3 but marks unassigned)
        G_ = sdsl::int_vector<2>(m_, 3);

        // std::cout << "[MPHF::initialize] n=" << n_ << " target_m=" << target_m << " primes(r): {"
        //           << primes_[0] << ", " << primes_[1] << ", " << primes_[2] << "}"
        //           << " segment_starts(d): {" << segment_starts_[0] << ", " << segment_starts_[1] << ", "
        //           << segment_starts_[2] << "}"
        //           << " multipliers(a): {" << multipliers_[0] << ", " << multipliers_[1] << ", "
        //           << multipliers_[2] << "}"
        //           << " biases(b): {" << biases_[0] << ", " << biases_[1] << ", " << biases_[2] << "}"
        //           << " m=" << m_ << "\n";

        return true;
    }

    // ========== STEP 2: Triple Generation ==========
    /**
     * @brief Generate triples for all keys using the three hash functions
     */
    std::vector<Triple> generate_triples(const std::vector<uint64_t>& keys) {
        std::vector<Triple> triples;
        triples.reserve(keys.size());
        for (auto x : keys) {
            Triple t = compute_triple(x);
            triples.push_back(t);
            // std::cout << "[MPHF::triples] key=" << x << " -> (" << t.v0 << ", " << t.v1 << ", " << t.v2
            //           << ")\n";
        }
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
        peeling_order.clear();
        if (m_ == 0 || triples.empty()) {
            return false;
        }

        // Build adjacency: incident edges per vertex and degree counts
        std::vector<std::vector<uint32_t>> incident(m_);
        incident.reserve(m_);
        for (uint32_t ei = 0; ei < triples.size(); ++ei) {
            const Triple& t = triples[ei];
            incident[t.v0].push_back(ei);
            incident[t.v1].push_back(ei);
            incident[t.v2].push_back(ei);
        }
        std::vector<uint32_t> degree(m_, 0);
        for (uint32_t v = 0; v < m_; ++v) {
            degree[v] = static_cast<uint32_t>(incident[v].size());
        }

        // Queue of vertices with degree 1
        std::queue<uint32_t> q;
        for (uint32_t v = 0; v < m_; ++v) {
            if (degree[v] == 1)
                q.push(v);
        }

        // Track removed edges
        std::vector<uint8_t> edge_removed(triples.size(), 0);

        // Process vertices of degree 1
        while (!q.empty()) {
            uint32_t v = q.front();
            q.pop();

            if (degree[v] != 1)
                continue;  // stale

            // Find the unique non-removed edge incident to v
            uint32_t ei = UINT32_MAX;
            for (uint32_t e : incident[v]) {
                if (!edge_removed[e]) {
                    ei = e;
                    break;
                }
            }
            if (ei == UINT32_MAX)
                continue;  // already handled

            // Output order: push this edge
            peeling_order.push_back(triples[ei]);
            edge_removed[ei] = 1;

            const Triple& t = triples[ei];
            uint32_t vertices_local[3] = {t.v0, t.v1, t.v2};
            for (int idx = 0; idx < 3; ++idx) {
                uint32_t u = vertices_local[idx];
                if (degree[u] > 0) {
                    degree[u] -= 1;
                    if (degree[u] == 1)
                        q.push(u);
                }
            }
        }

        bool ok = (peeling_order.size() == triples.size());
        if (!ok) {
            LOG_WARN("[MPHF::peeling] Failed: peeled " << peeling_order.size() << "/" << triples.size()
                      << " edges (cycle remains)");
        } else {
            LOG_INFO("[MPHF::peeling] Success: peeled all " << triples.size() << " edges");
        }
        return ok;
    }

    // ========== STEP 4: G Array Assignment ==========
    /**
     * @brief Assign values to G array based on peeling order
     * For each triple (v0, v1, v2) in reverse order of peeling:
     * 1. Find first unvisited vertex index j
     * 2. Set G[vj] = (j - G[v0] - G[v1] - G[v2]) mod 3
     * 3. Mark all three vertices as visited
     * @param peeling_order Reverse order of peeling
     * @return void
     */
    void assign_g_values(const std::vector<Triple>& peeling_order) {
        if (m_ == 0)
            return;
        sdsl::int_vector<1> visited(m_, 0);

        // Process in reverse order
        for (auto it = peeling_order.rbegin(); it != peeling_order.rend(); ++it) {
            const Triple& t = *it;
            uint32_t vertices[3] = {t.v0, t.v1, t.v2};

            // Find first unvisited vertex index j
            int j = -1;
            for (int idx = 0; idx < 3; ++idx) {
                if (!visited[vertices[idx]]) {
                    j = idx;
                    break;
                }
            }
            if (j == -1) {
                // Should not happen; all three already assigned
                continue;
            }

            // Sum of current G values modulo 3
            uint32_t s = (G_[t.v0] + G_[t.v1] + G_[t.v2]) % 3;

            // Need (G[v0] + G[v1] + G[v2]) % 3 == j
            uint32_t need = static_cast<uint32_t>((3 + j - static_cast<int>(s)) % 3);
            G_[t.v(j)] = need;

            // Mark all as visited (only one was newly assigned, but the others are effectively fixed now)
            visited[t.v0] = visited[t.v1] = visited[t.v2] = 1;

            // std::cout << "[MPHF::assignG] triple=(" << t.v0 << ", " << t.v1 << ", " << t.v2 << ") j=" << j
            //           << " set G[" << t.v(j) << "]=" << need << "\n";
        }
    }

    // ========== STEP 5: Compactification ==========
    /**
     * @brief Build structures for compacting hash function to [0,n)
     */
    void build_compactification(const std::vector<Triple>& triples) {
        used_positions_ = sdsl::bit_vector(m_, 0);
        for (const auto& t : triples) {
            uint32_t j = static_cast<uint32_t>((G_[t.v0] + G_[t.v1] + G_[t.v2]) % 3);
            uint32_t pos = (j == 0 ? t.v0 : (j == 1 ? t.v1 : t.v2));
            used_positions_[pos] = 1;
        }
        sdsl::util::init_support(rank_support_, &used_positions_);
        // std::cout << "[MPHF::compact] built bitvector with " << triples.size() << " used positions\n";
    }

    // ========== STEP 6: Fingerprint Generation ==========
    /**
     * @brief Build the fingerprint array for membership queries.
     */
    void build_fingerprints(const std::vector<uint64_t>& keys) {
        if (n_ == 0)
            return;
        Q_ = sdsl::int_vector<>(n_, 0, FINGERPRINT_BITS);
        for (const auto& key : keys) {
            uint32_t idx = query(key);
            Q_[idx] = fingerprint(key);
        }
    }

    // ========== HELPER FUNCTIONS ==========
    /**
     * @brief Compute hash function h_k(x) for k ∈ {0,1,2}
     * h_k(x) = d_k + ((a_k · x + b_k) mod r_k)
     * with r_k = primes_[k], a_k = multipliers_[k], b_k = biases_[k], d_k = segment_starts_[k].
     */
    uint32_t hash_function(uint64_t x, int k) const {
        const size_t i = static_cast<size_t>(k);
        const uint64_t r = primes_[i];
        uint64_t mapped = mod_mul(x, multipliers_[i], r);  // in [0, r)
        mapped += biases_[i];
        if (mapped >= r)
            mapped -= r;  // single correction instead of modulo
        return static_cast<uint32_t>(segment_starts_[i] + mapped);
    }

    /**
     * @brief Compute triple (v0, v1, v2) for a given key
     * Each hash function maps to its own segment of the vertex space
     */
    Triple compute_triple(uint64_t key) const {
        return Triple(key, hash_function(key, 0), hash_function(key, 1), hash_function(key, 2));
    }

    /**
     * @brief Convert position in [0,m) to compact position in [0,n)
     */
    uint32_t compact_position(uint32_t position) const {
        // rank before position in [0, position)
        return static_cast<uint32_t>(rank_support_(position));
    }

    /**
     * @brief Computes a fingerprint for a key.
     */
    uint8_t fingerprint(uint64_t key) const {
        // A simple fingerprint. Can be replaced with something more robust.
        return static_cast<uint8_t>(key & ((1 << FINGERPRINT_BITS) - 1));
    }
};

}  // namespace hashing
}  // namespace cltj
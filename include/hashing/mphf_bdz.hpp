#pragma once
#include "mixers.hpp"
#include "mphf_utils.hpp"
#include "mphf_types.hpp"
#include "mphf_build_tracer.hpp"
#include "storage/baseline.hpp"
#include "key_policies.hpp"
#include <util/logger.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <random>
#include <sdsl/bit_vectors.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>
#include <utility>
#include <vector>
#include <type_traits>

namespace cltj {
namespace hashing {

/**
 * @brief Size breakdown of all MPHF components in bytes.
 *
 * Components:
 *   - g_bytes        : G array (main peeling payload).
 *   - used_pos_bytes : B bitvector marking used slots.
 *   - rank_bytes     : rank support over B.
 *   - q_bytes        : key payload (FullKey, QuotientKey, ...).
 *   - other_bytes    : serialized metadata (n, prime deltas, retry_count, multipliers, biases, n_peeled).
 *   - fallback_bytes : fallback system residual array (sorted residual keys).
 */
struct SizeBreakdown {
    size_t g_bytes = 0;
    size_t used_pos_bytes = 0;
    size_t rank_bytes = 0;
    size_t q_bytes = 0;
    size_t other_bytes = 0;
    size_t fallback_bytes = 0;

    size_t total_bytes() const {
        return g_bytes + used_pos_bytes + rank_bytes + q_bytes + other_bytes + fallback_bytes;
    }
};

/**
 * @brief Minimal Perfect Hash Function (MPHF) Builder using MWHC/BDZ algorithm
 *
 * Algorithm overview:
 * 1. Apply premix32 to the input keys, then map each key to a triple (v0, v1, v2)
 *    using 3 hash functions
 * 2. Build a 3-uniform hypergraph and perform "peeling" to get topological order
 * 3. Assign values to array G such that each triple has a unique "winner" vertex
 * 4. Use a bitvector to compact the hash function to minimal range [0, n)
 *
 * Fallback system: if peeling leaves a small residual 2-core, keep the peeled BDZ
 * path and store the residual keys in a sorted array for binary-search lookup.
 *
 * @tparam StorageStrategy  storage layout for G, B and rank support.
 * @tparam KeyPolicy        payload policy for membership / reconstruction.
 */
template <typename StorageStrategy = BaselineStorage, typename KeyPolicy = policies::NoKey>
class MPHF {
  private:
    // Core data structures

    StorageStrategy storage_;  // Unified storage for G array, bitvector B, and rank support
    KeyPolicy key_policy_;  // Policy for key-based payloads (membership, reconstruction)
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
    uint8_t retry_count_;  // Number of retries used in last build (stored for serialization)
    uint32_t last_peeled_ = 0;  // Edges peeled in last try_build (for tracing)

    // Fallback system
    static constexpr uint32_t MAX_FALLBACK_RESIDUAL = 10000;  // max 2-core size accepted as fallback
    static constexpr int MIN_RETRIES_BEFORE_FALLBACK = 2;  // full-peel attempts before fallback kicks in
    std::vector<uint32_t> residual_keys_;  // sorted premixed keys of the accepted 2-core
    uint32_t n_peeled_ = 0;  // |peeled subset|; residual keys live in [n_peeled_, n_)

  public:
    using size_type = size_t;  // Required for sdsl::size_in_bytes

    MPHF()
        : m_(0),
          n_(0),
          retry_count_(0),
          n_peeled_(0),
          primes_{0, 0, 0},
          multipliers_{0, 0, 0},
          biases_{0, 0, 0},
          segment_starts_{0, 0, 0} {}

    MPHF(const MPHF&) = delete;
    MPHF& operator=(const MPHF&) = delete;
    MPHF(MPHF&&) = default;
    MPHF& operator=(MPHF&&) = default;

    /**
     * @brief Build MPHF for given keys (no tracing).
     * Delegates to the traced overload with a no-op tracer (zero overhead).
     */
    bool build(const std::vector<uint32_t>& keys) {
        hashing::MphfBuildTracer<false> noop("");
        return build(keys, noop);
    }

    /**
     * @brief Build MPHF with tracing support.
     * Repeats try_build() with different seeds until success or MAX_RETRIES is exhausted.
     * Timing is only measured when Tracer::is_enabled is true (zero-cost otherwise).
     */
    template <typename Tracer>
    bool build(const std::vector<uint32_t>& keys, Tracer& tracer) {
        n_ = keys.size();
        if (n_ == 0)
            return false;

        for (int retry = 0; retry < MAX_RETRIES; ++retry) {
            retry_count_ = retry;
            tracer.on_try_start(retry);
            bool ok = try_build(keys, retry);
            tracer.on_try_result(retry, m_, n_, last_peeled_, ok);
            if (ok)
                return true;
        }

        retry_count_ = MAX_RETRIES;
        return false;
    }

    uint32_t n() const { return n_; }
    uint32_t m() const { return m_; }
    int retry_count() const { return retry_count_; }
    uint32_t n_peeled() const { return n_peeled_; }
    uint32_t n_residual() const { return static_cast<uint32_t>(residual_keys_.size()); }
    bool has_fallback() const { return !residual_keys_.empty(); }

    /**
     * @brief Get detailed size breakdown of all MPHF components
     * @return SizeBreakdown struct with individual component sizes
     */
    SizeBreakdown get_size_breakdown() const {
        auto storage_breakdown = storage_.get_size_breakdown();
        SizeBreakdown breakdown;
        breakdown.g_bytes = storage_breakdown.g_bytes;
        breakdown.used_pos_bytes = storage_breakdown.used_pos_bytes;
        breakdown.rank_bytes = storage_breakdown.rank_bytes;
        breakdown.q_bytes = key_policy_.size_in_bytes();
        // Metadata: n (4) + 3×prime_deltas (3) + retry_count (1) + multipliers (24)
        //           + biases (24) + n_peeled (4) = 60 bytes
        // Note: m_ and primes_ are not serialized (computed/reconstructed)
        breakdown.other_bytes = sizeof(n_) + 3 * sizeof(uint8_t) + sizeof(retry_count_) +
            sizeof(multipliers_) + sizeof(biases_) + sizeof(n_peeled_);
        breakdown.fallback_bytes = sizeof(uint32_t)  // residual_count field
            + residual_keys_.size() * sizeof(uint32_t);
        return breakdown;
    }

    /**
     * @brief Get total size in bytes (SDSL compatible)
     * @return Total size in bytes of all MPHF components
     */
    size_t size_in_bytes() const { return get_size_breakdown().total_bytes(); }

    const std::array<uint64_t, 3>& get_primes() const { return primes_; }
    const std::array<uint64_t, 3>& get_multipliers() const { return multipliers_; }
    const std::array<uint64_t, 3>& get_biases() const { return biases_; }
    const std::array<uint64_t, 3>& get_segment_starts() const { return segment_starts_; }

    /**
     * @brief Serialize the MPHF to an output stream.
     * Conforms to the SDSL serialization interface.
     * Persists the peeled BDZ state, KeyPolicy payload and Fallback system state.
     * @param out The output stream.
     * @param v The structure tree node (for visualization).
     * @param name The name for the structure tree node.
     * @return The number of bytes written.
     */
    size_t serialize(std::ostream& out, sdsl::structure_tree_node* v = nullptr, std::string name = "") const {
        sdsl::structure_tree_node* child =
            sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_t written_bytes = 0;

        // Core query state
        written_bytes += storage_.serialize(out, child, "storage_");
        written_bytes += sdsl::write_member(n_, out, child, "n_");

        // Hash parameters, encoded as p_j - target_segment
        const uint64_t target_segment = compute_target_segment();
        for (int k = 0; k < 3; ++k) {
            uint64_t delta = primes_[k] - target_segment;
            assert(delta <= 255 && "Prime delta out of uint8_t range - unexpected!");
            uint8_t delta_byte = static_cast<uint8_t>(delta);
            written_bytes += sdsl::write_member(delta_byte, out, child, "prime_delta_" + std::to_string(k));
        }
        written_bytes += sdsl::write_member(retry_count_, out, child, "retry_count_");

        // Hash coefficients a_k, b_k.
        for (int k = 0; k < 3; ++k) {
            written_bytes +=
                sdsl::write_member(multipliers_[k], out, child, "multiplier_" + std::to_string(k));
        }
        for (int k = 0; k < 3; ++k) {
            written_bytes += sdsl::write_member(biases_[k], out, child, "bias_" + std::to_string(k));
        }

        // KeyPolicy payload
        written_bytes += key_policy_.serialize(out, child, "key_policy_");

        // Fallback system state
        written_bytes += sdsl::write_member(n_peeled_, out, child, "n_peeled_");
        uint32_t residual_count = static_cast<uint32_t>(residual_keys_.size());
        written_bytes += sdsl::write_member(residual_count, out, child, "residual_count");
        for (uint32_t rk : residual_keys_) {
            written_bytes += sdsl::write_member(rk, out, child, "rk");
        }

        sdsl::structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    /**
     * @brief Load the MPHF from an input stream.
     * Reconstructs the peeled BDZ state, KeyPolicy payload and Fallback system state.
     * @param in The input stream.
     */
    void load(std::istream& in) {
        // Core query state
        storage_.load(in);
        sdsl::read_member(n_, in);

        // Hash parameters, decoded from p_j - target_segment
        const uint64_t target_segment = compute_target_segment();
        for (int k = 0; k < 3; ++k) {
            uint8_t delta;
            sdsl::read_member(delta, in);
            primes_[k] = target_segment + delta;
        }
        sdsl::read_member(retry_count_, in);

        // Hash coefficients a_k, b_k.
        for (int k = 0; k < 3; ++k) {
            sdsl::read_member(multipliers_[k], in);
        }
        for (int k = 0; k < 3; ++k) {
            sdsl::read_member(biases_[k], in);
        }

        m_ = static_cast<uint32_t>(primes_[0] + primes_[1] + primes_[2]);

        // Rebuild derived hash state
        segment_starts_[0] = 0;
        segment_starts_[1] = primes_[0];
        segment_starts_[2] = primes_[0] + primes_[1];

        // KeyPolicy only needs the hash parameters rebound before load().
        policies::KeyInitContext ctx{n_, primes_, multipliers_, biases_, segment_starts_, 0};
        key_policy_.bind_context(ctx);
        key_policy_.load(in);

        // Fallback system state
        sdsl::read_member(n_peeled_, in);
        uint32_t residual_count = 0;
        sdsl::read_member(residual_count, in);
        residual_keys_.resize(residual_count);
        for (uint32_t i = 0; i < residual_count; ++i) {
            sdsl::read_member(residual_keys_[i], in);
        }
    }

    /**
     * @brief Single attempt to build MPHF.
     *
     * Algorithm:
     * 1. Initialize hash functions and storage for this retry.
     * 2. Generate one triple per key.
     * 3. Peel the 3-uniform hypergraph to obtain a topological ordering.
     * 4. If a small residual 2-core survives and is eligible for the Fallback
     *    system, accept the partial peel; otherwise request another retry.
     * 5. Assign G values on the peeled subset and build rank support.
     * 6. Initialize the KeyPolicy on the peeled prefix and store residual keys
     *    in sorted order for binary-search lookup.
     *
     * @return true on full peel or accepted fallback; false to request a retry.
     */
    bool try_build(const std::vector<uint32_t>& keys, int retry_count) {
        if (!initialize_hash_functions(keys, retry_count)) {
            return false;
        }

        std::vector<Triple> triples = generate_triples(keys);

        std::vector<Triple> peeling_order;
        std::vector<uint8_t> edge_removed;
        bool fully_peeled = perform_peeling(triples, peeling_order, edge_removed);

        uint32_t residual_count = static_cast<uint32_t>(triples.size() - peeling_order.size());

        if (!fully_peeled) {
            if (residual_count > MAX_FALLBACK_RESIDUAL || retry_count < MIN_RETRIES_BEFORE_FALLBACK)
                return false;
            LOG_WARN(
                "[MPHF::fallback] Accepting partial peel: peeled="
                << peeling_order.size() << "/" << triples.size() << " residual=" << residual_count
            );
        }

        n_peeled_ = static_cast<uint32_t>(peeling_order.size());

        assign_g_values(peeling_order);
        storage_.build_rank();

        // Residual keys are stored sorted for binary-search lookup.
        residual_keys_.clear();
        if (!fully_peeled) {
            residual_keys_.reserve(residual_count);
            for (uint32_t i = 0; i < static_cast<uint32_t>(triples.size()); ++i) {
                if (!edge_removed[i]) {
                    residual_keys_.push_back(triples[i].mixed_key);
                }
            }
            std::sort(residual_keys_.begin(), residual_keys_.end());
        }

        // Quotient width uses the max mixed key over the full input.
        uint32_t max_mixed_key = 0;
        if constexpr (KeyPolicy::needs_input_stats) {
            for (auto k : keys) {
                uint32_t mixed_key = premix32(k);
                if (mixed_key > max_mixed_key)
                    max_mixed_key = mixed_key;
            }
        }

        // KeyPolicy only covers the peeled prefix.
        policies::KeyInitContext ctx{
            n_peeled_, primes_, multipliers_, biases_, segment_starts_, max_mixed_key
        };
        key_policy_.init(ctx);
        for (uint32_t i = 0; i < static_cast<uint32_t>(keys.size()); ++i) {
            if (!fully_peeled && !edge_removed[i])
                continue;
            auto triple = compute_triple(keys[i]);
            int which_h = determine_which_h(triple.v0, triple.v1, triple.v2);
            uint32_t idx = query(keys[i]);
            key_policy_.store(idx, triple.mixed_key, triple, which_h);
        }

        return true;
    }

    /**
     * @brief Query the MPHF for a key
     * Algorithm:
     * 1. Compute triple (v0, v1, v2)
     * 2. If the key belongs to the Fallback system, return its residual index
     * 3. Compute j = (G[v0] + G[v1] + G[v2]) mod 3
     * 4. Select vertex based on j
     * 5. Apply rank operation for compactification
     * @param key Key to query
     * @return Hash value in range [0, n)
     */
    uint32_t query(uint32_t key) const {
        auto triple = compute_triple(key);

        // Residual keys bypass the BDZ path.
        if (!residual_keys_.empty()) {
            auto it = std::lower_bound(residual_keys_.begin(), residual_keys_.end(), triple.mixed_key);
            if (it != residual_keys_.end() && *it == triple.mixed_key) {
                return n_peeled_ + static_cast<uint32_t>(it - residual_keys_.begin());
            }
        }

        if (storage_.m() == 0) {
            return 0;
        }

        // Peeled BDZ path.
        uint32_t j = (storage_.g_get(triple.v0) + storage_.g_get(triple.v1) + storage_.g_get(triple.v2)) % 3;
        uint32_t selected_vertex = triple.v(static_cast<int>(j));
        return storage_.rank(selected_vertex);
    }

    /**
     * @brief Check if a key is in the set and return the slot (only enabled if KeyPolicy supports it).
     * Returns {true, slot} if key is in the set, {false, 0} otherwise.
     * For peeled keys, slot is the BDZ rank; for fallback keys, slot is n_peeled_ + offset.
     */
    template <typename K = KeyPolicy>
    std::enable_if_t<K::supports_contains, std::pair<bool, uint32_t>> locate(uint32_t key) const {
        if (n_ == 0)
            return {false, 0};

        auto triple = compute_triple(key);

        // Fallback path: check residual keys via binary search.
        if (!residual_keys_.empty()) {
            auto it = std::lower_bound(residual_keys_.begin(), residual_keys_.end(), triple.mixed_key);
            if (it != residual_keys_.end() && *it == triple.mixed_key)
                return {true, n_peeled_ + static_cast<uint32_t>(it - residual_keys_.begin())};
        }

        // Peeled BDZ path.
        int which_h = determine_which_h(triple.v0, triple.v1, triple.v2);
        uint32_t selected_vertex = triple.v(which_h);
        if (!storage_.is_vertex_occupied(selected_vertex))
            return {false, 0};
        uint32_t idx = storage_.rank(selected_vertex);
        if (idx >= n_peeled_)
            return {false, 0};
        if (!key_policy_.verify(idx, triple.mixed_key, which_h))
            return {false, 0};
        return {true, idx};
    }

    /**
     * @brief Check if a key is in the set (only enabled if KeyPolicy supports it).
     */
    template <typename K = KeyPolicy>
    std::enable_if_t<K::supports_contains, bool> contains(uint32_t key) const {
        return locate(key).first;
    }

  private:
    /**
     * @brief Compute target segment size for prime selection
     * @return The target segment size (~m/3) used as base for prime selection
     */
    uint64_t compute_target_segment() const {
        const uint64_t target_m = static_cast<uint64_t>(std::ceil(1.25 * static_cast<double>(n_)));
        return std::max<uint64_t>(3, (target_m + 2) / 3);  // ceil(target_m/3)
    }

    // ========== STEP 1: Hash Function Initialization ==========
    /**
     * @brief Initialize the three hash functions h0, h1, h2
     * Uses separate primes for each hash function; retry 0 picks three consecutive
     * primes near m/3, later retries bump one prime at a time and resample a_k, b_k
     * to vary the hypergraph while keeping m essentially constant.
     */
    bool initialize_hash_functions(const std::vector<uint32_t>& keys, int retry_count) {
        const uint64_t target_segment = compute_target_segment();

        if (retry_count == 0) {
            uint64_t base = target_segment;
            uint64_t p0 = next_prime(base);
            uint64_t p1 = next_prime(p0 + 1);
            uint64_t p2 = next_prime(p1 + 1);
            primes_[0] = p0;
            primes_[1] = p1;
            primes_[2] = p2;
        } else {
            int order[3] = {2, 1, 0};
            int idx = order[(retry_count - 1) % 3];
            primes_[static_cast<size_t>(idx)] = next_prime(primes_[static_cast<size_t>(idx)] + 1);
        }

        assert(
            std::max({primes_[0], primes_[1], primes_[2]}) <= std::numeric_limits<uint32_t>::max() &&
            "p_k must fit in uint32_t so that a_k*x stays within uint64_t in hash_function"
        );

        // Compute segment starts and total m
        segment_starts_[0] = 0;
        segment_starts_[1] = primes_[0];
        segment_starts_[2] = primes_[0] + primes_[1];
        m_ = static_cast<uint32_t>(primes_[0] + primes_[1] + primes_[2]);

        // Use SplitMix64 mixer for better seed diversity
        uint64_t final_seed = splitmix64(SEED ^ static_cast<uint64_t>(retry_count));
        std::mt19937_64 rng(final_seed);
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
        storage_.initialize(m_);

        LOG_INFO(
            "[MPHF::init_hash] retry=" << retry_count << " m=" << m_ << " primes={" << primes_[0] << ", "
                                       << primes_[1] << ", " << primes_[2] << "} multipliers={"
                                       << multipliers_[0] << ", " << multipliers_[1] << ", "
                                       << multipliers_[2] << "} biases={" << biases_[0] << ", " << biases_[1]
                                       << ", " << biases_[2] << "}"
        );

        return true;
    }

    // ========== STEP 2: Triple Generation ==========
    /**
     * @brief Generate triples for all keys using the three hash functions
     */
    std::vector<Triple> generate_triples(const std::vector<uint32_t>& keys) {
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
     * @brief Perform peeling algorithm to find processing order.
     * @param triples Input triples
     * @param peeling_order Output order (topological sort of peeled edges)
     * @param edge_removed_out Output: edge_removed[i]=1 iff edge i was peeled.
     *        Populated regardless of success/failure; used by try_build for fallback.
     * @return true if all edges were peeled (full success)
     */
    bool perform_peeling(
        const std::vector<Triple>& triples,
        std::vector<Triple>& peeling_order,
        std::vector<uint8_t>& edge_removed_out
    ) {
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

        last_peeled_ = static_cast<uint32_t>(peeling_order.size());
        bool ok = (last_peeled_ == triples.size());
        if (!ok) {
            // Per-segment degree diagnostics (initial degrees, before peeling)
            uint32_t seg_bounds[4] = {
                0, static_cast<uint32_t>(segment_starts_[1]), static_cast<uint32_t>(segment_starts_[2]), m_
            };
            uint32_t seg_min[3], seg_max[3], seg_deg1[3];
            for (int s = 0; s < 3; ++s) {
                seg_min[s] = UINT32_MAX;
                seg_max[s] = 0;
                seg_deg1[s] = 0;
                for (uint32_t v = seg_bounds[s]; v < seg_bounds[s + 1]; ++v) {
                    uint32_t d = static_cast<uint32_t>(incident[v].size());
                    if (d < seg_min[s])
                        seg_min[s] = d;
                    if (d > seg_max[s])
                        seg_max[s] = d;
                    if (d == 1)
                        seg_deg1[s]++;
                }
            }

            uint32_t mixed_key_min = triples[0].mixed_key, mixed_key_max = triples[0].mixed_key;
            for (const auto& t : triples) {
                if (t.mixed_key < mixed_key_min)
                    mixed_key_min = t.mixed_key;
                if (t.mixed_key > mixed_key_max)
                    mixed_key_max = t.mixed_key;
            }
            uint64_t mixed_key_range = static_cast<uint64_t>(mixed_key_max) - mixed_key_min + 1;
            double density = static_cast<double>(triples.size()) / static_cast<double>(mixed_key_range);

            LOG_WARN(
                "[MPHF::peeling] Failed: peeled "
                << peeling_order.size() << "/" << triples.size() << " edges (cycle remains)"
                << " | seg_min_deg={" << seg_min[0] << "," << seg_min[1] << "," << seg_min[2] << "}"
                << " seg_max_deg={" << seg_max[0] << "," << seg_max[1] << "," << seg_max[2] << "}"
                << " seg_deg1={" << seg_deg1[0] << "," << seg_deg1[1] << "," << seg_deg1[2] << "}"
                << " mixed_keys=[" << mixed_key_min << ".." << mixed_key_max << "] density=" << density
            );
        } else {
            LOG_INFO("[MPHF::peeling] Success: peeled all " << triples.size() << " edges");
        }
        edge_removed_out = std::move(edge_removed);
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
            uint32_t s = (storage_.g_get(t.v0) + storage_.g_get(t.v1) + storage_.g_get(t.v2)) % 3;

            // Need (G[v0] + G[v1] + G[v2]) % 3 == j
            uint32_t need = static_cast<uint32_t>((3 + j - static_cast<int>(s)) % 3);
            storage_.g_set(t.v(j), need);

            // Mark all as visited (only one was newly assigned, but the others are effectively fixed now)
            visited[t.v0] = visited[t.v1] = visited[t.v2] = 1;

            // std::cout << "[MPHF::assignG] triple=(" << t.v0 << ", " << t.v1 << ", " << t.v2 << ") j=" << j
            //           << " set G[" << t.v(j) << "]=" << need << "\n";
        }
    }

    // ========== HELPER FUNCTIONS ==========
    /**
     * @brief Compute hash function h_k(x) for k ∈ {0,1,2}
     * h_k(x) = d_k + ((a_k · x + b_k) mod r_k)
     * with r_k = primes_[k], a_k = multipliers_[k], b_k = biases_[k], d_k = segment_starts_[k].
     *
     * Since a_k < p_k < 2^32, x < 2^32, the product a_k·x fits in uint64_t with no overflow.
     * Reminder: p_k ≈ m/3, m = 1.25·n => p_k ≈ 0.4167·n, and n < 2^32 => p_k < 2^31.
     */
    uint32_t hash_function(uint32_t x, int k) const {
        const size_t i = static_cast<size_t>(k);
        const uint64_t r = primes_[i];
        uint64_t mapped = (x * multipliers_[i]) % r;  // in [0, r)
        mapped += biases_[i];
        if (mapped >= r)
            mapped -= r;  // single correction instead of modulo
        return static_cast<uint32_t>(segment_starts_[i] + mapped);
    }

    /**
     * @brief Compute triple (v0, v1, v2) for a given key.
     * Applies premix32 before hashing to destroy arithmetic structure.
     * triple.mixed_key stores the mixed value premix32(key); the key policy
     * operates on that mixed value (not the original key) so that quotienting remains valid.
     */
    Triple compute_triple(uint32_t key) const {
        uint32_t mixed_key = premix32(key);
        return Triple(
            mixed_key, hash_function(mixed_key, 0), hash_function(mixed_key, 1), hash_function(mixed_key, 2)
        );
    }

    /**
     * @brief Determine which hash function index j ∈ {0,1,2} was used.
     * j = (G[v0] + G[v1] + G[v2]) % 3
     */
    int determine_which_h(uint32_t v0, uint32_t v1, uint32_t v2) const {
        return static_cast<int>((storage_.g_get(v0) + storage_.g_get(v1) + storage_.g_get(v2)) % 3);
    }
};

}  // namespace hashing
}  // namespace cltj

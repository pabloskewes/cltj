#pragma once

// Note: the enabled specialization (KeyDumper<true>) uses std::filesystem,
// which requires C++17. This header is only included from H-CLTJ targets
// that explicitly set CXX_STANDARD 17 in CMakeLists.txt.

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include <filesystem>

namespace cltj {
namespace hashing {

namespace {

/**
 * @brief Resolve MPHF key dump directory.
 * Checks environment; falls back to fallback path.
 */
static inline std::string resolve_dump_dir(const char* fallback) {
    const char* env = std::getenv("CLTJ_MPHF_DUMP_DIR");
    if (env && env[0] != '\0')
        return std::string(env);
    return std::string(fallback);
}

/**
 * @brief Resolve JSONL log path for MPHF key dumps.
 * Checks environment; falls back to dir + "/mphf_dumps.jsonl".
 */
static inline std::string resolve_dump_jsonl(const std::string& dir) {
    const char* env = std::getenv("CLTJ_MPHF_DUMP_JSONL");
    if (env && env[0] != '\0')
        return std::string(env);
    return dir + "/mphf_dumps.jsonl";
}

/**
 * @brief FNV-1a 64-bit checksum over the raw bytes of a `uint64_t` array.
 *
 * Fowler-Noll-Vo hash function, variant 1a (FNV-1a), 64-bit version.
 * Source: http://www.isthe.com/chongo/tech/comp/fnv/
 */
static inline uint64_t fnv1a64(const std::vector<uint64_t>& keys) {
    constexpr uint64_t basis = 14695981039346656037ULL;  // 0xcbf29ce484222325
    constexpr uint64_t prime = 1099511628211ULL;  // 0x00000100000001b3
    uint64_t h = basis;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(keys.data());
    for (size_t i = 0; i < keys.size() * sizeof(uint64_t); ++i) {
        h ^= data[i];
        h *= prime;
    }
    return h;
}

}  // namespace

/**
 * @brief Primary template for MPHF key dumper.
 *
 * This template is used when the dumper is not enabled.
 * All methods are inlined and do nothing.
 */
template <bool Enabled>
struct KeyDumper {
    static constexpr bool is_enabled = false;

    explicit KeyDumper(const char*) {}
    void dump(const std::vector<uint64_t>&, uint32_t, uint64_t) {}
};

/**
 * @brief Dumper for MPHF key sets on build failure.
 *
 * Writes one binary file per failed node, plus a JSONL log entry.
 *
 * Binary format (40-byte header + payload):
 *   [4]  magic     "MKEY"  (file signature for format identification)
 *   [4]  version   uint32 = 1
 *   [4]  trie_id   uint32
 *   [4]  _pad      uint32 = 0   (8-byte alignment for node_pos)
 *   [8]  node_pos  uint64
 *   [8]  n_keys    uint64
 *   [8]  checksum  uint64 (FNV-1a 64-bit over the key array bytes)
 *   [n*8] keys     uint64_t[]
 *
 * JSONL log: one line per dump with metadata (node context, key range, density).
 * Binary dir:  CLTJ_MPHF_DUMP_DIR  env var (fallback: ".")
 * JSONL path:  CLTJ_MPHF_DUMP_JSONL env var (fallback: {dir}/mphf_dumps.jsonl)
 */
template <>
struct KeyDumper<true> {
    static constexpr bool is_enabled = true;

    std::string dir_;
    std::ofstream dump_log_;

    explicit KeyDumper(const char* dir) : dir_(resolve_dump_dir(dir)) {
        std::filesystem::create_directories(dir_);
        dump_log_.open(resolve_dump_jsonl(dir_), std::ios::app);
    }

    void dump(const std::vector<uint64_t>& keys, uint32_t trie_id, uint64_t node_pos) {
        uint64_t n = keys.size();
        if (n == 0)
            return;

        uint64_t key_min = keys[0], key_max = keys[0];
        for (uint64_t k : keys) {
            if (k < key_min)
                key_min = k;
            if (k > key_max)
                key_max = k;
        }
        uint64_t key_range = key_max - key_min + 1;
        double density = static_cast<double>(n) / static_cast<double>(key_range);
        uint64_t checksum = fnv1a64(keys);

        std::string fname = "mphf_dump_t" + std::to_string(trie_id) + "_pos" + std::to_string(node_pos) +
            "_n" + std::to_string(n) + ".bin";
        std::string fpath = dir_ + "/" + fname;

        {
            const char magic[4] = {'M', 'K', 'E', 'Y'};
            uint32_t version = 1;
            uint32_t pad = 0;
            std::ofstream bin(fpath, std::ios::binary | std::ios::trunc);
            bin.write(magic, 4);
            bin.write(reinterpret_cast<const char*>(&version), 4);
            bin.write(reinterpret_cast<const char*>(&trie_id), 4);
            bin.write(reinterpret_cast<const char*>(&pad), 4);
            bin.write(reinterpret_cast<const char*>(&node_pos), 8);
            bin.write(reinterpret_cast<const char*>(&n), 8);
            bin.write(reinterpret_cast<const char*>(&checksum), 8);
            bin.write(
                reinterpret_cast<const char*>(keys.data()), static_cast<std::streamsize>(n * sizeof(uint64_t))
            );
        }

        // Hex checksum (16 chars, zero-padded) for readability in the log.
        char hex_csum[17];
        std::snprintf(hex_csum, sizeof(hex_csum), "%016llx", static_cast<unsigned long long>(checksum));
        dump_log_ << "{\"type\":\"key_dump\""
                  << ",\"trie_id\":" << trie_id << ",\"node_pos\":" << node_pos << ",\"n_keys\":" << n
                  << ",\"file\":\"" << fname << "\""
                  << ",\"fnv1a64\":\"" << hex_csum << "\""
                  << ",\"key_min\":" << key_min << ",\"key_max\":" << key_max << ",\"density\":" << density
                  << "}\n";
        // Flush immediately so the log survives a crash mid-build.
        dump_log_.flush();
    }
};

}  // namespace hashing
}  // namespace cltj

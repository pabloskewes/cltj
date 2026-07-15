#pragma once

// KeyDumper and the MPHF key-dump binary format.
// The enabled specialization (KeyDumper<true>) uses std::filesystem, which requires C++17.
// This header is only included from H-CLTJ targets that explicitly set CXX_STANDARD 17 in CMakeLists.txt.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace cltj {
namespace hashing {

// ============================================================
// Binary format for MPHF key dumps
// ============================================================

constexpr char DUMP_MAGIC[4] = {'M', 'K', 'E', 'Y'};
constexpr uint32_t DUMP_VERSION = 2;

/**
 * @brief On-disk header for MPHF key dump files (40 bytes, little-endian).
 *
 * Layout:
 *   [4]  magic     "MKEY"
 *   [4]  version   uint32 = 1
 *   [4]  trie_id   uint32
 *   [4]  pad       uint32 = 0  (8-byte alignment for node_pos)
 *   [8]  node_pos  uint64
 *   [8]  n_keys    uint64
 *   [8]  checksum  uint64 (FNV-1a 64-bit over the key array bytes)
 *   [n*4] keys     uint32_t[]
 */
struct KeyDumpHeader {
    char magic[4];  // DUMP_MAGIC
    uint32_t version;  // DUMP_VERSION
    uint32_t trie_id;
    uint32_t pad;  // = 0, keeps node_pos 8-byte aligned
    uint64_t node_pos;
    uint64_t n_keys;
    uint64_t checksum;  // fnv1a64 over raw key bytes
};
static_assert(sizeof(KeyDumpHeader) == 40, "KeyDumpHeader must be 40 bytes");

/**
 * @brief FNV-1a 64-bit checksum over the raw bytes of a uint32_t array.
 * Reference: http://www.isthe.com/chongo/tech/comp/fnv/  (fnv_64a_buf)
 */
inline uint64_t fnv1a64(const std::vector<uint32_t>& keys) {
    constexpr uint64_t basis = 14695981039346656037ULL;
    constexpr uint64_t prime = 1099511628211ULL;
    uint64_t h = basis;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(keys.data());
    for (size_t i = 0; i < keys.size() * sizeof(uint32_t); ++i) {
        h ^= data[i];
        h *= prime;
    }
    return h;
}

/**
 * @brief Build a fully-initialised KeyDumpHeader for the given key set.
 * Sets magic, version, trie_id, pad, node_pos, n_keys, and checksum.
 */
inline KeyDumpHeader make_dump_header(
    uint32_t trie_id, uint64_t node_pos, const std::vector<uint32_t>& keys
) {
    KeyDumpHeader hdr{};
    hdr.magic[0] = DUMP_MAGIC[0];
    hdr.magic[1] = DUMP_MAGIC[1];
    hdr.magic[2] = DUMP_MAGIC[2];
    hdr.magic[3] = DUMP_MAGIC[3];
    hdr.version = DUMP_VERSION;
    hdr.trie_id = trie_id;
    hdr.pad = 0;
    hdr.node_pos = node_pos;
    hdr.n_keys = static_cast<uint64_t>(keys.size());
    hdr.checksum = fnv1a64(keys);
    return hdr;
}

/**
 * @brief Write a key dump binary file.
 * @return true on success, false on I/O error.
 */
inline bool write_dump_file(
    const std::string& path, const KeyDumpHeader& hdr, const std::vector<uint32_t>& keys
) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
        return false;
    out.write(reinterpret_cast<const char*>(&hdr), sizeof(KeyDumpHeader));
    out.write(
        reinterpret_cast<const char*>(keys.data()),
        static_cast<std::streamsize>(keys.size() * sizeof(uint32_t))
    );
    return out.good();
}

/**
 * @brief Read and validate a key dump binary file.
 * @return Empty string on success; human-readable error on failure.
 * On success, hdr and keys are populated.
 */
inline std::string read_dump_file(const std::string& path, KeyDumpHeader& hdr, std::vector<uint32_t>& keys) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return "cannot open '" + path + "'";

    in.read(reinterpret_cast<char*>(&hdr), sizeof(KeyDumpHeader));
    if (!in)
        return "failed to read 40-byte header from '" + path + "'";

    if (hdr.magic[0] != DUMP_MAGIC[0] || hdr.magic[1] != DUMP_MAGIC[1] || hdr.magic[2] != DUMP_MAGIC[2] ||
        hdr.magic[3] != DUMP_MAGIC[3]) {
        char got[5] = {hdr.magic[0], hdr.magic[1], hdr.magic[2], hdr.magic[3], '\0'};
        return std::string("invalid magic '") + got + "' (expected MKEY)";
    }
    if (hdr.version != DUMP_VERSION)
        return "unsupported version " + std::to_string(hdr.version) + " (expected " +
            std::to_string(DUMP_VERSION) + ")";

    keys.resize(hdr.n_keys);
    in.read(
        reinterpret_cast<char*>(keys.data()), static_cast<std::streamsize>(hdr.n_keys * sizeof(uint32_t))
    );
    if (!in)
        return "failed to read " + std::to_string(hdr.n_keys) + " keys";

    return "";
}

// ============================================================
// KeyDumper
// ============================================================

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
 * @brief Resolve MPHF key dump JSONL path.
 * Checks environment; falls back to directory + "/mphf_dumps.jsonl".
 */
static inline std::string resolve_dump_jsonl(const std::string& dir) {
    const char* env = std::getenv("CLTJ_MPHF_DUMP_JSONL");
    if (env && env[0] != '\0')
        return std::string(env);
    return dir + "/mphf_dumps.jsonl";
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
    void dump(const std::vector<uint32_t>&, uint32_t, uint64_t) {}
};

/**
 * @brief Dumper for MPHF key sets on build failure.
 *
 * Writes one binary file per failed node, plus a JSONL log entry.
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

    void dump(const std::vector<uint32_t>& keys, uint32_t trie_id, uint64_t node_pos) {
        if (keys.empty())
            return;

        KeyDumpHeader hdr = make_dump_header(trie_id, node_pos, keys);

        std::string fname = "mphf_dump_t" + std::to_string(trie_id) + "_pos" + std::to_string(node_pos) +
            "_n" + std::to_string(hdr.n_keys) + ".bin";
        write_dump_file(dir_ + "/" + fname, hdr, keys);

        uint32_t key_min = keys[0], key_max = keys[0];
        for (uint32_t k : keys) {
            if (k < key_min)
                key_min = k;
            if (k > key_max)
                key_max = k;
        }
        double density = static_cast<double>(hdr.n_keys) / static_cast<double>(key_max - key_min + 1);

        char hex_csum[17];
        std::snprintf(hex_csum, sizeof(hex_csum), "%016llx", static_cast<unsigned long long>(hdr.checksum));
        dump_log_ << "{\"type\":\"key_dump\""
                  << ",\"trie_id\":" << trie_id << ",\"node_pos\":" << node_pos
                  << ",\"n_keys\":" << hdr.n_keys << ",\"file\":\"" << fname << "\""
                  << ",\"fnv1a64\":\"" << hex_csum << "\""
                  << ",\"key_min\":" << key_min << ",\"key_max\":" << key_max << ",\"density\":" << density
                  << "}\n";
        dump_log_.flush();
    }
};

}  // namespace hashing
}  // namespace cltj

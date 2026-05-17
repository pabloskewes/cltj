#pragma once

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <util/instrument.hpp>

namespace cltj {
namespace query {

/**
 * @brief JSONL tracer for query-time diagnostics (hash path).
 *
 * Controlled by CMake option ``CLTJ_TRACE_QUERY`` -> ``cltj::TRACE_QUERY``.
 * When disabled, all methods are empty inlines (zero overhead).
 *
 * Output path: env var ``CLTJ_QUERY_TRACE_FILE`` if set,
 * otherwise the ``fallback_path`` given to the constructor.
 */
template <bool Enabled>
struct QueryTracer {
    static constexpr bool is_enabled = false;

    explicit QueryTracer(const char*) {}

    void set_query_id(uint64_t) {}
    void on_session_start(const char*) {}

    void on_pure_hash_frame(uint64_t, uint64_t, const std::vector<uint64_t>&, uint64_t) {}

    void on_exists_result(uint64_t, uint64_t, bool) {}

    void flush() {}
};

template <>
struct QueryTracer<true> {
    static constexpr bool is_enabled = true;

    std::ofstream out_;
    uint64_t query_id_ = 0;

    static const char* resolve_path(const char* fallback_path) {
        const char* env_path = std::getenv("CLTJ_QUERY_TRACE_FILE");
        if (env_path != nullptr && env_path[0] != '\0')
            return env_path;
        return fallback_path;
    }

    explicit QueryTracer(const char* fallback_path) : out_(resolve_path(fallback_path), std::ios::app) {}

    void set_query_id(uint64_t qid) { query_id_ = qid; }

    void on_session_start(const char* note) {
        out_ << "{\"type\":\"session_start\"";
        if (note != nullptr && note[0] != '\0')
            out_ << ",\"note\":\"" << note << "\"";
        out_ << "}\n";
        flush();
    }

    void on_pure_hash_frame(
        uint64_t depth, uint64_t var_id, const std::vector<uint64_t>& children_sizes, uint64_t min_iter_idx
    ) {
        out_ << "{\"type\":\"pure_hash_frame\""
             << ",\"query_id\":" << query_id_ << ",\"depth\":" << depth << ",\"var_id\":" << var_id
             << ",\"min_iter_idx\":" << min_iter_idx << ",\"children_sizes\":[";
        for (size_t i = 0; i < children_sizes.size(); ++i) {
            if (i)
                out_ << ',';
            out_ << children_sizes[i];
        }
        out_ << "]}\n";
        flush();
    }

    void on_exists_result(uint64_t candidate, uint64_t iter_idx, bool found) {
        out_ << "{\"type\":\"exists_result\""
             << ",\"query_id\":" << query_id_ << ",\"c\":" << candidate << ",\"iter\":" << iter_idx
             << ",\"found\":" << (found ? "true" : "false") << "}\n";
    }

    void flush() { out_.flush(); }
};

}  // namespace query
}  // namespace cltj

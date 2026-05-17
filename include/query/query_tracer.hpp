#pragma once

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <util/instrument.hpp>

namespace cltj {
namespace query {

inline constexpr uint64_t CANDIDATE_FRAME_VALUE_LIMIT = 100;

/**
 * @brief JSONL tracer for query-time diagnostics.
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

    template <class Tuple>
    void on_candidate_frame(
        const char*, uint64_t, uint64_t, const Tuple&, uint64_t, const std::vector<uint64_t>&
    ) {}

    template <class Tuple>
    void on_exists_result(uint64_t, uint64_t, const Tuple&, uint64_t, uint64_t, bool) {}

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

    template <class Tuple>
    void write_prefix(const Tuple& prefix, uint64_t depth) {
        out_ << "\"prefix\":[";
        for (uint64_t i = 0; i < depth; ++i) {
            if (i)
                out_ << ',';
            out_ << "{\"var\":" << static_cast<uint64_t>(prefix[i].first)
                 << ",\"value\":" << static_cast<uint64_t>(prefix[i].second) << "}";
        }
        out_ << "]";
    }

    template <class Tuple>
    void on_candidate_frame(
        const char* mode,
        uint64_t depth,
        uint64_t var_id,
        const Tuple& prefix,
        uint64_t candidate_count,
        const std::vector<uint64_t>& candidates
    ) {
        out_ << "{\"type\":\"candidate_frame\""
             << ",\"mode\":\"" << mode << "\""
             << ",\"query_id\":" << query_id_
             << ",\"depth\":" << depth
             << ",\"var_id\":" << var_id << ",";
        write_prefix(prefix, depth);
        out_ << ",\"candidate_count\":" << candidate_count;
        if (candidate_count <= CANDIDATE_FRAME_VALUE_LIMIT) {
            out_ << ",\"candidates\":[";
            for (size_t i = 0; i < candidates.size(); ++i) {
                if (i)
                    out_ << ',';
                out_ << candidates[i];
            }
            out_ << "]";
        } else {
            out_ << ",\"candidates_omitted\":true";
        }
        out_ << "}\n";
        flush();
    }

    template <class Tuple>
    void on_exists_result(
        uint64_t depth,
        uint64_t var_id,
        const Tuple& prefix,
        uint64_t candidate,
        uint64_t iter_idx,
        bool found
    ) {
        out_ << "{\"type\":\"exists_result\""
             << ",\"query_id\":" << query_id_
             << ",\"depth\":" << depth
             << ",\"var_id\":" << var_id << ",";
        write_prefix(prefix, depth);
        out_ << ",\"c\":" << candidate
             << ",\"iter\":" << iter_idx
             << ",\"found\":" << (found ? "true" : "false") << "}\n";
        flush();
    }

    void flush() { out_.flush(); }
};

template <bool Enabled, class Tuple>
struct CandidateFrame {
    CandidateFrame(QueryTracer<Enabled>&, const char*, uint64_t, uint64_t, const Tuple&) {}

    void add(uint64_t) {}
    void emit() {}
};

template <class Tuple>
struct CandidateFrame<true, Tuple> {
    QueryTracer<true>& tracer_;
    const char* mode_;
    uint64_t depth_;
    uint64_t var_id_;
    const Tuple& prefix_;
    uint64_t count_ = 0;
    std::vector<uint64_t> candidates_;

    CandidateFrame(
        QueryTracer<true>& tracer,
        const char* mode,
        uint64_t depth,
        uint64_t var_id,
        const Tuple& prefix
    )
        : tracer_(tracer), mode_(mode), depth_(depth), var_id_(var_id), prefix_(prefix) {}

    void add(uint64_t candidate) {
        ++count_;
        if (candidates_.size() <= CANDIDATE_FRAME_VALUE_LIMIT)
            candidates_.push_back(candidate);
    }

    void emit() {
        tracer_.on_candidate_frame(mode_, depth_, var_id_, prefix_, count_, candidates_);
    }
};

}  // namespace query
}  // namespace cltj

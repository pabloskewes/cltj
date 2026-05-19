#pragma once

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
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
 * Output directory: env var ``CLTJ_QUERY_TRACE_DIR`` if set,
 * otherwise the ``fallback_dir`` given to the constructor.
 * One trace file is written per query as `query-<qid>.jsonl`.
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
    std::string trace_dir_;
    uint64_t query_id_ = 0;
    bool has_active_query_ = false;
    uint64_t exists_true_count_ = 0;
    uint64_t exists_false_count_ = 0;

    static std::string resolve_dir(const char* fallback_dir) {
        const char* env_dir = std::getenv("CLTJ_QUERY_TRACE_DIR");
        if (env_dir != nullptr && env_dir[0] != '\0')
            return env_dir;
        if (fallback_dir != nullptr && fallback_dir[0] != '\0')
            return fallback_dir;
        return "query_traces";
    }

    static bool ensure_dir(const std::string& dir) {
        if (dir.empty())
            return false;
        return mkdir(dir.c_str(), 0755) == 0 || errno == EEXIST;
    }

    explicit QueryTracer(const char* fallback_dir) : trace_dir_(resolve_dir(fallback_dir)) {}

    ~QueryTracer() { finalize_query(); }

    void reset_exists_counts() {
        exists_true_count_ = 0;
        exists_false_count_ = 0;
    }

    void write_exists_summary() {
        if (!out_.is_open())
            return;
        out_ << "{\"type\":\"exists_summary\""
             << ",\"query_id\":" << query_id_
             << ",\"found_true_count\":" << exists_true_count_
             << ",\"found_false_count\":" << exists_false_count_
             << "}\n";
    }

    void finalize_query() {
        if (!has_active_query_)
            return;
        write_exists_summary();
        out_.flush();
        out_.close();
        has_active_query_ = false;
    }

    void open_query_file() {
        if (!ensure_dir(trace_dir_))
            return;
        const std::string file_path = trace_dir_ + "/query-" + std::to_string(query_id_) + ".jsonl";
        out_.open(file_path.c_str(), std::ios::out | std::ios::trunc);
        if (!out_.is_open())
            return;
        has_active_query_ = true;
        reset_exists_counts();
    }

    void ensure_query_file_open() {
        if (!has_active_query_)
            open_query_file();
    }

    void set_query_id(uint64_t qid) {
        if (has_active_query_ && qid == query_id_)
            return;
        finalize_query();
        query_id_ = qid;
        open_query_file();
    }

    void on_session_start(const char* note) {
        ensure_query_file_open();
        if (!has_active_query_)
            return;
        out_ << "{\"type\":\"session_start\"";
        if (note != nullptr && note[0] != '\0')
            out_ << ",\"note\":\"" << note << "\"";
        out_ << "}\n";
        flush();
    }

    void on_pure_hash_frame(
        uint64_t depth, uint64_t var_id, const std::vector<uint64_t>& children_sizes, uint64_t min_iter_idx
    ) {
        ensure_query_file_open();
        if (!has_active_query_)
            return;
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
        ensure_query_file_open();
        if (!has_active_query_)
            return;
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
        ensure_query_file_open();
        if (!has_active_query_)
            return;
        if (found) {
            ++exists_true_count_;
            return;
        }
        ++exists_false_count_;
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

    void flush() {
        if (has_active_query_)
            out_.flush();
    }
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

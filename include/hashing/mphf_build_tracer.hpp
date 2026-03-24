#pragma once

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>

namespace cltj {
namespace hashing {

/**
 * @brief Primary template for MPHF build tracer.
 * 
 * This template is used when the tracer is not enabled.
 * All methods are inlined and do nothing.
 */
template <bool Enabled>
struct MphfBuildTracer {
    static constexpr bool is_enabled = false;

    explicit MphfBuildTracer(const char*) {}
    void on_node_start(uint32_t, uint64_t, uint64_t, uint32_t) {}
    void on_try_start(int) {}
    void on_try_result(int, uint32_t, uint32_t, uint32_t, bool) {}
    void on_node_end(bool) {}
    void flush() {}
};

/**
 * @brief Tracer for MPHF build events.
 * 
 * Writes events to a file in JSONL format.
 * The file path can be overridden by setting the CLTJ_MPHF_TRACE_FILE environment variable.
 * If the environment variable is not set, the file path is the fallback path.
 * The fallback path is the path of the file passed to the constructor.
 */
template <>
struct MphfBuildTracer<true> {
    static constexpr bool is_enabled = true;

    std::ofstream out_;
    std::chrono::steady_clock::time_point node_t0_;
    std::chrono::steady_clock::time_point retry_t0_;

    static const char* resolve_path(const char* fallback_path) {
        const char* env_path = std::getenv("CLTJ_MPHF_TRACE_FILE");
        if (env_path != nullptr && env_path[0] != '\0') {
            return env_path;
        }
        return fallback_path;
    }

    explicit MphfBuildTracer(const char* path) : out_(resolve_path(path), std::ios::app) {}

    void on_node_start(uint32_t trie_id, uint64_t node_pos, uint64_t n_children, uint32_t threshold) {
        node_t0_ = std::chrono::steady_clock::now();
        out_ << "{\"type\":\"node_start\""
             << ",\"trie_id\":" << trie_id << ",\"node_pos\":" << node_pos << ",\"n_children\":" << n_children
             << ",\"threshold\":" << threshold << "}\n";
    }

    void on_try_start(int) {
        retry_t0_ = std::chrono::steady_clock::now();
    }

    void on_try_result(int retry, uint32_t m, uint32_t n, uint32_t peeled, bool success) {
        double elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - retry_t0_).count();
        double frac = n > 0 ? static_cast<double>(peeled) / n : 0.0;
        out_ << "{\"type\":\"try_result\""
             << ",\"retry\":" << retry << ",\"m\":" << m << ",\"n\":" << n << ",\"peeled\":" << peeled
             << ",\"peeled_frac\":" << frac << ",\"success\":" << (success ? "true" : "false")
             << ",\"elapsed_ms\":" << elapsed_ms << "}\n";
    }

    void on_node_end(bool success) {
        double elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - node_t0_).count();
        out_ << "{\"type\":\"node_end\""
             << ",\"success\":" << (success ? "true" : "false") << ",\"elapsed_ms\":" << elapsed_ms << "}\n";
        out_.flush();
    }

    void flush() { out_.flush(); }
};

}  // namespace hashing
}  // namespace cltj

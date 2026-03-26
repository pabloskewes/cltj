/**
 * @file repro_mphf_build.cpp
 * @brief Reproduce an MPHF build from a binary key dump produced by KeyDumper.
 *
 * Reads a .bin dump (key_dumper.hpp format), verifies its checksum,
 * prints key-range diagnostics, and re-runs the MPHF build with a live
 * MphfBuildTracer<true>.  Useful for isolating and studying peeling failures
 * without re-running the full H-CLTJ build.
 *
 * Usage:
 *   repro-mphf-build <dump.bin> [--trace <path>] [--threshold <n>] [--no-verify]
 *
 * Exit code:
 *   0  build succeeded
 *   1  build failed (all retries exhausted) or I/O error
 */

#include <hashing/key_dumper.hpp>
#include <hashing/key_policies.hpp>
#include <hashing/mphf_bdz.hpp>
#include <hashing/mphf_build_tracer.hpp>
#include <hashing/storage/glgh.hpp>
#include <CLI11.hpp>

#include <cstdio>
#include <iomanip>
#include <iostream>
#include <string>

using mphf_type = cltj::hashing::MPHF<cltj::hashing::GlGhStorage, cltj::hashing::policies::QuotientKey>;

int main(int argc, char** argv) {
    CLI::App app{
        "Reproduce MPHF build from a key dump file.\n"
        "Always runs with full tracing (MphfBuildTracer<true>).\n"
        "Uses the same MPHF type as production (GlGhStorage + QuotientKey)."
    };

    std::string dump_file;
    std::string trace_file;
    uint32_t threshold = 0;
    bool no_verify = false;

    app.add_option("dump_file", dump_file, "Binary key dump (.bin) produced by KeyDumper")->required();
    app.add_option("--trace", trace_file, "Output JSONL trace file (default: <dump_file>.trace.jsonl)");
    app.add_option(
        "--threshold", threshold, "Node threshold used during original build (for JSONL metadata)"
    );
    app.add_flag("--no-verify", no_verify, "Skip FNV-1a checksum verification");

    CLI11_PARSE(app, argc, argv);

    if (trace_file.empty())
        trace_file = dump_file + ".trace.jsonl";

    // --- Read and validate dump ---
    cltj::hashing::KeyDumpHeader hdr{};
    std::vector<uint64_t> keys;
    std::string err = cltj::hashing::read_dump_file(dump_file, hdr, keys);
    if (!err.empty()) {
        std::cerr << "error: " << err << "\n";
        return 1;
    }

    // Guard against corrupt dump with n_keys=0
    if (keys.empty()) {
        std::cerr << "error: dump has 0 keys\n";
        return 1;
    }

    // Verify checksum
    if (!no_verify) {
        uint64_t computed = cltj::hashing::fnv1a64(keys);
        if (computed != hdr.checksum) {
            char exp_hex[17], got_hex[17];
            std::snprintf(exp_hex, sizeof(exp_hex), "%016llx", (unsigned long long)hdr.checksum);
            std::snprintf(got_hex, sizeof(got_hex), "%016llx", (unsigned long long)computed);
            std::cerr << "error: checksum mismatch\n"
                      << "  expected: " << exp_hex << "\n"
                      << "  computed: " << got_hex << "\n";
            return 1;
        }
    }

    // Key-range diagnostics
    uint64_t key_min = keys[0], key_max = keys[0];
    for (uint64_t k : keys) {
        if (k < key_min)
            key_min = k;
        if (k > key_max)
            key_max = k;
    }
    double density = static_cast<double>(hdr.n_keys) / static_cast<double>(key_max - key_min + 1);

    std::cout << "=== repro-mphf-build ===\n"
              << "  dump      : " << dump_file << "\n"
              << "  trie_id   : " << hdr.trie_id << "\n"
              << "  node_pos  : " << hdr.node_pos << "\n"
              << "  n_keys    : " << hdr.n_keys << "\n"
              << "  threshold : " << threshold << "\n"
              << "  key_range : [" << key_min << ".." << key_max << "]\n"
              << std::fixed << std::setprecision(6) << "  density   : " << density << "\n"
              << "  checksum  : " << (no_verify ? "(skipped)" : "OK") << "\n"
              << "  trace     : " << trace_file << "\n"
              << "========================\n";

    // Run MPHF build with live tracer
    cltj::hashing::MphfBuildTracer<true> tracer(trace_file.c_str());
    // Mark session boundary so repeated runs in the same trace file are distinguishable.
    tracer.on_session_start(dump_file.c_str());
    // Pass the real threshold so the JSONL node_start event is complete.
    tracer.on_node_start(hdr.trie_id, hdr.node_pos, hdr.n_keys, threshold);

    mphf_type mphf;
    bool ok = mphf.build(keys, tracer);

    tracer.on_node_end(ok);
    tracer.flush();

    if (ok) {
        std::cout << "Result: SUCCESS (retry=" << mphf.retry_count() << ")\n";
        return 0;
    } else {
        std::cout << "Result: FAILED (all " << mphf.retry_count() << " retries exhausted)\n";
        return 1;
    }
}

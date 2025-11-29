// Minimal standalone tool to count distinct S, P, O from a triples .dat file
// Format per line: <S> <P> <O> as unsigned integers separated by spaces
// - Exact distinct for P using unordered_set
// - KMV (bottom-k) sketch for S and O using 64-bit hashing (splitmix64)
// CLI: count-distinct-spo <path-to-.dat> [--kmv-k N] [--max-lines N] [--outdir DIR]

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

#include <CLI11.hpp>
#include <util/csv_util.hpp>
#include <util/logger.hpp>
#include <index/cltj_index_spo_lite.hpp>
#include <trie/cltj_compact_trie.hpp>

namespace {

// SplitMix64: fast 64-bit hash (suitable as a mixer to uniform [0, 2^64))
static inline uint64_t splitmix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// KMV (bottom-k) sketch maintaining k smallest hashes seen so far.
// We store the k-th order statistic (max among kept) in a max-heap of size k.
struct KMV {
    explicit KMV(size_t k_) : k(k_) {}
    void offer(uint64_t key) {
        // hash to 64-bit and interpret as unsigned
        uint64_t h = splitmix64(key);
        if (heap.size() < k) {
            heap.push(h);
        } else if (h < heap.top()) {
            heap.pop();
            heap.push(h);
        }
    }
    // Estimate distinct count using n_hat ≈ (k-1) / U_k where U_k = x_k / 2^64
    // If fewer than k items seen, return exact = heap.size().
    double estimate() const {
        if (heap.empty())
            return 0.0;
        if (heap.size() < k)
            return static_cast<double>(heap.size());
        const long double two64 = static_cast<long double>(std::numeric_limits<uint64_t>::max()) + 1.0L;
        long double xk = static_cast<long double>(heap.top());
        long double Uk = xk / two64;  // in (0,1)
        if (Uk <= 0.0L) {
            // Extremely unlikely; fallback to k
            return static_cast<double>(k);
        }
        return static_cast<double>((static_cast<long double>(k - 1)) / Uk);
    }
    size_t size() const { return heap.size(); }
    size_t capacity() const { return k; }
    uint64_t kth_hash() const { return heap.empty() ? 0ULL : heap.top(); }

  private:
    size_t k;
    // max-heap to keep smallest k hashes: top() == current k-th smallest
    std::priority_queue<uint64_t, std::vector<uint64_t>, std::less<uint64_t>> heap;
};

struct Args {
    std::string dat_path;
    std::string out_dir = "data/trie_analysis";
    uint64_t max_lines = 0;  // 0 = no limit
    size_t kmv_k = 200000;  // ~1/sqrt(k) ≈ 0.22% rel. error
    std::string index_path;  // optional: if set, perform index root check and exit
};

// Check and print root degrees from a CLTJ index
int check_index_roots(const std::string& index_path) {
    try {
        LOG_INFO("Loading index from " << index_path << "...");
        cltj::cltj_index_spo_lite<cltj::compact_trie> index;
        sdsl::load_from_file(index, index_path);
        LOG_INFO("Index loaded. Size: " << sdsl::size_in_bytes(index) << " bytes");

        for (int i = 0; i < 6; ++i) {
            const auto* trie = index.get_trie(i);
            if (!trie) {
                LOG_INFO("Trie " << i << ": null");
                continue;
            }
            auto root_deg = trie->children(0);
            LOG_INFO("Trie " << i << " root children: " << root_deg);
        }
        return 0;
    } catch (const std::exception& e) {
        LOG_ERROR("Error loading index: " << e.what());
        return 1;
    }
}

// Count distinct S, P, O from a .dat file using KMV for S/O and exact for P
int count_distinct_spo(const Args& args) {
    // Ensure output directory exists
    {
        std::string cmd = std::string("mkdir -p ") + args.out_dir;
        std::system(cmd.c_str());
    }

    std::ifstream in(args.dat_path);
    if (!in) {
        LOG_ERROR("Error: cannot open input file: " << args.dat_path);
        return 1;
    }

    KMV kmv_s(args.kmv_k);
    KMV kmv_o(args.kmv_k);
    std::unordered_set<uint64_t> distinct_p;
    distinct_p.reserve(16384);

    uint64_t triples = 0;
    uint64_t s, p, o;
    while (in >> s >> p >> o) {
        ++triples;
        kmv_s.offer(s);
        kmv_o.offer(o);
        distinct_p.insert(p);
        if (args.max_lines && triples >= args.max_lines)
            break;
        if ((triples % 50000000ULL) == 0ULL) {
            LOG_INFO("Processed " << triples << " lines...");
        }
    }

    double est_S = kmv_s.estimate();
    double est_O = kmv_o.estimate();
    uint64_t distinct_P = static_cast<uint64_t>(distinct_p.size());

    LOG_INFO("Triples: " << triples);
    LOG_INFO("Distinct S (KMV k=" << args.kmv_k << "): ~" << static_cast<uint64_t>(est_S));
    LOG_INFO("Distinct P (exact): " << distinct_P);
    LOG_INFO("Distinct O (KMV k=" << args.kmv_k << "): ~" << static_cast<uint64_t>(est_O));

    // Write CSV via util::CSVWriter
    try {
        std::vector<std::string> header = {
            "triples", "distinct_S_est", "distinct_P", "distinct_O_est", "kmv_k", "max_lines"
        };
        util::CSVWriter writer(args.out_dir + "/distinct_counts.csv", header, 1);
        writer.add_row(
            {std::to_string(triples),
             std::to_string(static_cast<uint64_t>(est_S)),
             std::to_string(distinct_P),
             std::to_string(static_cast<uint64_t>(est_O)),
             std::to_string(args.kmv_k),
             std::to_string(args.max_lines)}
        );
        writer.flush();
    } catch (...) {
        LOG_WARN("Warning: failed to write CSV via CSVWriter");
    }

    // Write JSON
    {
        std::string json_path = args.out_dir + "/distinct_counts.json";
        std::ofstream js(json_path);
        if (js) {
            js << "{\n";
            js << "  \"triples\": " << triples << ",\n";
            js << "  \"distinct_S_est\": " << static_cast<uint64_t>(est_S) << ",\n";
            js << "  \"distinct_P\": " << distinct_P << ",\n";
            js << "  \"distinct_O_est\": " << static_cast<uint64_t>(est_O) << ",\n";
            js << "  \"kmv_k\": " << args.kmv_k << ",\n";
            js << "  \"max_lines\": " << args.max_lines << "\n";
            js << "}\n";
        }
    }

    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Args args;
    CLI::App app{"Count distinct S, P, O from a triples .dat file (KMV for S/O, exact for P)"};
    app.add_option("input", args.dat_path, "Path to triples .dat (S P O)");
    app.add_option("--kmv-k", args.kmv_k, "KMV k (controls error ~1/sqrt(k))");
    app.add_option("--max-lines", args.max_lines, "Max lines to read (0 = all)");
    app.add_option("--outdir", args.out_dir, "Output directory");
    app.add_option(
        "--check-index", args.index_path, "Path to .cltj index to print root degrees (children(0)) and exit"
    );
    CLI11_PARSE(app, argc, argv);

    // Route to appropriate function based on arguments
    if (!args.index_path.empty()) {
        return check_index_roots(args.index_path);
    }

    // Require input .dat when not using --check-index
    if (args.dat_path.empty()) {
        LOG_ERROR("Error: input .dat required when --check-index is not used");
        return 1;
    }

    return count_distinct_spo(args);
}

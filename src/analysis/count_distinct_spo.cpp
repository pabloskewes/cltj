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
};

bool parse_args(int argc, char** argv, Args& args) {
    if (argc < 2)
        return false;
    args.dat_path = argv[1];
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--kmv-k" && i + 1 < argc) {
            args.kmv_k = static_cast<size_t>(std::stoull(argv[++i]));
        } else if (a == "--max-lines" && i + 1 < argc) {
            args.max_lines = std::stoull(argv[++i]);
        } else if (a == "--outdir" && i + 1 < argc) {
            args.out_dir = argv[++i];
        } else if (a == "-h" || a == "--help") {
            return false;
        } else {
            std::cerr << "Unknown argument: " << a << "\n";
            return false;
        }
    }
    return true;
}

void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " <path-to-.dat> [--kmv-k N] [--max-lines N] [--outdir DIR]\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Args args;
    if (!parse_args(argc, argv, args)) {
        print_usage(argv[0]);
        return 1;
    }

    // Ensure output directory exists
    {
        std::string cmd = std::string("mkdir -p ") + args.out_dir;
        std::system(cmd.c_str());
    }

    std::ifstream in(args.dat_path);
    if (!in) {
        std::cerr << "Error: cannot open input file: " << args.dat_path << "\n";
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
            std::cout << "Processed " << triples << " lines..." << std::endl;
        }
    }

    double est_S = kmv_s.estimate();
    double est_O = kmv_o.estimate();
    uint64_t distinct_P = static_cast<uint64_t>(distinct_p.size());

    std::cout << "Triples: " << triples << "\n";
    std::cout << "Distinct S (KMV k=" << args.kmv_k << "): ~" << static_cast<uint64_t>(est_S) << "\n";
    std::cout << "Distinct P (exact): " << distinct_P << "\n";
    std::cout << "Distinct O (KMV k=" << args.kmv_k << "): ~" << static_cast<uint64_t>(est_O) << "\n";

    // Write CSV
    {
        std::string csv_path = args.out_dir + "/distinct_counts.csv";
        std::ofstream csv(csv_path);
        if (csv) {
            csv << "triples,distinct_S_est,distinct_P,distinct_O_est,kmv_k,max_lines\n";
            csv << triples << "," << static_cast<uint64_t>(est_S) << "," << distinct_P << ","
                << static_cast<uint64_t>(est_O) << "," << args.kmv_k << "," << args.max_lines << "\n";
        }
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

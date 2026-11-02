// Cost per key of enumerating the children of every hashed node of an H-CLTJ index, reading
// m_seq[v .. v + deg(v)) against walking the MPHF key cursor. Single thread.
#include <CLI11.hpp>
#include <index/cltj_index_metatrie.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using clock_type = std::chrono::steady_clock;

static const char* trie_names[6] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};

static volatile uint64_t g_sink = 0;

struct NodeResult {
    uint64_t degree;
    double seq_min, seq_median, cursor_min, cursor_median;  // ns per key
};

struct Totals {
    uint64_t nodes = 0;
    uint64_t keys = 0;
    double seq_min_ns = 0, seq_median_ns = 0, cursor_min_ns = 0, cursor_median_ns = 0;
    uint64_t mismatches = 0;

    void add(const NodeResult& r) {
        nodes++;
        keys += r.degree;
        seq_min_ns += r.seq_min * r.degree;
        seq_median_ns += r.seq_median * r.degree;
        cursor_min_ns += r.cursor_min * r.degree;
        cursor_median_ns += r.cursor_median * r.degree;
    }

    void add(const Totals& o) {
        nodes += o.nodes;
        keys += o.keys;
        seq_min_ns += o.seq_min_ns;
        seq_median_ns += o.seq_median_ns;
        cursor_min_ns += o.cursor_min_ns;
        cursor_median_ns += o.cursor_median_ns;
        mismatches += o.mismatches;
    }
};

static std::vector<uint64_t> hashed_nodes(cltj::compact_metatrie_hash<>* trie) {
    std::vector<uint64_t> out;
    uint64_t bv_size = trie->louds_size();
    uint64_t num_zeros = 0;
    for (auto& [deg, cnt] : trie->children_histogram())
        num_zeros += cnt;

    std::vector<uint64_t> current_level = {0};
    while (!current_level.empty()) {
        std::vector<uint64_t> next_level;
        for (auto node : current_level) {
            uint64_t n_children = trie->children(node);
            if (trie->node_has_hash(node))
                out.push_back(node);
            for (uint64_t n = 1; n <= n_children; n++) {
                if (node + 1 + n > num_zeros)
                    break;
                uint64_t child_node = trie->child(node, n);
                if (child_node + 1 < bv_size)
                    next_level.push_back(child_node);
            }
        }
        current_level = std::move(next_level);
    }
    return out;
}

static uint64_t sum_seq(const sdsl::int_vector<>& seq, uint64_t node, uint64_t degree) {
    uint64_t sum = 0;
    for (uint64_t k = 0; k < degree; k++)
        sum += seq[node + k];
    return sum;
}

static uint64_t sum_cursor(cltj::compact_metatrie_hash<>* trie, uint64_t node, uint64_t& count) {
    uint64_t sum = 0;
    count = 0;
    for (auto cur = trie->hash_keys(node); cur.next(); count++)
        sum += cur.key();
    return sum;
}

// Untimed key-by-key check: the cursor must emit seq[node + slot] for slot = 0 .. degree - 1.
// Leaves in reference the sum of the node's labels; it also warms both forms before timing.
static uint64_t check_node(cltj::compact_metatrie_hash<>* trie, uint64_t node, uint64_t degree, uint64_t& reference) {
    uint64_t mismatches = 0;
    uint64_t emitted = 0;
    reference = 0;
    for (auto cur = trie->hash_keys(node); cur.next(); emitted++) {
        if (cur.slot() != emitted || emitted >= degree || cur.key() != trie->seq[node + emitted])
            mismatches++;
        else
            reference += cur.key();
    }
    if (emitted != degree)
        mismatches++;
    return mismatches;
}

static double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    return n % 2 == 1 ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2;
}

static double elapsed_ns(clock_type::time_point a, clock_type::time_point b) {
    return std::chrono::duration<double, std::nano>(b - a).count();
}

static void print_totals(const std::string& label, const Totals& t) {
    double k = t.keys > 0 ? static_cast<double>(t.keys) : 1.0;
    std::cerr << std::left << std::setw(6) << label << std::right << " nodes=" << std::setw(6) << t.nodes
              << " keys=" << std::setw(11) << t.keys << std::fixed << std::setprecision(3)
              << "  ns/key seq=" << t.seq_min_ns / k << " (" << t.seq_median_ns / k << ")"
              << " cursor=" << t.cursor_min_ns / k << " (" << t.cursor_median_ns / k << ")"
              << " ratio=" << t.cursor_min_ns / t.seq_min_ns << " (" << t.cursor_median_ns / t.seq_median_ns << ")"
              << "  mismatches=" << t.mismatches << std::endl;
}

int main(int argc, char** argv) {
    CLI::App app{"Cost per key of m_seq against the MPHF key cursor over all hashed nodes of an H-CLTJ index"};
    std::string index_path;
    std::string csv_path;
    uint32_t reps = 8;
    app.add_option("index", index_path, "Path to the .hcltj index")->required()->check(CLI::ExistingFile);
    app.add_option("--reps", reps, "Timed repetitions of each form per node, a multiple of 4 (default: 8)")
        ->check(CLI::PositiveNumber);
    app.add_option("--csv", csv_path, "Per-node CSV path (default: stdout). Summary and progress go to stderr");
    CLI11_PARSE(app, argc, argv);
    if (reps % 4 != 0) {
        std::cerr << "ERROR: --reps must be a multiple of 4" << std::endl;
        return 1;
    }

    std::ofstream csv_file;
    if (!csv_path.empty()) {
        csv_file.open(csv_path);
        if (!csv_file) {
            std::cerr << "ERROR: cannot open " << csv_path << std::endl;
            return 1;
        }
    }
    std::ostream& csv = csv_path.empty() ? std::cout : csv_file;

    std::cerr << "Loading index: " << index_path << std::endl;
    cltj::compact_ltj_metatrie_hash index;
    try {
        if (!sdsl::load_from_file(index, index_path)) {
            std::cerr << "ERROR: cannot load index: " << index_path << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: cannot load index: " << index_path << ": " << e.what() << std::endl;
        return 1;
    }

    csv << "trie,node_pos,degree,seq_ns_per_key_min,seq_ns_per_key_median,cursor_ns_per_key_min,"
           "cursor_ns_per_key_median\n";
    csv << std::setprecision(6);

    Totals per_trie[6];
    std::vector<double> seq_ns(reps), cursor_ns(reps);
    for (int t = 0; t < 6; t++) {
        auto* trie = index.get_trie(t);
        const auto& seq = trie->seq;
        std::vector<uint64_t> nodes = hashed_nodes(trie);
        std::cerr << trie_names[t] << ": " << nodes.size() << " hashed nodes" << std::endl;
        if (nodes.size() != trie->mphf_count()) {
            std::cerr << "ERROR: " << trie_names[t] << ": BFS found " << nodes.size() << " hashed nodes, the trie has "
                      << trie->mphf_count() << " MPHFs" << std::endl;
            return 1;
        }

        for (uint64_t node : nodes) {
            const uint64_t degree = trie->children(node);
            uint64_t reference;
            uint64_t mismatches = check_node(trie, node, degree, reference);

            for (uint32_t r = 0; r < reps; r++) {
                uint64_t s_seq, s_cur, count;
                // ABBA order: over every 4 reps each form runs first twice and follows itself once.
                if (r % 4 == 0 || r % 4 == 3) {
                    auto a = clock_type::now();
                    s_seq = sum_seq(seq, node, degree);
                    auto b = clock_type::now();
                    s_cur = sum_cursor(trie, node, count);
                    auto c = clock_type::now();
                    seq_ns[r] = elapsed_ns(a, b);
                    cursor_ns[r] = elapsed_ns(b, c);
                } else {
                    auto a = clock_type::now();
                    s_cur = sum_cursor(trie, node, count);
                    auto b = clock_type::now();
                    s_seq = sum_seq(seq, node, degree);
                    auto c = clock_type::now();
                    cursor_ns[r] = elapsed_ns(a, b);
                    seq_ns[r] = elapsed_ns(b, c);
                }
                g_sink = g_sink + s_seq + s_cur;
                if (s_seq != reference || s_cur != reference || count != degree)
                    mismatches++;
            }

            NodeResult res;
            res.degree = degree;
            res.seq_min = *std::min_element(seq_ns.begin(), seq_ns.end()) / degree;
            res.seq_median = median(seq_ns) / degree;
            res.cursor_min = *std::min_element(cursor_ns.begin(), cursor_ns.end()) / degree;
            res.cursor_median = median(cursor_ns) / degree;
            per_trie[t].add(res);
            per_trie[t].mismatches += mismatches;
            if (mismatches > 0)
                std::cerr << "  MISMATCH " << trie_names[t] << " node=" << node << " count=" << mismatches
                          << std::endl;

            csv << trie_names[t] << ',' << node << ',' << degree << ',' << res.seq_min << ',' << res.seq_median
                << ',' << res.cursor_min << ',' << res.cursor_median << '\n';
        }
    }
    csv.flush();
    if (!csv) {
        std::cerr << "ERROR: writing the CSV failed" << std::endl;
        return 1;
    }

    Totals total;
    std::cerr << "\n=== SUMMARY (reps=" << reps << ", ns/key weighted by keys, min over reps (median)) ===" << std::endl;
    for (int t = 0; t < 6; t++) {
        print_totals(trie_names[t], per_trie[t]);
        total.add(per_trie[t]);
    }
    print_totals("TOTAL", total);
    std::cerr << "checksum sink: " << g_sink << std::endl;

    if (total.mismatches > 0) {
        std::cerr << "RESULT: MISMATCHES DETECTED" << std::endl;
        return 1;
    }
    std::cerr << "RESULT: m_seq and cursor agree on all " << total.nodes << " nodes and " << total.keys
              << " keys" << std::endl;
    return 0;
}

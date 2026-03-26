#include <index/cltj_index_metatrie.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <numeric>
#include "CLI11.hpp"

using namespace std;

const char* trie_names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};

void print_summary(int trie_id, const map<uint64_t, uint64_t>& hist) {
    uint64_t total_nodes = 0;
    uint64_t max_children = 0;
    uint64_t ge_100 = 0, ge_1000 = 0, ge_10000 = 0;

    for (auto& [deg, count] : hist) {
        total_nodes += count;
        if (deg > max_children)
            max_children = deg;
        if (deg >= 100)
            ge_100 += count;
        if (deg >= 1000)
            ge_1000 += count;
        if (deg >= 10000)
            ge_10000 += count;
    }

    cout << "Trie " << trie_id << " (" << trie_names[trie_id] << "): " << total_nodes << " nodes, "
         << "max_children=" << max_children << ", "
         << "nodes>=100: " << ge_100 << ", "
         << "nodes>=1000: " << ge_1000 << ", "
         << "nodes>=10000: " << ge_10000 << endl;
}

void write_csv(const string& path, const map<uint64_t, uint64_t>& hist) {
    ofstream out(path);
    out << "children,nodes\n";
    for (auto& [deg, count] : hist)
        out << deg << "," << count << "\n";
}

int main(int argc, char** argv) {
    CLI::App app{"Children-per-node histograms for each trie in an H-CLTJ index"};
    string index_file;
    string outdir;
    app.add_option("index", index_file, "Path to .hcltj index file")->required();
    app.add_option("--outdir", outdir, "Directory to save per-trie CSV files");
    CLI11_PARSE(app, argc, argv);

    cout << "Loading index from " << index_file << " ..." << endl;
    cltj::compact_ltj_metatrie_hash index;
    sdsl::load_from_file(index, index_file);
    cout << "Index loaded (" << sdsl::size_in_bytes(index) << " bytes)" << endl;
    cout << endl;

    if (!outdir.empty())
        system(("mkdir -p " + outdir).c_str());

    for (int t = 0; t < 6; t++) {
        auto* trie = index.get_trie(t);
        auto hist = trie->children_histogram();
        print_summary(t, hist);

        if (!outdir.empty()) {
            string csv_path = outdir + "/trie_" + to_string(t) + "_" + trie_names[t] + ".csv";
            write_csv(csv_path, hist);
            cout << "  -> " << csv_path << endl;
        }
    }

    return 0;
}

#include <index/cltj_index_metatrie.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include "CLI11.hpp"

using namespace std;

const char* trie_names[] = {"SPO", "SOP", "POS", "PSO", "OSP", "OPS"};

void write_csv(const string& path,
               const vector<cltj::compact_metatrie_hash::NodeInfo>& nodes) {
    ofstream out(path);
    out << "parent,depth,key,n_children,is_leaf\n";
    for (auto& n : nodes)
        out << n.parent << "," << n.depth << ","
            << n.key << "," << n.n_children << "," << (n.is_leaf ? 1 : 0) << "\n";
}

int main(int argc, char** argv) {
    CLI::App app{"Dump per-node trie structure from an H-CLTJ index"};
    string index_file;
    string outdir;
    app.add_option("index", index_file, "Path to .hcltj index file")->required();
    app.add_option("--outdir", outdir, "Directory to save per-trie CSV files")->required();
    CLI11_PARSE(app, argc, argv);

    cout << "Loading index from " << index_file << " ..." << endl;
    cltj::compact_ltj_metatrie_hash index;
    sdsl::load_from_file(index, index_file);
    cout << "Index loaded (" << sdsl::size_in_bytes(index) << " bytes)" << endl;

    system(("mkdir -p " + outdir).c_str());

    for (int t = 0; t < 6; t++) {
        auto* trie = index.get_trie(t);
        auto nodes = trie->dump_nodes();
        string csv_path = outdir + "/trie_" + to_string(t) + "_" + trie_names[t] + "_nodes.csv";
        write_csv(csv_path, nodes);
        cout << "Trie " << t << " (" << trie_names[t] << "): "
             << nodes.size() << " nodes -> " << csv_path << endl;
    }

    return 0;
}

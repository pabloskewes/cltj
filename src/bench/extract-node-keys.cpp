// Children of real trie nodes, from a dataset of "s p o" lines. A family is a
// trie and a level: pso = predicate -> subjects (level-1 nodes of PSO), pos =
// predicate -> objects (POS), osp = object -> subjects (OSP). For each family it
// writes the size of every node with at least --min children, and for each
// target size the node whose size is closest in log scale, as sorted uint32.

#include <CLI11.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Family {
    std::string name;
    int parent;  // 0 = s, 1 = p, 2 = o
    int child;
    std::vector<uint64_t> pairs;
};

void read_dataset(const std::string& path, std::vector<Family>& fams, uint32_t max_label[3], uint64_t& triples) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f)
        throw std::runtime_error("cannot open " + path);
    std::vector<char> buf(1 << 26);
    uint32_t t[3] = {0, 0, 0};
    uint64_t cur = 0;
    int field = 0;
    bool in_num = false;
    triples = 0;
    max_label[0] = max_label[1] = max_label[2] = 0;
    size_t got;
    auto emit = [&]() {
        ++triples;
        for (int k = 0; k < 3; ++k)
            max_label[k] = std::max(max_label[k], t[k]);
        for (auto& fam : fams)
            fam.pairs.push_back((uint64_t(t[fam.parent]) << 32) | t[fam.child]);
    };
    while ((got = std::fread(buf.data(), 1, buf.size(), f)) > 0) {
        for (size_t i = 0; i < got; ++i) {
            char c = buf[i];
            if (c >= '0' && c <= '9') {
                cur = cur * 10 + (c - '0');
                in_num = true;
            } else {
                if (in_num) {
                    if (field < 3)
                        t[field] = static_cast<uint32_t>(cur);
                    ++field;
                    cur = 0;
                    in_num = false;
                }
                if (c == '\n') {
                    if (field == 3)
                        emit();
                    field = 0;
                }
            }
        }
    }
    if (in_num && field < 3)
        t[field++] = static_cast<uint32_t>(cur);
    if (field == 3)
        emit();
    std::fclose(f);
}

}  // namespace

int main(int argc, char** argv) {
    CLI::App app{"Extract the children of real trie nodes of chosen sizes"};
    std::string dataset, outdir, families = "pso,pos,osp", targets = "4096,65536,1048576,10485760,0";
    uint64_t min_children = 4000;
    app.add_option("dataset", dataset, "Dataset with one \"s p o\" triple per line")->required();
    app.add_option("-o,--outdir", outdir, "Output directory")->required();
    app.add_option("--families", families, "Subset of pso,pos,osp");
    app.add_option("--targets", targets, "Target sizes, comma separated (0 = the largest node)");
    app.add_option("--min", min_children, "Smallest node size listed (the H threshold)");
    CLI11_PARSE(app, argc, argv);

    std::vector<Family> fams;
    std::stringstream fs(families);
    std::string tok;
    while (std::getline(fs, tok, ',')) {
        if (tok == "pso") fams.push_back({tok, 1, 0, {}});
        else if (tok == "pos") fams.push_back({tok, 1, 2, {}});
        else if (tok == "osp") fams.push_back({tok, 2, 0, {}});
        else throw std::runtime_error("unknown family " + tok);
    }
    std::vector<uint64_t> tgt;
    std::stringstream ts(targets);
    while (std::getline(ts, tok, ','))
        tgt.push_back(std::stoull(tok));

    uint32_t max_label[3];
    uint64_t triples = 0;
    read_dataset(dataset, fams, max_label, triples);
    std::cerr << "triples=" << triples << " max_s=" << max_label[0] << " max_p=" << max_label[1]
              << " max_o=" << max_label[2] << std::endl;
    {
        std::ofstream meta(outdir + "/dataset.csv");
        meta << "triples,max_s,max_p,max_o\n"
             << triples << ',' << max_label[0] << ',' << max_label[1] << ',' << max_label[2] << '\n';
    }

    std::ofstream nodes(outdir + "/nodes.csv");
    nodes << "family,parent,n,first,last,universe,target,file\n";
    for (auto& fam : fams) {
        std::sort(fam.pairs.begin(), fam.pairs.end());
        fam.pairs.erase(std::unique(fam.pairs.begin(), fam.pairs.end()), fam.pairs.end());
        std::cerr << fam.name << ": " << fam.pairs.size() << " distinct pairs" << std::endl;

        struct Run { uint32_t parent; uint64_t beg, n; };
        std::vector<Run> runs;
        for (uint64_t i = 0; i < fam.pairs.size();) {
            uint64_t j = i;
            uint32_t par = fam.pairs[i] >> 32;
            while (j < fam.pairs.size() && (fam.pairs[j] >> 32) == par)
                ++j;
            if (j - i >= min_children)
                runs.push_back({par, i, j - i});
            i = j;
        }
        std::ofstream sizes(outdir + "/sizes-" + fam.name + ".csv");
        sizes << "parent,n\n";
        for (auto& r : runs)
            sizes << r.parent << ',' << r.n << '\n';

        for (uint64_t t : tgt) {
            const Run* best = nullptr;
            double best_d = 1e300;
            for (auto& r : runs) {
                double d = t == 0 ? -double(r.n) : std::fabs(std::log(double(r.n)) - std::log(double(t)));
                if (d < best_d) {
                    best_d = d;
                    best = &r;
                }
            }
            if (!best)
                continue;
            std::string file = fam.name + "-" + std::to_string(best->parent) + "-n" + std::to_string(best->n) + ".u32";
            std::vector<uint32_t> keys(best->n);
            for (uint64_t k = 0; k < best->n; ++k)
                keys[k] = static_cast<uint32_t>(fam.pairs[best->beg + k]);
            std::ofstream out(outdir + "/" + file, std::ios::binary);
            out.write(reinterpret_cast<const char*>(keys.data()), keys.size() * sizeof(uint32_t));
            nodes << fam.name << ',' << best->parent << ',' << best->n << ',' << keys.front() << ',' << keys.back()
                  << ',' << (uint64_t(keys.back()) - keys.front() + 1) << ',' << t << ',' << file << '\n';
            nodes.flush();
        }
        std::vector<uint64_t>().swap(fam.pairs);
    }
    return 0;
}

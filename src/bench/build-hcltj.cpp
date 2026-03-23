#include <CLI11.hpp>
#include <index/cltj_index_metatrie.hpp>
#include <iostream>

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

int main(int argc, char** argv) {
    CLI::App app{"Build H-CLTJ index (metatrie + MPHF hash overlay)"};
    std::string dataset;
    uint32_t threshold = 1000;
    app.add_option("dataset", dataset, "Input dataset file")->required();
    app.add_option("-t,--threshold", threshold, "Min children to hash a node (default: 1000)");
    CLI11_PARSE(app, argc, argv);

    try {
        std::string index_name = dataset + ".hcltj";
        std::vector<cltj::spo_triple> D;

        std::ifstream ifs(dataset);
        uint32_t s, p, o;
        cltj::spo_triple spo;
        do {
            ifs >> s >> p >> o;
            if (ifs.fail())
                break;
            spo[0] = s;
            spo[1] = p;
            spo[2] = o;
            D.emplace_back(spo);
        } while (!ifs.eof());

        std::cout << "D.size()=" << D.size() << std::endl;
        D.shrink_to_fit();
        std::cout << "Dataset: " << 3 * D.size() * sizeof(uint32_t) << " bytes." << std::endl;

        auto start = timer::now();
        cltj::compact_ltj_metatrie_hash index(D);
        auto stop = timer::now();
        std::cout << "Trie build: " << duration_cast<seconds>(stop - start).count() << "s" << std::endl;

        // Free D before overlay build to reduce peak memory
        std::vector<cltj::spo_triple>().swap(D);

        std::cout << "Building hash overlay (threshold=" << threshold << ")..." << std::endl;
        start = timer::now();
        uint32_t total_mphfs = 0;
        for (int i = 0; i < 6; i++) {
            index.get_trie(i)->build_hash_overlay(threshold);
            uint32_t n = static_cast<uint32_t>(index.get_trie(i)->mphf_count());
            std::cout << "  trie " << i << ": " << n << " MPHFs" << std::endl;
            total_mphfs += n;
        }
        stop = timer::now();
        std::cout << "Hash overlay: " << total_mphfs << " MPHFs total, "
                  << duration_cast<seconds>(stop - start).count() << "s" << std::endl;

        sdsl::store_to_file(index, index_name);
        std::cout << "Index saved to " << index_name << std::endl;
        std::cout << sdsl::memory_monitor::peak() << " bytes peak." << std::endl;

    } catch (const char* msg) {
        std::cerr << msg << std::endl;
    }
    return 0;
}
// Builds X-CLTJ with the children of the three roots in a root bitmap (cltj_index_metatrie_rb).
// Usage: build-xcltj-rb <dataset> [-o <output>]   (default output: <dataset>.xcltj-rb)
#include <CLI11.hpp>
#include <index/cltj_index_metatrie_rb.hpp>

#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    CLI::App app{"Build an X-CLTJ index with a root bitmap"};
    std::string dataset, index_name;
    app.add_option("dataset", dataset, "Triples file, one `s p o` per line")->required();
    app.add_option("-o,--output", index_name, "Output index (default: <dataset>.xcltj-rb)");
    CLI11_PARSE(app, argc, argv);
    if (index_name.empty())
        index_name = dataset + ".xcltj-rb";

    try {
        std::vector<cltj::spo_triple> D;
        std::ifstream ifs(dataset);
        uint32_t s, p, o;
        cltj::spo_triple spo;
        while (ifs >> s >> p >> o) {
            spo[0] = s;
            spo[1] = p;
            spo[2] = o;
            D.emplace_back(spo);
        }
        std::cout << "D.size()=" << D.size() << std::endl;
        D.shrink_to_fit();

        auto start = std::chrono::high_resolution_clock::now();
        cltj::compact_ltj_metatrie_rb index(D);
        auto stop = std::chrono::high_resolution_clock::now();

        if (!sdsl::store_to_file(index, index_name)) {
            std::cerr << "Failed to write index to " << index_name << std::endl;
            return 1;
        }
        std::cout << "Index saved: " << sdsl::size_in_bytes(index) << " bytes." << std::endl;
        std::cout << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << " seconds."
                  << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

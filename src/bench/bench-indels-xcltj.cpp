//
// Created by adrian on 13/1/25.
//

#include <iostream>
#include <index/cltj_index_metatrie_dyn.hpp>
#include <util/rdf_util.hpp>


std::vector<cltj::spo_triple> get_triples(const std::string &updates) {

    std::ifstream ifn(updates);
    std::string line;
    cltj::spo_triple triple;
    std::vector<cltj::spo_triple> triples;
    while (std::getline(ifn, line)) {
        triple = util::rdf::ids::get_triple(line);
        triples.push_back(triple);
    }
    ifn.close();
    return triples;

}

void remove(const std::string &index, const std::vector<cltj::spo_triple> &triples) {
    std::cout << "Removing triples" << std::endl;
    cltj::compact_ltj_metatrie_dyn cltj;
    sdsl::load_from_file(cltj, index);
    auto start = std::chrono::high_resolution_clock::now();
    for(const auto &triple : triples){
        cltj.remove(triple);
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Removal time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns" << std::endl;
}

void insert(const std::string &index, const std::vector<cltj::spo_triple> &triples) {
    std::cout << "Inserting triples" << std::endl;
    cltj::compact_ltj_metatrie_dyn cltj;
    sdsl::load_from_file(cltj, index);
    auto start = std::chrono::high_resolution_clock::now();
    for(const auto &triple : triples){
        cltj.insert(triple);
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Insertion time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns" << std::endl;

    sdsl::store_to_file(cltj, index + ".insertions");
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <index> <updates>" << std::endl;
        return 0;
    }

    std::string index = argv[1];
    std::string updates = argv[2];

    std::vector<cltj::spo_triple> triples = get_triples(updates);
    insert(index, triples);
    remove(index + ".insertions", triples);

}
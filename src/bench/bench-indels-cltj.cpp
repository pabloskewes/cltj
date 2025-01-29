//
// Created by adrian on 13/1/25.
//

#include <iostream>
#include <index/cltj_index_spo_dyn.hpp>
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

void remove(cltj::compact_dyn_ltj &cltj, const std::vector<cltj::spo_triple> &triples) {
    std::cout << "Removing triples" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    for(const auto &triple : triples){
        cltj.remove(triple);
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Removal time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns" << std::endl;
}

void insert(cltj::compact_dyn_ltj &cltj, const std::vector<cltj::spo_triple> &triples) {
    std::cout << "Inserting triples" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    for(const auto &triple : triples){
        cltj.insert(triple);
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Insertion time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns" << std::endl;
}

int main(int argc, char *argv[]) {

    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <index> <insertions> <updates>" << std::endl;
        return 0;
    }

    std::string index = argv[1];
    std::string insertions = argv[2];
    std::string updates = argv[3];

    std::vector<cltj::spo_triple> triples = get_triples(insertions);
    cltj::compact_dyn_ltj cltj;
    sdsl::load_from_file(cltj, index);
    insert(cltj, triples);
    triples = get_triples(updates);
    insert(cltj, triples);
    remove(cltj, triples);

}
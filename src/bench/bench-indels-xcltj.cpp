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

std::vector<cltj::spo_triple> get_triples(const std::string &updates, const uint64_t ntriples) {

    std::ifstream ifn(updates);
    std::string line;
    cltj::spo_triple triple;
    std::vector<cltj::spo_triple> triples;
    while (std::getline(ifn, line) && triples.size() < ntriples) {
        triple = util::rdf::ids::get_triple(line);
        triples.push_back(triple);
    }
    ifn.close();
    return triples;

}

uint64_t remove(cltj::compact_ltj_metatrie_dyn &cltj, const std::vector<cltj::spo_triple> &triples) {
    std::cout << "Removing triples" << std::endl;
    uint64_t i = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for(const auto &triple : triples){
        i += cltj.remove(triple);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Removal time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns" << std::endl;
    return i;
}

uint64_t insert(cltj::compact_ltj_metatrie_dyn &cltj, const std::vector<cltj::spo_triple> &triples) {
    std::cout << "Inserting triples" << std::endl;
    uint64_t i = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for(const auto &triple : triples){
        i += cltj.insert(triple);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Insertion time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns" << std::endl;
    return i;
}

void insert_out(cltj::compact_ltj_metatrie_dyn &cltj, const std::vector<cltj::spo_triple> &triples) {
    std::cout << "Inserting triples" << std::endl;
    uint64_t i = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for(const auto &triple : triples){
        cltj.insert(triple);
        ++i;
        if(i % 100000 == 0){
            std::cout << "Inserted " << i << " triples" << std::endl;
            auto end = std::chrono::high_resolution_clock::now();
            std::cout << "Insertion time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()/(i+1) << " ns/query" << std::endl;

        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Insertion time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns" << std::endl;

}

int main(int argc, char *argv[]) {

    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <index> <updates> <split>" << std::endl;
        return 0;
    }

    std::string index = argv[1];
    std::string insertions = argv[2];
    bool split = std::stoi(argv[3]);

    std::vector<cltj::spo_triple> triples = get_triples(insertions);
    std::cout << "Triples: " << triples.size() << std::endl;
    cltj::compact_ltj_metatrie_dyn cltj;
    sdsl::load_from_file(cltj, index);
    if(split) cltj.split();
    cltj.check_leaves();
    auto r = remove(cltj, triples);
    std::cout << "removed: " << r << std::endl;
    cltj.flatten();
    if(split) cltj.split();
    auto i = insert(cltj, triples);
    std::cout << "inserted: " << i << std::endl;

}
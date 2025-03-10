//
// Created by adrian on 10/3/25.
//

#include <iostream>
#include <utility>
#include <chrono>
#include <triple_pattern.hpp>
#include <query/ltj_algorithm.hpp>
#include <util/time_util.hpp>
#include <util/file_util.hpp>
#include <index/cltj_index_spo_lite.hpp>

int main(int argc, char *argv[]) {
    //typedef ring::c_ring ring_type;
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <index> <spo-file>" << std::endl;
        return 0;
    }

    std::string index = argv[1];
    std::string spo_file = argv[2];
    cltj::compact_ltj graph;
    sdsl::load_from_file(graph, index);

   const sdsl::bit_vector &bv = graph.tries[0].bv;
    uint64_t m = 0;
    for(uint64_t i = 0; i < bv.size(); ++i){
        if(bv[i]){
            m++;
        }
    }
    std::ofstream ofs(spo_file);
    sdsl::write_member(bv.size(), ofs);
    sdsl::write_member(m, ofs);
    for(uint64_t i = 0; i < bv.size(); ++i){
        if(bv[i]){
            sdsl::write_member(i, ofs);
        }
    }
    ofs.close();
}
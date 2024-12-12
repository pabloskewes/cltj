#include <cltj_parser.hpp>
//
// Created by adrian on 12/12/24.
//
#include <iostream>
#include <cstdint>
#include <dict_map.hpp>
#include <ltj_algorithm.hpp>
#include <index/cltj_index_spo_dyn.hpp>

int main(int argc, char *argv[]) {
    //typedef ring::c_ring ring_type;
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <dataset>" << std::endl;
        return 0;
    }

    std::string dataset      = argv[1];
    std::string index_name   = dataset + ".cltj.cds";
    std::string dict_so_name = dataset + ".cltj.so";
    std::string dict_p_name  = dataset +  ".cltj.p";


    std::cout << "Index: " << index_name << std::endl;
    std::cout << "mapSO: " << dict_so_name << std::endl;
    std::cout << "mapP : " << dict_p_name << std::endl;
    cltj::compact_dyn_ltj graph;
    dict::basic_map dict_so;
    dict::basic_map dict_p;
    sdsl::load_from_file(graph, index_name);
    sdsl::load_from_file(dict_so, dict_so_name);
    sdsl::load_from_file(dict_p, dict_p_name);

    std::string line;
    while(true) {
        std::getline(std::cin, line);
        if(line == "q") break;
        auto ut = cltj::parser::get_triple(line);
        cltj::spo_triple triple;
        triple[0] = dict_so.locate(ut[0]);
        triple[1] = dict_p.locate(ut[1]);
        triple[2] = dict_so.locate(ut[2]);
        auto r = graph.remove_and_report(triple);
        std::cout << "removed: " << r.removed << " dict: [" << r.to_remove[0] << ", " << r.to_remove[1] << ", " << r.to_remove[2] << "]" << std::endl;
        if(r.to_remove[0]) {
            dict_so.eliminate(triple[0]);
        }
        if(r.to_remove[1]) {
            dict_p.eliminate(triple[1]);
        }
        if(r.to_remove[2]) {
            dict_so.eliminate(triple[2]);
        }
    }

    return 0;
}

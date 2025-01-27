//
// Created by adrian on 10/12/24.
//

#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <cltj_config.hpp>
#include <random>
#include <algorithm>
#include <triple_pattern.hpp>
#include <index/cltj_index_spo_lite.hpp>
#include <query/ltj_algorithm.hpp>
#include <query/ltj_iterator_lite.hpp>
#include <util/rdf_util.hpp>

int main(int argc, char **argv){
    if(argc != 3){
        std::cout<< argv[0] << " <dataset> <index>" << std::endl;
        return 0;
    }

    std::string dataset = argv[1];
    std::string index = argv[2];

    cltj::compact_ltj cltj;
    sdsl::load_from_file(cltj, index);



    std::ifstream ifs(dataset);
    std::string f_data = dataset + ".20";
    std::ofstream of_dataset(f_data);

    typedef ltj::ltj_iterator_lite<cltj::compact_ltj , uint8_t, uint64_t> iterator_type;
    typedef ltj::ltj_algorithm<iterator_type,
        ltj::veo::veo_simple<iterator_type, ltj::util::trait_size> > algorithm_type;
    typedef ::util::results_collector<typename algorithm_type::tuple_type> results_type;

    uint64_t k = 0;
    cltj::spo_triple spo;
    std::string query_string;
    do {
        std::getline(ifs, query_string);
        if(query_string.empty()) break;
        std::vector<ltj::triple_pattern> query = ::util::rdf::ids::get_query(query_string);
        results_type res;
        algorithm_type ltj(&query, &cltj);
        ltj.join(res, 1, 600);
        if(res.size() == 0) {
            of_dataset << query_string << std::endl;
        }
        ++k;

    } while (!ifs.eof());

    of_dataset.close();


}

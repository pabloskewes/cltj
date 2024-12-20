//
// Created by adrian on 13/12/24.
//

#include <api/cltj_dyn_rdf.hpp>

int main(int argc, char **argv) {
    typedef cltj::cltj_dyn_rdf::tuple_type tuple_type;
    {
        cltj::cltj_dyn_rdf  cltj(argv[1]);
        std::string query = "?x <http://www.wikidata.org/prop/direct/P8> ?y . ?z <http://www.wikidata.org/prop/direct/P13> ?y";
        ::util::results_collector<tuple_type> res;
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << a << " ";
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;
        res.clear();
        cltj.insert("<http://www.wikidata.org/entity/Q5> <http://www.wikidata.org/prop/direct/P13> <http://www.wikidata.org/entity/Q6>");
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << a << " ";
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;
        res.clear();
        cltj.remove("<http://www.wikidata.org/entity/Q5> <http://www.wikidata.org/prop/direct/P13> <http://www.wikidata.org/entity/Q6>");
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << a << " ";
            }
            std::cout << std::endl;
        }

        sdsl::store_to_file(cltj, "cltj-star.bin");
    }
    std::cout << std::endl;
    {
        cltj::cltj_dyn_rdf  cltj;
        std::cout << "Reading from file" << std::endl;
        sdsl::load_from_file(cltj, "cltj-star.bin");
        std::string query = "?x <http://www.wikidata.org/prop/direct/P8> ?y . ?z <http://www.wikidata.org/prop/direct/P13> ?y";
        ::util::results_collector<tuple_type> res;
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << a << " ";
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;
        res.clear();
        cltj.insert("<http://www.wikidata.org/entity/Q5> <http://www.wikidata.org/prop/direct/P13> <http://www.wikidata.org/entity/Q6>");
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << a << " ";
            }
            std::cout << std::endl;
        }

        std::cout << std::endl;
        res.clear();
        cltj.remove("<http://www.wikidata.org/entity/Q5> <http://www.wikidata.org/prop/direct/P13> <http://www.wikidata.org/entity/Q6>");
        cltj.query(query, res);
        for(uint64_t i = 0; i < res.size(); ++i) {
            for(auto &a : res[i]) {
                std::cout << a << " ";
            }
            std::cout << std::endl;
        }
    }


}
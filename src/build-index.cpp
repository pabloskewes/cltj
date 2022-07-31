#include <iostream>
#include "ring.hpp"
#include <fstream>
#include <sdsl/construct.hpp>
#include <ltj_algorithm.hpp>

using namespace std;

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

template<class ring>
void build_index(const std::string &dataset, const std::string &output){
    vector<spo_triple> D, E;

    std::ifstream ifs(dataset);
    uint64_t s, p , o;
    do {
        ifs >> s >> p >> o;
        D.push_back(spo_triple(s, p, o));
    } while (!ifs.eof());

    D.shrink_to_fit();
    cout << "--Indexing " << D.size() << " triples" << endl;
    memory_monitor::start();
    auto start = timer::now();

    ring A(D);
    auto stop = timer::now();
    memory_monitor::stop();
    cout << "  Index built  " << sdsl::size_in_bytes(A) << " bytes" << endl;

    sdsl::store_to_file(A, output);
    cout << "Index saved" << endl;
    cout << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
    cout << memory_monitor::peak() << " bytes." << endl;

}

int main(int argc, char **argv)
{

    if(argc != 3){
        std::cout << "Usage: " << argv[0] << "<dataset> [ring|c-ring]" << std::endl;
        return 0;
    }

    std::string dataset = argv[1];
    std::string type    = argv[2];
    if(type == "ring"){
        std::string index_name = dataset + ".ring";
        build_index<ring::ring<>>(dataset, index_name);
    }else if (type == "c-ring"){
        std::string index_name = dataset + ".c-ring";
        build_index<ring::c_ring>(dataset, index_name);
    }else{
        std::cout << "Usage: " << argv[0] << "<dataset> [ring|c-ring]" << std::endl;
    }

    return 0;
}


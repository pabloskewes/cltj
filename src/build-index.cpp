#include <iostream>
#include "ring.hpp"
#include <fstream>
#include <sdsl/construct.hpp>
#include <ltj_algorithm.hpp>

using namespace std;

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;


int main(int argc, char **argv)
{

    typedef ring::ring<> ring_type;
    //typedef ring::c_ring ring_type;

    vector<spo_triple> D, E;

    std::ifstream ifs(argv[1]);
    uint64_t s, p , o;
    do {
        ifs >> s >> p >> o;
        D.push_back(spo_triple(s, p, o));
    } while (!ifs.eof());

    D.shrink_to_fit();
    cout << "--Indexing " << D.size() << " triples" << endl;
    memory_monitor::start();
    auto start = timer::now();

    ring_type A(D);
    auto stop = timer::now();
    memory_monitor::stop();
    cout << "  Index built  " << sdsl::size_in_bytes(A) << " bytes" << endl;

    sdsl::store_to_file(A, string(argv[1]));
    cout << "Index saved" << endl;
    cout << duration_cast<seconds>(stop-start).count() << " seconds." << endl;
    cout << memory_monitor::peak() << " bytes." << endl;
    return 0;
}


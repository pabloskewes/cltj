#include <iostream>
#include <cltj_index_spo_lite.hpp>

using namespace std;

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

int main(int argc, char **argv) {
    if (argc != 2) {
        cout << argv[0] << " <dataset>" << endl;
        return 0;
    }

    std::string dataset = argv[1];
    std::string index_name = dataset + ".cltj";
    uint64_t queries = 100000;
    uint64_t steps = 20;

    vector<cltj::spo_triple> D;

    std::ifstream ifs(dataset);
    uint32_t s, p, o;
    cltj::spo_triple spo;
    do {
        ifs >> s >> p >> o;
        spo[0] = s;
        spo[1] = p;
        spo[2] = o;
        D.emplace_back(spo);
    } while (!ifs.eof());

    D.shrink_to_fit();
    std::cout << "Dataset: " << 3 * D.size() * sizeof(::uint32_t) << " bytes." << std::endl;
    //sdsl::memory_monitor::start();


    auto new_size = D.size() / 100 * 20;
    D.resize(new_size);
    cltj::compact_ltj index(D);
    std::cout << "Index: " << sdsl::size_in_bytes(index) << " bytes." << std::endl;
    return 0;
}

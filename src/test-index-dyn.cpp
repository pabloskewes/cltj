#include <iostream>
#include <cltj_index_spo_dyn.hpp>

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
    uint64_t queries = 10000;

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

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(D.begin(), D.end(), g);

    cltj::cltj_index_spo_dyn<cltj::compact_trie_dyn<>> index;

    uint64_t block_size = D.size() / 10;
    //sdsl::memory_monitor::stop();
    for (uint64_t beg = 0; beg < D.size(); beg += block_size) {
        std::cout << "Insert [" << beg << ", " << beg + block_size - 1 << "]" << std::endl;
        for (uint64_t i = beg; i < beg + block_size; ++i) {
            index.insert(D[i]);
        }
        std::cout << "Check Insert [" << beg << ", " << beg + block_size - 1 << "]" << std::endl;
        for (uint64_t i = 0; i < queries; ++i) {
            auto q = rand() % block_size + beg;
            auto r = index.test_exists(D[q]);
            if (r < 6) {
                std::cout << "Error looking for (" << D[q][0] << ", " << D[q][1] << ", " << D[q][2] << ") appears in "
                        << r << "tries." << std::endl;
                exit(0);
            }
        }
        std::cout << "Remove [" << beg << ", " << beg + block_size - 1 << "]" << std::endl;
        for (uint64_t i = beg; i < beg + block_size; ++i) {
            index.remove(D[i]);
        }
        std::cout << "Check Remove [" << beg << ", " << beg + block_size - 1 << "]" << std::endl;
        for (uint64_t i = 0; i < queries; ++i) {
            auto q = rand() % block_size + beg;
            auto r = index.test_exists(D[q]);
            if (r > 0) {
                std::cout << "Error looking for (" << D[q][0] << ", " << D[q][1] << ", " << D[q][2] << ") appears in "
                        << r << "tries." << std::endl;
                exit(0);
            }
        }
        for (uint64_t i = beg; i < beg+block_size; ++i) {
            index.insert(D[i]);
        }
    }

    std::cout << "Size: " << sdsl::size_in_bytes(index) << std::endl;
    sdsl::store_to_file(index, index_name);
    return 0;
}

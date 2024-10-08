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

    uint64_t block_size = D.size() / 100;
    //sdsl::memory_monitor::stop();
    uint64_t beg = 0;
    for (uint64_t k = 0; k < 20; ++k ) {
        uint64_t last = std::min(beg+block_size, D.size()-1);
        std::cout << "=======================================" << std::endl;
        std::cout << "Working with range [" << 0 << ", " << last - 1 << "]" << std::endl;
        std::cout << "\r insertions: 0% (0/" << last-beg << ")" << std::flush;
        auto start = timer::now();
        for (uint64_t i = beg; i < last; ++i) {
            index.insert(D[i]);
            if((i-beg) % ((last-beg)/1000) == 0) {
                float per = ((i-beg) / (float) (last-beg+1)) * 100;
                std::cout << "\r insertions: " << per <<  "% (" << (i-beg) << "/" << last-beg << ")" << std::flush;
            }
        }
        auto stop = timer::now();
        auto ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << " insertions: 100% (" << last-beg << "/" << last-beg << ") takes" << ms_time << " ms" << std::endl;

        std::cout << "\r check_insertions: 0% (0/" << last-beg << ")" << std::flush;
        start = timer::now();
        for (uint64_t i = 0; i < queries; ++i) {
            auto q = std::min(rand() % block_size + beg, D.size());
            auto r = index.test_exists(D[q]);
            if (r < 6) {
                std::cout << "Error looking for (" << D[q][0] << ", " << D[q][1] << ", " << D[q][2] << ") appears in "
                        << r << "tries." << std::endl;
                exit(0);
            }
            if((i-beg) % 1000 == 0) {
                float per = ((i-beg) / (float) (last-beg+1)) * 100;
                std::cout << "\r check_insertions:: " << per <<  "% (" << (i-beg) << "/" << last-beg << ")" << std::flush;
            }
        }
        stop = timer::now();
        ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "check_insertions: 100% (" << last-beg << "/" << last-beg << ") takes" << ms_time << " ms" << std::endl;

        std::cout << "\r remove: 0% (0/" << last-beg << ")" << std::flush;
        start = timer::now();
        for (uint64_t i = beg; i < last; ++i) {
            index.remove(D[i]);
            if((i-beg) % 1000 == 0) {
                float per = ((i-beg) / (float) (last-beg+1)) * 100;
                std::cout << "\r remove: " << per <<  "% (" << (i-beg) << "/" << last-beg << ")\r" << std::flush;
            }
        }
        stop = timer::now();
        ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "remove: 100% (" << last-beg << "/" << last-beg << ") takes" << ms_time << " ms" << std::endl;

        std::cout << "\r check_remove: 0% (0/" << last-beg << ")" << std::flush;
        start = timer::now();
        for (uint64_t i = 0; i < queries; ++i) {
            auto q = std::min(rand() % block_size + beg, D.size()-1);
            auto r = index.test_exists(D[q]);
            if (r > 0) {
                std::cout << "Error looking for (" << D[q][0] << ", " << D[q][1] << ", " << D[q][2] << ") appears in "
                        << r << "tries." << std::endl;
                exit(0);
            }
            if((i-beg) % 1000 == 0) {
                float per = ((i-beg) / (float) (last-beg+1)) * 100;
                std::cout << "\r check_remove: " << per <<  "% (" << (i-beg) << "/" << last-beg << ")" << std::flush;
            }
        }
        stop = timer::now();
        ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "check_remove: 100% (" << last-beg << "/" << last-beg << ") ms=" << ms_time << std::endl;

        std::cout << "\r insertions: 0% (0/" << last-beg << ")\r" << std::flush;
        start = timer::now();
        for (uint64_t i = beg; i < last; ++i) {
            index.insert(D[i]);
            if((i-beg) % 1000 == 0) {
                float per = ((i-beg) / (float) (last-beg+1)) * 100;
                std::cout << "\r insertions: " << per <<  "% (" << (i-beg) << "/" << last-beg << ")" << std::flush;
            }
        }
        stop = timer::now();
        ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "insertions: 100% (" << last-beg << "/" << last-beg << ") takes" << ms_time << " ms" << std::endl;
        std::cout << "size: " << sdsl::size_in_bytes(index) << " bytes" << std::endl;
        std::cout << "=======================================" << std::endl << std::endl;
        beg += block_size;
    }

    sdsl::store_to_file(index, index_name);
    return 0;
}

#include <iostream>
#include <cltj_index_spo_dyn.hpp>

#include "hybridBV/hybridBVId.h"

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

    std::random_device rd;
    std::mt19937 g(rd());
    //std::shuffle(D.begin(), D.end(), g);

    cltj::cltj_index_spo_dyn<cltj::compact_trie_dyn_v2<>> index;


    uint64_t block_size = D.size() / 100;
    //uint64_t block_size = 20;
    //sdsl::memory_monitor::stop();
    uint64_t beg = 0;
    for (uint64_t k = 0; k < steps; ++k ) {
        uint64_t last = std::min(beg+block_size, D.size()-1);
        std::cout << "=======================================" << std::endl;
        std::cout << "Working with range [" << 0 << ", " << last - 1 << "]" << std::endl;
        std::cout << "\r insertions: 0% (0/" << last-beg << ")" << std::flush;
        auto start = timer::now();
        for (uint64_t i = beg; i < last; ++i) {
            index.insert(D[i]);
           // index.print();
           /* for(uint64_t ii = 0; ii <= i; ++ii) {
                auto r = index.test_exists(D[ii]);
                if (r < 6) {
                    std::cout << "====================" << std::endl;
                    index.print();
                    std::cout << "i=" << i << std::endl;
                    std::cout << "Error looking for D[" << ii << "]=(" << D[ii][0] << ", " << D[ii][1] << ", " << D[ii][2] << ") appears in "
                            << r << " tries." << std::endl;
                    index.test_exists_error(D[ii]);
                    exit(0);
                }
            }*/
            if((i-beg) % ((last-beg)/1000) == 0) {
                float per = ((i-beg) / (float) (last-beg+1)) * 100;
                std::cout << "\r insertions: " << per <<  "% (" << (i-beg) << "/" << last-beg << ")" << std::flush;
            }
        }
        auto stop = timer::now();
        auto ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "\r insertions: 100% (" << last-beg << "/" << last-beg << ") takes " << ms_time << " ms [" << index.n_triples << "]" << std::endl;

        /*std::cout << "\r check_insertions: 0% (0/" << queries << ")" << std::flush;
        start = timer::now();
        for (uint64_t i = 0; i < queries; ++i) {
            auto q = std::min(rand() % block_size + beg, D.size());
            auto r = index.test_exists(D[q]);
            if (r < 6) {
                std::cout << "Error looking for D[" << q << "]=(" << D[q][0] << ", " << D[q][1] << ", " << D[q][2] << ") appears in "
                        << r << " tries." << std::endl;
                exit(0);
            }
            if(i % 1000 == 0) {
                float per = (i / (float) queries) * 100;
                std::cout << "\r check_insertions: " << per <<  "% (" << i << "/" << queries << ")" << std::flush;
            }
        }
        stop = timer::now();
        ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "\r check_insertions: 100% (" << queries << "/" << queries << ") takes " << ms_time << " ms in " << queries << " queries" << std::endl;*/

        std::cout << "\r check_insertions: 0% (0/" << last-beg << ")" << std::flush;
        start = timer::now();
        for (uint64_t i = beg; i < last; ++i) {
            auto r = index.test_exists(D[i]);
            if (r < 6) {
                index.print();
                std::cout << "Error looking for D[" << i << "]=(" << D[i][0] << ", " << D[i][1] << ", " << D[i][2] << ") appears in "
                        << r << " tries." << std::endl;
                index.test_exists_error(D[i]);
                exit(0);
            }
            if(i % 1000 == 0) {
                float per = ((i-beg) / (float) (last-beg)) * 100;
                std::cout << "\r check_insertions: " << per <<  "% (" << (i-beg) << "/" << (last-beg) << ")" << std::flush;
            }
        }
        stop = timer::now();
        ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "\r check_insertions: 100% (" << last-beg << "/" << last-beg << ") takes " << ms_time << " ms in " << last-beg << " queries" << std::endl;

        std::cout << "\r remove: 0% (0/" << last-beg << ")" << std::flush;
        start = timer::now();
        for (uint64_t i = beg; i < last; ++i) {
            /*std::cout << "i=" << i << std::endl;
            if(i == 112) {
                std::cout << "aa " << std::endl;
            }*/
            if(i == 173076) {
                std::cout << "aaa" << std::endl;
            }
           /*auto a = index.test_exists(D[i]);
           if (a < 6) {
               std::cout << "====================" << std::endl;
               index.print();
               std::cout << "Error looking for D[" << i << "]=(" << D[i][0] << ", " << D[i][1] << ", " << D[i][2] << ") appears in "
                       << a << " tries." << std::endl;
               index.test_exists_error(D[i]);
               exit(0);
           }*/
            //std::cout <<"i=" << i << std::endl;
            index.remove(D[i]);
            bool ok = index.check_last();
            std::cout << "check : " << ok << " at i=" << i << std::endl;
            if(!ok) {
                index.check_last_print();
                exit(0);
            }
           // std::cout << "check: " << ok << std::endl;

            //if(i ==2) index.print();
            /*for(uint64_t ii = 0; ii <= i; ++ii) {
               auto r = index.test_exists(D[ii]);
                ok = index.check();
                std::cout << "check after test: " << ok << std::endl;
                if(!ok) {
                    index.check_print();
                    exit(0);
                }
               if (r > 0) {
                   std::cout << "====================" << std::endl;
                   index.print();
                   std::cout << "Error looking for D[" << ii << "]=(" << D[ii][0] << ", " << D[ii][1] << ", " << D[ii][2] << ") appears in "
                           << r << " tries." << std::endl;
                   index.test_exists_error(D[ii]);
                   exit(0);
               }
           }*/

            if((i-beg) % 1000 == 0) {
                float per = ((i-beg) / (float) (last-beg)) * 100;
                std::cout << "\r remove: " << per <<  "% (" << (i-beg) << "/" << last-beg << ")" << std::flush;
            }
        }
        stop = timer::now();
        ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "\r remove: 100% (" << last-beg << "/" << last-beg << ") takes " << ms_time << " ms [" << index.n_triples << "]" << std::endl;
        auto ok = index.check();
        if(!ok) {
            index.check_print();
            exit(0);
        }
        /*std::cout << "\r check_remove: 0% (0/" << queries << ")" << std::flush;
        start = timer::now();
        for (uint64_t i = 0; i < queries; ++i) {
            auto q = std::min(rand() % block_size + beg, D.size()-1);
            auto r = index.test_exists(D[q]);
            if (r > 0) {
                std::cout << "Error looking for D[" << q << "]=(" << D[q][0] << ", " << D[q][1] << ", " << D[q][2] << ") appears in "
                        << r << " tries." << std::endl;
                exit(0);
            }
            if((i-beg) % 1000 == 0) {
                float per = (i / (float) queries) * 100;
                std::cout << "\r check_remove: " << per <<  "% (" << i << "/" << queries << ")" << std::flush;
            }
        }
        stop = timer::now();
        ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "\r check_remove: 100% (" << queries << "/" << queries << ") takes " << ms_time << " ms in " << queries << " queries" << std::endl;*/

        std::cout << "\r check_remove: 0% (0/" << queries << ")" << std::flush;
        start = timer::now();
        for (uint64_t i = beg; i < last ; ++i) {
            auto r = index.test_exists(D[i]);
            if (r > 0) {
                std::cout << "Error looking for D[" << i << "]=(" << D[i][0] << ", " << D[i][1] << ", " << D[i][2] << ") appears in "
                        << r << " tries." << std::endl;
                index.test_exists_error(D[i]);
                exit(0);
            }
            if((i-beg) % 1000 == 0) {
                float per = ((i-beg) / (float) (last-beg)) * 100;
                std::cout << "\r check_remove: " << per <<  "% (" << i << "/" << last-beg << ")" << std::flush;
            }
        }
        stop = timer::now();
        ms_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
        std::cout << "\r check_remove: 100% (" << last-beg << "/" << last-beg << ") takes " << ms_time << " ms in " << last-beg << " queries" << std::endl;

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
        std::cout << "\r insertions: 100% (" << last-beg << "/" << last-beg << ") takes " << ms_time << " ms" << std::endl;
        std::cout << "size: " << sdsl::size_in_bytes(index) << " bytes" << std::endl;
        std::cout << "=======================================" << std::endl << std::endl;
        beg  = last;
    }

    sdsl::store_to_file(index, index_name);
    return 0;
}

//
// Created by adrian on 2/10/24.
//

#include <cltj_compact_trie_dyn_v2.hpp>
#include <cltj_index_spo_dyn.hpp>

int main() {

    if(0) {
            std::vector<cltj::spo_triple> D;
            cltj::spo_triple t1{5, 8, 2};
            cltj::spo_triple t2{5, 8, 3};
            //D.push_back(t1);
            //D.push_back(t2);
            cltj::cltj_index_spo_dyn<cltj::compact_trie_dyn_v2<>> index(D);
            index.insert(t1);
            index.insert(t2);
            index.print();
    }

    if(1) {
        std::vector<cltj::spo_triple> D;
        cltj::spo_triple t1{5, 8, 2};
        cltj::spo_triple t2{5, 8, 3};
        D.push_back(t1);
        D.push_back(t2);
        cltj::cltj_index_spo_dyn<cltj::compact_trie_dyn_v2<>> index(D.begin(), D.end());
        index.print();
        std::cout << "============" << std::endl;
        index.insert(t2);
        index.print();
        std::cout << "============" << std::endl;
        cltj::spo_triple t3{4, 8, 2};
        index.insert(t3);
        std::cout << index.test_exists(t3) << std::endl;
        index.print();
        std::cout << "======" << std::endl;
        index.remove(t1);
        std::cout << index.test_exists(t1) << std::endl;
        index.print();
        std::cout << "======" << std::endl;
        index.remove(t2);
        index.print();
        cltj::spo_triple t4{4, 8, 1};
        index.remove(t4);
        index.print();
        std::cout << "======" << std::endl;
        index.remove(t3);
        index.print();
        std::cout << "Done" << std::endl;
    }

    if(0) {
        std::vector<cltj::spo_triple> D;
        cltj::spo_triple t1{5, 8, 2};
        cltj::spo_triple t2{5, 8, 3};
        cltj::spo_triple t3{4, 8, 2};
        cltj::spo_triple t4{4, 7, 1};
        D.push_back(t3);
        D.push_back(t2);
        cltj::cltj_index_spo_dyn<cltj::compact_trie_dyn_v2<>> index(D);
        index.insert(t3);
        index.print();
        std::cout << "===============" << std::endl;
        index.insert(t2);
        index.print();
        std::cout << "===============" << std::endl;
        index.insert(t1);
        index.insert(t2);
        /*for(auto i = 0; i < 100; ++i) {
            auto t = t3;
            t[2] = i;
            index.insert(t);
        }*/
        std::cout << "====== IN MEMORY =====" << std::endl;
        index.print();
        std::cout << "======================" << std::endl;
        std::cout << "Size in bytes: " << sdsl::size_in_bytes(index) << std::endl;
        std::cout << "Write into a file" << std::flush;
        sdsl::store_to_file(index, "proba.txt");
        std::cout << " done." << std::endl;
        std::cout << "===============" << std::endl;
        index.remove(t3);
        index.print();
        sdsl::load_from_file(index, "proba.txt");
        std::cout << "Size in bytes: " << sdsl::size_in_bytes(index) << std::endl;
        std::cout << "====== IN MEMORY =====" << std::endl;
        index.print();
        std::cout << "======================" << std::endl;
        index.remove(t3);
        index.print();
    }




}

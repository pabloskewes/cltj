//
// Created by adrian on 2/10/24.
//

#include <cltj_compact_trie_dyn.hpp>
#include <cltj_index_spo_dyn.hpp>

int main() {



    std::vector<cltj::spo_triple> D;
    cltj::spo_triple t1{5, 8, 2};
    cltj::spo_triple t2{5, 8, 3};
    D.push_back(t1);
    D.push_back(t2);
    cltj::cltj_index_spo_dyn<cltj::compact_trie_dyn> index(D);
    cltj::spo_triple t3{4, 8, 2};
    index.insert(t3);
    index.print();
    std::cout << "======" << std::endl;
    index.remove(t1);
    index.print();
    std::cout << "======" << std::endl;
    index.remove(t2);
    index.print();
    cltj::spo_triple t4{4, 8, 1};
    index.remove(t4);
    std::cout << "======" << std::endl;
    index.remove(t3);
    index.print();
    std::cout << "Done" << std::endl;



}

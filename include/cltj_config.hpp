#ifndef CLTJ_CONFIG_H
#define CLTJ_CONFIG_H

#include <iostream>
#include <cltj_compact_trie.hpp>


namespace cltj{
    using namespace std;
/*
    Current Iterator should be set to the iterator that wants to be used
    The class that is assigned to CurrentIterator must implement the Iterator abstract class 
*/
//typedef CompactTrieIterator CurrentIterator;
//typedef CompactTrie CTrie;

    typedef compact_trie CTrie;
    typedef std::array<uint32_t, 3> spo_triple;
    typedef std::array<std::array<uint64_t, 3>, 6> orders_type;

    const static orders_type spo_orders = {{{0, 1, 2}, {0, 2, 1}, {1, 2, 0}, {1, 0, 2}, {2, 0, 1}, {2, 1, 0}}};
    //                                         SPO       SOP        POS        PSO        OSP        OPS;

}

#endif

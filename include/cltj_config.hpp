#ifndef CLTJ_CONFIG_H
#define CLTJ_CONFIG_H

#include <iostream>
#include "cltj_compact_trie.hpp"


namespace cltj{
    using namespace std;
/*
    Current Iterator should be set to the iterator that wants to be used
    The class that is assigned to CurrentIterator must implement the Iterator abstract class 
*/
//typedef CompactTrieIterator CurrentIterator;
//typedef CompactTrie CTrie;

    typedef CompactTrieIV CTrie;
}
#endif

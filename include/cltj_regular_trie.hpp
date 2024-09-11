#ifndef REGULAR_TRIE_H
#define REGULAR_TRIE_H

#include <iostream>
#include <map> 

using namespace std;
namespace cltj{
    class Trie{
        public:
            map<uint32_t, Trie*> children;

            ~Trie(){
                for(auto &child: children){
                    delete child.second;
                }
            }

            Trie* insert(uint32_t, bool &inserted);
    };


    Trie* Trie::insert(uint32_t value, bool &inserted){
        auto it = children.find(value);
        Trie* node;
        if(it == children.end()){
            node = new Trie();
            children[value] = node;
            inserted = true;
        }else {
            node = children[value];
            inserted = false;
        }
        return node;
    }
}
#endif
#ifndef REGULAR_TRIE_V2_H
#define REGULAR_TRIE_V2_H

#include <iostream>
#include <unordered_map>

using namespace std;
namespace cltj{
    class TrieV2{
        public:
            unordered_map<uint32_t, TrieV2*> children;

            ~TrieV2(){
                for(auto &child: children){
                    delete child.second;
                }
                children.clear();
            }

            TrieV2* insert(uint32_t value, bool &inserted){
                auto it = children.find(value);
                TrieV2* node;
                if(it == children.end()){
                    node = new TrieV2();
                    children.insert({value, node});
                    inserted = true;
                }else {
                    node = it->second;
                    inserted = false;
                }
                return node;
            }

            std::vector<uint32_t> get_children() const {

                std::vector<uint32_t> res;
                for(const auto &v : children){
                    res.push_back(v.first);
                }
                std::sort(res.begin(), res.end());
                return res;
            }

            TrieV2* to_child(uint32_t v) const {
                const auto it = children.find(v);
                return it->second;
            }
    };




}
#endif
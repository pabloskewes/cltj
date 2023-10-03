#ifndef CLTJ_COMPACT_TRIE_IV_H
#define CLTJ_COMPACT_TRIE_IV_H

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sdsl/vectors.hpp>
#include <sdsl/select_support_mcl.hpp>
#include "cltj_utils.hpp"
#include "cltj_trie_interface.hpp"

using namespace std;
using namespace sdsl;

namespace cltj {
    class CompactTrieIV: public TrieInterface{
        public:
        // private:
            bit_vector B;
            int_vector<> seq;
            // wm_int<> wt;

            /*
                Initializes rank and select support for B
            */
        void initializeSupport(){
                util::init_support(b_rank1,&B);
                util::init_support(b_rank0,&B);
                util::init_support(b_sel1,&B);
                util::init_support(b_sel0,&B);
        }

        // public:

            //Rank & Support arrays
            rank_support_v<1> b_rank1;
            rank_support_v<0> b_rank0;
            select_support_mcl<0> b_sel0;
            select_support_mcl<1> b_sel1;

            CompactTrieIV(bit_vector b, int_vector<> s){
                B = b;
                seq = s;
                initializeSupport();
            }
            /*
                Constructor from file with representation
            */
            CompactTrieIV(string file_name){
                loadFromFile(file_name);
            };

            /*
                Destructor
            */
            ~CompactTrieIV(){};

            uint64_t size(){
                return size_in_bytes(B) + size_in_bytes(seq) + size_in_bytes(b_rank1) + \
                size_in_bytes(b_rank0) + size_in_bytes(b_sel0) + size_in_bytes(b_sel1);
            }
            /*
                Recives index in bit vector
                Returns index of next 0
            */
            uint32_t succ0(uint32_t it){
                uint32_t cant_0 = b_rank0(it);
                return b_sel0(cant_0 + 1);
            }
            
            /*
                Recives index in bit vector
                Returns index of previous 0
            */
            uint32_t prev0(uint32_t it){
                uint32_t cant_0 = b_rank0(it);
                return b_sel0(cant_0);
            }

            /*
                Recives index of current node and the child that is required
                Returns index of the nth child of current node
            */
            uint32_t child(uint32_t it, uint32_t n){
                return b_sel0(b_rank1(it+n)) + 1;
            }

            /*
                Recives index of node whos children we want to count
                Returns how many children said node has
            */
            uint32_t childrenCount(uint32_t it){
                return succ0(it) - it;
            }
            // subtree size assuming a node at depth 1
            // depth -1 node is just n (flag m_at_root)
            // depth 0 node is the same as children count
            // depth 2 is just 1
            //**** This function only handles depth 1. ****
            const uint64_t subtree_size(uint64_t it){
                uint64_t n_children = childrenCount(it);

                uint64_t leftmost_leaf, rightmost_leaf;

                leftmost_leaf = child(child(it, 1), 1);
                it = child(it, n_children);

                n_children = childrenCount(it);
                rightmost_leaf = child(it, n_children);
                return rightmost_leaf - leftmost_leaf + 1;
            }
            /*
                Recives node index
                Returns index of position in parent
            */
            uint32_t getPosInParent(uint32_t it){
                return b_sel1(b_rank0(it));
            }

            /*
                Recives index of node
                Return which child of its parent it is
            */
            uint32_t childRank(uint32_t it){
                uint32_t pos = getPosInParent(it);
                return pos - prev0(pos);
            }

            /*
                Recives index of node
                Returns index of parent node
            */  
            uint32_t parent(uint32_t it){
                uint32_t pos = getPosInParent(it);
                return prev0(pos) + 1;
            }

            /*
                Returns key that corresponds to given node(it)
            */
            uint32_t key_at(uint32_t it){
                return seq[b_rank0(it)-2];
            }

            pair<uint32_t, uint32_t> binary_search_seek(uint32_t val, uint32_t i, uint32_t f){
                if(seq[f]<val)return make_pair(0,f+1);
                uint32_t mid; 
                while(i<f){
                    mid = (i + f)/2;
                    if(seq[mid]<val)i = mid+1;
                    else if(seq[mid]>=val)f = mid;
                }
                return make_pair(seq[i], i);
            }
            
            /*
                Stores Compact Trie Iterator to file saving the size of B, B and S.
            */
            void storeToFile(string file_name){
                string B_file = file_name + ".B";
                string IV_file = file_name + ".IV";
                util::bit_compress(seq);
                store_to_file(B, B_file);
                store_to_file(seq, IV_file);
            }

            /*
                Loads Compact Trie from file restoring B and the Wavelet Tree
            */
            void loadFromFile(string file_name){
                string B_file = file_name + ".B";
                string IV_file = file_name + ".IV";
                load_from_file(B, B_file);
                load_from_file(seq, IV_file);
                util::bit_compress(seq);
                initializeSupport();
            }

            uint32_t getMaxSequence(){
                uint32_t maximum = 0;
                for(auto val: seq){
                    if(val > maximum)maximum = val;
                }
                return maximum;
            }

            uint32_t find_seq_number(uint32_t key){
                uint32_t count = 0;
                for(auto val: seq){
                    count++;
                    if(val == key){
                        return count;
                    }
                }
                return count;
            }
    };
}
#endif
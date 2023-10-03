#ifndef CLTJ_COMPACT_TRIE_ITERATOR_IV_H
#define CLTJ_COMPACT_TRIE_ITERATOR_IV_H

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sdsl/vectors.hpp>
#include "cltj_iterator.hpp"
#include "cltj_utils.hpp"
#include "cltj_compact_trie_iv.hpp"

namespace cltj{

    using namespace std;
    using namespace sdsl;

    class CompactTrieIVIterator: public Iterator{
        public:
        // private:
            bool debug = false;
            bool at_end;
            bool at_root;
            bool key_flag;
            int depth;
            uint32_t it;
            uint32_t parent_it;
            uint32_t pos_in_parent;
            uint32_t key_val;
            uint32_t tuple;
            CompactTrieIV* compactTrie;

        // public:

            /*
            Constructor from CompactTrie
            */
            CompactTrieIVIterator(CompactTrieIV* ct, uint32_t tup){
                this->compactTrie = ct;
                it = 2;
                at_root = true;
                at_end = false;
                depth = -1;
                tuple = tup;
                key_flag = false;
            }

            CompactTrieIVIterator(){}

            /*
                Destructor
            */
            ~CompactTrieIVIterator(){};

            int get_depth(){
                return depth;
            }
            /*
                Returns the key of the current position of the iterator
            */
            uint32_t key(){
                if(at_end){
                    throw "Iterator is atEnd";
                }
                else if(at_root){
                    throw "Root doesnt have key";
                }
                else{
                    if(key_flag){
                        key_flag = false;
                        return key_val;
                    }
                    else return compactTrie->key_at(it);
                }
            }

            /*
                Returns true if the iterator is past the last child of a node
            */
            bool atEnd(){
                return at_end;
            }

            /*
                Moves the iterator to the first key on the next level
            */
            void open(){

                if(at_root){
                    at_root = false;
                }
                if(at_end){
                    throw "Iterator is atEnd";
                }
                else{
                    bool has_children = compactTrie->childrenCount(it) != 0;
                    if(has_children){
                        parent_it = it;
                        it = compactTrie->child(it, 1);
                        pos_in_parent = 1;
                        depth++;
                        //cout<<"printing key in open "<<compactTrie->key_at(it)<<endl;
                    }
                    else throw "Node has no children";
                }
            }

            uint32_t getChildrenCount() const{
                return compactTrie->childrenCount(it);
            }
            const uint64_t subtree_size() const{
                return compactTrie->subtree_size(it);
            }
            /*
                Moves the iterator to the next key
            */
            void next(){
                if(at_root){
                    throw "At root, doesn't have next";
                }
                if(at_end){
                    throw "Iterator is atEnd";
                }

                uint32_t parent_child_count = compactTrie->childrenCount(parent_it);
                if(parent_child_count == pos_in_parent){
                    at_end = true;
                }
                else{
                    pos_in_parent++;
                    it = compactTrie->child(parent_it, pos_in_parent);
                }
            }
            
            /*
                Moves the iterator to the parent of the current node
            */
            void up(){
                if(at_root){
                    throw "At root, cant go up";
                }
                depth--;
                it = parent_it;
                at_end = false;
                if(it==2){
                    at_root = true;
                    // cout<<"subi hasta la root"<<endl;
                }
                else{
                    pos_in_parent = compactTrie->childRank(it);
                    parent_it = compactTrie->parent(it);
                }
            }

            /*
                Moves the iterator to the closes position that is equal or bigger than seek key
                If not possible then it moves the iterator to the end
            */
            void seek(uint32_t seek_key){
                if(debug)cout<<"Se llama a seek de "<<seek_key<<endl;
                if(at_root){
                    throw "At root, cant seek";
                }
                if(at_end){
                    throw "At end, cant seek";
                }

                // Nos indica cuantos hijos tiene el padre de el it actual ->O(1)
                uint32_t parent_child_count = compactTrie->childrenCount(parent_it);
                // Nos indica cuantos 0s hay hasta it - 2, es decir la posición en el string de el char correspondiente a la posición del it -> O(1)
                uint32_t i = compactTrie->b_rank0(it)-2;
                // Nos indica la posición en el string de el char correspondiente a la posición del ultimo hijo del padre del it. -> O(1)
                uint32_t f = compactTrie->b_rank0(compactTrie->child(parent_it, parent_child_count))-2;
                
                bool found = false;
                if(debug)cout<<"i y f "<<i<<" "<<f<<endl;
                if(debug)cout<<"parent_child_count "<<parent_child_count<<endl;
                if(debug)cout<<"it "<<it<<endl;

                if(debug)cout<<"calling binary_search "<<endl;
                auto new_info = compactTrie->binary_search_seek(seek_key, i, f);
                auto val = new_info.first;
                auto pos = new_info.second;
                
                if(pos == f+1){
                    at_end = true;
                }
                else{
                    it = compactTrie->b_sel0(pos+2)+1;
                    pos_in_parent = compactTrie->childRank(it);
                    key_flag = true;
                    key_val = val;
                }
            }

            /*
                Stores Compact Trie Iterator to file saving the size of B, B and S.
            */
            void storeToFile(string file_name){
                compactTrie->storeToFile(file_name);
                // ofstream stream(file_name);
                // if(stream.is_open()){
                //     stream<<B.size()<<'\n';
                //     for(uint64_t i=0; i<B.size(); i++){
                //         stream<<B[i]<<" ";
                //     }
                //     stream<<'\n';
                //     for(uint64_t i=0; i<wt.size(); i++){
                //         stream<<wt[i]<<" ";
                //     }
                // }
                // stream.close();
            }

            /*
                Loads Compact Trie from file restoring B and the Wavelet Tree
            */
            // void loadFromFile(string file_name){
            //     ifstream stream(file_name);
            //     uint64_t B_size;
            //     string s;
            //     uint64_t val;
            //     at_root = true;
            //     at_end = false;
            //     it = 2;

            //     if(stream.is_open()){
            //         stream>>B_size;
            //         B = bit_vector(B_size);
            //         for(uint64_t i=0; i<B_size; i++){
            //             stream>>val;
            //             B[i] = val;
            //         }
            //         stream.ignore(numeric_limits<streamsize>::max(),'\n');
            //         getline(stream, s);
            //     }
            //     stream.close();
                
            //     construct_im(wt, s, 'd');
            //     initializeSupport();
            // }
            /*
                Returns the iterator to the start of the data
            */
            void backToStart(){
                //Confirmar que esto no causa problemas
                it = 2;
                at_end = false;
                at_root = true;
            }
            
            /*
                Returns pointer to CompactTrie from which the iterator is constructed 
            */
            TrieInterface* getCompactTrie(){
                return compactTrie;
            }

            // Temporal 
            void getIteratorPos(){
                cout<<"it esta en: "<<it<<endl;
            }

            uint32_t getTuple(){
                return tuple;
            }
    };
}
#endif
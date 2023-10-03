#ifndef TABLE_INDEXING_H
#define TABLE_INDEXING_H

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include <cltj_regular_trie.hpp>

#include <sdsl/vectors.hpp>
#include <cltj_iterator.hpp>
#include <cltj_index.hpp>
#include <cltj_utils.hpp>
#include <cltj_config.hpp>

using namespace std;

using namespace sdsl;
namespace cltj{

    class TableIndexer{
        public:
        // private:

        vector<vector<uint32_t> > table;
        vector<string> orders;
        uint32_t dim;
        bool all_orders;
        Trie* root;
        bit_vector B;
        string S;
        int_vector<> seq;
        vector<CTrie *> compactTries;
        cltj_index* index;
        bool index_created = false;

        /*
            Turns uint64_t vector in to bitvector B 
        */
        void toBitvector(vector<uint32_t> &b){
            B = bit_vector(b.size(), 0);

            for(int i=0; i<b.size(); i++){
                B[i]=b[i];
            }
        }

        /*
            Turns uint64_t vector in to string S
        */
        void toSequence(vector<uint32_t> &s){
            ostringstream stream;

            for(auto &val: s){
                stream<<val<<" ";
            }
            S = stream.str();
        }

        /*
            Creates a traditional trie with the table contents following the order in index
        */
        void createRegularTrie(std::vector<uint32_t> &idx){
            Trie* node;
            for(int j=0; j<table[0].size(); j++){
                node = root;
                for(int i=0; i<table.size(); i++){
                    node = node->insert(table[idx[i]][j]);
                }
            }
        }

        /*
            Turns Trie into a bitvector B and a sequence S
        */
        void toCompactForm(){
            vector<uint32_t> b;
            vector<uint32_t> s;
            map<uint32_t, Trie*> node_children;
            queue<Trie*> q;
            Trie* node;

            q.push(root);
            b.push_back(1);
            b.push_back(0);

            while(!q.empty()){
                node = q.front();
                q.pop();
                if(node->hasChildren()){
                    node_children = node->getChildren();
                    for(const auto &child: node_children){
                        s.push_back(child.first);
                        b.push_back(1);
                        q.push(child.second);
                    }   
                }
                b.push_back(0);
            }

            toBitvector(b);

            seq.resize(s.size());
            for(auto i=0; i<s.size(); i++){
                seq[i] = s[i];
            } 

            // cout<<"printing seq"<<endl;
            // for(auto v: seq){
            //     cout<<v<<" ";
            // }
            // cout<<endl;
            // toSequence(s);
        }

        /*
            Creates indexes for all the orders necessary 
        */
        //TODO: adrian aqui crea indices
        void createIndexes(){
            for(auto &value: orders){
                vector<string> order = parse(value, ' ');
                std::vector<uint32_t> idx(order.size());
                for(int j=0; j<order.size(); j++){
                    idx[j] = stoi(order[j]);
                }
                root = new Trie();
                createRegularTrie(idx);
                toCompactForm();
                CTrie *ct = new CTrie(B,seq);
                // CompactTrie *ct = new CompactTrie(B,S);
                compactTries.push_back(ct);
                delete root;
            }
        }

        void clearData(){
            table.clear();
            orders.clear();
            dim = 0;
            all_orders = false;
            for(auto p : compactTries){
                delete p;
            }
            compactTries.clear();
        }
        // public:

        TableIndexer(){}

        ~TableIndexer(){
            if(index_created){
                delete index;

                for(auto ctrie : compactTries){
                    delete ctrie;
                }
            }
        }

        cltj_index loadIndex(string file_name){
            string file_extention = file_name.substr(file_name.size()-4, 4);
            if(file_extention != ".txt" && file_extention != ".dat") {
                return cltj_index(file_name);
                // Index ind(file_name);
                // return ind;
            }
            else{
                throw "Index must be built before queries can be answered, run \n\
                > ./build_index "+file_name;
            }
        }

        void saveIndex(){
            index->save();
        }

        void readTable(string &file_name){
            ifstream reader(file_name);
            string line;
            bool first_line = true;
            bool second_line = false;
            uint32_t value;

            while(reader.is_open() && getline(reader, line)){
                if(first_line && line.substr(0,4) == "dim:"){
                    dim = stoi(line.substr(4));
                    table.resize(dim);
                    first_line = false;
                    second_line = true;
                }
                else if(second_line && line.substr(0, 7) == "orders:"){
                    orders = parse(line.substr(7), ',');
                    if(orders.size() == 0) all_orders = true;
                    second_line = false;
                }
                else if(dim!=0 && (all_orders || orders.size()!=0)){
                    vector<string> line_values = parse(line, ' ');

                    for(int i=0; i<dim; i++){
                        table[i].push_back(stoi(line_values[i]));
                    }
                }
                else{
                    throw "File doesn't follow format:\n\
                    * First line should indicate dimension of the table -> dim:n \n\
                    * Second line should indicate which orders need to be indexed -> orders: 0 1 2, 1 2 0 \n\
                    * If all the orders need to be indexed then use -> orders: \n\
                    * The rest of the file should have the table, all the lines should have the same amount \n\
                    of values and it should be equal to dim. ";
                }
            }
            reader.close();

            if(all_orders){
                vector<uint32_t> rows(dim);
                ostringstream stream;
                for(int i=0; i<dim; i++){
                    rows[i] = i;
                }

                do{
                    stream.str("");
                    bool first = true;
                    for(auto &value: rows){
                        if(!first)stream<<" ";
                        stream<<value;
                        first=false;
                    }
                    orders.push_back(stream.str());
                }while(next_permutation(rows.begin(), rows.end()));
            }
        }
        /*
            Recives a file with the table that needs to be indexed.
            First line of the file indicates the dimensions of the table
            Second line of the file indicates which orders need to be indexed.
        */
        void indexNewTable(string file_name){
            // clearData();
            index_created = true;
            cout<<"building index"<<endl;
            createIndexes();
            // compactTrie.store_to_file();
            index = new cltj_index(dim ,orders, compactTries, file_name);

            cout<<"Index built "<<index->size()<< " bytes."<<endl;
            // ind.save();
            // return index;
        } 
    };
}
#endif
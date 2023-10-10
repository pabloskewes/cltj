#ifndef CLTJ_INDEX_H
#define CLTJ_INDEX_H

#include <iostream>
#include <string>
#include "cltj_iterator.hpp"
#include <experimental/filesystem>
#include "cltj_utils.hpp"
#include "cltj_config.hpp"

namespace cltj{


    using namespace std;
    namespace fs = std::experimental::filesystem;
    class cltj_index{
        public:
        // private:
            uint32_t dim;
            vector<string> orders;
            map<string, CTrie*> orders_tries;
            // vector<Iterator*> iterators;
            string folder = "../data/";
            bool loaded = false;

            void set_orders_tries(vector<CTrie*> &tries){
                for(int i=0; i<orders.size(); i++){
                    orders_tries[orders[i]] = tries[i];
                }
            }

        // public:
            cltj_index(uint32_t d, vector<string> &ord, vector<CTrie*> &tries, string file_name){
                //SE ASUME QUE EL PRIMER ORDEN DEL INDICE SIEMPRE ES EL ORDEN EN EL QUE VIENE LA TABLA 
                dim = d;
                orders = ord;
                // iterators = its;
                set_orders_tries(tries);
                /* The folder where the index file will be saved will be in the data folder 
                with the name of the file that was indexed */
                uint32_t s = file_name.size();
                folder = file_name.substr(0, s-4) + "/";
            }

            cltj_index(string folder_name){
                folder = folder_name;
                loaded = true;
                load();
            }
            
            ~cltj_index(){
                // cout<<"calling destructor"<<endl;
                if(loaded){
                    for(auto order_trie : orders_tries){
                        delete order_trie.second;
                    }
                }
            }

            

            uint64_t size(){
                uint64_t orders_size =  sizeof(vector<string>);
                for(auto val: orders){
                    orders_size+=sizeof(val);
                }

                uint64_t map_size = sizeof(orders_tries);
                for(auto p: orders_tries){
                    map_size += sizeof(p.first);
                    map_size += p.second->size();
                }

                return orders_size + map_size;
            }

            /*
                Saves index representation in folder with the same name of the entry file
            */
            void save(){
                fs::create_directory(folder);
                bool first = true;

                ofstream stream(folder+"info.txt");
                if(stream.is_open()){
                    stream<<"dim: "<<dim<<'\n';
                    stream<<"orders:";
                    for(auto order: orders){
                        cout<<"--"<<order<<"--"<<endl;
                        if(!first)stream<<",";
                        stream<<order;
                        first = false;
                    }
                }
                stream.close();

                uint32_t i = 0;
                for(auto p: orders_tries){
                    p.second->storeToFile(folder+"order"+to_string(i));
                    // it->storeToFile(folder+"order"+to_string(i)+".txt");
                    i++;
                }
            }

            /*
                Loads index representation from folder
            */
            void load(){
                string line;
                ifstream stream(folder + "info.txt");
                if(stream.is_open()){
                    getline(stream, line);
                    if(line.substr(0,4) == "dim:"){
                        dim = stoi(line.substr(4));
                        getline(stream, line);
                        if(line.substr(0, 7) == "orders:"){
                            orders = parse(line.substr(7), ',');
                        }
                    }
                    else{
                        throw "Info file from " + folder + " index doesn't have the appropiate format";
                    }
                }
                else{
                    throw "Path to folder is not valid, check that after the folder name there is a /";
                }
                stream.close();

                for(int i=0; i<orders.size(); i++){
                    CTrie *ct = new CTrie(folder+"order"+to_string(i));
                    this->orders_tries[orders[i]] = ct;
                }
            }
            /*
                Returns a pointer to the trie associated with the o order
            */
            CTrie* getTrie(string &o){
                return orders_tries[o];
            }

            /*
                Returns dimension of the table that is stored in the index
            */
            uint32_t getDim(){
                return dim;
            }

            vector<string> getOrders(){
                return orders;
            } 
            /*
                Resets iterators to their begining state
            */    
            // void resetIterators(){
            //     for(auto p: orders_iterators){
            //         p.second->backToStart();
            //     }
            // }
    };
}
#endif
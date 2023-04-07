/*
 * query-index.cpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <utility>
#include "ring.hpp"
#include <chrono>
#include <triple_pattern.hpp>
#include <ltj_algorithm.hpp>
#include <ltj_iterator.hpp>
#include "utils.hpp"
#include <ring_muthu.hpp>
#include <time.hpp>

using namespace std;

//#include<chrono>
//#include<ctime>

using namespace ::util::time;

bool get_file_content(string filename, vector<string> & vector_of_strings)
{
    // Open the File
    ifstream in(filename.c_str());
    // Check if object is valid
    if(!in)
    {
        cerr << "Cannot open the File : " << filename << endl;
        return false;
    }
    string str;
    // Read the next line from File until it reaches the end.
    while (getline(in, str))
    {
        // Line contains string of length > 0 then save it in vector
        if(str.size() > 0)
            vector_of_strings.push_back(str);
    }
    //Close The File
    in.close();
    return true;
}

std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(' ');
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(' ');
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s) {
    return rtrim(ltrim(s));
}

std::vector<std::string> tokenizer(const std::string &input, const char &delimiter){
    std::stringstream stream(input);
    std::string token;
    std::vector<std::string> res;
    while(getline(stream, token, delimiter)){
        res.emplace_back(trim(token));
    }
    return res;
}

bool is_variable(string & s)
{
    return (s.at(0) == '?');
}

uint8_t get_variable(string &s, std::unordered_map<std::string, uint8_t> &hash_table_vars){
    auto var = s.substr(1);
    auto it = hash_table_vars.find(var);
    if(it == hash_table_vars.end()){
        uint8_t id = hash_table_vars.size();
        hash_table_vars.insert({var, id });
        return id;
    }else{
        return it->second;
    }
}

uint64_t get_constant(string &s){
    return std::stoull(s);
}

ring::triple_pattern get_triple(string & s, std::unordered_map<std::string, uint8_t> &hash_table_vars) {
    vector<string> terms = tokenizer(s, ' ');

    ring::triple_pattern triple;
    if(is_variable(terms[0])){
        triple.var_s(get_variable(terms[0], hash_table_vars));
    }else{
        triple.const_s(get_constant(terms[0]));
    }
    if(is_variable(terms[1])){
        triple.var_p(get_variable(terms[1], hash_table_vars));
    }else{
        triple.const_p(get_constant(terms[1]));
    }
    if(is_variable(terms[2])){
        triple.var_o(get_variable(terms[2], hash_table_vars));
    }else{
        triple.const_o(get_constant(terms[2]));
    }
    return triple;
}

std::string get_type(const std::string &file){
    auto p = file.find_last_of('.');
    return file.substr(p+1);
}


void query(const std::string &queries){
    vector<string> dummy_queries;
    bool result = get_file_content(queries, dummy_queries);

    std::ifstream ifs;
    uint64_t nQ = 0;
    uint64_t same = 0, diff = 0;

    if(result)
    {

        int count = 1;
        for (string& query_string : dummy_queries) {

            //vector<Term*> terms_created;
            //vector<Triple*> query;
            std::unordered_map<std::string, uint8_t> hash_table_vars;
            std::vector<ring::triple_pattern> query;
            vector<string> tokens_query = tokenizer(query_string, '.');
            for (string& token : tokens_query) {
                auto triple_pattern = get_triple(token, hash_table_vars);
                query.push_back(triple_pattern);
            }

            std::unordered_map<uint8_t, uint64_t> ht;
            for(const auto & tp : query){
                uint64_t s_aux = -1, p_aux = -1, o_aux = -1;
                if(tp.s_is_variable()){
                    s_aux = tp.term_s.value;
                    auto it = ht.find(s_aux);
                    if(it == ht.end()){
                        ht.insert({s_aux, 1});
                    }else {
                        ++it->second;
                    }
                }
                if(tp.p_is_variable()){
                    p_aux = tp.term_p.value;
                    auto it = ht.find(p_aux);
                    if(it == ht.end()){
                        ht.insert({p_aux, 1});
                    }else {
                        ++it->second;
                    }
                }

                if(tp.o_is_variable()){
                    o_aux = tp.term_o.value;
                    if(o_aux != s_aux){
                        auto it = ht.find(o_aux);
                        if(it == ht.end()){
                            ht.insert({o_aux, 1});
                        }else {
                            ++it->second;
                        }
                    }
                }

            }

            uint64_t num_lonely = 0, num_no_lonely = 0;
            for(const auto &v : ht){
                if(v.second > 1){
                    ++num_no_lonely;
                }else{
                    ++num_lonely;
                }
            }


            if(num_no_lonely <= 1){
                ++same;
                if(num_no_lonely == 1){
                    cout << nQ <<  ";" << "TYPE2" << endl;
                }else{
                    cout << nQ <<  ";" << "TYPE1" << endl;
                }

            }else{
                ++diff;
                cout << nQ << ";" << "TYPE3" << endl;
            }

            nQ++;

            // cout << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << std::endl;

            //cout << "RESULTS QUERY " << count << ": " << number_of_results << endl;
            count += 1;
            /*std::unordered_map<uint8_t, std::string> ht;
            for(const auto &p : hash_table_vars){
                ht.insert({p.second, p.first});
            }*/

            //cout << "Query Details:" << endl;
            //ltj.print_query(ht);
            //ltj.print_gao(ht);
            //cout << "##########" << endl;
            //ltj.print_results(res, ht);

        }
        std::cerr << "Total: " << nQ << " same=" << same << " diff=" << diff << endl;

    }
}


int main(int argc, char* argv[])
{

    typedef ring::ring<> ring_type;
    //typedef ring::c_ring ring_type;
    if(argc != 2){
        std::cout << "Usage: " << argv[0] << " <queries>" << std::endl;
        return 0;
    }
    std::string queries = argv[1];
    query(queries);

	return 0;
}


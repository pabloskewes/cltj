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
#include <chrono>
#include <index/cltj_index_spo_dyn.hpp>
#include <triple_pattern.hpp>
#include <ltj_algorithm.hpp>
#include <cltj_parser.hpp>
#include <dict_map.hpp>

#include "utils.hpp"
#include <time.hpp>

using namespace std;

//#include<chrono>
//#include<ctime>

using namespace ::util::time;

bool get_file_content(string filename, vector<string> &vector_of_strings) {
    // Open the File
    ifstream in(filename.c_str());
    // Check if object is valid
    if (!in) {
        cerr << "Cannot open the File : " << filename << endl;
        return false;
    }
    string str;
    // Read the next line from File until it reaches the end.
    while (getline(in, str)) {
        // Line contains string of length > 0 then save it in vector
        if (str.size() > 0)
            vector_of_strings.push_back(str);
    }
    //Close The File
    in.close();
    return true;
}

std::string ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(' ');
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(' ');
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s) {
    return rtrim(ltrim(s));
}

std::vector<std::string> tokenizer(const std::string &input, const char &delimiter) {
    std::stringstream stream(input);
    std::string token;
    std::vector<std::string> res;
    while (getline(stream, token, delimiter)) {
        res.emplace_back(trim(token));
    }
    return res;
}

bool is_variable(string &s) {
    return (s.at(0) == '?');
}

uint8_t get_variable(string &s, std::unordered_map<std::string, uint8_t> &hash_table_vars,
                     std::unordered_set<uint8_t> &vars_in_p, bool in_p = false) {
    auto var = s.substr(1);
    auto it = hash_table_vars.find(var);
    if (it == hash_table_vars.end()) {
        uint8_t id = hash_table_vars.size();
        hash_table_vars.insert({var, id});
        if(in_p) vars_in_p.insert(id);
        return id;
    } else {
        return it->second;
    }
}

uint64_t get_constant(string &s) {
    return std::stoull(s);
}

template<class map_type>
ltj::triple_pattern get_triple(cltj::parser::user_triple_type &terms,
    std::unordered_map<std::string, uint8_t> &hash_table_vars,
    std::unordered_set<uint8_t> &vars_in_p,
    map_type &so_mapping, map_type &p_mapping) {

    ltj::triple_pattern triple;
    if (is_variable(terms[0])) {
        triple.var_s(get_variable(terms[0], hash_table_vars, vars_in_p));
    } else {
        triple.const_s(so_mapping.locate(terms[0]));
    }
    if (is_variable(terms[1])) {
        triple.var_p(get_variable(terms[1], hash_table_vars, vars_in_p, true));
    } else {
        triple.const_p(p_mapping.locate(terms[1]));
    }
    if (is_variable(terms[2])) {
        triple.var_o(get_variable(terms[2], hash_table_vars, vars_in_p));
    } else {
        triple.const_o(so_mapping.locate( terms[2]));
    }
    return triple;
}

std::string get_type(const std::string &file) {
    auto p = file.find_last_of('.');
    return file.substr(p + 1);
}


template<class index_scheme_type, class trait_type, class map_type>
void query(const std::string &file, const std::string &queries, const uint64_t limit) {
    vector<string> dummy_queries;
    bool result = get_file_content(queries, dummy_queries);

    index_scheme_type graph;
    dict::basic_map dict_so;
    dict::basic_map dict_p;

    sdsl::load_from_file(graph, file + ".cltj");
    sdsl::load_from_file(dict_so, file + ".so");
    sdsl::load_from_file(dict_p, file + ".p");
    //TODO: load dictionaries

    std::cout << "Index loaded : " << sdsl::size_in_bytes(graph) << " bytes." << std::endl;
    std::cout << "DictSO loaded: " << sdsl::size_in_bytes(dict_so) << " bytes." << std::endl;
    std::cout << "DictP loaded : " << sdsl::size_in_bytes(dict_p) << " bytes." << std::endl;

    std::ifstream ifs;
    uint64_t nQ = 0;

    //::util::time::usage::usage_type start, stop;
    typedef ltj::ltj_iterator_lite<index_scheme_type, uint8_t, uint64_t> iterator_type;
#if ADAPTIVE
    typedef ltj::ltj_algorithm<iterator_type,
        ltj::veo::veo_adaptive<iterator_type, trait_type> > algorithm_type;

#else
    typedef ltj::ltj_algorithm<iterator_type,
            ltj::veo::veo_simple<iterator_type, trait_type>> algorithm_type;
#endif
    //typedef std::vector<typename algorithm_type::tuple_type> results_type;
    //typedef algorithm_type::results_type results_type;
    typedef ::util::results_collector<typename algorithm_type::tuple_type> results_type;
    if (result) {
        int count = 1;
        for (string &query_string: dummy_queries) {
            //vector<Term*> terms_created;
            //vector<Triple*> query;
            results_type res;
            std::vector<std::vector<std::string>> res_str;
            std::unordered_map<std::string, uint8_t> hash_table_vars;
            std::unordered_set<uint8_t> vars_in_p;
            std::vector<ltj::triple_pattern> query;

            auto t0 = std::chrono::high_resolution_clock::now();
            std::vector<cltj::parser::user_triple_type> tokens = cltj::parser::get_query(query_string);
            for (cltj::parser::user_triple_type &token: tokens) {
                auto triple_pattern = get_triple(token, hash_table_vars, vars_in_p,
                    dict_so, dict_p);
                query.push_back(triple_pattern);
            }
            auto t1 = std::chrono::high_resolution_clock::now();
            algorithm_type ltj(&query, &graph);
            ltj.join(res, limit, 600);
            auto t2 = std::chrono::high_resolution_clock::now();
            std::vector<string> tmp_str(hash_table_vars.size());
            auto l = std::min(res.buckets, res.size());
            res_str.reserve(res.buckets);
            for(uint64_t i = 0; i < l; ++i) {
                const auto &tuple = res[i];
                for(uint64_t j = 0; j < tuple.size(); ++j) {
                    auto id = tuple[j].first;
                    if(vars_in_p.find(id) != vars_in_p.end()) {
                        tmp_str[id] = dict_p.extract(tuple[j].second);
                    }else {
                        tmp_str[id] = dict_so.extract(tuple[j].second);
                    }
                }
                res_str.push_back(tmp_str);
            }
            auto t3 = std::chrono::high_resolution_clock::now();
            /*std::unordered_map<uint8_t, std::string> ht;
            for(const auto &p : hash_table_vars){
                ht.insert({p.second, p.first});
            }*/

            //cout << "Query Details:" << endl;
            //ltj.print_query(ht);
            //ltj.print_gao(ht);
            //cout << "##########" << endl;
            //ltj.print_results(res, ht);
            auto str_to_id = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            auto run = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
            auto id_to_str = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t2).count();
            auto total = std::chrono::duration_cast<std::chrono::nanoseconds>(t3 - t0).count();
            cout << nQ << ";" << res.size() << ";" << str_to_id << ";" << run << ";" << id_to_str << ";" << total << endl;
            nQ++;
            //Reset caches in dictionary
            dict_so.reset_cache();
            dict_p.reset_cache();
            // cout << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << std::endl;

            //cout << "RESULTS QUERY " << count << ": " << number_of_results << endl;
            count += 1;
        }
    }
}


int main(int argc, char *argv[]) {
    //typedef ring::c_ring ring_type;
    if (argc != 5) {
        std::cout << "Usage: " << argv[0] << " <dataset> <queries> <limit> <type>" << std::endl;
        return 0;
    }

    std::string dataset = argv[1];
    std::string queries = argv[2];
    uint64_t limit = std::atoll(argv[3]);
    std::string type = argv[4];

    if (type == "normal") {
        query<cltj::compact_dyn_ltj, ltj::util::trait_distinct, dict::basic_map>(dataset, queries, limit);
    } else if (type == "star") {
        query<cltj::compact_dyn_ltj, ltj::util::trait_size, dict::basic_map>(dataset, queries, limit);
    } else {
        std::cout << "Type of index: " << type << " is not supported." << std::endl;
    }


    return 0;
}


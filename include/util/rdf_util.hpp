//
// Created by adrian on 11/12/24.
//

#ifndef RDF_UTIL_HPP
#define RDF_UTIL_HPP

#include <triple_pattern.hpp>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <cltj_config.hpp>
#include <cstdint>
#include <vector>
#include <regex>


namespace util {

    namespace rdf {

        typedef std::unordered_map<std::string, uint8_t> ht_var_id_type;

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

        bool is_variable(std::string &s) {
            return (s.at(0) == '?');
        }

        uint64_t get_constant(std::string &s) {
            return std::stoull(s);
        }


        namespace ids {

            static uint8_t get_variable(std::string &s, std::unordered_map<std::string, uint8_t> &hash_table_vars) {
                auto var = s.substr(1);
                auto it = hash_table_vars.find(var);
                if (it == hash_table_vars.end()) {
                    uint8_t id = hash_table_vars.size();
                    hash_table_vars.insert({var, id});
                    return id;
                } else {
                    return it->second;
                }
            }

            static ltj::triple_pattern get_triple_pattern(std::string &s, std::unordered_map<std::string, uint8_t> &hash_table_vars) {
                std::vector<std::string> terms = tokenizer(s, ' ');

                ltj::triple_pattern triple;
                if (is_variable(terms[0])) {
                    triple.var_s(get_variable(terms[0], hash_table_vars));
                } else {
                    triple.const_s(get_constant(terms[0]));
                }
                if (is_variable(terms[1])) {
                    triple.var_p(get_variable(terms[1], hash_table_vars));
                } else {
                    triple.const_p(get_constant(terms[1]));
                }
                if (is_variable(terms[2])) {
                    triple.var_o(get_variable(terms[2], hash_table_vars));
                } else {
                    triple.const_o(get_constant(terms[2]));
                }
                return triple;
            }

            inline cltj::spo_triple get_triple(const std::string &s) {
                std::vector<std::string> terms = tokenizer(s, ' ');
                cltj::spo_triple triple;
                triple[0] = get_constant(terms[0]);
                triple[1] = get_constant(terms[1]);
                triple[2] = get_constant(terms[2]);
                return triple;
            }

            inline std::vector<ltj::triple_pattern> get_query(const std::string &s) {
                std::unordered_map<std::string, uint8_t> hash_table_vars;
                std::vector<ltj::triple_pattern> query;
                std::vector<std::string> tokens_query = tokenizer(s, '.');
                for (std::string &token: tokens_query) {
                    auto triple_pattern = get_triple_pattern(token, hash_table_vars);
                    query.push_back(triple_pattern);
                }
                return query;
            }

        }

        namespace str {

            static uint8_t get_variable(std::string &s, ht_var_id_type &ht_var_id, std::vector<bool> &var_in_p, bool in_p = false) {
                auto var = s.substr(1);
                auto it = ht_var_id.find(var);
                if (it == ht_var_id.end()) {
                    uint8_t id = ht_var_id.size();
                    ht_var_id.insert({var, id});
                    var_in_p.push_back(in_p);
                    return id;
                } else {
                    return it->second;
                }
            }

            template<class map_type>
            std::pair<bool, ltj::triple_pattern> get_triple_pattern(cltj::user_triple &terms,
                                                ht_var_id_type &ht_var_id,
                                                std::vector<bool> &var_in_p,
                                                map_type &so_mapping, map_type &p_mapping) {

                ltj::triple_pattern triple;
                if (is_variable(terms[0])) {
                    triple.var_s(get_variable(terms[0], ht_var_id, var_in_p));
                } else {
                    auto v = so_mapping.locate(terms[0]);
                    if(v == 0) return {false, triple};
                    triple.const_s(v);
                }
                if (is_variable(terms[1])) {
                    triple.var_p(get_variable(terms[1], ht_var_id, var_in_p, true));
                } else {
                    auto v = p_mapping.locate(terms[1]);
                    if(v == 0) return {false, triple};
                    triple.const_p(v);
                }
                if (is_variable(terms[2])) {
                    triple.var_o(get_variable(terms[2], ht_var_id, var_in_p));
                } else {
                    auto v = so_mapping.locate( terms[2]);
                    if(v == 0) return {false, triple};
                    triple.const_o(v);
                }
                return {true, triple};
            }

            static const std::regex regex("(?:\".*\"|[^[:space:]])+");

            static cltj::user_triple get_triple(const std::string &str){

                std::sregex_iterator reg_it(str.begin(), str.end(), regex);
                cltj::user_triple vec;
                for(auto i = 0; i < 3; ++i) {
                    vec[i] = reg_it->str();
                    ++reg_it;
                }
                return vec;
            }

            static std::vector<cltj::user_triple> get_query(const std::string &str){

                std::sregex_iterator reg_it(str.begin(), str.end(), regex);
                std::sregex_iterator reg_end;
                std::vector<cltj::user_triple> vec;
                cltj::user_triple t;
                while(reg_it != reg_end) {
                    t[0] = (reg_it++)->str();
                    t[1] = (reg_it++)->str();
                    t[2] = (reg_it++)->str();
                    vec.push_back(t);
                    if(reg_it == reg_end) break;
                    ++reg_it; //skipping the point

                }
                return vec;
            }
        }
    }
}

#endif //STRING_UTIL_HPP

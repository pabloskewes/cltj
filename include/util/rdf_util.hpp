//
// Created by adrian on 11/12/24.
//

#ifndef RDF_UTIL_HPP
#define RDF_UTIL_HPP

#include <triple_pattern.hpp>
#include <unordered_map>
#include <sstream>


namespace util {

    namespace rdf {

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

        uint8_t get_variable(std::string &s, std::unordered_map<std::string, uint8_t> &hash_table_vars) {
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

        uint64_t get_constant(std::string &s) {
            return std::stoull(s);
        }

        inline ltj::triple_pattern get_triple_pattern(std::string &s, std::unordered_map<std::string, uint8_t> &hash_table_vars) {
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

        inline cltj::spo_triple get_triple(std::string &s) {
            std::vector<std::string> terms = tokenizer(s, ' ');
            cltj::spo_triple triple;
            triple[0] = get_constant(terms[0]);
            triple[1] = get_constant(terms[1]);
            triple[2] = get_constant(terms[2]);
            return triple;
        }
    }
}

#endif //STRING_UTIL_HPP

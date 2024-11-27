//
// Created by adrian on 9/10/24.
//

#ifndef CLTJ_PARSER_HPP
#define CLTJ_PARSER_HPP

#include <cstdint>
#include <vector>
#include <regex>

namespace cltj {

    namespace parser {

        typedef std::array<std::string, 3> user_triple_type;
        static const std::regex regex("(?:\".*\"|[^[:space:]])+");

        static user_triple_type get_triple(const std::string &str){

            std::sregex_iterator reg_it(str.begin(), str.end(), regex);
            user_triple_type vec;
            for(auto i = 0; i < 3; ++i) {
                vec[i] = reg_it->str();
                ++reg_it;
            }
            return vec;
        }

        static std::vector<user_triple_type> get_query(const std::string &str){

            std::sregex_iterator reg_it(str.begin(), str.end(), regex);
            std::sregex_iterator reg_end;
            std::vector<user_triple_type> vec;
            user_triple_type t;
            while(reg_it != reg_end) {
                t[0] = (reg_it++)->str();
                t[1] = (reg_it++)->str();
                t[2] = (reg_it++)->str();
                ++reg_it; //skipping the point
                vec.push_back(t);
            }
            return vec;
        }

    }
}
#endif //CLTJ_HELPER_HPP

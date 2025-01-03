//
// Created by Adrián on 24/10/23.
//

#ifndef CLTJ_RESULTS_PRINTER_HPP
#define CLTJ_RESULTS_PRINTER_HPP


#include <array>
#include <fstream>
#include <vector>
#include <cstdint>
#include <iostream>

namespace util {

    template<class Var, class Cons>
    class results_printer {

    public:
        typedef std::vector<std::pair<Var, Cons>> value_type;
        typedef uint64_t size_type;

    private:

        size_type m_cnt = 0;

        void copy(const results_printer &o) {
            m_cnt = o.m_cnt;
        }

    public:

        results_printer() = default;

        inline void add(const value_type &val){
            std::vector<Cons> values(val.size());
            for(const auto &pair : val){
                values[pair.first] = pair.second;
            }
            for(const auto &v : values){
                std::cout << v << ";";
            }
            std::cout << std::endl;
            ++m_cnt;
        }

        void clear() {
            m_cnt = 0;
        }

        inline size_type size(){
            return m_cnt;
        }

        //! Copy constructor
        results_printer(const results_printer &o) {
            copy(o);
        }

        //! Move constructor
        results_printer(results_printer &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        results_printer &operator=(const results_printer &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        results_printer &operator=(results_printer &&o) {
            if (this != &o) {
                m_cnt = std::move(o.m_cnt);
            }
            return *this;
        }

        void swap(results_printer &o) {
            std::swap(m_cnt, o.m_cnt);
        }

    };

}
#endif //CLTJ_RESULTS_PRINTER_HPP
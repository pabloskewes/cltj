//
// Created by Adrián on 24/10/23.
//

#ifndef CLTJ_RESULTS_COLLECTOR_HPP
#define CLTJ_RESULTS_COLLECTOR_HPP

#endif //CLTJ_RESULTS_COLLECTOR_HPP

#include <array>

namespace util {

    template<class Type>
    class results_collector {

    public:
        typedef Type value_type;
        typedef uint64_t size_type;
        constexpr static size_type buckets = (1ULL << 20);

    private:

        value_type* m_results;
        size_type m_cnt = 0;

        void copy(const results_collector &o) {
            for(size_type i = 0; i < buckets; ++i){
                m_results[i] = o.m_results[i];
            }
            m_cnt = o.m_cnt;
        }

    public:

        results_collector(){
            m_results = new value_type[buckets];
        }

        ~results_collector(){
            delete [] m_results;
        }

        inline void add(const value_type &val){
            m_results[(m_cnt & (buckets-1))] = val;
            ++m_cnt;
        }

        inline size_type size(){
            return m_cnt;
        }

        //! Copy constructor
        results_collector(const results_collector &o) {
            copy(o);
        }

        //! Move constructor
        results_collector(results_collector &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        results_collector &operator=(const results_collector &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        results_collector &operator=(results_collector &&o) {
            if (this != &o) {
                m_results = std::move(o.m_results);
                m_cnt = std::move(o.m_cnt);
            }
            return *this;
        }

        void swap(results_collector &o) {
            std::swap(m_results, o.m_results);
            std::swap(m_cnt, o.m_cnt);
        }

    };

}
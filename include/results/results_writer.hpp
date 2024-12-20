//
// Created by Adrián on 24/10/23.
//

#ifndef CLTJ_RESULTS_WRITER_HPP
#define CLTJ_RESULTS_WRITER_HPP


#include <array>

namespace util {

    template<class Type>
    class results_writer {

    public:
        typedef Type value_type;
        typedef uint64_t size_type;
        constexpr static size_type buckets = (1ULL << 20);

    private:

        value_type* m_results;
        size_type m_cnt = 0;

        void copy(const results_writer &o) {
            for(size_type i = 0; i < buckets; ++i){
                m_results[i] = o.m_results[i];
            }
            m_cnt = o.m_cnt;
        }

    public:

        results_writer(){
            m_results = new value_type[buckets];
        }

        ~results_writer(){
            delete [] m_results;
        }

        inline void add(const value_type &val){
            m_results[(m_cnt & (buckets-1))] = val;
            ++m_cnt;
        }

        inline value_type operator[](const size_type i) {
            return m_results[i];
        }

        inline size_type size(){
            return m_cnt;
        }

        inline void clear() {
            m_cnt = 0;
        }

        //! Copy constructor
        results_writer(const results_writer &o) {
            copy(o);
        }

        //! Move constructor
        results_writer(results_writer &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        results_writer &operator=(const results_writer &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        results_writer &operator=(results_writer &&o) {
            if (this != &o) {
                m_results = std::move(o.m_results);
                m_cnt = std::move(o.m_cnt);
            }
            return *this;
        }

        void swap(results_writer &o) {
            std::swap(m_results, o.m_results);
            std::swap(m_cnt, o.m_cnt);
        }

    };

}
#endif //CLTJ_RESULTS_WRITER_HPP
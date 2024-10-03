//
// Created by adrian on 28/9/24.
//

#ifndef DYN_BIT_VECTOR_HPP
#define DYN_BIT_VECTOR_HPP

namespace dyn_cds {

    extern "C" {
        #include "hybridBV/hybridBV.h"
    }

    class dyn_bit_vector {

    public:

        typedef uint64_t size_type;
        typedef uint64_t value_type;

    private:
        hybridBV m_B = nullptr;

        void copy(const dyn_bit_vector &o) {
            m_B = hybridClone(o.m_B);
        }

    public:

        dyn_bit_vector() {
            m_B = hybridCreate();
        }

        dyn_bit_vector(uint64_t* data, size_type n) {
            m_B = hybridCreateFromNoFree(data, n);
        }

        ~dyn_bit_vector() {
            if(m_B != nullptr) {
                hybridDestroy(m_B);
                m_B = nullptr;
            }
        }

        size_type size() const {
            return hybridLength(m_B);
        }

        value_type access(size_type i) const {
            return hybridAccess(m_B, i);
        }

        value_type operator[](size_type i) const {
            return access(i);
        }

        void insert(size_type i, value_type v) {
            hybridInsert(m_B, i, v);
        }

        value_type remove(size_type i) {
            return hybridDelete(m_B, i);
        }

        void set(size_type i, value_type v) {
            hybridWrite(m_B, i, v);
        }

        size_type rank(size_type i) const {
            return hybridRank(m_B, i-1);
        }

        size_type select(size_type i) const { //TODO: mellor que tivera succ en lugar de select [falar con Gonzalo]
            return hybridSelect(m_B, i);
        }


        //! Copy constructor
        dyn_bit_vector(const dyn_bit_vector &o) {
            copy(o);
        }

        //! Move constructor
        dyn_bit_vector(dyn_bit_vector &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        dyn_bit_vector &operator=(const dyn_bit_vector &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        dyn_bit_vector &operator=(dyn_bit_vector &&o) {
            if (this != &o) {
               m_B = o.m_B;
               o.m_B = nullptr; //prevents deleting the data
            }
            return *this;
        }

        void swap(dyn_bit_vector &o) {
            std::swap(m_B, o.m_B);
        }


    };
}

#endif //DYN_BIT_VECTOR_HPP

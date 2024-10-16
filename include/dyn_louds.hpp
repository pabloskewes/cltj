//
// Created by adrian on 28/9/24.
//

#ifndef DYN_LOUDS_HPP
#define DYN_LOUDS_HPP

#include <file.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>

namespace dyn_cds {

    extern "C" {
        #include "hybridBV/hybridBVId.h"
    }

    class dyn_louds {

    public:

        typedef uint64_t size_type;
        typedef uint64_t value_type;

    private:
        hybridBVId m_B = nullptr;

        void copy(const dyn_louds &o) {
            m_B = hybridBVIdClone(o.m_B);
        }

    public:

        dyn_louds() = default;

        dyn_louds(uint width) {
            m_B = hybridBVIdCreate(width);
        }

        dyn_louds(uint64_t* data, uint64_t* id_data, size_type n, uint width) {
            m_B = hybridBVIdCreateFrom64(data, id_data, n, width);
        }

        ~dyn_louds() {
            if(m_B != nullptr) {
                hybridBVIdDestroy(m_B);
                m_B = nullptr;
            }
        }

        size_type size() const {
            return hybridBVIdLength(m_B);
        }

        std::pair<bool, value_type> access(size_type i) const {
            value_type id;
            auto bit = hybridBVIdAccess(m_B, i, &id);
            return {bit, id};
        }

        value_type operator[](size_type i) const {
            return hybridBVIdAccessId(m_B, i);
        }

        void insert(size_type i, value_type bit, value_type v, bool first) {
            hybridBVIdInsert(m_B, i, bit , v, first);
        }

        void remove(size_type i, bool more) {
            hybridBVIdDelete(m_B, i, more);
        }

        void set(size_type i, value_type v) {
            hybridBVIdWriteBV(m_B, i, v); //TODO: penso que non se vai usar
        }

        size_type rank(size_type i) const {
            return hybridBVIdRank(m_B, i-1);
        }

        size_type select(size_type i) const { //TODO: mellor que tivera succ en lugar de select [falar con Gonzalo]
            return hybridBVIdSelect(m_B, i);
        }


        //! Copy constructor
        dyn_louds(const dyn_louds &o) {
            copy(o);
        }

        //! Move constructor
        dyn_louds(dyn_louds &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        dyn_louds &operator=(const dyn_louds &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        dyn_louds &operator=(dyn_louds &&o) {
            if (this != &o) {
               m_B = o.m_B;
               o.m_B = nullptr; //prevents deleting the data
            }
            return *this;
        }

        void swap(dyn_louds &o) {
            std::swap(m_B, o.m_B);
        }
        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = hybridBVIdSpace(m_B) * 8;
            FILE* f = util::file::create_c_file();
            hybridBVIdSave(m_B, f);
            util::file::begin_c_file(f);
            util::file::c_file_to_ostream(f, out);
            util::file::close_c_file(f);
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            FILE* f = util::file::create_c_file();
            int64_t pos = in.tellg();
            util::file::istream_to_c_file(f, in);
            util::file::begin_c_file(f);
            m_B = hybridBVIdLoad(f);
            int64_t offset = ftell(f);
            in.seekg(pos + offset, std::istream::beg);
            util::file::close_c_file(f);
        }

        bool check() {
            return checkOnes(m_B);
        }

        void check_print() {
            checkOnesPrint(m_B);
        }


    };
}

#endif //dyn_louds_HPP

//
// Created by adrian on 28/9/24.
//

#ifndef DYN_ARRAY_HPP
#define DYN_ARRAY_HPP

namespace dyn_cds {

    extern "C" {
        #include "hybridBV/hybridId.h"
    }

    class dyn_array {

    public:

        typedef uint64_t size_type;
        typedef uint64_t value_type;

    private:
        hybridId m_B = nullptr;

        void copy(const dyn_array &o) {
            m_B = hybridIdClone(o.m_B);
        }

    public:

        dyn_array() = default;

        dyn_array(uint width ) {
            m_B = hybridIdCreate(width);
        }

        dyn_array(uint64_t* data, size_type n, uint width) {
            m_B = hybridIdCreateFrom64NoFree(data, n, width);
        }

        ~dyn_array() {
            if(m_B != nullptr) {
                hybridIdDestroy(m_B);
                m_B = nullptr;
            }
        }

        size_type size() const {
            return hybridIdLength(m_B);
        }

        value_type access(size_type i) const {
            return hybridIdAccess(m_B, i);
        }

        value_type operator[](size_type i) const {
            return access(i);
        }

        void insert(size_type i, value_type v) {
            hybridIdInsert(m_B, i, v);
        }

        void remove(size_type i) {
            hybridIdDelete(m_B, i);
        }

        void set(size_type i, value_type v) {
            hybridIdWrite(m_B, i, v);
        }


        //! Copy constructor
        dyn_array(const dyn_array &o) {
            copy(o);
        }

        //! Move constructor
        dyn_array(dyn_array &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        dyn_array &operator=(const dyn_array &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        dyn_array &operator=(dyn_array &&o) {
            if (this != &o) {
               m_B = o.m_B;
               o.m_B = nullptr; //prevents deleting the data
            }
            return *this;
        }

        void swap(dyn_array &o) {
            std::swap(m_B, o.m_B);
        }

        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = hybridIdSpace(m_B);
            FILE* f = util::file::create_c_file();
            hybridIdSave(m_B, f);
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
            m_B = hybridIdLoad(f);
            int64_t offset = ftell(f);
            in.seekg(pos + offset, std::istream::beg);
            util::file::close_c_file(f);
        }


    };
}

#endif //DYN_BIT_VECTOR_HPP

//
// Created by adrian on 12/12/24.
//

#ifndef CLTJ_DYN_IDS_HPP
#define CLTJ_DYN_IDS_HPP

#include <query/ltj_algorithm.hpp>
#include <index/cltj_index_spo_dyn.hpp>
#include <util/rdf_util.hpp>

using namespace std::chrono;
using timer = std::chrono::high_resolution_clock;

namespace cltj {

    class cltj_dyn_ids {

    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;
        typedef cltj::compact_dyn_ltj index_type;
        typedef ltj::util::trait_size trait_type;
        typedef ltj::ltj_iterator_lite<index_type, uint8_t, uint64_t> iterator_type;
        typedef ltj::ltj_algorithm<iterator_type, ltj::veo::veo_adaptive<iterator_type, trait_type> > algorithm_type;
        typedef typename algorithm_type::tuple_type tuple_type;

    private:

        index_type m_index;

        void copy(const cltj_dyn_ids &o){
            m_index = o.m_index;
        }

        void build(const std::string &dataset){
            vector<cltj::spo_triple> D;
            uint64_t id_so = 0, id_p = 0;
            std::ifstream ifs(dataset);
            std::cout << "============================================================" << std::endl;
            std::cout << "Reading data... " << std::flush;
            //STEP1: read the data
            uint32_t s, p, o;
            cltj::spo_triple spo;
            auto start = timer::now();
            do {
                ifs >> s >> p >> o;
                if(ifs.eof()) break;
                spo[0] = s; spo[1] = p; spo[2] = o;
                D.emplace_back(spo);

            } while (!ifs.eof());
            D.shrink_to_fit();
            auto stop = timer::now();
            auto secs = duration_cast<seconds>(stop-start).count();
            std::cout << "done. [" << secs << " secs. ]" << std::endl;

            size_type size_data = 3*D.size()*sizeof(::uint32_t);

            //STEP2: Build index
            std::cout << "Building index... " << std::flush;
            start = timer::now();
            m_index = index_type(D);
            stop = timer::now();
            secs = duration_cast<seconds>(stop-start).count();
            std::cout << "done. [" << secs << " secs. ]" << std::endl;

            size_type size_index = sdsl::size_in_bytes(m_index);

            std::cout << std::endl;
            std::cout << "============================================================" << std::endl;
            std::cout << "Index    : " << size_index << " bytes. " << std::endl;
            std::cout << "Bin. data: " << size_data << " bytes. " << std::endl;
            std::cout << "Ratio    : " << (size_index) / (double) size_data * 100 << "%" << std::endl;
            std::cout << "============================================================" << std::endl;

        }

    public:

        cltj_dyn_ids() = default;

        explicit cltj_dyn_ids(const std::string &dataset){
            build(dataset);
        }

        template<class result_type>
        bool query(const std::string &query_str, result_type &res, size_type limit = 1000, size_type timeout_seconds = 600) {

            std::vector<ltj::triple_pattern> query = ::util::rdf::ids::get_query(query_str);
            algorithm_type ltj(&query, &m_index);
            ltj.join(res, limit, timeout_seconds);
            return true;
        }

        void insert(const std::string &triple) {
            auto spo = ::util::rdf::ids::get_triple(triple);
            m_index.insert(spo);
        }

        bool remove(const std::string &triple) {
            auto spo = ::util::rdf::ids::get_triple(triple);
            return m_index.remove(spo);
        }

        //! Copy constructor
        cltj_dyn_ids(const cltj_dyn_ids &o) {
            copy(o);
        }

        //! Move constructor
        cltj_dyn_ids(cltj_dyn_ids &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        cltj_dyn_ids &operator=(const cltj_dyn_ids &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        cltj_dyn_ids &operator=(cltj_dyn_ids &&o) {
            if (this != &o) {
                m_index = std::move(o.m_index);
            }
            return *this;
        }

        void swap(cltj_dyn_ids &o) {
            std::swap(m_index, o.m_index);
        }

        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_index.serialize(out, child, "index");
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            m_index.load(in);
        }


    };

}
#endif //CLTJ_DYN_IDS_HPP

//
// Created by Diego Arroyuelo on 20/09/24.
//

#ifndef CLTJ_INDEX_METATRIE_HPP
#define CLTJ_INDEX_METATRIE_HPP

#include <metatrie/cltj_compact_metatrie.hpp>
#include <cltj_config.hpp>

namespace cltj {

    template<class Trie>
    class cltj_index_metatrie {

    public:
        typedef uint64_t size_type;
        typedef uint32_t value_type;
        typedef Trie trie_type;

    private:
        std::array<trie_type, 6> m_tries;

        trie_type create_full_trie(vector<spo_triple> &D, uint8_t order){

            std::sort(D.begin(), D.end(), comparator_order(order));

            uint64_t c0 = 1, cur_value = D[0][spo_orders[order][0]];
            std::vector<uint64_t> v0;
            std::vector<uint64_t> seq;

            for (uint64_t i = 1; i < D.size(); i++) {
                if (D[i][spo_orders[order][0]] != D[i-1][spo_orders[order][0]]) {
                    seq.push_back(cur_value);
                    cur_value = D[i][spo_orders[order][0]];
                    v0.push_back(c0);
                    c0 = 1;
                } else c0++;
            }
            seq.push_back(cur_value);
            v0.push_back(c0);
            
            uint64_t c1, c2;
            std::vector<uint64_t> v1, v2;

            for (uint64_t i = 0, k = 0; i < v0.size(); i++) {
                c1 = 1;
                c2 = 1;
                for (uint64_t j = 1; j < v0[i]; j++) {
                    k++;
                    if (D[k][spo_orders[order][1]] != D[k-1][spo_orders[order][1]]) {
                        v2.push_back(c2);
                        seq.push_back(D[k-1][spo_orders[order][1]]);
                        c2 = 1;
                        c1++;
                    } else c2++;
                }
                seq.push_back(D[k][spo_orders[order][1]]);
                v2.push_back(c2);
                v1.push_back(c1);
                k++;
            }
            uint64_t c = v0.size()+1; 

            for (uint64_t i = 0; i < v1.size(); i++)
                c += v1[i];

            for (uint64_t i = 0; i < v2.size(); i++)
                c += v2[i];

            sdsl::bit_vector bv = sdsl::bit_vector(c, 1);
            sdsl::int_vector<> seq_compact = sdsl::int_vector<>(seq.size()+D.size()+1);

            std::cout << "Full bv.size()=" << bv.size() << " seq=" << seq_compact.size() << std::endl;

            uint64_t j = 0;

            bv[0] = 0;
            bv[v0.size()] = 0;
            j = v0.size();

            for (uint64_t i = 0; i < v1.size(); i++) {
                j += v1[i];
                bv[j] = 0;
            }

            for (uint64_t i = 0; i < v2.size(); i++) {
                j += v2[i];
                bv[j] = 0;
            }

            for (j = 0; j < seq.size(); j++)
                seq_compact[j] = seq[j];

            for (uint64_t i = 0; i < D.size(); i++)
                seq_compact[j++] = D[i][spo_orders[order][2]];
            seq_compact[j] = 0; //mock

/*            cout << "*** Trie para el orden " << (uint64_t)order << " ***" << endl;
            for (uint64_t i = 0; i < bv.size(); i++) {
                cout << bv[i];
            }
            cout << endl;
            for (uint64_t i = 0; i < seq_compact.size(); i++) {
                cout << seq_compact[i] << " ";
            }
            cout << endl;
*/
            return trie_type(bv, seq_compact);            
        }


        trie_type create_partial_trie(vector<spo_triple> &D, uint8_t order){

            std::sort(D.begin(), D.end(), comparator_order(order));

            uint64_t c0 = 1;
            std::vector<uint64_t> v0;
            std::vector<uint64_t> seq;

            for (uint64_t i = 1; i < D.size(); i++) {
                if (D[i][spo_orders[order][0]] != D[i-1][spo_orders[order][0]]) {
                    v0.push_back(c0);
                    c0 = 1;
                } else c0++;
            }
            v0.push_back(c0);

            uint64_t c1, c2;
            std::vector<uint64_t> v1, v2;

            for (uint64_t i = 0, k = 0; i < v0.size(); i++) {
                c1 = 1;
                c2 = 1;
                for (uint64_t j = 1; j < v0[i]; j++) {
                    k++;
                    if (D[k][spo_orders[order][1]] != D[k-1][spo_orders[order][1]]) {
                        v2.push_back(c2);
                        seq.push_back(D[k-1][spo_orders[order][1]]);
                        c2 = 1;
                        c1++;
                    } else c2++;
                }
                seq.push_back(D[k][spo_orders[order][1]]);
                v2.push_back(c2);
                v1.push_back(c1);
                k++;
            }

            uint64_t c = 0;
            for (uint64_t i = 0; i < v1.size(); i++)
                c += v1[i];

            sdsl::bit_vector bv = sdsl::bit_vector(c+1, 1);
            sdsl::int_vector<> seq_compact(seq.size()+1);

            std::cout << "Partial bv.size()=" << bv.size() << " seq=" << seq_compact.size() << std::endl;

            bv[0] = 0;
            uint64_t j = 1;
            for (uint64_t i = 0; i < v1.size(); i++) {
                j += v1[i];
                bv[j-1] = 0;
            }

            for (j = 0; j < seq.size(); j++)
                seq_compact[j] = seq[j];
            seq_compact[seq.size()] = 0; //mock
/*
            cout << "*** Trie para el orden " << (uint64_t)order << " ***" << endl;
            for (uint64_t i = 0; i < bv.size(); i++) {
                cout << bv[i];
            }
            cout << endl;
            for (uint64_t i = 0; i < seq_compact.size(); i++) {
                cout << seq_compact[i] << " ";
            }
            cout << endl;
*/          
            return trie_type(bv, seq_compact);
        }

        void copy(const cltj_index_metatrie &o){
            m_tries = o.m_tries;
        }

    public:

        const std::array<trie_type, 6> &tries = m_tries;
        cltj_index_metatrie() = default;

        cltj_index_metatrie(vector<spo_triple> &D) {
            m_tries[0] = create_full_trie(D, 0);  // trie for SPO
            m_tries[1] = create_partial_trie(D, 1); // trie for SOP
	        m_tries[2] = create_full_trie(D, 2);  // trie for POS
	        m_tries[3] = create_partial_trie(D, 3); // trie for PSO
	        m_tries[4] = create_full_trie(D, 4);  // trie for OSP
	        m_tries[5] = create_partial_trie(D, 5); // trie for OPS
        }

        //! Copy constructor
        cltj_index_metatrie(const cltj_index_metatrie &o) {
            copy(o);
        }

        //! Move constructor
        cltj_index_metatrie(cltj_index_metatrie &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        cltj_index_metatrie &operator=(const cltj_index_metatrie &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        cltj_index_metatrie &operator=(cltj_index_metatrie &&o) {
            if (this != &o) {
                m_tries = std::move(o.m_tries);
            }
            return *this;
        }

        void insert(const spo_triple &triple) {
            std::cout << "Insert operation is not supported (static version)." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        void remove(const spo_triple &triple) {
            std::cout << "Remove operation is not supported (static version)." << std::endl;
            std::exit(EXIT_FAILURE);
        }

        void swap(cltj_index_metatrie &o) {
            // m_bp.swap(bp_support.m_bp); use set_vector to set the supported bit_vector
            std::swap(m_tries, o.m_tries);
        }

        inline trie_type* get_trie(size_type i){
            return &m_tries[i];
        }

        size_type serialize(std::ostream &out, sdsl::structure_tree_node *v = nullptr, std::string name = "") const {
            sdsl::structure_tree_node *child = sdsl::structure_tree::add_child(v, name, sdsl::util::class_name(*this));
            size_type written_bytes = 0;
            for(const auto & trie : m_tries){
                written_bytes += trie.serialize(out, child, "tries");
            }
            sdsl::structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

        void load(std::istream &in) {
            for(auto & trie : m_tries){
                trie.load(in);
            }
        }

    };

    typedef cltj::cltj_index_metatrie<cltj::compact_metatrie> compact_ltj_metatrie;
    //typedef cltj::cltj_index_metatrie<cltj::uncompact_trie_v2> uncompact_ltj;

}

#endif 

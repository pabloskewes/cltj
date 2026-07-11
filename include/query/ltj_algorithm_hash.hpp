/*
 * ltj_algorithm_hash.hpp
 * Copyright (C) 2020 Author removed for double-blind evaluation
 *
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RING_LTJ_ALGORITHM_HASH_HPP
#define RING_LTJ_ALGORITHM_HASH_HPP

#include <cassert>
#include <triple_pattern.hpp>
// #include <ltj_iterator.hpp>
#include <dict/dict_map.hpp>
#include <query/alternation_complexity.hpp>
#include <query/intersection_stats.hpp>
#include <query/ltj_iterator_basic.hpp>
#include <query/ltj_iterator_lite.hpp>
#include <query/ltj_iterator_metatrie_hash.hpp>
#include <query/query_tracer.hpp>
#include <results/results.hpp>
#include <util/rdf_util.hpp>
#include <veo/veo_adaptive.hpp>
#include <veo/veo_simple.hpp>

#define EXPT_TIME_SOL 0

namespace ltj {

using cltj::TRACE_QUERY;

template <
    class iterator_t = ltj_iterator_lite<cltj::compact_ltj, uint8_t, uint64_t>,
    class veo_t = veo::veo_adaptive<iterator_t, util::trait_size>>
class ltj_algorithm_hash {
  public:
    typedef uint64_t value_type;
    typedef uint64_t size_type;
    typedef iterator_t ltj_iter_type;
    typedef typename ltj_iter_type::var_type var_type;
    typedef typename ltj_iter_type::index_scheme_type index_scheme_type;
    typedef typename ltj_iter_type::value_type const_type;
    typedef veo_t veo_type;
    typedef unordered_map<var_type, vector<ltj_iter_type*>> var_to_iterators_type;
    typedef vector<pair<var_type, const_type>> tuple_type;
    typedef vector<std::string> tuple_str_type;
    typedef chrono::high_resolution_clock::time_point time_point_type;
    // typedef ::util::results_collector<tuple_type> results_type;

  private:
    const vector<triple_pattern>* m_ptr_triple_patterns;
    veo_type m_veo;
    index_scheme_type* m_ptr_ring;
    vector<ltj_iter_type> m_iterators;
    var_to_iterators_type m_var_to_iterators;
    bool m_is_empty = false;
    std::vector<IntersectionStats> m_stats;
    cltj::query::QueryTracer<cltj::TRACE_QUERY> m_tracer{"query_traces"};

    void copy(const ltj_algorithm_hash& o) {
        m_ptr_triple_patterns = o.m_ptr_triple_patterns;
        m_veo = o.m_veo;
        m_ptr_ring = o.m_ptr_ring;
        m_iterators = o.m_iterators;
        m_var_to_iterators = o.m_var_to_iterators;
        m_is_empty = o.m_is_empty;
        m_stats = o.m_stats;
    }

    inline void add_var_to_iterator(const var_type var, ltj_iter_type* ptr_iterator) {
        auto it = m_var_to_iterators.find(var);
        if (it != m_var_to_iterators.end()) {
            it->second.push_back(ptr_iterator);
        } else {
            vector<ltj_iter_type*> vec = {ptr_iterator};
            m_var_to_iterators.insert({var, vec});
        }
    }

    void from_id_to_str(
        tuple_type& t,
        tuple_str_type& t_str,
        const std::vector<bool>& in_p,
        dict::basic_map& dict_so,
        dict::basic_map& dict_p
    ) {
        for (auto& p : t) {
            if (in_p[p.first]) {
                t_str[p.first] = dict_p.extract(p.second);
            } else {
                t_str[p.first] = dict_so.extract(p.second);
            }
        }
    }

  public:
    const std::vector<IntersectionStats>& get_stats() const { return m_stats; }

    void set_query_id(uint64_t qid) { m_tracer.set_query_id(qid); }

    ltj_algorithm_hash() = default;

    ltj_algorithm_hash(const vector<triple_pattern>* triple_patterns, index_scheme_type* ring) {
        m_ptr_triple_patterns = triple_patterns;
        m_ptr_ring = ring;

        size_type i = 0;
        m_iterators.reserve(m_ptr_triple_patterns->size());
        for (const auto& triple : *m_ptr_triple_patterns) {
            // Bulding iterators
            m_iterators.emplace_back(ltj_iter_type(&triple, m_ptr_ring));
            if (m_iterators[i].is_empty()) {
                m_is_empty = true;
                return;
            }

            // For each variable we add the pointers to its iterators
            if (triple.o_is_variable()) {
                add_var_to_iterator(triple.term_o.value, &(m_iterators[i]));
            }
            if (triple.p_is_variable()) {
                add_var_to_iterator(triple.term_p.value, &(m_iterators[i]));
            }
            if (triple.s_is_variable()) {
                add_var_to_iterator(triple.term_s.value, &(m_iterators[i]));
            }
            ++i;
        }

        m_veo = veo_type(m_ptr_triple_patterns, &m_iterators, &m_var_to_iterators, m_ptr_ring);
    }

    //! Copy constructor
    ltj_algorithm_hash(const ltj_algorithm_hash& o) { copy(o); }

    //! Move constructor
    ltj_algorithm_hash(ltj_algorithm_hash&& o) { *this = move(o); }

    //! Copy Operator=
    ltj_algorithm_hash& operator=(const ltj_algorithm_hash& o) {
        if (this != &o) {
            copy(o);
        }
        return *this;
    }

    //! Move Operator=
    ltj_algorithm_hash& operator=(ltj_algorithm_hash&& o) {
        if (this != &o) {
            m_ptr_triple_patterns = move(o.m_ptr_triple_patterns);
            m_veo = move(o.m_veo);
            m_ptr_ring = move(o.m_ptr_ring);
            m_iterators = move(o.m_iterators);
            m_var_to_iterators = move(o.m_var_to_iterators);
            m_is_empty = o.m_is_empty;
            m_stats = move(o.m_stats);
        }
        return *this;
    }

    void swap(ltj_algorithm_hash& o) {
        std::swap(m_ptr_triple_patterns, o.m_ptr_triple_patterns);
        std::swap(m_veo, o.m_veo);
        std::swap(m_ptr_ring, o.m_ptr_ring);
        std::swap(m_iterators, o.m_iterators);
        std::swap(m_var_to_iterators, o.m_var_to_iterators);
        std::swap(m_is_empty, o.m_is_empty);
        std::swap(m_stats, o.m_stats);
    }

    /**
   *
   * @param res               Results
   * @param limit_results     Limit of results
   * @param timeout_seconds   Timeout in seconds
   */
    template <class results_type>
    void join(/*vector<tuple_type> &res,*/
              results_type& res,
              const size_type limit_results = 0,
              const size_type timeout_seconds = 0
    ) {
        if (m_is_empty)
            return;
        time_point_type start = chrono::high_resolution_clock::now();
        tuple_type t(m_veo.size());
        search(0, t, res, start, limit_results, timeout_seconds);
    };

    /**
   *
   * @param res               Results
   * @param limit_results     Limit of results
   * @param timeout_seconds   Timeout in seconds
   */
    template <class results_type>
    void join_str(/*vector<tuple_type> &res,*/
                  results_type& res,
                  const std::vector<bool>& in_p,
                  dict::basic_map& dict_so,
                  dict::basic_map& dict_p,
                  const size_type limit_results = 0,
                  const size_type timeout_seconds = 0
    ) {
        if (m_is_empty)
            return;
        time_point_type start = chrono::high_resolution_clock::now();
        tuple_type t(m_veo.size());
        tuple_str_type t_str(m_veo.size());
        search_str(0, t, t_str, res, in_p, dict_so, dict_p, start, limit_results, timeout_seconds);
    };

    /**
   *
   * @param j                 Index of the variable
   * @param tuple             Tuple of the current search
   * @param res               Results
   * @param start             Initial time to check timeout
   * @param limit_results     Limit of results
   * @param timeout_seconds   Timeout in seconds
   */
    template <class results_type>
    bool search_str(
        const size_type j,
        tuple_type& tuple,
        tuple_str_type& t_str,
        results_type& res,
        const std::vector<bool>& in_p,
        dict::basic_map& dict_so,
        dict::basic_map& dict_p,
        const time_point_type start,
        const size_type limit_results = 0,
        const size_type timeout_seconds = 0
    ) {
        //(Optional) Check timeout
        if (timeout_seconds > 0) {
            time_point_type stop = chrono::high_resolution_clock::now();
            auto sec = chrono::duration_cast<chrono::seconds>(stop - start).count();
            if (sec > timeout_seconds) {
                return false;
            }
        }

        //(Optional) Check limit
        if (limit_results > 0 && res.size() == limit_results) {
            return false;
        }

        if (j == m_veo.size()) {
            // Report results
            // res.emplace_back(tuple);
            from_id_to_str(tuple, t_str, in_p, dict_so, dict_p);
            res.add(t_str);
        } else {
            var_type x_j = m_veo.next();
            // cout << "Variable: " << (uint64_t) x_j << endl;
            // cout << (uint64_t) x_j << endl;
            vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
            bool ok;
            if (itrs.size() == 1 && itrs[0]->in_last_level()) {  // Lonely variables
                // cout << "Seeking (last level)" << endl;
                auto results = itrs[0]->seek_all(x_j);
                cltj::query::CandidateFrame<TRACE_QUERY, tuple_type> frame(
                    m_tracer, "lonely", j, x_j, tuple
                );
                // cout << "Results: " << results.size() << endl;
                // cout << "Seek (last level): (" << (uint64_t) x_j << ": size=" <<
                // results.size() << ")" <<endl;
                for (const auto& c : results) {
                    // 1. Adding result to tuple
                    tuple[j] = {x_j, c};
                    frame.add(c);
                    // 2. Going down in the trie by setting x_j = c (\mu(t_i) in paper)
                    itrs[0]->down(x_j, c);
                    m_veo.down();
                    // 2. Search with the next variable x_{j+1}
                    ok = search_str(
                        j + 1, tuple, t_str, res, in_p, dict_so, dict_p, start, limit_results, timeout_seconds
                    );
                    if (!ok)
                        return false;
                    // 4. Going up in the trie by removing x_j = c
                    itrs[0]->up(x_j);
                    m_veo.up();
                }
                frame.emit();
            } else {
                // TODO: add hash path here (same as search()) if join_str() needs it
                value_type c = seek(x_j);
                // cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")"
                // <<endl;
                while (c != 0) {  // If empty c=0
                    // 1. Adding result to tuple
                    tuple[j] = {x_j, c};
                    // 2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
                    for (ltj_iter_type* iter : itrs) {
                        iter->down(x_j, c);
                    }
                    m_veo.down();
                    // 3. Search with the next variable x_{j+1}
                    ok = search_str(
                        j + 1, tuple, t_str, res, in_p, dict_so, dict_p, start, limit_results, timeout_seconds
                    );
                    if (!ok)
                        return false;
                    // 4. Going up in the tries by removing x_j = c
                    for (ltj_iter_type* iter : itrs) {
                        iter->up(x_j);
                    }
                    m_veo.up();
                    // 5. Next constant for x_j
                    c = seek(x_j, c + 1);
                    // cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")"
                    // <<endl;
                }
            }
            m_veo.done();
        }
        return true;
    };

    /**
   *
   * @param j                 Index of the variable
   * @param tuple             Tuple of the current search
   * @param res               Results
   * @param start             Initial time to check timeout
   * @param limit_results     Limit of results
   * @param timeout_seconds   Timeout in seconds
   */
    template <class results_type>
    bool search(
        const size_type j,
        tuple_type& tuple,
        results_type& res,
        const time_point_type start,
        const size_type limit_results = 0,
        const size_type timeout_seconds = 0
    ) {
        //(Optional) Check timeout
        if (timeout_seconds > 0) {
            time_point_type stop = chrono::high_resolution_clock::now();
            auto sec = chrono::duration_cast<chrono::seconds>(stop - start).count();
            if (sec > timeout_seconds) {
                return false;
            }
        }

        //(Optional) Check limit
        if (limit_results > 0 && res.size() == limit_results) {
            return false;
        }

        if (j == m_veo.size()) {
            // Report results
            // res.emplace_back(tuple);
            res.add(tuple);
#if EXPT_TIME_SOL
            if (res.size() % 1000 == 0) {
                time_point_type stop = chrono::high_resolution_clock::now();
                auto sec = chrono::duration_cast<chrono::seconds>(stop - start).count();
                std::cerr << res.size() << ";" << sec << std::endl;
            }
#endif
            /*cout << "Add result" << endl;
      for(const auto &dat : tuple){
          cout << "{" << (uint64_t) dat.first << "=" << dat.second << "} ";
      }
      cout << endl;*/
        } else {
            var_type x_j = m_veo.next();
            // cout << "Variable: " << (uint64_t) x_j << endl;
            // cout << (uint64_t) x_j << endl;
            vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
            bool ok;
            if (itrs.size() == 1 && itrs[0]->in_last_level()) {  // Lonely variables
                // cout << "Seeking (last level)" << endl;
                auto results = itrs[0]->seek_all(x_j);
                // cout << "Results: " << results.size() << endl;
                // cout << "Seek (last level): (" << (uint64_t) x_j << ": size=" <<
                // results.size() << ")" <<endl;
                for (const auto& c : results) {
                    // 1. Adding result to tuple
                    tuple[j] = {x_j, c};
                    // 2. Going down in the trie by setting x_j = c (\mu(t_i) in paper)
                    itrs[0]->down(x_j, c);
                    m_veo.down();
                    // 2. Search with the next variable x_{j+1}
                    ok = search(j + 1, tuple, res, start, limit_results, timeout_seconds);
                    if (!ok)
                        return false;
                    // 4. Going up in the trie by removing x_j = c
                    itrs[0]->up(x_j);
                    m_veo.up();
                }
            } else {
                // Compute children sizes
                std::vector<size_type> children_sizes(itrs.size());
                size_type min_idx = 0;
                for (size_type i = 0; i < itrs.size(); ++i) {
                    children_sizes[i] = itrs[i]->children(state_for(itrs[i], x_j));
                    if (children_sizes[i] < children_sizes[min_idx])
                        min_idx = i;
                }

                IntersectionStats stats;
                if constexpr (COLLECT_QUERY_STATS) {
                    stats.variable_id = x_j;
                    stats.depth = j;
                    stats.list_sizes.assign(children_sizes.begin(), children_sizes.end());
                }

                // Partition iterators by whether their current node supports hash.
                std::vector<ltj_iter_type*> sorted_itrs, hashed_itrs;
                for (ltj_iter_type* iter : itrs) {
                    if (iter->current_node_has_hash(x_j))
                        hashed_itrs.push_back(iter);
                    else
                        sorted_itrs.push_back(iter);
                }

                if (sorted_itrs.empty()) {
                    // All iterators hashed: Pure hash path.
                    auto [beg, end_pos] = itrs[min_idx]->children_range();
                    const auto* scan_trie = itrs[min_idx]->resolve_trie();
                    m_tracer.on_pure_hash_frame(j, x_j, children_sizes, min_idx);
                    cltj::query::CandidateFrame<TRACE_QUERY, tuple_type> frame(
                        m_tracer, "hash", j, x_j, tuple
                    );

                    for (size_type pos = beg; pos < end_pos; ++pos) {
                        value_type c = scan_trie->seq[pos];

                        if (candidate_in_hashed_iterators(x_j, c, hashed_itrs, itrs[min_idx])) {
                            if constexpr (COLLECT_QUERY_STATS)
                                stats.result_size++;
                            tuple[j] = {x_j, c};
                            frame.add(c);

                            assert(
                                itrs[min_idx]->resolve_trie() == scan_trie
                                && "min iterator trie changed during hash scan"
                            );
                            // min_iter: position already known from scan, skip binary search
                            itrs[min_idx]->set_level_status(pos, beg, end_pos - beg);
                            position_hashed_iterators(x_j, c, hashed_itrs, itrs[min_idx]);
                            for (ltj_iter_type* iter : itrs) {
                                iter->down(x_j, c);
                            }
                            m_veo.down();
                            ok = search(j + 1, tuple, res, start, limit_results, timeout_seconds);
                            if (!ok)
                                return false;
                            for (ltj_iter_type* iter : itrs) {
                                iter->up(x_j);
                            }
                            m_veo.up();
                        }
                    }
                    for (ltj_iter_type* iter : itrs) {
                        iter->leap_done();
                    }
                    frame.emit();
                } else {
                    // At least one iterator is sorted: leapfrog on sorted_itrs, hash-filter on hashed_itrs.
                    value_type c = seek_on_iterators(x_j, sorted_itrs);
                    cltj::query::CandidateFrame<TRACE_QUERY, tuple_type> frame(
                        m_tracer, "hybrid", j, x_j, tuple
                    );
                    while (c != 0) {  // If empty c=0
                        if (candidate_in_hashed_iterators(x_j, c, hashed_itrs)) {
                            if constexpr (COLLECT_QUERY_STATS)
                                stats.result_size++;
                            tuple[j] = {x_j, c};
                            frame.add(c);
                            position_hashed_iterators(x_j, c, hashed_itrs);
                            for (ltj_iter_type* iter : itrs) {
                                iter->down(x_j, c);
                            }
                            m_veo.down();
                            ok = search(j + 1, tuple, res, start, limit_results, timeout_seconds);
                            if (!ok)
                                return false;
                            for (ltj_iter_type* iter : itrs) {
                                iter->up(x_j);
                            }
                            m_veo.up();
                        }
                        c = seek_on_iterators(x_j, sorted_itrs, c + 1);
                    }
                    for (ltj_iter_type* iter : hashed_itrs) {
                        iter->leap_done();
                    }
                    frame.emit();
                }

                if constexpr (COLLECT_QUERY_STATS) {
                    stats.alternation_complexity = calculate_alternation_complexity(itrs, x_j);
                    m_stats.push_back(stats);
                }
            }
            m_veo.done();
        }
        return true;
    };

    /**
   * @brief Returns which component of the triple pattern variable @p x_j refers to.
   */
    state_type state_for(ltj_iter_type* iter, const var_type x_j) {
        state_type state = o;
        if (iter->is_variable_subject(x_j))
            state = s;
        else if (iter->is_variable_predicate(x_j))
            state = p;
        return state;
    }

    /**
   * @brief Checks whether candidate @p c is contained in all hashed iterators.
   */
    bool candidate_in_hashed_iterators(
        const var_type x_j,
        const value_type c,
        const vector<ltj_iter_type*>& hashed_itrs,
        ltj_iter_type* skip = nullptr
    ) {
        for (ltj_iter_type* iter : hashed_itrs) {
            if (iter == skip)
                continue;
            if (!iter->hash_contains(x_j, c))
                return false;
        }
        return true;
    }

    /**
   * @brief Positions hashed iterators at candidate @p c so down() can descend.
   */
    void position_hashed_iterators(
        const var_type x_j,
        const value_type c,
        const vector<ltj_iter_type*>& hashed_itrs,
        ltj_iter_type* skip = nullptr
    ) {
        for (ltj_iter_type* iter : hashed_itrs) {
            if (iter == skip)
                continue;
            iter->exists(state_for(iter, x_j), c);
        }
    }

    /**
   * @brief Runs leapfrog seek over the provided iterator set for variable @p x_j.
   * @param x_j   Variable being intersected
   * @param itrs  Iterators that participate in the leapfrog
   * @param c     Lower bound candidate. If unknown, use -1 to start from the minimum
   * @return      The next constant in the intersection, or 0 if the intersection is empty
   */
    value_type seek_on_iterators(const var_type x_j, const vector<ltj_iter_type*>& itrs, value_type c = -1) {
        value_type c_i, c_prev = 0, i = 0, n_ok = 0;

        while (true) {
            // Compute leap for each triple that contains x_j
            // std::cout << "Leap of " << (::uint64_t) x_j << " in iterator: " << i <<
            // std::endl;
            if (c == -1) {
                c_i = itrs[i]->leap(x_j);
            } else {
                c_i = itrs[i]->leap(x_j, c);
            }
            // std::cout << "Gets " << (::uint64_t) c_i << std::endl;
            if (c_i == 0) {
                for (auto& itr : itrs) {
                    itr->leap_done();
                }
                return 0;  // Empty intersection
            }
            n_ok = (c_i == c_prev) ? n_ok + 1 : 1;
            if (n_ok == itrs.size()) {
                return c_i;
            }
            c = c_prev = c_i;
            i = (i + 1 == itrs.size()) ? 0 : i + 1;
        }
        for (auto& itr : itrs) {
            itr->leap_done();
        }
    }

    /**
   *
   * @param x_j   Variable
   * @param c     Constant. If it is unknown the value is -1
   * @return      The next constant that matches the intersection between the
   * triples of x_j. If the intersection is empty, it returns 0.
   */

    value_type seek(const var_type x_j, value_type c = -1) {
        vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
        return seek_on_iterators(x_j, itrs, c);
    }

    void print_veo(unordered_map<uint8_t, string>& ht) {
        cout << "veo: ";
        for (uint64_t j = 0; j < m_veo.size(); ++j) {
            cout << "?" << ht[m_veo.next()] << " ";
        }
        cout << endl;
    }

    void print_query(unordered_map<uint8_t, string>& ht) {
        cout << "Query: " << endl;
        for (size_type i = 0; i < m_ptr_triple_patterns->size(); ++i) {
            m_ptr_triple_patterns->at(i).print(ht);
            if (i < m_ptr_triple_patterns->size() - 1) {
                cout << " . ";
            }
        }
        cout << endl;
    }

    void print_results(vector<tuple_type>& res, unordered_map<uint8_t, string>& ht) {
        cout << "Results: " << endl;
        uint64_t i = 1;
        for (tuple_type& tuple : res) {
            cout << "[" << i << "]: ";
            for (pair<var_type, value_type>& pair : tuple) {
                cout << "?" << ht[pair.first] << "=" << pair.second << " ";
            }
            cout << endl;
            ++i;
        }
    }
};
}  // namespace ltj

#endif  // RING_LTJ_ALGORITHM_HASH_HPP
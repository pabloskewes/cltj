/*
 * ltj_algorithm.hpp
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

#ifndef RING_LTJ_ALGORITHM_HPP
#define RING_LTJ_ALGORITHM_HPP

#include <functional>
#include <triple_pattern.hpp>
// #include <ltj_iterator.hpp>
#include <dict/dict_map.hpp>
#include <query/alternation_complexity.hpp>
#include <query/intersection_cache.hpp>
#include <query/intersection_stats.hpp>
#include <query/ltj_iterator_basic.hpp>
#include <query/ltj_iterator_lite.hpp>
#include <query/ltj_iterator_metatrie.hpp>
#include <query/query_tracer.hpp>
#include <results/results.hpp>
#include <util/rdf_util.hpp>
#include <veo/veo_adaptive.hpp>
#include <veo/veo_simple.hpp>
#ifdef LTJ_VEO_TRACE
#include <veo/veo_trace.hpp>
#endif

#define EXPT_TIME_SOL 0

namespace ltj {

using cltj::TRACE_QUERY;

template <
    class iterator_t = ltj_iterator_lite<cltj::compact_ltj, uint8_t, uint64_t>,
    class veo_t = veo::veo_adaptive<iterator_t, util::trait_size>>
class ltj_algorithm {

public:
  typedef uint64_t value_type;
  typedef uint64_t size_type;
  typedef iterator_t ltj_iter_type;
  typedef typename ltj_iter_type::var_type var_type;
  typedef typename ltj_iter_type::index_scheme_type index_scheme_type;
  typedef typename ltj_iter_type::value_type const_type;
  typedef veo_t veo_type;
  typedef unordered_map<var_type, vector<ltj_iter_type *>>
      var_to_iterators_type;
  typedef vector<pair<var_type, const_type>> tuple_type;
  typedef vector<std::string> tuple_str_type;
  typedef chrono::high_resolution_clock::time_point time_point_type;
  // typedef ::util::results_collector<tuple_type> results_type;

private:
  const vector<triple_pattern> *m_ptr_triple_patterns;
  veo_type m_veo;
  index_scheme_type *m_ptr_ring;
  vector<ltj_iter_type> m_iterators;
  var_to_iterators_type m_var_to_iterators;
  bool m_is_empty = false;
  std::vector<IntersectionStats> m_stats;
  cltj::query::QueryTracer<cltj::TRACE_QUERY> m_tracer{"query_traces"};
  std::function<void(const IntersectionStats &)> m_stats_sink;
#ifdef LTJ_ICACHE_HOOKS
  std::vector<uint64_t> m_ic_nodes;
#endif
#ifdef LTJ_ICACHE
  icache::cache_type m_icache;
  // Entry the frame at each depth records into, or nullptr.
  std::vector<icache::entry *> m_ic_fill;
#endif
#ifdef LTJ_ICACHE_STATS
  // Operations of the frame at each depth, without those of the frames below.
  std::vector<uint64_t> m_ic_own;
  std::vector<uint64_t *> m_ic_outer;
  uint64_t m_ic_no_frame_ops = 0;
  uint64_t *m_ic_ops = &m_ic_no_frame_ops;
#endif
#ifdef LTJ_ICACHE_CHECK
  icache::check_type m_ic_check;
#endif

  void copy(const ltj_algorithm &o) {
    m_ptr_triple_patterns = o.m_ptr_triple_patterns;
    m_veo = o.m_veo;
    m_ptr_ring = o.m_ptr_ring;
    m_iterators = o.m_iterators;
    m_var_to_iterators = o.m_var_to_iterators;
    m_is_empty = o.m_is_empty;
    m_stats = o.m_stats;
  }

  inline void
  add_var_to_iterator(const var_type var, ltj_iter_type *ptr_iterator) {
    auto it = m_var_to_iterators.find(var);
    if (it != m_var_to_iterators.end()) {
      it->second.push_back(ptr_iterator);
    } else {
      vector<ltj_iter_type *> vec = {ptr_iterator};
      m_var_to_iterators.insert({var, vec});
    }
  }

  void record_stats(const IntersectionStats &stats) {
    if (m_stats_sink)
      m_stats_sink(stats);
    else
      m_stats.push_back(stats);
  }

  void from_id_to_str(
      tuple_type &t,
      tuple_str_type &t_str,
      const std::vector<bool> &in_p,
      dict::basic_map &dict_so,
      dict::basic_map &dict_p
  ) {
    for (auto &p : t) {
      if (in_p[p.first]) {
        t_str[p.first] = dict_p.extract(p.second);
      } else {
        t_str[p.first] = dict_so.extract(p.second);
      }
    }
  }

public:
  const std::vector<IntersectionStats> &get_stats() const {
    return m_stats;
  }

  /// Sends each record to @p sink instead of keeping it in get_stats().
  void set_stats_sink(std::function<void(const IntersectionStats &)> sink) {
    m_stats_sink = std::move(sink);
  }

  void set_query_id(uint64_t qid) {
    m_tracer.set_query_id(qid);
  }

  ltj_algorithm() = default;

  ltj_algorithm(
      const vector<triple_pattern> *triple_patterns,
      index_scheme_type *ring
  ) {

    m_ptr_triple_patterns = triple_patterns;
    m_ptr_ring = ring;

    size_type i = 0;
    m_iterators.reserve(m_ptr_triple_patterns->size());
    for (const auto &triple : *m_ptr_triple_patterns) {
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

    m_veo = veo_type(
        m_ptr_triple_patterns, &m_iterators, &m_var_to_iterators, m_ptr_ring
    );
#ifdef LTJ_ICACHE
    for (const auto &[x, itrs] : m_var_to_iterators)
      m_icache.add_variable(x, itrs);
#endif
  }

  //! Copy constructor
  ltj_algorithm(const ltj_algorithm &o) {
    copy(o);
  }

  //! Move constructor
  ltj_algorithm(ltj_algorithm &&o) {
    *this = move(o);
  }

  //! Copy Operator=
  ltj_algorithm &operator=(const ltj_algorithm &o) {
    if (this != &o) {
      copy(o);
    }
    return *this;
  }

  //! Move Operator=
  ltj_algorithm &operator=(ltj_algorithm &&o) {
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

  void swap(ltj_algorithm &o) {
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
            results_type &res,
            const size_type limit_results = 0,
            const size_type timeout_seconds = 0
  ) {
    if (m_is_empty)
      return;
    time_point_type start = chrono::high_resolution_clock::now();
    tuple_type t(m_veo.size());
#ifdef LTJ_ICACHE
    m_ic_fill.assign(m_veo.size(), nullptr);
#endif
#ifdef LTJ_ICACHE_STATS
    m_ic_own.assign(m_veo.size(), 0);
    m_ic_outer.assign(m_veo.size(), nullptr);
    m_ic_ops = &m_ic_no_frame_ops;
#endif
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
                results_type &res,
                const std::vector<bool> &in_p,
                dict::basic_map &dict_so,
                dict::basic_map &dict_p,
                const size_type limit_results = 0,
                const size_type timeout_seconds = 0
  ) {
    if (m_is_empty)
      return;
    time_point_type start = chrono::high_resolution_clock::now();
    tuple_type t(m_veo.size());
    tuple_str_type t_str(m_veo.size());
    search_str(
        0, t, t_str, res, in_p, dict_so, dict_p, start, limit_results,
        timeout_seconds
    );
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
      tuple_type &tuple,
      tuple_str_type &t_str,
      results_type &res,
      const std::vector<bool> &in_p,
      dict::basic_map &dict_so,
      dict::basic_map &dict_p,
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
#ifdef LTJ_VEO_TRACE
      veo::trace::record(tuple, j, x_j);
#endif
      // cout << "Variable: " << (uint64_t) x_j << endl;
      // cout << (uint64_t) x_j << endl;
      vector<ltj_iter_type *> &itrs = m_var_to_iterators[x_j];
      bool ok;
      if (itrs.size() == 1 && itrs[0]->in_last_level()) { // Lonely variables
        // cout << "Seeking (last level)" << endl;
        auto results = itrs[0]->seek_all(x_j);
        cltj::query::CandidateFrame<TRACE_QUERY, tuple_type> frame(
            m_tracer, "lonely", j, x_j, tuple);
        // cout << "Results: " << results.size() << endl;
        // cout << "Seek (last level): (" << (uint64_t) x_j << ": size=" <<
        // results.size() << ")" <<endl;
        for (const auto &c : results) {
          // 1. Adding result to tuple
          tuple[j] = {x_j, c};
          frame.add(c);
          // 2. Going down in the trie by setting x_j = c (\mu(t_i) in paper)
          itrs[0]->down(x_j, c);
          m_veo.down();
          // 2. Search with the next variable x_{j+1}
          ok = search_str(
              j + 1, tuple, t_str, res, in_p, dict_so, dict_p, start,
              limit_results, timeout_seconds
          );
          if (!ok)
            return false;
          // 4. Going up in the trie by removing x_j = c
          itrs[0]->up(x_j);
          m_veo.up();
        }
        frame.emit();
      } else {
        value_type c = seek(x_j);
        // cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")"
        // <<endl;
        while (c != 0) { // If empty c=0
          // 1. Adding result to tuple
          tuple[j] = {x_j, c};
          // 2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
          for (ltj_iter_type *iter : itrs) {
            iter->down(x_j, c);
          }
          m_veo.down();
          // 3. Search with the next variable x_{j+1}
          ok = search_str(
              j + 1, tuple, t_str, res, in_p, dict_so, dict_p, start,
              limit_results, timeout_seconds
          );
          if (!ok)
            return false;
          // 4. Going up in the tries by removing x_j = c
          for (ltj_iter_type *iter : itrs) {
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
      tuple_type &tuple,
      results_type &res,
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
#ifdef LTJ_ICACHE_CHECK
      ++m_ic_check.results;
      for (const auto &tv : tuple) {
        icache::mix(m_ic_check.result_hash, tv.first);
        icache::mix(m_ic_check.result_hash, tv.second);
      }
#endif
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
#ifdef LTJ_VEO_TRACE
      veo::trace::record(tuple, j, x_j);
#endif
      // cout << "Variable: " << (uint64_t) x_j << endl;
      // cout << (uint64_t) x_j << endl;
      vector<ltj_iter_type *> &itrs = m_var_to_iterators[x_j];
      bool ok;
      if (itrs.size() == 1 && itrs[0]->in_last_level()) { // Lonely variables
        // cout << "Seeking (last level)" << endl;
        auto results = itrs[0]->seek_all(x_j);
        // cout << "Results: " << results.size() << endl;
        // cout << "Seek (last level): (" << (uint64_t) x_j << ": size=" <<
        // results.size() << ")" <<endl;
        for (const auto &c : results) {
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
#ifdef LTJ_ICACHE_HOOKS
        if (const icache::entry *e = icache_open(j, tuple, x_j, itrs)) {
          if (!icache_replay(
                  *e, j, tuple, x_j, itrs, res, start, limit_results,
                  timeout_seconds
              ))
            return false;
          m_veo.done();
          return true;
        }
#endif
        IntersectionStats stats;
        if constexpr (COLLECT_QUERY_STATS) {
          stats.variable_id = x_j;
          stats.depth = j;
          for (ltj_iter_type *iter : itrs) {
            state_type state = o;
            if (iter->is_variable_subject(x_j)) {
              state = s;
            } else if (iter->is_variable_predicate(x_j)) {
              state = p;
            }
            stats.list_sizes.push_back(iter->children(state));
          }
        }

        value_type c = seek(x_j);
        cltj::query::CandidateFrame<TRACE_QUERY, tuple_type> frame(
            m_tracer, "leapfrog", j, x_j, tuple);
        // cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")"
        // <<endl;
        while (c != 0) { // If empty c=0
          if constexpr (COLLECT_QUERY_STATS) stats.result_size++;

          // 1. Adding result to tuple
          tuple[j] = {x_j, c};
          frame.add(c);
#ifdef LTJ_ICACHE_HOOKS
          icache_record(j, x_j, c, itrs);
#endif
          // 2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
          for (ltj_iter_type *iter : itrs) {
            iter->down(x_j, c);
          }
          m_veo.down();
          // 3. Search with the next variable x_{j+1}
          ok = search(j + 1, tuple, res, start, limit_results, timeout_seconds);
          if (!ok)
            return false;
          // 4. Going up in the tries by removing x_j = c
          for (ltj_iter_type *iter : itrs) {
            iter->up(x_j);
          }
          m_veo.up();
          // 5. Next constant for x_j
          c = seek(x_j, c + 1);
          // cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")"
          // <<endl;
        }
        frame.emit();
#ifdef LTJ_ICACHE
        icache_close(j);
#endif

        if constexpr (COLLECT_QUERY_STATS) {
          stats.alternation_complexity =
              calculate_alternation_complexity(itrs, x_j);
          record_stats(stats);
        }
      }
      m_veo.done();
    }
    return true;
  };

  /**
   *
   * @param x_j   Variable
   * @param c     Constant. If it is unknown the value is -1
   * @return      The next constant that matches the intersection between the
   * triples of x_j. If the intersection is empty, it returns 0.
   */

  value_type seek(const var_type x_j, value_type c = -1) {
    vector<ltj_iter_type *> &itrs = m_var_to_iterators[x_j];
    value_type c_i, c_prev = 0, i = 0, n_ok = 0;

    while (true) {
      // Compute leap for each triple that contains x_j
      // std::cout << "Leap of " << (::uint64_t) x_j << " in iterator: " << i <<
      // std::endl;
      if (c == -1) {
        c_i = itrs[i]->leap(x_j);
      } else {
        c_i = itrs[i]->leap(x_j, c);
        LTJ_ICACHE_OP(1);
      }
      // std::cout << "Gets " << (::uint64_t) c_i << std::endl;
      if (c_i == 0) {
        for (auto &itr : itrs) {
          itr->leap_done();
        }
        return 0; // Empty intersection
      }
      n_ok = (c_i == c_prev) ? n_ok + 1 : 1;
      if (n_ok == itrs.size()) {
        return c_i;
      }
      c = c_prev = c_i;
      i = (i + 1 == itrs.size()) ? 0 : i + 1;
    }
    for (auto &itr : itrs) {
      itr->leap_done();
    }
  }

#ifdef LTJ_ICACHE_HOOKS
  // The entry whose intersection the frame at depth j replays, or nullptr
  // after preparing the frame to run.
  const icache::entry *icache_open(
      const size_type j,
      const tuple_type &tuple,
      const var_type x_j,
      const vector<ltj_iter_type *> &itrs
  ) {
#ifdef LTJ_ICACHE
    m_ic_fill[j] = nullptr;
#ifdef LTJ_ICACHE_STATS
    ++m_icache.stats.frames;
    m_ic_outer[j] = m_ic_ops;
    m_ic_own[j] = 0;
    m_ic_ops = &m_ic_own[j];
#endif
    if (!m_icache.eligible(x_j, j ? int64_t(tuple[j - 1].first) : -1))
      return nullptr;
    m_ic_nodes.resize(itrs.size());
    for (size_type i = 0; i < itrs.size(); ++i)
      m_ic_nodes[i] = itrs[i]->icache_node(x_j);
    const icache::entry *e =
        m_icache.open(x_j, m_ic_nodes.data(), itrs.size(), &m_ic_fill[j]);
#ifdef LTJ_ICACHE_STATS
    if (e) {
      m_ic_ops = m_ic_outer[j];
      m_icache.stats.served += e->size();
      m_icache.stats.hit_lists += itrs.size();
      m_icache.stats.ops_saved += e->ops;
    }
#endif
    return e;
#else
    return nullptr;
#endif
  }

  // Called with every iterator of the frame positioned at c, before down().
  void icache_record(
      const size_type j,
      const var_type x_j,
      const value_type c,
      const vector<ltj_iter_type *> &itrs
  ) {
#ifdef LTJ_ICACHE_CHECK
    icache_check(j, x_j, c, itrs);
#endif
#ifdef LTJ_ICACHE
    icache::entry *e = m_ic_fill[j];
    if (!e)
      return;
    uint64_t *rec = e->push();
    if (!rec)
      return;
    if (e->size() == 1)
      for (size_type i = 0; i < itrs.size(); ++i)
        itrs[i]->icache_level(e->level() + 2 * i);
    rec[0] = c;
    for (size_type i = 0; i < itrs.size(); ++i)
      rec[1 + i] = itrs[i]->icache_pos();
#endif
  }

#ifdef LTJ_ICACHE
  // Only a frame that was not cut reaches this, so a cut frame never publishes.
  void icache_close(const size_type j) {
    icache::entry *e = m_ic_fill[j];
#ifdef LTJ_ICACHE_STATS
    m_icache.stats.ops_done += m_ic_own[j];
    m_ic_ops = m_ic_outer[j];
    if (e)
      e->ops = m_ic_own[j];
#endif
    if (e)
      m_icache.publish(e);
  }
#endif

  // Same calls as the leapfrog loop of search(), with the positions taken
  // from the entry.
  template <class results_type>
  bool icache_replay(
      const icache::entry &e,
      const size_type j,
      tuple_type &tuple,
      const var_type x_j,
      const vector<ltj_iter_type *> &itrs,
      results_type &res,
      const time_point_type start,
      const size_type limit_results,
      const size_type timeout_seconds
  ) {
    if (e.size())
      for (size_type i = 0; i < itrs.size(); ++i)
        itrs[i]->icache_restore_level(e.level() + 2 * i);
    for (uint64_t r = 0; r < e.size(); ++r) {
      const uint64_t *rec = e.result(r);
      const value_type c = rec[0];
      tuple[j] = {x_j, c};
      for (size_type i = 0; i < itrs.size(); ++i)
        itrs[i]->icache_restore(x_j, rec[1 + i]);
#ifdef LTJ_ICACHE_CHECK
      icache_check(j, x_j, c, itrs);
#endif
      for (ltj_iter_type *iter : itrs)
        iter->down(x_j, c);
      m_veo.down();
      if (!search(j + 1, tuple, res, start, limit_results, timeout_seconds))
        return false;
      for (ltj_iter_type *iter : itrs)
        iter->up(x_j);
      m_veo.up();
    }
    for (ltj_iter_type *iter : itrs)
      iter->leap_done();
#ifdef LTJ_ICACHE
    m_icache.release(&e);
#endif
    return true;
  }
#endif

#ifdef LTJ_ICACHE_CHECK
  void icache_check(
      const size_type j,
      const var_type x_j,
      const value_type c,
      const vector<ltj_iter_type *> &itrs
  ) {
    ++m_ic_check.positioned;
    icache::mix(m_ic_check.state_hash, j);
    icache::mix(m_ic_check.state_hash, x_j);
    icache::mix(m_ic_check.state_hash, c);
    for (const ltj_iter_type *iter : itrs)
      iter->icache_digest(m_ic_check.state_hash);
  }

  const icache::check_type &icache_check_result() const {
    return m_ic_check;
  }
#endif

#ifdef LTJ_ICACHE_STATS
  const icache::stats_type &icache_stats() const {
    return m_icache.stats;
  }
#endif

  void print_veo(unordered_map<uint8_t, string> &ht) {
    cout << "veo: ";
    for (uint64_t j = 0; j < m_veo.size(); ++j) {
      cout << "?" << ht[m_veo.next()] << " ";
    }
    cout << endl;
  }

  void print_query(unordered_map<uint8_t, string> &ht) {
    cout << "Query: " << endl;
    for (size_type i = 0; i < m_ptr_triple_patterns->size(); ++i) {
      m_ptr_triple_patterns->at(i).print(ht);
      if (i < m_ptr_triple_patterns->size() - 1) {
        cout << " . ";
      }
    }
    cout << endl;
  }

  void
  print_results(vector<tuple_type> &res, unordered_map<uint8_t, string> &ht) {
    cout << "Results: " << endl;
    uint64_t i = 1;
    for (tuple_type &tuple : res) {
      cout << "[" << i << "]: ";
      for (pair<var_type, value_type> &pair : tuple) {
        cout << "?" << ht[pair.first] << "=" << pair.second << " ";
      }
      cout << endl;
      ++i;
    }
  }
};
} // namespace ltj

#endif // RING_LTJ_ALGORITHM_HPP
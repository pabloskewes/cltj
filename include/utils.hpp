#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <set>
#include "ring.hpp"


namespace ring {

    namespace util {

        //TODO: refacer esto para que evite os forward steps
        template<class Ring>
        uint64_t get_size_interval(const triple_pattern &triple_pattern, Ring* ptr_ring) {
            if (triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {
                bwt_interval open_interval = ptr_ring->open_SPO();
                return open_interval.size();
            } else if (triple_pattern.s_is_variable() && !triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {
                bwt_interval open_interval = ptr_ring->open_PSO();
                auto cur_p = ptr_ring->next_P(open_interval, triple_pattern.term_p.value);
                if (cur_p != triple_pattern.term_p.value) {
                    return 0;
                }
                bwt_interval i_p = ptr_ring->down_P(cur_p);
                return i_p.size();
            } else if (triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && !triple_pattern.o_is_variable()) {
                bwt_interval open_interval = ptr_ring->open_OSP();
                auto cur_o = ptr_ring->next_O(open_interval, triple_pattern.term_o.value);
                if (cur_o != triple_pattern.term_o.value) {
                    return 0;
                }
                bwt_interval i_s = ptr_ring->down_O(cur_o);
                return i_s.size();
            } else if (!triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {
                bwt_interval open_interval = ptr_ring->open_SPO();
                auto cur_s = ptr_ring->next_S(open_interval, triple_pattern.term_s.value);
                if (cur_s != triple_pattern.term_s.value) {
                    return 0;
                }
                bwt_interval i_p = ptr_ring->down_S(cur_s);
                return i_p.size();

            } else if (!triple_pattern.s_is_variable() && !triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {

                //Interval in P
                bwt_interval open_interval = ptr_ring->open_PSO();
                auto p_aux = ptr_ring->next_P(open_interval, triple_pattern.term_p.value);
                //Is the constant of S in m_i_s?
                if (p_aux != triple_pattern.term_p.value) {
                    return 0;
                }

                //Interval in S
                bwt_interval i_s = ptr_ring->down_P(p_aux);
                auto s_aux = ptr_ring->next_S_in_P(i_s, triple_pattern.term_s.value);
                //Is the constant of O in m_i_o?
                if (s_aux != triple_pattern.term_s.value) {
                    return 0;
                }
                bwt_interval i_o = ptr_ring->down_P_S(i_s, s_aux);
                return i_o.size();

            } else if (!triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && !triple_pattern.o_is_variable()) {
                bwt_interval open_interval = ptr_ring->open_SOP();
                auto cur_s = ptr_ring->next_S(open_interval, triple_pattern.term_s.value);
                if (cur_s != triple_pattern.term_s.value) {
                    return 0;
                }
                bwt_interval i_o = ptr_ring->down_S(cur_s);
                auto cur_o = ptr_ring->next_O_in_S(i_o, triple_pattern.term_o.value);
                if (cur_o != triple_pattern.term_o.value) {
                    return 0;
                }
                bwt_interval i_p = ptr_ring->down_S_O(i_o, cur_o);
                return i_p.size();

            } else if (triple_pattern.s_is_variable() && !triple_pattern.p_is_variable() && !triple_pattern.o_is_variable()) {


                //Interval in O
                bwt_interval open_interval = ptr_ring->open_OPS();
                auto o_aux = ptr_ring->next_O(open_interval, triple_pattern.term_o.value);
                //Is the constant of S in m_i_s?
                if (o_aux != triple_pattern.term_o.value) {
                    return 0;
                }

                //Interval in P
                bwt_interval i_p = ptr_ring->down_O(o_aux);
                auto p_aux = ptr_ring->next_P_in_O(i_p, triple_pattern.term_p.value);
                //Is the constant of O in m_i_o?
                if (p_aux != triple_pattern.term_p.value) {
                    return 0;
                }
                bwt_interval i_s = ptr_ring->down_O_P(i_p, p_aux);
                return i_s.size();
            }
            return 0;
        }
    }


    template<class Iterator>
    uint64_t get_size_interval(const Iterator &iter) {
        if(iter.cur_s == -1 && iter.cur_p == -1 && iter.cur_o == -1){
            return iter.i_s.size();
        } else if (iter.cur_s == -1 && iter.cur_p != -1 && iter.cur_o == -1) {
            return iter.i_s.size(); //i_s = i_o
        } else if (iter.cur_s == -1 && iter.cur_p == -1 && iter.cur_o != -1) {
            return iter.i_s.size(); //i_s = i_p
        } else if (iter.cur_s != -1 && iter.cur_p == -1 && iter.cur_o == -1) {
            return iter.i_o.size(); //i_o = i_p
        } else if (iter.cur_s != -1 && iter.cur_p != -1 && iter.cur_o == -1) {
            return iter.i_o.size();
        } else if (iter.cur_s != -1 && iter.cur_p == -1 && iter.cur_o != -1) {
            return iter.i_p.size();
        } else if (iter.cur_s == -1 && iter.cur_p != -1 && iter.cur_o != -1) {
            return iter.i_s.size();
        }
        return 0;
    }
}


template<class Ring>
uint64_t get_size_interval(ring::triple_pattern &triple_pattern, Ring &graph) {
    if (triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {
        ring::bwt_interval open_interval = graph.open_SPO();
        return open_interval.size();
    } else if (triple_pattern.s_is_variable() && !triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {
        ring::bwt_interval open_interval = graph.open_PSO();
        auto cur_p = graph.next_P(open_interval, triple_pattern.term_p.value);
        if (cur_p == 0 || cur_p != triple_pattern.term_p.value) {
            return 0;
        } else{
            ring::bwt_interval i_p = graph.down_P(cur_p);
            return i_p.size();
        }
    } else if (triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && !triple_pattern.o_is_variable()) {
        ring::bwt_interval open_interval = graph.open_OSP();
        auto cur_o = graph.next_O(open_interval, triple_pattern.term_o.value);
        if (cur_o == 0 || cur_o != triple_pattern.term_o.value) {
            return 0;
        } else{
            ring::bwt_interval i_s = graph.down_O(cur_o);
            return i_s.size();
        }
    } else if (!triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {
        ring::bwt_interval open_interval = graph.open_SPO();
        auto cur_s = graph.next_S(open_interval, triple_pattern.term_s.value);
        if (cur_s == 0 || cur_s != triple_pattern.term_s.value) {
            return 0;
        } else{
            ring::bwt_interval i_s = graph.down_S(cur_s);
            return i_s.size();
        }
    } else if (!triple_pattern.s_is_variable() && !triple_pattern.p_is_variable() && triple_pattern.o_is_variable()) {
        ring::bwt_interval open_interval = graph.open_SPO();
        auto cur_s = graph.next_S(open_interval, triple_pattern.term_s.value);
        if (cur_s == 0 || cur_s != triple_pattern.term_s.value) {
            return 0;
        } else{
            ring::bwt_interval i_s = graph.down_S(cur_s);
            auto cur_p = graph.next_P_in_S(i_s, cur_s, triple_pattern.term_p.value);
            if (cur_p == 0 || cur_p != triple_pattern.term_p.value) {
              return 0;
            }
            ring::bwt_interval i_p = graph.down_S_P(i_s, cur_s, cur_p);
            return i_p.size();
        }
    } else if (!triple_pattern.s_is_variable() && triple_pattern.p_is_variable() && !triple_pattern.o_is_variable()) {
        ring::bwt_interval open_interval = graph.open_SOP();
        auto cur_s = graph.next_S(open_interval, triple_pattern.term_s.value);
        if (cur_s == 0 || cur_s != triple_pattern.term_s.value) {
            return 0;
        } else{
            ring::bwt_interval i_s = graph.down_S(cur_s);
            auto cur_o = graph.next_O_in_S(i_s, triple_pattern.term_o.value);
            if (cur_o == 0 || cur_o != triple_pattern.term_o.value) {
              return 0;
            }
            ring::bwt_interval i_o = graph.down_S_O(i_s, cur_o);
            return i_o.size();
        }
    } else if (triple_pattern.s_is_variable() && !triple_pattern.p_is_variable() && !triple_pattern.o_is_variable()) {
        ring::bwt_interval open_interval = graph.open_POS();
        auto cur_p = graph.next_P(open_interval, triple_pattern.term_p.value);
        if (cur_p == 0 || cur_p != triple_pattern.term_p.value) {
            return 0;
        } else{
            ring::bwt_interval i_p = graph.down_P(cur_p);
            auto cur_o = graph.next_O_in_P(i_p, cur_p, triple_pattern.term_o.value);
            if (cur_o == 0 || cur_o != triple_pattern.term_o.value) {
              return 0;
            }
            ring::bwt_interval i_o = graph.down_P_O(i_p, cur_p, cur_o);
            return i_o.size();
        }
    }
    return 0;
}

bool compare_by_second(pair<uint8_t, uint64_t> a, pair<uint8_t, uint64_t> b) {
    return a.second < b.second;
}

// Cambiar retorno
template<class Ring>
vector<uint8_t> get_gao_min_gen(vector<ring::triple_pattern> &query, Ring &graph) {
    std::map<uint8_t , vector<uint64_t>> triple_values;
    std::map<uint8_t, vector<ring::triple_pattern*>> triples_var;
    for (auto& triple_pattern : query) { //TODO: esto está ben (solo refactoring)
        uint64_t triple_size = get_size_interval(triple_pattern, graph);
        if (triple_pattern.s_is_variable()) {
          triple_values[(uint8_t) triple_pattern.term_s.value].push_back(triple_size);
          triples_var[(uint8_t) triple_pattern.term_s.value].push_back(&triple_pattern);
        }
        if (triple_pattern.p_is_variable()) {
          triple_values[(uint8_t) triple_pattern.term_p.value].push_back(triple_size);
          triples_var[(uint8_t) triple_pattern.term_p.value].push_back(&triple_pattern);
        }
        if (triple_pattern.o_is_variable()) {
          triple_values[(uint8_t) triple_pattern.term_o.value].push_back(triple_size);
          triples_var[(uint8_t) triple_pattern.term_o.value].push_back(&triple_pattern);
        }
    }

    vector<uint8_t> gao;
    vector<pair<uint8_t, uint64_t>> varmin_pairs;
    vector<uint8_t> single_vars;
    map<uint8_t, bool> selectable_vars;
    map<uint8_t, bool> selected_vars;
    map<uint8_t , set<uint8_t>> related_vars;

    for(auto it = (triples_var).cbegin(); it != (triples_var).cend(); ++it) {
        for (auto triple_pattern : triples_var[it->first]) {
            if (triple_pattern->s_is_variable() && it->first == (uint8_t) triple_pattern->term_s.value) {
                related_vars[it->first].insert((uint8_t) triple_pattern->term_s.value);
            }
            if (triple_pattern->p_is_variable() && it->first == (uint8_t) triple_pattern->term_p.value) {
                related_vars[it->first].insert((uint8_t) triple_pattern->term_p.value);
            }
            if (triple_pattern->o_is_variable() && it->first == (uint8_t) triple_pattern->term_o.value) {
                related_vars[it->first].insert((uint8_t) triple_pattern->term_o.value);
            }
        }    
    }


    for(auto it = (triple_values).cbegin(); it != (triple_values).cend(); ++it) {
        if (triple_values[it->first].size() == 1) {
            single_vars.push_back(it->first);
        } else {
            varmin_pairs.push_back(pair<uint8_t, uint64_t>(it->first, *min_element(triple_values[it->first].begin(),
                                                                                   triple_values[it->first].end())));
            selectable_vars[it->first] = false;
        }
    }
    

    sort((varmin_pairs).begin(), (varmin_pairs).end(), compare_by_second);
    if (varmin_pairs.size() > 0) {
        selectable_vars[varmin_pairs[0].first] = true;
    }
    for (int i = 0; i < varmin_pairs.size(); i++) {
        for (pair<uint8_t, uint64_t> varmin_pair : varmin_pairs) {
            if (selectable_vars[varmin_pair.first] && !selected_vars[varmin_pair.first]) {
                gao.push_back(varmin_pair.first);
                selected_vars[varmin_pair.first] = true;
                for (set<uint8_t>::iterator it=related_vars[varmin_pair.first].begin(); it!=related_vars[varmin_pair.first].end(); ++it) {
                    selectable_vars[*it] = true;
                }
                break;
            }
        }
    }

    for (pair<uint8_t, uint64_t> varmin_pair : varmin_pairs) {
        if (!selected_vars[varmin_pair.first]) {
            gao.push_back(varmin_pair.first);
        }
    }

    for (uint8_t s : single_vars) {
        gao.push_back(s);
    }

    return gao;

}



#endif 

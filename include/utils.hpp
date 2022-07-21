#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <set>
#include "Triple.h"
#include "ring.hpp"

uint64_t get_size_interval(ring::triple_pattern &triple_pattern, ring::ring & graph) {
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

bool compare_by_second(pair<string, int> a, pair<string, int> b) {
    return a.second < b.second;
}

// Cambiar retorno
vector<uint8_t> get_gao_min_gen(vector<ring::triple_pattern> &query, ring::ring &graph) {
    std::map<uint8_t , vector<uint64_t>> triple_values;
    std::map<uint8_t, vector<ring::triple_pattern*>> triples_var;
    for (auto& triple_pattern : query) {
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
            varmin_pairs.push_back(pair<uint8_t, uint64_t>(it->first, *min_element(triple_values[it->first].begin(), triple_values[it->first].end())));
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

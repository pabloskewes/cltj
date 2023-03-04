/*
 * ltj_iterator.hpp
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

#ifndef RING_LTJ_ITERATOR_BASE_HPP
#define RING_LTJ_ITERATOR_BASE_HPP

#define VERBOSE 0

#include <bwt_interval.hpp>
#include <triple_pattern.hpp>
#include <vector>

namespace ring {

    template<class var_t, class cons_t>
    class ltj_iterator_base {

    public:
        typedef cons_t value_type;
        typedef var_t var_type;
        typedef uint64_t size_type;

        //std::vector<value_type> leap_result_type;

    public:
        virtual bool is_empty() = 0;

        virtual void down(var_type var, size_type c) = 0;
        virtual void down(var_type var, size_type c, size_type k) = 0;

        virtual void up(var_type var) = 0;

        virtual value_type leap(var_type var) = 0;

        virtual value_type leap(var_type var, size_type c) = 0;

        virtual bool in_last_level() = 0;
        //Solo funciona en último nivel, en otro caso habría que reajustar
        virtual std::vector<uint64_t> seek_all(var_type var) = 0;

        virtual bool is_variable_subject(var_type var) = 0;
        virtual bool is_variable_predicate(var_type var) = 0;
        virtual bool is_variable_object(var_type var) = 0;

    };

}

#endif //RING_LTJ_ITERATOR_BASE_HPP
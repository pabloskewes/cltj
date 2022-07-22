/***
BSD 2-Clause License

Copyright (c) 2018, Adrián
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/


//
// Created by Adrián on 19/7/22.
//

#ifndef RING_TRIPLE_PATTERN_HPP
#define RING_TRIPLE_PATTERN_HPP


namespace ring {



    struct term_pattern {
        uint64_t value; //TODO: transform char of variable to uint64_t
        bool is_variable;
    };

    struct triple_pattern {
        term_pattern term_s;
        term_pattern term_p;
        term_pattern term_o;

        void const_s(uint64_t s){
            term_s.is_variable = false;
            term_s.value = s;
        }

        void const_o(uint64_t o){
            term_o.is_variable = false;
            term_o.value = o;
        }

        void const_p(uint64_t p){
            term_p.is_variable = false;
            term_p.value = p;
        }

        void var_s(uint64_t s){
            term_s.is_variable = true;
            term_s.value = s;
        }

        void var_o(uint64_t o){
            term_o.is_variable = true;
            term_o.value = o;
        }

        void var_p(uint64_t p){
            term_p.is_variable = true;
            term_p.value = p;
        }

        bool s_is_variable() const {
            return term_s.is_variable;
        }

        bool p_is_variable() const {
            return term_p.is_variable;
        }

        bool o_is_variable() const {
            return term_o.is_variable;
        }
    };
}


#endif //RING_TRIPLE_PATTERN_HPP

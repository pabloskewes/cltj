#ifndef CLTJ_TERM_H
#define CLTJ_TERM_H

#include <string>
#include <iostream>

namespace cltj{
    using namespace std;
    class Term {
        public:
        // private: 
            bool is_variable;
            string varname;
            uint32_t constant;
        // public:
            /*
                Empty constructor
            */
            Term();

            ~Term(){};

            /*
                Constructor for constant terms
            */
            Term(uint32_t c){
                is_variable = false;
                constant = c;
            }

            /*
                Constructor for variables
            */
            Term(string v){
                is_variable = true;
                varname = v;
            }

            /*
                Prints the values asociated with this term
            */
            void getValues(){
                if(is_variable)cout<<varname;
                else cout<<constant;
            }

            /*
                Returns true if the term is a variable
            */
            bool isVariable(){
                return is_variable;
            }

            /*
                Returns value asociated to the constant variable.
                Before calling it, it should be checked that the term is not a variable
            */
            uint32_t getConstant(){
                return constant;
            }
            
            /*
                Returns the variable name of the current term 
                Before calling it, it should be checked that the term is a variable
            */
            string getVariable(){
                return varname;
            }
    };
}

#endif 

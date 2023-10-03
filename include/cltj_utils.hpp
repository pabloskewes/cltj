#ifndef CLTJ_UTILS_H
#define CLTJ_UTILS_H

#include <fstream>
#include <sstream> 
#include <set>
#include <map>
#include <cltj_term.hpp>
#include <cltj_tuple.hpp>
#include <algorithm>

namespace cltj {
    using namespace std;
    bool onlySpaces(string &s){
        for(auto c : s){
            if(c!=' ')return false;
        }
        return true;
    }
    /*
        Parses string (line) by a single char (delimiter)
        Returns vector with all the parts of the parsed string 
    */
    vector<string> parse(string line, char delimiter){
        uint32_t first = 0;
        vector<string> results;

        for(int i=0; i<line.size(); i++){
            if(line[i] == delimiter && first!=i){
                results.push_back(line.substr(first, i-first));
                first = i+1;
            }
        }

        if(first !=line.size()){
            string last_bit = line.substr(first, line.size()-first);
            if(!onlySpaces(last_bit))results.push_back(last_bit);
        }

        return results;
    }

    /*
        Removes leading and trailing characters that are equal to to_cut
    */
    string trim(string &s, char to_cut){
        uint32_t beg=0, end=s.size();
        for(int i=0; i<s.size(); i++){
            if(s[i]==to_cut)beg++;
            else break;
        }
        for(int i=s.size()-1; i>=0; i--){
            if(s[i]==to_cut)end--;
            else break;
        }
        return s.substr(beg, end-beg);
    }

    /*
        Recives contents of file and saves each line in a vector of strings
    */
    bool get_file_content(string filename, vector<string> & vector_of_strings)
    {
        // Open the File
        ifstream in(filename.c_str());
        // Check if object is valid
        if(!in)
        {
            cerr << "Cannot open the File : " << filename << endl;
            return false;
        }
        string str;
        // Read the next line from File untill it reaches the end.
        while (getline(in, str))
        {

            // Line contains string of length > 0 then save it in vector
            if(str.size() > 0) vector_of_strings.push_back(str);
            
        }
        //Close The File
        in.close();
        return true;
    }

    /*
        Checks that all elements in s are digits, in turn discovering if s is a number
    */
    bool is_number(string & s)
    {
        return !s.empty() && find_if(s.begin(),
            s.end(), [](unsigned char c) { return !isdigit(c); }) == s.end();
    }

    /*
        Returns a tuple consisting of the Terms that are found in s. 
        Also adds the new variables to the variables mapping
    */
    Tuple get_tuple(string &sa, map<string, set<uint32_t>> & variable_tuple_mapping, uint32_t &i, std::vector<std::string> &processed_vars){
        vector<string> terms_string = parse(trim(sa, ' '), ' ');
        vector<Term> new_terms;
        uint32_t value;

        for(auto s: terms_string){
            // cout<<"term "<<terms_string[i]<<endl;
            if(is_number(s)){
                istringstream iss(s);
                iss >> value;
                new_terms.push_back(Term(value));
            }
            else{
                s = s.substr(1);
                if(variable_tuple_mapping.find(s) == variable_tuple_mapping.end()){
                    processed_vars.emplace_back(s);
                }
                variable_tuple_mapping[s].insert(i);
                new_terms.push_back(Term(s));
            }
        }
        return Tuple(new_terms);
    }
}
#endif
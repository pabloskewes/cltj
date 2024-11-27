//
// Created by adrian on 27/11/24.
//

#include <cstdint>
#include <string>
#include <iostream>
#include <regex>

std::vector<std::string> regex_tokenizer(std::string input, const std::regex &reg) {
    std::sregex_iterator reg_it(input.begin(), input.end(), reg);
    std::sregex_iterator reg_end;
    std::vector<std::string> vec;
    while(reg_it != reg_end) {
        vec.push_back(reg_it->str());
        ++reg_it;
    }
    return vec;
}

int main() {

    const std::regex regex("(?:\".*\"|[^[:space:]])+");

    std::string line;
    while(true){
        std::getline(std::cin, line);
        if(line == "q") break;
        auto terms = regex_tokenizer(line, regex);
        std::cout << "Size: " << terms.size() << std::endl;
        for(uint64_t i = 0; i < terms.size(); ++i) {
            std::cout << "terms[" <<i << "]=" << terms[i] << std::endl;
        }
        std::cout << std::endl;
    }


}

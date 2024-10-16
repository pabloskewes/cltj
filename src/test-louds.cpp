//
// Created by adrian on 11/10/24.
//

#include <dyn_louds.hpp>

int main() {

    dyn_cds::dyn_louds index(18);

    index.insert(0, 1, 7);
    index.insert(0, 1, 3, true); //first position but there are more children
    index.insert(2, 0, 10);

    index.insert(3, 1, 3);
    index.insert(4, 1, 1);
    index.insert(5, 0, 2);

    index.insert(3, 1, 8);
    index.insert(3, 1, 2, true);

    for(auto i = 0; i < index.size(); ++i) {
        auto p = index.access(i);
        std::cout << p.first << ", " << p.second << std::endl;
    }
    std::cout << "=============================" << std::endl;
    index.remove(2);
    index.remove(2, true);

    for(auto i = 0; i < index.size(); ++i) {
        auto p = index.access(i);
        std::cout << p.first << ", " << p.second << std::endl;
    }




}
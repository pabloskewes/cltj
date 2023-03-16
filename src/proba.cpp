//
// Created by Adrián on 16/3/23.
//
#include <sdsl/wavelet_trees.hpp>
#include <wt_range_iterator.hpp>

int main(int argc, char* argv[]) {

    sdsl::wm_int<> wm;
    sdsl::int_vector<> vec = {2, 3, 6 , 8, 2, 1, 2, 3, 4,5 ,6, 3 ,4, 3,5, 8};
    sdsl::construct_im(wm , vec);

    sdsl::wt_range_iterator<sdsl::wm_int<>> iterator(&wm, sdsl::range_type{3, 9});

    auto v = iterator.next();
    while(v != 0){
        std::cout << v << std::endl;
        v = iterator.next();
    }
}

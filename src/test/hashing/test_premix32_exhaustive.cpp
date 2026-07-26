#include <cassert>
#include <cstdint>
#include <iostream>
#include <hashing/mixers.hpp>

int main() {
    constexpr uint64_t total = uint64_t{1} << 32;
    constexpr uint64_t step = uint64_t{1} << 28;

    std::cout << "Testing unpremix32(premix32(x)) over all uint32 values" << std::endl;

    for (uint64_t i = 0; i < total; i++) {
        uint32_t x = static_cast<uint32_t>(i);
        if (cltj::hashing::unpremix32(cltj::hashing::premix32(x)) != x) {
            std::cerr << "Mismatch at x=" << x << std::endl;
            return 1;
        }

        if ((i & (step - 1)) == 0) {
            std::cout << "  " << ((i * 100) / total) << "%" << std::endl;
        }
    }

    std::cout << "  100%" << std::endl;
    std::cout << "All uint32 values passed" << std::endl;
    return 0;
}

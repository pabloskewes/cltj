#include <hashing/storage/rank_support_glgh.hpp>
#include <sdsl/bit_vectors.hpp>
#include <cassert>
#include <iostream>

using namespace cltj::hashing;
using namespace sdsl;

int main() {
    std::cout << "Testing rank_support_glgh (boilerplate test)...\n";
    
    // Target bitvector B: 10101010101010101010 (10 ones)
    bit_vector B(20, 0);
    B[0]  = 1;
    B[2]  = 1;
    B[4]  = 1;
    B[6]  = 1;
    B[8]  = 1;
    B[10] = 1;
    B[12] = 1;
    B[14] = 1;
    B[16] = 1;
    B[18] = 1;

    // Build Gl and Gh so that B[v] = ~(Gl[v] & Gh[v])
    bit_vector Gl(20, 0);
    bit_vector Gh(20, 0);
    for (size_t i = 0; i < 20; ++i) {
        if (B[i] == 1) {
            // We want B[i] = 1 => ~(Gl[i] & Gh[i]) = 1 => Gl[i] & Gh[i] = 0.
            Gl[i] = 0;
            Gh[i] = 0;
        } else {
            // We want B[i] = 0 => ~(Gl[i] & Gh[i]) = 0 => Gl[i] & Gh[i] = 1.
            Gl[i] = 1;
            Gh[i] = 1;
        }
    }
    
    // Create rank support using Gl and Gh
    rank_support_glgh<1> rs(&Gl, &Gh);
    
    // Test rank queries
    assert(rs.rank(0) == 0);   // rank(0) = 0 (no 1s before position 0)
    assert(rs.rank(1) == 1);   // rank(1) = 1 (one 1 at position 0)
    assert(rs.rank(2) == 1);   // rank(2) = 1 (one 1 at position 0)
    assert(rs.rank(3) == 2);   // rank(3) = 2 (ones at positions 0, 2)
    assert(rs.rank(5) == 3);   // rank(5) = 3 (ones at positions 0, 2, 4)
    assert(rs.rank(20) == 10); // rank(20) = 10 (all 10 ones)
    
    // Test operator()
    assert(rs(0) == 0);
    assert(rs(1) == 1);
    assert(rs(20) == 10);
    
    // Test size
    assert(rs.size() == 20);
    
    std::cout << "All tests passed! rank_support_glgh works correctly.\n";
    std::cout << "Rank queries:\n";
    for (size_t i = 0; i <= 20; i += 5) {
        std::cout << "  rank(" << i << ") = " << rs.rank(i) << "\n";
    }
    
    return 0;
}


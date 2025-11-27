#include <hashing/storage/rank_support_glgh.hpp>
#include <sdsl/bit_vectors.hpp>
#include <cassert>
#include <iostream>

using namespace cltj::hashing;
using namespace sdsl;

int main() {
    std::cout << "Testing rank_support_glgh (boilerplate test)...\n";
    
    // Create a simple bitvector: 1010101010...
    bit_vector bv(20, 0);
    bv[0] = 1;
    bv[2] = 1;
    bv[4] = 1;
    bv[6] = 1;
    bv[8] = 1;
    bv[10] = 1;
    bv[12] = 1;
    bv[14] = 1;
    bv[16] = 1;
    bv[18] = 1;
    // Pattern: 10101010101010101010 (10 ones)
    
    // Create rank support
    rank_support_glgh<1> rs(&bv);
    
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


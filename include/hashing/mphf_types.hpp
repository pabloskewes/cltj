#pragma once
#include <cassert>
#include <cstdint>

namespace cltj {
namespace hashing {

struct Triple {
    uint32_t mixed_key;  // key after applying the pre-hash mixer
    uint32_t v0, v1, v2;  // vertices of the hypergraph

    Triple() : mixed_key(0), v0(0), v1(0), v2(0) {}

    Triple(uint32_t mixed_key_value, uint32_t vertex0, uint32_t vertex1, uint32_t vertex2)
        : mixed_key(mixed_key_value), v0(vertex0), v1(vertex1), v2(vertex2) {}

    uint32_t v(int idx) const {
        switch (idx) {
            case 0:
                return v0;
            case 1:
                return v1;
            case 2:
                return v2;
            default:
                assert(false && "Triple index out of range");
                return 0u;
        }
    }
};

}  // namespace hashing
}  // namespace cltj

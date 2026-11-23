#ifndef LTJ_VEO_TRACE_HPP
#define LTJ_VEO_TRACE_HPP

#include <cstdint>

namespace ltj {
namespace veo {

// Per-query plan signature (independent of the enumeration order) and trie_switch() count.
template <class Dummy = void>
struct trace_t {
    static uint64_t signature, decisions, switches;

    static inline uint64_t mix(uint64_t x) {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdULL;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53ULL;
        x ^= x >> 33;
        return x;
    }

    template <class Tuple, class Var>
    static inline void record(const Tuple &t, uint64_t j, Var x) {
        uint64_t h = 0x9e3779b97f4a7c15ULL;
        for (uint64_t i = 0; i < j; ++i)
            h = mix(h ^ ((uint64_t)t[i].first << 40 | (uint64_t)t[i].second));
        signature += mix(h ^ (0xabcULL + (uint64_t)x));
        ++decisions;
    }

    static void reset() {
        signature = decisions = switches = 0;
    }
};

template <class Dummy> uint64_t trace_t<Dummy>::signature = 0;
template <class Dummy> uint64_t trace_t<Dummy>::decisions = 0;
template <class Dummy> uint64_t trace_t<Dummy>::switches = 0;

typedef trace_t<> trace;

} // namespace veo
} // namespace ltj

#endif // LTJ_VEO_TRACE_HPP

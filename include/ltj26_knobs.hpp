#pragma once
// LTJ-26 throwaway diagnostic knobs. Not for merging.
//   LTJ26_LONELY_REVERSE=1   reverse the order of lonely variables (X and H)
//   LTJ26_ENUM=key|spo       H pure-hash scan order when the next level trie-switches
//   LTJ26_SHORTCUT=1         H trie_switch: skip m_has_hash read on nodes below threshold
//   LTJ26_MIXED_MIN=1        H mixed frames whose smallest list is hashed: scan it, exists() on sorted ones
//   LTJ26_SORTED_SCAN=1      H pure-hash frames whose next down() switches trie: filter, sort by id, descend
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#ifdef LTJ26_PROF
#define LTJ26_NOINLINE __attribute__((noinline))
#else
#define LTJ26_NOINLINE
#endif

namespace ltj26 {

inline bool env_flag(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && std::strcmp(v, "0") != 0 && v[0] != '\0';
}

inline bool lonely_reverse() {
    static const bool v = env_flag("LTJ26_LONELY_REVERSE");
    return v;
}

enum enum_order_t { ENUM_SLOT = 0, ENUM_KEY = 1, ENUM_SPO = 2 };

inline int enum_order() {
    static const int v = [] {
        const char* e = std::getenv("LTJ26_ENUM");
        if (e == nullptr)
            return (int)ENUM_SLOT;
        if (!std::strcmp(e, "key"))
            return (int)ENUM_KEY;
        if (!std::strcmp(e, "spo"))
            return (int)ENUM_SPO;
        return (int)ENUM_SLOT;
    }();
    return v;
}

inline bool shortcut() {
    static const bool v = env_flag("LTJ26_SHORTCUT");
    return v;
}

inline bool sorted_scan() {
    static const bool v = env_flag("LTJ26_SORTED_SCAN");
    return v;
}

inline bool mixed_min() {
    static const bool v = env_flag("LTJ26_MIXED_MIN");
    return v;
}

inline void print_knobs() {
    std::cerr << "[ltj26] LONELY_REVERSE=" << lonely_reverse() << " ENUM=" << enum_order()
              << " SHORTCUT=" << shortcut() << " SORTED_SCAN=" << sorted_scan() << " MIXED_MIN=" << mixed_min()
#ifdef LTJ26_PROF
              << " PROF=1"
#endif
              << std::endl;
}

}  // namespace ltj26

// LTJ26_COUNT: per-query counters (throwaway). Compiled out unless -DLTJ26_COUNT.
namespace ltj26 {
struct counters_t {
    uint64_t sw = 0;       // trie_switch() calls
    uint64_t loc = 0;      // hash_locate() calls
    uint64_t cont = 0;     // hash_contains() calls
    uint64_t seekall = 0;  // seek_all() calls
    uint64_t root_loc = 0; // hash_locate() on a full-trie root (inside trie_switch)
};
inline counters_t g_cnt;
}  // namespace ltj26
#ifdef LTJ26_COUNT
#define LTJ26_INC(f) (++ltj26::g_cnt.f)
#else
#define LTJ26_INC(f) ((void)0)
#endif

#pragma once
// LTJ-26 throwaway diagnostic knobs. Not for merging.
//   LTJ26_LONELY_REVERSE=1   reverse the order of lonely variables (X and H)
//   LTJ26_ENUM=key|spo       H pure-hash scan order when the next level trie-switches
//   LTJ26_SHORTCUT=1         H trie_switch: skip m_has_hash read on nodes below threshold
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

inline void print_knobs() {
    std::cerr << "[ltj26] LONELY_REVERSE=" << lonely_reverse() << " ENUM=" << enum_order()
              << " SHORTCUT=" << shortcut()
#ifdef LTJ26_PROF
              << " PROF=1"
#endif
              << std::endl;
}

}  // namespace ltj26

#pragma once

// Central instrumentation flags for CLTJ.
// Each flag is controlled via a CMake option that defines the corresponding macro.
// When disabled, `if constexpr (flag)` blocks are eliminated at compile time (zero overhead).

namespace cltj {

#ifdef CLTJ_COLLECT_QUERY_STATS_ENABLED
inline constexpr bool COLLECT_QUERY_STATS = true;
#else
inline constexpr bool COLLECT_QUERY_STATS = false;
#endif

#ifdef CLTJ_COLLECT_MPHF_BUILD_TRACE_ENABLED
inline constexpr bool COLLECT_MPHF_BUILD_TRACE = true;
#else
inline constexpr bool COLLECT_MPHF_BUILD_TRACE = false;
#endif

#ifdef CLTJ_DUMP_MPHF_KEYS_ENABLED
inline constexpr bool DUMP_MPHF_KEYS = true;
#else
inline constexpr bool DUMP_MPHF_KEYS = false;
#endif

}  // namespace cltj

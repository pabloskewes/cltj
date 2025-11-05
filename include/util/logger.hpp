#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

// Compile-time controls (may be overridden via compiler defs)
#ifndef ENABLE_LOGGING
#define ENABLE_LOGGING 1
#endif

// Numeric levels to allow compile-time comparisons in preprocessor
#ifndef MIN_LOG_LEVEL
#define MIN_LOG_LEVEL 10
#endif

namespace cltj {
namespace logger {

enum Level : int { Debug = 10, Info = 20, Warn = 30, Error = 40 };

inline const char *levelToString(Level lvl) {
  switch (lvl) {
  case Debug:
    return "DEBUG";
  case Info:
    return "INFO";
  case Warn:
    return "WARN";
  case Error:
    return "ERROR";
  default:
    return "INFO";
  }
}

inline std::string formatTimestamp() {
  using namespace std::chrono;
  const auto now = system_clock::now();
  const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
  std::time_t tt = system_clock::to_time_t(now);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &tt);
#else
  localtime_r(&tt, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S") << '.' << std::setw(3)
      << std::setfill('0') << ms.count();
  return oss.str();
}

inline void
write(Level lvl, const char *file, int line, const std::string &message) {
  auto &out = (lvl == Error) ? std::cerr : std::cout;
  out << '[' << formatTimestamp() << "] [" << std::left << std::setw(5)
      << levelToString(lvl) << "] [GENERAL] " << message << " (" << file << ':'
      << line << ")\n";
}

} // namespace logger
} // namespace cltj

// Macros: ensure message construction happens only when enabled and level
// passes

#if ENABLE_LOGGING && (MIN_LOG_LEVEL <= 10)
#define LOG_DEBUG(msg)                                                         \
  do {                                                                         \
    std::ostringstream _cltj_log_ss;                                           \
    _cltj_log_ss << msg;                                                       \
    ::cltj::logger::write(                                                     \
        ::cltj::logger::Debug, __FILE__, __LINE__, _cltj_log_ss.str()          \
    );                                                                         \
  } while (0)
#else
#define LOG_DEBUG(msg)                                                         \
  do {                                                                         \
  } while (0)
#endif

#if ENABLE_LOGGING && (MIN_LOG_LEVEL <= 20)
#define LOG_INFO(msg)                                                          \
  do {                                                                         \
    std::ostringstream _cltj_log_ss;                                           \
    _cltj_log_ss << msg;                                                       \
    ::cltj::logger::write(                                                     \
        ::cltj::logger::Info, __FILE__, __LINE__, _cltj_log_ss.str()           \
    );                                                                         \
  } while (0)
#else
#define LOG_INFO(msg)                                                          \
  do {                                                                         \
  } while (0)
#endif

#if ENABLE_LOGGING && (MIN_LOG_LEVEL <= 30)
#define LOG_WARN(msg)                                                          \
  do {                                                                         \
    std::ostringstream _cltj_log_ss;                                           \
    _cltj_log_ss << msg;                                                       \
    ::cltj::logger::write(                                                     \
        ::cltj::logger::Warn, __FILE__, __LINE__, _cltj_log_ss.str()           \
    );                                                                         \
  } while (0)
#else
#define LOG_WARN(msg)                                                          \
  do {                                                                         \
  } while (0)
#endif

#if ENABLE_LOGGING && (MIN_LOG_LEVEL <= 40)
#define LOG_ERROR(msg)                                                         \
  do {                                                                         \
    std::ostringstream _cltj_log_ss;                                           \
    _cltj_log_ss << msg;                                                       \
    ::cltj::logger::write(                                                     \
        ::cltj::logger::Error, __FILE__, __LINE__, _cltj_log_ss.str()          \
    );                                                                         \
  } while (0)
#else
#define LOG_ERROR(msg)                                                         \
  do {                                                                         \
  } while (0)
#endif

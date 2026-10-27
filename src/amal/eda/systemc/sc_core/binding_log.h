/**
 * @file binding_log.h
 * @brief Header-only fmt diagnostics shared by the native binding adapters.
 */
#ifndef PYSYSTEMC_BINDING_LOG_H
#define PYSYSTEMC_BINDING_LOG_H
#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif
#include <fmt/color.h>
#include <fmt/format.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace pysystemc
{

/**
 * @brief Return whether debug diagnostics were explicitly enabled.
 */
inline bool debug_enabled()
{
  const char* level = std::getenv("PYSYSTEMC_LOG_LEVEL");
  return level && std::strcmp(level, "DEBUG") == 0;
}

/**
 * @brief Honor color selection on stderr; explicit always overrides NO_COLOR.
 */
inline bool color_enabled()
{
  const char* mode = std::getenv("PYSYSTEMC_COLOR");
  if (mode && std::strcmp(mode, "always") == 0) return true;
  if (mode && std::strcmp(mode, "never") == 0) return false;
  if (std::getenv("NO_COLOR")) return false;
#if defined(_WIN32)
  return _isatty(_fileno(stderr));
#else
  return isatty(fileno(stderr));
#endif
}

/**
 * @brief Emit an error or optional debug message using fmt.
 * @param error True for an unconditional red error, false for cyan debug output.
 * @param message Already formatted diagnostic; never interpreted as a format string.
 */
inline void log(bool error, const std::string& message)
{
  if (!error && !debug_enabled()) return;
  const auto label = error ? "ERROR" : "DEBUG";
  if (color_enabled())
  {
    fmt::print(stderr, fmt::fg(error ? fmt::color::red : fmt::color::cyan), "[pysystemc:C++:{}] {}\n", label, message);
  }
  else
  {
    fmt::print(stderr, "[pysystemc:C++:{}] {}\n", label, message);
  }
  std::fflush(stderr);
}

}  // namespace pysystemc
#endif

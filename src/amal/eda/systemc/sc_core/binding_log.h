/**
 * @file binding_log.h
 * @brief Header-only diagnostics for the native binding adapters.
 */
#ifndef PYSYSTEMC_BINDING_LOG_H
#define PYSYSTEMC_BINDING_LOG_H

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <system_error>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace pysystemc
{

/**
 * @brief Return whether debug diagnostics were explicitly enabled.
 * @return True only when PYSYSTEMC_LOG_LEVEL is exactly "DEBUG".
 */
inline bool debug_enabled()
{
  const char* level = std::getenv("PYSYSTEMC_LOG_LEVEL");
  return level && std::strcmp(level, "DEBUG") == 0;
}

/**
 * @brief Honor color selection on stderr; explicit always overrides NO_COLOR.
 * @return Whether ANSI color sequences should be written.
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
 * @brief Emit an error or optional debug message to stderr.
 * @param error True for an unconditional red error, false for cyan debug output.
 * @param message Formatted diagnostic bytes borrowed only for this call.
 * @throws std::system_error If writing the complete message to stderr fails.
 * @details Writes fmt-compatible three-digit RGB ANSI sequences when color is enabled,
 * and flushes stderr before returning. One complete message is passed to fwrite so
 * concurrent calls retain stdio's whole-write behavior. No message or stream state is
 * retained. This function does not acquire the Python GIL or call Python, so callers
 * may invoke it from a native process thread. Flush errors remain in stderr's error
 * indicator, matching the prior behavior.
 */
inline void log(bool error, const std::string& message)
{
  if (!error && !debug_enabled()) return;
  const char* label = error ? "ERROR" : "DEBUG";
  const bool use_color = color_enabled();
  std::string output;
  output.reserve(message.size() + 64);
  if (use_color)
  {
    output += error ? "\x1b[38;2;255;000;000m" : "\x1b[38;2;000;255;255m";
  }
  output += "[pysystemc:C++:";
  output += label;
  output += "] ";
  output.append(message);
  output += '\n';
  if (use_color) output += "\x1b[0m";
  if (std::fwrite(output.data(), 1, output.size(), stderr) != output.size())
  {
    throw std::system_error(errno, std::generic_category(), "cannot write to file");
  }
  std::fflush(stderr);
}

}  // namespace pysystemc
#endif

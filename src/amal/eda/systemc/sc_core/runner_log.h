/**
 * @file runner_log.h
 * @brief Header-only spdlog notes for the JIT C++ example runner, using external fmt.
 */
#ifndef PYSYSTEMC_RUNNER_LOG_H
#define PYSYSTEMC_RUNNER_LOG_H
#ifndef FMT_HEADER_ONLY
#define FMT_HEADER_ONLY
#endif
#define SPDLOG_FMT_EXTERNAL
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstdlib>
#include <cstring>

namespace pysystemc
{

/**
 * @brief Return the runner's dedicated stderr logger without changing global logging. Color honors
 * PYSYSTEMC_COLOR=auto/always/never; explicit always overrides NO_COLOR.
 */
inline std::shared_ptr<spdlog::logger> runner_logger()
{
  static auto logger = []
  {
    auto value = spdlog::stderr_color_mt("pysystemc-runner");
    value->set_pattern("%^[pysystemc:C++:%l]%$ %v");
    auto sink = std::static_pointer_cast<spdlog::sinks::stderr_color_sink_mt>(value->sinks().front());
    const char* mode = std::getenv("PYSYSTEMC_COLOR");
    const bool always = mode && std::strcmp(mode, "always") == 0;
    const bool never = (mode && std::strcmp(mode, "never") == 0) || (!always && std::getenv("NO_COLOR"));
    sink->set_color_mode(always  ? spdlog::color_mode::always
                         : never ? spdlog::color_mode::never
                                 : spdlog::color_mode::automatic);
    value->flush_on(spdlog::level::info);
    return value;
  }();
  return logger;
}

/**
 * @brief Emit a visible informational note; message text is never a format string.
 */
inline void runner_note(const std::string& message)
{
  runner_logger()->info("{}", message);
}

}  // namespace pysystemc
#endif

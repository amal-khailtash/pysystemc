/** @file sc_core_user_exception_probe.h
 * @brief Copyable custom exception value used by both simulation references.
 */
#ifndef PYSYSTEMC_USER_EXCEPTION_PROBE_H
#define PYSYSTEMC_USER_EXCEPTION_PROBE_H
#include <stdexcept>

/** @brief Native exception with copy-owned message and payload. */
struct UserProcessException : std::runtime_error
{
  int code;
  /** @brief Own the message and preserve an arbitrary integer payload. */
  explicit UserProcessException(int value) : std::runtime_error("native user exception"), code(value)
  {
  }
};
#endif

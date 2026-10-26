/** @file Exact legacy string formatting and mutable character operations. */
#pragma once
#include <sysc/utils/sc_string.h>

#include <string>
namespace pysystemc_datatypes
{
/** @brief Borrow a native printf argument without retaining it. */
template <class T>
const T& string_argument(const T& value)
{
  return value;
}
/** @brief Borrow null-terminated string bytes for one native formatting call. */
inline const char* string_argument(const std::string& value)
{
  return value.c_str();
}
/** @brief Return independent formatted native string storage.
 * @details The caller must match native printf format specifiers and argument
 * types. Hold the GIL; arguments are borrowed only during the call. Native
 * allocation/report exceptions propagate. No callbacks or references persist.
 */
template <class... Args>
sc_dt::sc_string_old string_format(const char* format, const Args&... args)
{
  return sc_dt::sc_string_old::to_string(format, string_argument(args)...);
}
/** @brief Apply the native incremental formatter and return its live owner.
 * @details Format/type compatibility is a native precondition. The GIL stays
 * held; source storage is borrowed only during the call. Native reports and
 * allocations propagate, with no retained callbacks or Python references.
 */
template <class T>
sc_dt::sc_string_old& string_fmt(sc_dt::sc_string_old& owner, const T& value)
{
  return owner.fmt(string_argument(value));
}
/** @brief Assign a character through the exact native mutable subscript.
 * @details Owner is borrowed for the GIL-held call. Mutable indexing is unchecked:
 * valid bounds are a native precondition. Native reports and copy-on-write
 * semantics apply; no references or callbacks are retained.
 */
inline void string_setitem(sc_dt::sc_string_old& owner, int index, char value)
{
  owner[index] = value;
}
/** @brief Concatenate native bytes with an old string into independent storage.
 * @details Both sources are borrowed for the GIL-held call. Native allocation
 * exceptions propagate; the result retains no source or Python references.
 */
inline sc_dt::sc_string_old string_radd(const char* left, const sc_dt::sc_string_old& right)
{
  return left + right;
}
}  // namespace pysystemc_datatypes

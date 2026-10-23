/** @file time_operations.cpp
 * @brief Select native free time operators omitted by cppyy reverse dispatch.
 */
#include <systemc>

namespace pysystemc_public
{
/** @brief Multiply a borrowed time by a left numeric factor using the native free overload.
 * @param factor Native double scaling factor, preserving native rounding.
 * @param time Borrowed live native time value, unchanged by the operation.
 * @return Independent native time value owned by the returned Python proxy.
 * @details GIL remains held; no callbacks or scheduler entry. Native conversion
 * and arithmetic exceptions propagate; no Python callback state is retained.
 */
sc_core::sc_time time_left_multiply(double factor, const sc_core::sc_time& time)
{
  return sc_core::operator*(factor, time);
}
}  // namespace pysystemc_public

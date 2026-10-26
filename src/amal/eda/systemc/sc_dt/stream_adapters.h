/** @file Exact native datatype stream expressions with borrowed live streams. */
#pragma once
#include <sysc/utils/sc_string.h>

#include <istream>
#include <ostream>
namespace pysystemc_datatypes
{
/** @brief Select the native datatype insertion and extraction overloads.
 * @details Hold the GIL and keep both stream and value owners live. Calls never
 * suspend or retain Python references/callbacks. Returned stream references are
 * borrowed. Native stream flags, report actions and I/O/allocation exceptions
 * propagate unchanged; input views also require their parent owners to be live.
 */
template <class Value>
struct StreamOperators
{
  /** @brief Insert a borrowed native value and return the borrowed output stream. */
  static std::ostream& insert(std::ostream& stream, const Value& value)
  {
    return stream << value;
  }
  /** @brief Extract into live value storage and return the borrowed input stream. */
  static std::istream& extract(std::istream& stream, Value& value)
  {
    return stream >> value;
  }
};
}  // namespace pysystemc_datatypes

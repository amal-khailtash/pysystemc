/** @file Independent native stream reference for Python datatype strings. */
#pragma once

#include <sstream>
#include <string>
#include <systemc>

namespace native_datatype_string_probe
{
/** @brief Render a borrowed SystemC datatype through native stream insertion.
 * @tparam Value SystemC datatype with a native output-stream insertion overload.
 * @param value Live native value; this helper does not retain or modify it.
 * @return Owned text copied from a fresh std::ostringstream.
 * @details Called synchronously with the GIL held. It does not invoke the
 * scheduler, retain callbacks or Python references, or transfer ownership.
 * Native stream and allocation exceptions propagate to the caller.
 */
template <class Value>
struct Reference
{
  /** @brief Render a borrowed native value without invoking Python operators. */
  static std::string text(const Value& value)
  {
    std::ostringstream stream;
    stream << value;
    return stream.str();
  }
};
}  // namespace native_datatype_string_probe

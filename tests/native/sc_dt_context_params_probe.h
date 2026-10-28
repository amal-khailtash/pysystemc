/** @file sc_dt_context_params_probe.h
 *  @brief Formatting helpers for native fixed-point parameter references.
 *  @details These helpers borrow parameter objects only for the duration of
 *  each call. They retain no native state and invoke no Python callbacks.
 */
#ifndef PYSYSTEMC_TEST_SC_DT_CONTEXT_PARAMS_PROBE_H
#define PYSYSTEMC_TEST_SC_DT_CONTEXT_PARAMS_PROBE_H

#include <sstream>
#include <string>
#include <systemc>

namespace sc_dt_context_params_probe
{

/** @brief Capture a parameter's native print representation.
 *  @param value Borrowed parameter whose public print method is called.
 *  @return The exact bytes written by the native `print` method.
 */
template <class T>
std::string printed(const T& value)
{
  std::ostringstream output;
  value.print(output);
  return output.str();
}

/** @brief Capture a parameter's native dump representation.
 *  @param value Borrowed parameter whose public dump method is called.
 *  @return The exact bytes written by the native `dump` method.
 */
template <class T>
std::string dumped(const T& value)
{
  std::ostringstream output;
  value.dump(output);
  return output.str();
}

/** @brief Make multiline print or dump output safe for one-line records.
 *  @param value Arbitrary native text.
 *  @return Text with newlines replaced by `~` and spaces retained.
 */
inline std::string flatten(const std::string& value)
{
  std::string result(value);
  for (char& character : result)
  {
    if (character == '\n') character = '~';
  }
  return result;
}

}  // namespace sc_dt_context_params_probe

#endif  // PYSYSTEMC_TEST_SC_DT_CONTEXT_PARAMS_PROBE_H

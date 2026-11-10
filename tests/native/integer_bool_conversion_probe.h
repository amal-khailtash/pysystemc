/** @file Independent native references for bool construction and assignment.
 * @details Each call borrows a bool only for the duration of the call. A
 *          constructed integer owns independent native storage; assignment
 *          borrows the caller-owned target and returns that same target. The
 *          functions retain no Python references or callbacks. Calls cross the
 *          cppyy boundary with the GIL held, and native exceptions propagate.
 */
#pragma once

#include <string>
#include <systemc>

namespace integer_bool_conversion_probe
{
/** @brief Direct C++ constructor and assignment references for a target type.
 * @tparam Number A native SystemC integer specialization of arbitrary width.
 */
template <class Number>
struct Reference
{
  /** @brief Construct independent numeric storage directly from a bool.
   * @param value The bool borrowed for this synchronous construction.
   * @return A new integer value that owns its storage.
   * @throws Any native exception raised by the SystemC integer constructor.
   */
  static Number construct(bool value)
  {
    return Number(value);
  }

  /** @brief Assign a bool to a live target and return that same target.
   * @param target The caller-owned integer to update.
   * @param value The bool borrowed for this synchronous assignment.
   * @return A reference to @p target; ownership remains with the caller.
   * @throws Any native exception raised by the SystemC integer assignment.
   */
  static Number& assign(Number& target, bool value)
  {
    target = value;
    return target;
  }

  /** @brief Render a full-width decimal value without narrowing.
   * @param value The integer borrowed for this synchronous call.
   * @return Its exact decimal representation.
   */
  static std::string render(const Number& value)
  {
    return value.to_string(sc_dt::SC_DEC, false);
  }
};
}  // namespace integer_bool_conversion_probe

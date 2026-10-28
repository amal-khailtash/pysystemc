/** @file Exact native shift expressions for arbitrary numeric and count types. */
#pragma once
namespace pysystemc_datatypes
{
/** @brief Select exact native shift expressions without function-template deduction.
 * @details Both operands are borrowed for a GIL-held call. Native datatype results
 * own independent storage; scalar results copy their value. Native shift-count
 * preconditions, width/sign rules, allocation and configured reports apply.
 * No waits, callbacks or Python references are retained.
 */
template <class Value, class Count>
struct ShiftOperators
{
  /** @brief Return the independent native left-shift expression result. */
  static auto left(const Value& value, const Count& count)
  {
    return value << count;
  }
  /** @brief Return the independent native right-shift expression result. */
  static auto right(const Value& value, const Count& count)
  {
    return value >> count;
  }
};
}  // namespace pysystemc_datatypes

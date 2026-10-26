/** @file Exact native datatype operators with owned results and borrowed inputs. */
#pragma once

#include <string>

namespace pysystemc_datatypes
{
/** @brief Preserve the native free complement overload for any value type.
 * @details Inputs are borrowed during the call; results own native copied
 * storage. Call with the GIL held. Native allocation/report/callback exceptions
 * propagate. No waits, Python references or native kernel state are retained.
 */
template <class Value>
struct unary_operator
{
  /** @brief Return the independent native complement value. */
  static auto invert(const Value& value)
  {
    return ~value;
  }
};

/** @brief Borrow an exact native operand for the duration of an expression. */
template <class Value>
const Value& operator_argument(const Value& value)
{
  return value;
}
/** @brief Borrow the native bit-string pointer from call-local string storage. */
inline const char* operator_argument(const std::string& value)
{
  return value.c_str();
}

/** @brief Resolve exact C++ vector operators for arbitrary argument types.
 * @details Native overload resolution preserves four-state values and width
 * rules. Inputs remain borrowed for the call; results own native copied storage.
 * Compound calls mutate the borrowed left owner and return it without taking
 * ownership. Hold the GIL; native allocation, conversion and configured reports propagate.
 * Native observer callbacks retain their build settings. Calls never suspend.
 */
template <class Left, class Right>
struct binary_operator
{
  /** @brief Return the native bitwise conjunction value. */
  static auto bit_and(const Left& left, const Right& right)
  {
    return operator_argument(left) & operator_argument(right);
  }
  /** @brief Return the native bitwise disjunction value. */
  static auto bit_or(const Left& left, const Right& right)
  {
    return operator_argument(left) | operator_argument(right);
  }
  /** @brief Return the native bitwise exclusive-or value. */
  static auto bit_xor(const Left& left, const Right& right)
  {
    return operator_argument(left) ^ operator_argument(right);
  }
  /** @brief Mutate the borrowed left value and return the same owner. */
  static Left& compound_and(Left& left, const Right& right)
  {
    left &= operator_argument(right);
    return left;
  }
  /** @brief Mutate the borrowed left value with native disjunction. */
  static Left& compound_or(Left& left, const Right& right)
  {
    left |= operator_argument(right);
    return left;
  }
  /** @brief Mutate the borrowed left value with native exclusive-or. */
  static Left& compound_xor(Left& left, const Right& right)
  {
    left ^= operator_argument(right);
    return left;
  }
};
}  // namespace pysystemc_datatypes

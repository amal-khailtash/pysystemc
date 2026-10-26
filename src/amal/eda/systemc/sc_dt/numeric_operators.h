/** @file Exact native numeric expressions for arbitrary operand types. */
#pragma once
#include <string>
#include <type_traits>
namespace pysystemc_datatypes
{
/** @brief Borrow an exact native operand for a single numeric expression.
 * @details Standard strings supply call-local C strings for native overloads.
 * Hold the GIL and keep the source live. No references or callbacks persist.
 */
template <class T>
decltype(auto) numeric_operand(const T& value)
{
  if constexpr (std::is_same_v<T, std::string>)
    return value.c_str();
  else
    return (value);
}
/** @brief Select native binary expressions without cppyy overload deduction.
 * @details Operands are borrowed for a GIL-held call. Numeric results own
 * independent storage; comparisons copy a boolean. Native widths, signedness,
 * conversion, precision and configured reporting rules apply. Native allocation
 * and arithmetic exceptions propagate. No waits or callbacks are registered.
 */
template <class Left, class Right>
struct NumericOperators
{
#define PYSYSTEMC_NUMERIC_BINARY(name, op)                       \
  /** @brief Return the exact owned native expression result. */ \
  static auto name(const Left& left, const Right& right)         \
  {                                                              \
    return numeric_operand(left) op numeric_operand(right);      \
  }
  PYSYSTEMC_NUMERIC_BINARY(add, +)
  PYSYSTEMC_NUMERIC_BINARY(subtract, -)
  PYSYSTEMC_NUMERIC_BINARY(multiply, *)
  PYSYSTEMC_NUMERIC_BINARY(divide, /)
  PYSYSTEMC_NUMERIC_BINARY(remainder, %)
  PYSYSTEMC_NUMERIC_BINARY(bit_and, &)
  PYSYSTEMC_NUMERIC_BINARY(bit_or, |)
  PYSYSTEMC_NUMERIC_BINARY(bit_xor, ^)
  PYSYSTEMC_NUMERIC_BINARY(equal, ==)
  PYSYSTEMC_NUMERIC_BINARY(not_equal, !=)
  PYSYSTEMC_NUMERIC_BINARY(less, <)
  PYSYSTEMC_NUMERIC_BINARY(less_equal, <=)
  PYSYSTEMC_NUMERIC_BINARY(greater, >)
  PYSYSTEMC_NUMERIC_BINARY(greater_equal, >=)
#undef PYSYSTEMC_NUMERIC_BINARY
};
/** @brief Mutate existing numeric storage through an exact native expression.
 * @details Keep both operands live with the GIL held. Return the borrowed
 * existing target, preserving native mutation and report semantics. No source
 * references, callbacks or scheduler state persist. Native preconditions apply.
 */
template <class Target, class Source>
struct NumericMutation
{
#define PYSYSTEMC_NUMERIC_MUTATE(name, op)                                \
  /** @brief Mutate the live target and return its borrowed reference. */ \
  static Target& name(Target& target, const Source& source)               \
  {                                                                       \
    target op numeric_operand(source);                                    \
    return target;                                                        \
  }
  PYSYSTEMC_NUMERIC_MUTATE(assign, =)
  PYSYSTEMC_NUMERIC_MUTATE(add, +=)
  PYSYSTEMC_NUMERIC_MUTATE(subtract, -=)
  PYSYSTEMC_NUMERIC_MUTATE(multiply, *=)
  PYSYSTEMC_NUMERIC_MUTATE(divide, /=)
  PYSYSTEMC_NUMERIC_MUTATE(remainder, %=)
  PYSYSTEMC_NUMERIC_MUTATE(bit_and, &=)
  PYSYSTEMC_NUMERIC_MUTATE(bit_or, |=)
  PYSYSTEMC_NUMERIC_MUTATE(bit_xor, ^=)
  PYSYSTEMC_NUMERIC_MUTATE(left_shift, <<=)
  PYSYSTEMC_NUMERIC_MUTATE(right_shift, >>=)
#undef PYSYSTEMC_NUMERIC_MUTATE
};
/** @brief Dispatch native output-parameter arithmetic through argument lookup.
 * @details All three operands are borrowed for a GIL-held call. The output
 * retains its native representation, precision, observer and aliasing rules.
 * An arbitrary-precision sc_fxnum output must not alias either input: native
 * named functions delete output representation before reading input storage.
 * Native allocation exceptions and configured reports propagate. No Python
 * references, callbacks or kernel state are retained; callers keep owners live.
 */
template <class Target, class Left, class Right>
struct NumericFunctions
{
#define PYSYSTEMC_NUMERIC_FUNCTION(name)                                             \
  /** @brief Write the exact native named arithmetic result into the live output. */ \
  static void perform_##name(Target& target, const Left& left, const Right& right)   \
  {                                                                                  \
    name(target, numeric_operand(left), numeric_operand(right));                     \
  }
  PYSYSTEMC_NUMERIC_FUNCTION(add)
  PYSYSTEMC_NUMERIC_FUNCTION(sub)
  PYSYSTEMC_NUMERIC_FUNCTION(mult)
  PYSYSTEMC_NUMERIC_FUNCTION(div)
#undef PYSYSTEMC_NUMERIC_FUNCTION
};
}  // namespace pysystemc_datatypes

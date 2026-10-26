/** @file Native scalar signature names for direct fixed-point initialization. */
#pragma once
#include <string>
#include <type_traits>
namespace pysystemc_datatypes
{
/** @brief Resolve typedef wrappers to the exact native constructor scalar type.
 * @details Names describe native types, including aliases introduced by callers.
 * The GIL is held when entered from Python. Names own their text and no operand,
 * observer, callback or Python references are acquired or retained.
 */
template <class T>
struct ConstructorScalar
{
  /** @brief Return a constructor type name, or empty for a non-scalar datatype. */
  static std::string name()
  {
    if constexpr (std::is_same_v<T, int>) return "int";
    if constexpr (std::is_same_v<T, unsigned int>) return "unsigned int";
    if constexpr (std::is_same_v<T, long>) return "long";
    if constexpr (std::is_same_v<T, unsigned long>) return "unsigned long";
    if constexpr (std::is_same_v<T, sc_dt::int64>) return "sc_dt::int64";
    if constexpr (std::is_same_v<T, sc_dt::uint64>) return "sc_dt::uint64";
    if constexpr (std::is_same_v<T, float>) return "float";
    if constexpr (std::is_same_v<T, double>) return "double";
    return "";
  }
};
}  // namespace pysystemc_datatypes

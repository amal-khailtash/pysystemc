/** @file Exact native integer construction with independent owned storage. */
#pragma once
#include <type_traits>
namespace pysystemc_datatypes
{
/** @brief Construct any native integer from the exact native source type.
 * @details Borrow the source for the call; the returned number owns independent
 * numeric storage. Call with the GIL held. Native conversion, allocation and
 * configured report exceptions propagate. No waits, callbacks or source/Python
 * references are retained. Native template width/type preconditions apply.
 */
template <class Number, class Source>
Number integer_construct(const Source& source)
{
  if constexpr (std::is_base_of_v<sc_dt::sc_generic_base<Source>, Source>)
  {
    return Number(static_cast<const sc_dt::sc_generic_base<Source>&>(source));
  }
  else
  {
    return Number(source);
  }
}
/** @brief Assign an exact native source and return the borrowed live target.
 * @details Call with the GIL held and both owners live. No waits, callbacks or
 * Python references are retained. Native conversion/allocation/report exceptions
 * propagate. Generic sources bind to their exact native CRTP overload.
 */
template <class Number, class Source>
Number& integer_assign(Number& target, const Source& source)
{
  if constexpr (std::is_base_of_v<sc_dt::sc_generic_base<Source>, Source>)
  {
    target = static_cast<const sc_dt::sc_generic_base<Source>&>(source);
  }
  else
  {
    target = source;
  }
  return target;
}
/** @brief Expose exact conversion signatures without function-template deduction.
 * @details Static calls borrow live sources/targets with the GIL held. Returned
 * values own storage; assignment results borrow targets. No callbacks, waits or
 * Python references persist. Native conversion and report exceptions propagate.
 */
template <class Number, class Source>
struct IntegerConversion
{
  /** @brief Construct independent native numeric storage from a live source. */
  static Number construct(const Source& source)
  {
    return integer_construct<Number, Source>(source);
  }
  /** @brief Assign a live target and return its borrowed reference. */
  static Number& assign(Number& target, const Source& source)
  {
    return integer_assign<Number, Source>(target, source);
  }
};
}  // namespace pysystemc_datatypes

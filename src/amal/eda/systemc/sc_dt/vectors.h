/** @file Exact native vector constructor and assignment selection. */
#pragma once
#include <type_traits>
namespace pysystemc_datatypes
{
/** @brief Copy a native source into independent vector storage.
 * @details Calls hold the GIL, borrow the source for the call and retain no
 * callbacks or Python references. Native reports/allocation exceptions propagate.
 * Deprecated numeric base constructors use their native length/assignment
 * implementation when the native build disables their declarations.
 */
template <class Vector, class Source>
Vector vector_construct(const Source& source)
{
  if constexpr ((std::is_same_v<Vector, sc_dt::sc_bv_base> || std::is_same_v<Vector, sc_dt::sc_lv_base>)&&(
                  std::is_base_of_v<sc_dt::sc_signed, Source> || std::is_base_of_v<sc_dt::sc_unsigned, Source> ||
                  std::is_base_of_v<sc_dt::sc_int_base, Source> || std::is_base_of_v<sc_dt::sc_uint_base, Source>))
  {
    Vector result(source.length());
    result = source;
    return result;
  }
  else
  {
    return Vector(source);
  }
}
/** @brief Assign using the exact native source type and return the live target.
 * @details Both references are borrowed for the GIL-held call. No waits or
 * retained callbacks occur; native type/owner preconditions and reports apply.
 */
template <class Vector, class Source>
Vector& vector_assign(Vector& target, const Source& source)
{
  target = source;
  return target;
}
}  // namespace pysystemc_datatypes

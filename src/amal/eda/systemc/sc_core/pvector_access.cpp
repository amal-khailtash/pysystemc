/** @file pvector_access.cpp
 * @brief Avoid cppyy's unsupported pointer-reference conversion for sc_pvector.
 */
#include <systemc>
namespace pysystemc_public
{
/** @brief Select pointer slot access while retaining native growth semantics.
 * @tparam T Arbitrary native pointer type used by sc_pvector.
 * @details Vector owns slots, not pointees. Returned pointers remain borrowed;
 * callers retain pointees. Growth invalidates slot/iterator views, not pointees.
 * GIL remains held, no callbacks stored; conversion/allocation errors propagate.
 */
template <class T>
struct pvector_access
{
  /** @brief Return a borrowed pointer value from the mutable auto-growing index. */
  static T get(sc_core::sc_pvector<T>& values, unsigned int index)
  {
    return values[index];
  }
  /** @brief Copy a borrowed pointer into the native auto-growing slot. */
  static void set(sc_core::sc_pvector<T>& values, unsigned int index, T value)
  {
    values[index] = value;
  }
  /** @brief Return a pointer value without growing native storage. */
  static T fetch(sc_core::sc_pvector<T>& values, int index)
  {
    return values.fetch(index);
  }
};
}  // namespace pysystemc_public

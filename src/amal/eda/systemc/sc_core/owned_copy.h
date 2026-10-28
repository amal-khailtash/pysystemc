/** @file owned_copy.h
 * @brief Copy borrowed callback values into independently owned native storage.
 */
#pragma once

namespace pysystemc_interfaces
{
/** @brief Copy an arbitrary native callback value using its copy constructor.
 * @tparam T Native copy-constructible value type; default construction and
 * assignment are not required.
 * @details The input is borrowed for this synchronous operation. The returned
 * value owns its storage and transfers by value to the Python proxy. Call with
 * the GIL held in the callback; no callback or Python reference is retained.
 * Native allocation and copy-constructor exceptions propagate before dispatch
 * to the Python implementation. The caller's input is never assigned.
 */
template <class T>
struct owned_callback_copy
{
  /** @brief Construct an owning value from a borrowed native input. */
  static T copy(const T& input)
  {
    return T(input);
  }
};
}  // namespace pysystemc_interfaces

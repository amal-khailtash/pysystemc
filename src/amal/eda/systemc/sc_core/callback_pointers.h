/** @file callback_pointers.h
 * @brief Exact native callback-pointer values for otherwise unreflected typedefs.
 */
#pragma once

#include <sysc/utils/sc_list.h>
#include <sysc/utils/sc_pq.h>
#include <sysc/utils/sc_pvector.h>

#include <stdexcept>
#include <type_traits>

#include "report_handler_pointer.h"

namespace pysystemc
{
/** @brief Select the exact public native callback-pointer typedef. */
template <int Kind>
struct callback_pointer_traits;
/** @brief Native pointer-vector comparator signature. */
template <>
struct callback_pointer_traits<0>
{
  using type = sc_core::CFT;
};
/** @brief Native pointer-list mapper signature. */
template <>
struct callback_pointer_traits<1>
{
  using type = sc_core::sc_plist_map_fn;
};
/** @brief Own a value of an exact native callback typedef, without owning code.
 * @details This is a pointer-value adapter, not a reflected function-pointer
 * type. Native code must stay loaded for the lifetime of the value and every
 * container using it. Copies preserve pointer identity. No Python callbacks or
 * references are retained; construction, copy and comparison hold the GIL and
 * never invoke the callback. Null values may be copied but not invoked.
 */
template <int Kind>
class callback_pointer
{
 public:
  using pointer_type = typename callback_pointer_traits<Kind>::type;

  /** @brief Create a null pointer value. */
  callback_pointer() = default;
  /** @brief Copy an exact pointer value without acquiring code ownership. */
  callback_pointer(const callback_pointer&) = default;
  /** @brief Store an exact native pointer selected by a native factory. */
  explicit callback_pointer(pointer_type pointer) : m_value(pointer)
  {
  }
  /** @brief Check whether the stored value identifies native code. */
  bool valid() const
  {
    return m_value != nullptr;
  }
  /** @brief Compare exact native pointer identity, including null values. */
  bool operator==(const callback_pointer& other) const
  {
    return m_value == other.m_value;
  }
  /** @brief Compare exact native pointer inequality. */
  bool operator!=(const callback_pointer& other) const
  {
    return m_value != other.m_value;
  }
  /** @brief Borrow the exact pointer for synchronous native adapters.
   * @details cppyy may adapt the return value to std::function; native consumers
   * below read it in C++ instead, preserving the exact pointer identity.
   */
  pointer_type get() const
  {
    return m_value;
  }
  /** @brief Reject null callbacks before entering a native consumer. */
  void require_valid() const
  {
    if (!valid()) throw std::invalid_argument("cannot use a null native callback pointer");
  }

 private:
  pointer_type m_value = nullptr;
};

/** @brief Select a native function with compile-time exact signature checking.
 * @details No code or Python callback is owned. Signature mismatch fails template
 * instantiation. All native code remains borrowed for subsequent use.
 */
template <int Kind, typename callback_pointer<Kind>::pointer_type Function>
callback_pointer<Kind> make_callback_pointer()
{
  static_assert(std::is_same<decltype(Function), typename callback_pointer<Kind>::pointer_type>::value,
                "native function signature must match the callback typedef exactly");
  return callback_pointer<Kind>(Function);
}

/** @brief Sort borrowed vector slots through an exact native comparator.
 * @details qsort passes pointers to pointer slots, not pointees. The vector owns
 * slots but never pointees. GIL remains held; comparator exceptions must not cross
 * the C qsort ABI. No callback or pointee is retained by this synchronous call.
 */
template <class Vector>
void sort_callback_pointer(Vector& vector, const callback_pointer<0>& pointer)
{
  pointer.require_valid();
  vector.sort(pointer.get());
}

/** @brief Map list values with an exact native mapper and borrowed context.
 * @details List owns nodes, not payloads/context. Hold the GIL, keep them alive,
 * and do not erase the active traversal node in a mapper. Native exceptions
 * propagate; no callback or context ownership transfers.
 */
inline void map_callback_pointer(sc_core::sc_plist_base& list, const callback_pointer<1>& pointer, void* context)
{
  pointer.require_valid();
  list.mapcar(pointer.get(), context);
}

/** @brief Allocate a native queue retaining only an exact comparator pointer.
 * @details The returned queue owns its heap and must be deleted by the caller;
 * Python marks its proxy owning. Pointees and comparator code remain borrowed
 * through all queue operations. GIL remains held; allocation exceptions propagate.
 */
template <class Queue>
Queue* make_callback_queue(int size, const callback_pointer<0>& pointer)
{
  static_assert(std::is_base_of<sc_core::sc_ppq_base, Queue>::value, "queue must derive from sc_ppq_base");
  pointer.require_valid();
  return new Queue(size, pointer.get());
}

/** @brief Select an exact report handler without altering its pointer identity.
 * @details The token borrows native code and retains no Python callback. Hold
 * GIL on the simulation thread; callback registration owns Python retention.
 */
template <sc_core::sc_report_handler_proc Function>
report_handler_pointer make_report_callback_pointer()
{
  static_assert(std::is_same<decltype(Function), sc_core::sc_report_handler_proc>::value,
                "native function signature must match sc_report_handler_proc exactly");
  return {Function};
}
}  // namespace pysystemc

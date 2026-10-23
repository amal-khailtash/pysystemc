/** @file sc_core_priority_queue_probe.h
 * @brief Native support types for the public SystemC priority-queue audit.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_PRIORITY_QUEUE_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_PRIORITY_QUEUE_PROBE_H

#include <sysc/kernel/sc_event.h>
#include <sysc/kernel/sc_time.h>
#include <sysc/utils/sc_pq.h>

namespace sc_core_priority_queue_probe
{

/** @brief Pointer-backed item used to instantiate arbitrary sc_ppq element types. */
struct Item
{
  int key;      /**< Value used by the priority comparator. */
  int identity; /**< Stable caller-visible identity for duplicate keys. */
};

/** @brief Return one long-lived integer pointee for Python and C++ probes.
 * @param index Index in the fixed probe storage, from zero through four.
 * @return Borrowed pointer into probe-owned storage, valid for the process lifetime.
 */
inline int* integer_pointer(int index)
{
  static int values[5]{3, 1, 4, 1, 5};
  return &values[index];
}

/** @brief Return one long-lived typed item for arbitrary template instantiation.
 * @param index Index in the fixed probe storage, from zero through four.
 * @return Borrowed pointer into probe-owned storage, valid for the process lifetime.
 */
inline Item* item_pointer(int index)
{
  static Item items[5]{{8, 0}, {3, 1}, {8, 2}, {2, 3}, {5, 4}};
  return &items[index];
}

/** @brief Return one long-lived time value for the typed audit.
 * @param index Index in the fixed probe storage, from zero through three.
 * @return Borrowed pointer into probe-owned storage, valid for the process lifetime.
 */
inline sc_core::sc_time* time_pointer(int index)
{
  static sc_core::sc_time times[4]{sc_core::sc_time(4, sc_core::SC_NS), sc_core::sc_time(2, sc_core::SC_NS),
                                   sc_core::sc_time(1, sc_core::SC_NS), sc_core::sc_time(2, sc_core::SC_NS)};
  return &times[index];
}

/** @brief Read an integer behind a borrowed heap element pointer.
 * @param pointer Borrowed pointer to an integer retained by the caller.
 * @return The pointee value, copied without taking ownership.
 * @details This synchronous helper has no callbacks, GIL operations or exceptions.
 */
inline int read_integer(const void* pointer)
{
  return *static_cast<const int*>(pointer);
}

/** @brief Read the key behind a borrowed typed Item pointer.
 * @param pointer Borrowed Item pointer retained by the caller.
 * @return The pointee key, copied without taking ownership.
 * @details This synchronous helper has no callbacks, GIL operations or exceptions.
 */
inline int read_item_key(const void* pointer)
{
  return static_cast<const Item*>(pointer)->key;
}

/** @brief Compare two borrowed Item pointers using their keys.
 * @param left Borrowed first item pointer from the heap.
 * @param right Borrowed second item pointer from the heap.
 * @return Positive when left has higher priority; zero for equal keys.
 * @details The callback owns no storage or Python references and runs synchronously
 * on the caller's thread. The caller retains both items and the function pointer
 * for the complete queue lifetime. No GIL operations or exceptions are involved.
 */
inline int compare_items(const void* left, const void* right)
{
  const auto* lhs = static_cast<const Item*>(left);
  const auto* rhs = static_cast<const Item*>(right);
  return (lhs->key > rhs->key) - (lhs->key < rhs->key);
}

/** @brief Expose protected heap-index helpers for the native public API audit.
 * @details This probe owns its base queue's heap array. Inserted pointers and the
 * comparator callback remain caller-owned. Native callbacks are synchronous and
 * do not use Python or the GIL.
 */
class BaseProbe final : public sc_core::sc_ppq_base
{
 public:
  /** @brief Construct the base queue with native comparator and initial capacity.
   * @param size Initial allocation request, normalized by SystemC to at least two.
   * @param compare Borrowed native comparator retained by the caller.
   */
  BaseProbe(int size, compare_fn_t compare) : sc_ppq_base(size, compare)
  {
  }

  /** @brief Return the native parent index helper result. */
  int parent_index(int index) const
  {
    return parent(index);
  }

  /** @brief Return the native left-child index helper result. */
  int left_index(int index) const
  {
    return left(index);
  }

  /** @brief Return the native right-child index helper result. */
  int right_index(int index) const
  {
    return right(index);
  }

  /** @brief Invoke the protected native heap repair routine.
   * @param index Heap slot to repair in place.
   */
  void repair(int index)
  {
    heapify(index);
  }
};

/** @brief Compare two borrowed integer pointers for the base queue.
 * @param left Borrowed first integer pointer from the heap.
 * @param right Borrowed second integer pointer from the heap.
 * @return Positive when left has higher priority, negative when right does.
 * @details The caller owns both values and retains this synchronous callback for
 * the queue lifetime. No Python references, GIL operations or exceptions occur.
 */
inline int compare_ints(const void* left, const void* right)
{
  const auto lhs = *static_cast<const int*>(left);
  const auto rhs = *static_cast<const int*>(right);
  return (lhs > rhs) - (lhs < rhs);
}

/** @brief Rank earlier borrowed times ahead of later times.
 * @param left Borrowed sc_time pointer stored in the heap.
 * @param right Borrowed sc_time pointer stored in the heap.
 * @return Positive when left is earlier, negative when right is earlier.
 * @details The caller owns both sc_time values and retains this native callback
 * through queue destruction. Calls are synchronous without Python or GIL use.
 */
inline int compare_times(const void* left, const void* right)
{
  const auto* lhs = static_cast<const sc_core::sc_time*>(left);
  const auto* rhs = static_cast<const sc_core::sc_time*>(right);
  if (*lhs < *rhs) return 1;
  if (*lhs > *rhs) return -1;
  return 0;
}

/** @brief Compare null timed-event pointers without accessing private event storage.
 * @param left Borrowed timed-event pointer; this probe passes null pointers only.
 * @param right Borrowed timed-event pointer; this probe passes null pointers only.
 * @return Zero because both entries are null.
 * @details sc_event_timed constructors and destructors are private to SystemC.
 * The callback verifies the public typed-pointer queue surface without creating
 * or owning kernel-managed timed-event storage. No Python or GIL is used.
 */
inline int compare_null_timed_events(const void* left, const void* right)
{
  return (left != nullptr) - (right != nullptr);
}

/** @brief Report a borrowed sc_time pointer in nanoseconds.
 * @param value Borrowed time value retained by the caller.
 * @return A copied scalar count of nanoseconds.
 * @details This synchronous helper has no callbacks, GIL operations or exceptions.
 */
inline double time_nanoseconds(const sc_core::sc_time* value)
{
  return value->to_seconds() * 1.0e9;
}

/** @brief Return a typed null timed-event pointer without constructing private storage.
 * @return A null pointer; it owns no native or Python storage.
 * @details This avoids the private sc_event_timed constructor and destructor.
 */
inline sc_core::sc_event_timed* null_timed_event()
{
  return nullptr;
}

/** @brief Check a borrowed timed-event pointer without dereferencing kernel storage.
 * @param value Borrowed timed-event pointer, null in this probe.
 * @return True when the pointer is null.
 * @details This helper does not access or own kernel-managed timed-event storage.
 */
inline bool timed_event_is_null(const sc_core::sc_event_timed* value)
{
  return value == nullptr;
}

}  // namespace sc_core_priority_queue_probe

#endif  // PYSYSTEMC_TEST_SC_CORE_PRIORITY_QUEUE_PROBE_H

/** @file sc_core_lists_probe.h
 * @brief Small native payloads used to exercise public SystemC pointer lists.
 * @details List instances own their nodes but borrow every payload pointer.
 * The inline arrays below own the pointees for the duration of each process.
 */
#ifndef TESTS_NATIVE_SC_CORE_LISTS_PROBE_H
#define TESTS_NATIVE_SC_CORE_LISTS_PROBE_H

#include <sysc/utils/sc_list.h>

#include <array>

namespace sc_core_lists_probe
{

/** @brief Borrowed typed-list payload with a stable identity for assertions. */
struct Item
{
  int value;
  int identity;
};

/** @brief Mutable state passed by pointer to a synchronous map callback. */
struct MapState
{
  int total = 0;
  int calls = 0;
};

inline std::array<int, 8> integer_values{5, 10, 15, 20, 30, 35, 40, 16};
inline std::array<Item, 6> items{{{8, 0}, {3, 1}, {8, 2}, {2, 3}, {5, 4}, {13, 5}}};
inline MapState map_state{};

/**
 * @brief Return a borrowed pointer into the process-lifetime integer array.
 * @throws std::out_of_range When index is outside the helper array.
 */
inline int* integer_pointer(unsigned index)
{
  return &integer_values.at(index);
}

/** @brief Update a borrowed integer payload for list set-operation checks. */
inline void set_integer_value(unsigned index, int value)
{
  integer_values.at(index) = value;
}

/**
 * @brief Return a borrowed pointer into the process-lifetime item array.
 * @throws std::out_of_range When index is outside the helper array.
 */
inline Item* item_pointer(unsigned index)
{
  return &items.at(index);
}

/** @brief Return a borrowed typed-list address to exercise pointer iterator overloads. */
inline sc_core::sc_plist<Item*>* item_list_pointer(sc_core::sc_plist<Item*>& list)
{
  return &list;
}

/** @brief Read an integer payload stored in a void pointer. */
inline int read_integer(void* pointer)
{
  return *static_cast<int*>(pointer);
}

/** @brief Read an item payload through its arbitrary native pointer type. */
inline int read_item_value(Item* pointer)
{
  return pointer->value;
}

/** @brief Read an item's stable identity through its arbitrary native pointer type. */
inline int read_item_identity(Item* pointer)
{
  return pointer->identity;
}

/** @brief Return the address of callback state without transferring ownership. */
inline void* map_state_pointer()
{
  return &map_state;
}

/** @brief Check that mapcar forwards the exact caller-owned context pointer. */
inline bool is_map_state(void* pointer)
{
  return pointer == &map_state;
}

/**
 * @brief Native reference callback that accumulates borrowed integer values.
 * @details Invoked synchronously by mapcar. It neither retains the element or
 * context pointers nor enters Python, so no GIL operation is needed here.
 */
inline void accumulate_integer(void* value, void* context)
{
  auto* state = static_cast<MapState*>(context);
  state->total += *static_cast<int*>(value);
  ++state->calls;
}

}  // namespace sc_core_lists_probe

#endif  // TESTS_NATIVE_SC_CORE_LISTS_PROBE_H

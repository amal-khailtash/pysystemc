/** @file sc_core_lists_public.cpp
 * @brief Native reference for the public sc_plist linked-list families.
 * @details Each list owns its nodes and borrows stack-owned integer and Item
 * payloads. The map callback runs synchronously on this thread and retains no
 * callback state after mapcar returns. In the Python model, cppyy invokes the
 * Python map callback on the calling thread with the GIL available; the caller
 * keeps that callable alive for the duration of mapcar. Python callback
 * exceptions crossing this native call are outside this test's contract.
 */
#include <array>
#include <iostream>
#include <stdexcept>
#include <systemc>
#include <vector>

#include "sc_core_lists_probe.h"

namespace
{

/**
 * @brief Stop the native reference at the first incorrect observation.
 * @throws std::runtime_error When a native reference assertion fails.
 */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Read a base list into integers without changing its borrowed data. */
std::vector<int> values(sc_core::sc_plist_base& list)
{
  std::vector<int> result;
  sc_core::sc_plist_base_iter cursor(&list);
  while (!cursor.empty())
  {
    result.push_back(sc_core_lists_probe::read_integer(cursor.get()));
    cursor++;
  }
  return result;
}

}  // namespace

/** @brief Compare base and arbitrary-pointer typed list behavior with Python. */
int sc_main(int, char**)
{
  using sc_core_lists_probe::integer_pointer;
  using sc_core_lists_probe::Item;
  using sc_core_lists_probe::item_pointer;
  using sc_core_lists_probe::MapState;
  using sc_core_lists_probe::read_integer;
  using sc_core_lists_probe::read_item_identity;

  std::array<int, 8> borrowed{5, 10, 15, 20, 30, 35, 40, 16};
  std::vector<int> base_observation;
  {
    sc_core::sc_plist_base list;
    verify(list.empty() && list.size() == 0, "base initial state");
    auto h20 = list.push_back(&borrowed[3]);
    auto h10 = list.push_front(&borrowed[1]);
    auto h30 = list.push_back(&borrowed[4]);
    auto h15 = list.insert_before(h20, &borrowed[2]);
    auto h35 = list.insert_after(h20, &borrowed[5]);
    auto h40 = list.insert_before(nullptr, &borrowed[6]);
    list.insert_after(nullptr, &borrowed[0]);
    verify(list.size() == 7 && read_integer(list.front()) == 5 && read_integer(list.back()) == 40,
           "base push and insert operations");
    list.set(h15, &borrowed[7]);
    verify(read_integer(list.get(h15)) == 16, "base get and set handle");
    verify(values(list) == std::vector<int>({5, 10, 16, 20, 35, 30, 40}), "base forward traversal");

    sc_core::sc_plist_base_iter cursor(&list);
    cursor.set_handle(h35);
    verify(cursor.get_handle() == h35 && read_integer(cursor.get()) == 35, "base handle navigation");
    cursor.set(&borrowed[7]);
    verify(read_integer(cursor.get()) == 16, "base iterator set");
    cursor.remove();
    verify(cursor.get_handle() == h30 && read_integer(cursor.get()) == 30, "base iterator remove");
    cursor.remove(1);
    verify(cursor.get_handle() == h40 && read_integer(cursor.get()) == 40, "base directional iterator remove");
    cursor.set_handle(h20);
    cursor.remove(-1);
    verify(cursor.get_handle() == h15 && read_integer(cursor.get()) == 16, "base reverse directional removal");
    base_observation = values(list);
    cursor.reset(&list, true);
    std::vector<int> backward;
    while (!cursor.empty())
    {
      backward.push_back(read_integer(cursor.get()));
      if (backward.back() == 5) break;
      cursor--;
    }
    verify(backward == std::vector<int>({40, 16, 10, 5}), "base backward traversal and reset");
    verify(read_integer(list.remove(h15)) == 16, "base remove handle");
    verify(read_integer(list.pop_front()) == 5 && read_integer(list.pop_back()) == 40, "base pop endpoints");
    verify(list.size() == 1 && read_integer(list.front()) == 10 && read_integer(list.back()) == 10,
           "base list after removals");
    list.erase_all();
    verify(list.empty() && list.size() == 0, "base erase all");
  }
  verify(borrowed.front() == 5 && borrowed[7] == 16 && borrowed[6] == 40, "base preserves borrowed payloads");

  sc_core::sc_plist<Item*> typed;
  auto h0 = typed.push_back(item_pointer(0));
  auto h1 = typed.push_front(item_pointer(1));
  auto h2 = typed.push_back(item_pointer(2));
  auto h4 = typed.insert_after(h0, item_pointer(4));
  auto h3 = typed.insert_before(h0, item_pointer(3));
  typed.set(h0, item_pointer(5));
  verify(typed.size() == 5 && read_item_identity(typed.front()) == 1 && read_item_identity(typed.back()) == 2,
         "typed arbitrary pointer insertion");
  verify(read_item_identity(typed.get(h0)) == 5 && read_item_identity(typed.get(h3)) == 3, "typed handle get and set");
  sc_core::sc_plist_iter<Item*> typed_cursor(typed, true);
  sc_core::sc_plist_iter<Item*> typed_pointer_cursor(&typed, true);
  verify(read_item_identity(typed_pointer_cursor.get()) == 2, "typed iterator pointer constructor");
  typed_pointer_cursor.reset(&typed, false);
  verify(read_item_identity(typed_pointer_cursor.get()) == 1, "typed iterator pointer reset");
  verify(read_item_identity(typed_cursor.get()) == 2, "typed iterator from tail");
  typed_cursor--;
  verify(read_item_identity(*typed_cursor) == 4, "typed iterator dereference and decrement");
  typed_cursor.set(item_pointer(0));
  verify(read_item_identity(typed.get(h4)) == 0, "typed iterator set");
  typed_cursor.reset(&typed, false);
  verify(read_item_identity(typed_cursor.get()) == 1, "typed iterator pointer reset");
  typed_cursor++;
  verify(read_item_identity(typed_cursor.get()) == 3, "typed iterator increment");
  typed_cursor.remove(1);
  verify(read_item_identity(typed_cursor.get()) == 5, "typed iterator directional removal");
  typed_cursor.reset(typed, true);
  verify(read_item_identity(typed_cursor.get()) == 2, "typed iterator reference reset");
  verify(read_item_identity(typed.pop_front()) == 1 && read_item_identity(typed.pop_back()) == 2,
         "typed pop endpoints");
  verify(typed.size() == 2 && read_item_identity(typed.front()) == 5 && read_item_identity(typed.back()) == 0,
         "typed list after iterator removal and pops");
  verify(read_item_identity(typed.remove(h4)) == 0 && typed.size() == 1, "typed remove handle");
  typed.erase_all();
  verify(typed.empty() && typed.size() == 0, "typed erase all");

  sc_core::sc_plist<Item*> null_typed;
  null_typed.insert_before(nullptr, item_pointer(0));
  null_typed.insert_after(nullptr, item_pointer(1));
  verify(
    null_typed.size() == 2 && read_item_identity(null_typed.front()) == 1 && read_item_identity(null_typed.back()) == 0,
    "typed null-handle insertion");
  null_typed.erase_all();
  verify(null_typed.empty(), "typed null-handle list cleanup");

  sc_core::sc_plist_base mapped;
  mapped.push_back(integer_pointer(0));
  mapped.push_back(integer_pointer(1));
  mapped.push_back(integer_pointer(2));
  sc_core_lists_probe::map_state = MapState{};
  sc_core::sc_plist_map_fn callback = sc_core_lists_probe::accumulate_integer;
  mapped.mapcar(callback, sc_core_lists_probe::map_state_pointer());
  verify(sc_core_lists_probe::map_state.calls == 3 && sc_core_lists_probe::map_state.total == 30,
         "native map callback and context");

  std::cout << "SC_PLIST_REFERENCE base=";
  for (std::size_t index = 0; index < base_observation.size(); ++index)
  {
    if (index) std::cout << ',';
    std::cout << base_observation[index];
  }
  std::cout << " typed=1,5 map=30,3 borrowed=1\n";
  return 0;
}

/** @file sc_core_priority_queue_public.cpp
 * @brief Native reference for sc_ppq_base and arbitrary sc_ppq pointer elements.
 * @details All heap arrays are owned by their queue instances. The queues borrow
 * pointers to caller-owned stack items and borrow static native comparison
 * functions. Comparisons run synchronously on this thread without Python, GIL
 * operations or escaping exceptions.
 */
#include <array>
#include <iostream>
#include <stdexcept>
#include <systemc>

#include "sc_core_priority_queue_probe.h"

namespace
{

/** @brief Stop the native reference on the first mismatched observation. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

}  // namespace

/** @brief Compare base and typed queues across capacity growth and extraction. */
int sc_main(int, char**)
{
  using sc_core_priority_queue_probe::BaseProbe;
  using sc_core_priority_queue_probe::compare_ints;
  using sc_core_priority_queue_probe::compare_items;
  using sc_core_priority_queue_probe::compare_null_timed_events;
  using sc_core_priority_queue_probe::compare_times;
  using sc_core_priority_queue_probe::Item;

  std::array<int, 5> values{3, 1, 4, 1, 5};
  {
    BaseProbe queue(1, compare_ints);
    verify(queue.empty() && queue.size() == 0, "base initial state");
    verify(queue.parent_index(7) == 3 && queue.left_index(4) == 8 && queue.right_index(4) == 9,
           "protected index helpers");

    for (auto& value : values) queue.insert(&value);
    verify(!queue.empty() && queue.size() == 5, "base insertion and capacity growth");
    verify(*static_cast<int*>(queue.top()) == 5, "base top");
    queue.repair(1);
    verify(*static_cast<int*>(queue.top()) == 5, "protected heapify");

    constexpr std::array<int, 5> expected{5, 4, 3, 1, 1};
    for (std::size_t index = 0; index < expected.size(); ++index)
    {
      verify(!queue.empty(), "base nonempty before extraction");
      verify(*static_cast<int*>(queue.extract_top()) == expected[index], "base extraction ordering");
      verify(queue.size() == static_cast<int>(expected.size() - index - 1), "base extraction size");
    }
    verify(queue.empty() && queue.size() == 0, "base final empty state");
  }
  verify(values[0] == 3 && values[4] == 5, "base queue preserves borrowed values");

  std::array<Item, 5> items{{{8, 0}, {3, 1}, {8, 2}, {2, 3}, {5, 4}}};
  {
    sc_core::sc_ppq<Item*> queue(2, compare_items);
    verify(queue.empty() && queue.size() == 0, "typed initial state");
    for (auto& item : items) queue.insert(&item);
    verify(!queue.empty() && queue.size() == 5, "typed insertion and capacity growth");
    verify(queue.top()->key == 8, "typed top");

    constexpr std::array<int, 5> expected{8, 8, 5, 3, 2};
    for (std::size_t index = 0; index < expected.size(); ++index)
    {
      verify(!queue.empty(), "typed nonempty before extraction");
      Item* extracted = queue.extract_top();
      verify(extracted->key == expected[index], "typed extraction ordering with duplicates");
      verify(extracted >= items.data() && extracted < items.data() + items.size(), "typed result is borrowed item");
      verify(queue.size() == static_cast<int>(expected.size() - index - 1), "typed extraction size");
    }
    verify(queue.empty(), "typed final empty state");
  }
  verify(items[0].key == 8 && items[2].key == 8 && items[4].key == 5, "typed queue preserves borrowed items");

  std::array<sc_core::sc_time, 4> times{sc_core::sc_time(4, sc_core::SC_NS), sc_core::sc_time(2, sc_core::SC_NS),
                                        sc_core::sc_time(1, sc_core::SC_NS), sc_core::sc_time(2, sc_core::SC_NS)};
  {
    sc_core::sc_ppq<sc_core::sc_time*> queue(2, compare_times);
    for (auto& time : times) queue.insert(&time);
    verify(*queue.top() == sc_core::sc_time(1, sc_core::SC_NS), "sc_time pointer top");
    constexpr std::array<int, 4> expected{1, 2, 2, 4};
    for (std::size_t index = 0; index < expected.size(); ++index)
    {
      sc_core::sc_time* extracted = queue.extract_top();
      verify(*extracted == sc_core::sc_time(expected[index], sc_core::SC_NS), "sc_time pointer extraction ordering");
    }
    verify(queue.empty(), "sc_time pointer final empty state");
  }
  verify(times[0] == sc_core::sc_time(4, sc_core::SC_NS) && times[2] == sc_core::sc_time(1, sc_core::SC_NS),
         "sc_time queue preserves borrowed values");

  {
    sc_core::sc_ppq<sc_core::sc_event_timed*> queue(2, compare_null_timed_events);
    queue.insert(nullptr);
    queue.insert(nullptr);
    verify(queue.size() == 2 && queue.top() == nullptr, "sc_event_timed pointer typed top");
    verify(queue.extract_top() == nullptr && queue.extract_top() == nullptr, "sc_event_timed pointer typed extraction");
    verify(queue.empty(), "sc_event_timed pointer final empty state");
  }

  std::cout
    << "SC_PPQ_REFERENCE base=5,4,3,1,1 typed=8,8,5,3,2 time=1,2,2,4 event_timed=null protected=3,8,9 borrowed=1\n";
  return 0;
}

/** @file sc_core_clock_vector_public.cpp
 * @brief Native reference for sc_clock and arbitrary sc_pvector pointers.
 * @details Clocks own their signal, events and elaborated edge processes.
 * Pointer vectors own their slot storage but only borrow fixture pointees and
 * comparator functions. Calls and qsort comparisons are synchronous native
 * work with no Python callback or GIL interaction.
 */
#include <array>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>

#include "sc_core_clock_vector_probe.h"

namespace
{

/** @brief Stop the reference as soon as an API contract differs. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

int integer_comparisons = 0;

/** @brief Compare vector slots by their borrowed integer pointees. */
int compare_integer_slots(const void* left, const void* right)
{
  ++integer_comparisons;
  const int lhs = sc_core_clock_vector_probe::integer_slot_value(left);
  const int rhs = sc_core_clock_vector_probe::integer_slot_value(right);
  return (lhs > rhs) - (lhs < rhs);
}

/** @brief Count one expected native report while preserving the caller state. */
template <class F>
int report_id(F action)
{
  try
  {
    action();
  }
  catch (const sc_core::sc_report& report)
  {
    return report.get_id();
  }
  return -1;
}

}  // namespace

/** @brief Compare clock overloads, reports, edge timing and pointer containers. */
int sc_main(int, char**)
{
  using namespace sc_core;
  using namespace sc_core_clock_vector_probe;

  sc_clock default_clock;
  sc_clock named_clock("clock_named");
  sc_clock time_clock("clock_time", sc_time(8, SC_NS), 0.25, sc_time(2, SC_NS), false);
  sc_clock short_clock("clock_short", 10, SC_NS, 0.3);
  sc_clock short_default_duty_clock("clock_short_default", 6, SC_NS);
  sc_clock long_clock("clock_long", 12, SC_NS, 0.25, 3, SC_NS, false);
  sc_clock legacy_clock("clock_legacy", 14, 0.5, 2, false);
  sc_clock fallback_clock("clock_fallback", sc_time(4, SC_NS), 0.0);

  verify(default_clock.period() == sc_time(1, SC_NS), "default clock period");
  verify(named_clock.period() == sc_time(1, SC_NS), "named default clock period");
  verify(std::string(time_clock.kind()) == "sc_clock", "clock kind");
  verify(time_clock.period() == sc_time(8, SC_NS), "sc_time clock constructor");
  verify(time_clock.duty_cycle() == 0.25 && time_clock.start_time() == sc_time(2, SC_NS) && !time_clock.posedge_first(),
         "sc_time clock characteristics");
  verify(short_clock.period() == sc_time(10, SC_NS) && short_clock.duty_cycle() == 0.3 && short_clock.posedge_first() &&
           short_clock.start_time() == SC_ZERO_TIME,
         "short time-unit clock constructor");
  verify(short_default_duty_clock.period() == sc_time(6, SC_NS) && short_default_duty_clock.duty_cycle() == 0.5 &&
           short_default_duty_clock.start_time() == SC_ZERO_TIME && short_default_duty_clock.posedge_first(),
         "short time-unit clock constructor default duty");
  verify(long_clock.period() == sc_time(12, SC_NS) && long_clock.start_time() == sc_time(3, SC_NS) &&
           !long_clock.posedge_first(),
         "long time-unit clock constructor");
  verify(legacy_clock.period() == sc_time(14, SC_NS) && legacy_clock.start_time() == sc_time(2, SC_NS) &&
           !legacy_clock.posedge_first(),
         "legacy clock constructor");
  verify(fallback_clock.duty_cycle() == 0.5, "invalid duty cycle fallback");
  verify(protected_clock_helper(), "protected native clock helper");
  verify(sc_clock::time_stamp() == sc_time_stamp(), "static clock timestamp");

  const std::array<int, 5> report_ids = {
    report_id([] { sc_clock invalid("clock_zero", SC_ZERO_TIME); }),
    report_id([] { sc_clock invalid("clock_high_zero", sc_time(1, SC_PS), 0.000001); }),
    report_id([] { sc_clock invalid("clock_low_zero", sc_time(1, SC_PS), 0.999999); }),
    report_id([&] { time_clock.write(true); }),
    output_port_registration_report_id(time_clock),
  };
  verify(report_ids == std::array<int, 5>{101, 102, 103, 125, 128}, "exact native clock report IDs");

  verify(!default_clock.read() && !named_clock.read(), "initial positive-edge-first clock value");
  verify(time_clock.read() && long_clock.read() && legacy_clock.read(), "initial negative-edge-first clock value");
  sc_start(2.25, SC_NS);
  verify(default_clock.read() && named_clock.read() && !time_clock.read() && short_clock.read() && long_clock.read() &&
           !legacy_clock.read(),
         "clock values after start offsets");
  sc_start(1, SC_NS);
  verify(!short_clock.read() && !long_clock.read(), "clock duty-cycle edges");
  verify(sc_time_stamp() == sc_time(3.25, SC_NS), "clock simulation time");
  verify(sc_clock::time_stamp() == sc_time_stamp(), "static clock timestamp after simulation");

  sc_pvector<int*> items;
  for (unsigned index = 0; index < 3; ++index) items.push_back(integer_pointer(index));
  verify(items.size() == 3 && *items.fetch(1) == 1 && *items[0] == 3, "integer pointer vector access");
  items[4] = integer_pointer(3);
  verify(items.size() == 5 && items[3] == nullptr, "index resize and value initialization");
  items.put(integer_pointer(4), 3);
  verify(*items.fetch(3) == 5 && converted_size(items) == 5, "put and vector conversion");
  verify(iterator_storage_agrees(items) && const_storage_agrees(items), "integer raw storage and iterator aliases");

  sc_pvector<int*> copied(items);
  sc_pvector<int*> assigned;
  assign(assigned, copied);
  verify(copied.size() == 5 && assigned.size() == 5 && *assigned.fetch(4) == 1,
         "integer copy construction and assignment");
  items.sort(compare_integer_slots);
  constexpr std::array<int, 5> sorted_expected{1, 1, 3, 4, 5};
  for (std::size_t index = 0; index < sorted_expected.size(); ++index)
    verify(*items[index] == sorted_expected[index], "integer pointer sorting");
  verify(*copied.fetch(0) == 3, "copy owns slots and borrows pointees");
  items.decr_count();
  items.decr_count(2);
  verify(items.size() == 2, "decr_count overloads");
  items.erase_all();
  verify(items.size() == 0 && const_storage_agrees(items), "erase_all and empty storage");

  sc_pvector<Owner*> owners;
  for (unsigned index = 0; index < 3; ++index) owners.push_back(owner_pointer(index));
  sc_pvector<Owner*> owner_copy(owners);
  verify(owner_value(owners.fetch(0)) == 17 && owner_value(owners.fetch(1)) == 4 &&
           owner_value(owners.fetch(2)) == 29 && owner_value(owner_copy[1]) == 4 &&
           owner_vector_points_to_fixture(owner_copy, 1, 1) && iterator_storage_agrees(owners),
         "arbitrary Owner pointer vector");

  sc_pvector<sc_time*> times;
  for (unsigned index = 0; index < 3; ++index) times.push_back(time_pointer(index));
  sc_pvector<sc_time*> time_assigned;
  assign(time_assigned, times);
  verify(time_assigned.size() == 3 && converted_size(time_assigned) == 3, "sc_time pointer assignment and conversion");
  verify(time_nanoseconds(time_assigned.fetch(0)) == 3 && time_nanoseconds(time_assigned.fetch(1)) == 1 &&
           time_nanoseconds(time_assigned.fetch(2)) == 2,
         "arbitrary sc_time pointer values");

  sc_pvector<Item*> typed_items;
  typed_items[0] = item_pointer(0);
  typed_items[2] = item_pointer(1);
  verify(typed_items.size() == 3 && typed_items[1] == nullptr && item_vector_value(typed_items, 0) == 11 &&
           item_vector_points_to_fixture(typed_items, 2, 1),
         "arbitrary Item pointer indexing and auto-growth");
  typed_items[1] = item_pointer(2);
  verify(item_vector_value(typed_items, 1) == 19, "arbitrary Item pointer assignment");

  std::cout << "SC_CORE_CLOCK_VECTOR clock=" << std::fixed << std::setprecision(0)
            << time_clock.period().to_seconds() * 1e9 << ',' << std::setprecision(2) << time_clock.duty_cycle() << ','
            << std::setprecision(0) << time_clock.start_time().to_seconds() * 1e9 << ',' << time_clock.posedge_first()
            << ",unit_default=6,0.50,0,edges=" << default_clock.read() << time_clock.read() << short_clock.read()
            << long_clock.read() << legacy_clock.read() << ",time=" << std::setprecision(0)
            << sc_time_stamp().to_seconds() * 1e9 << ",reports=" << report_ids[0] << ',' << report_ids[1] << ','
            << report_ids[2] << ',' << report_ids[3] << ',' << report_ids[4] << ",protected=1"
            << " vector=1,1,3,4,5 owners=17,4,29 items=11,19 times=3,1,2 copied=" << converted_size(copied)
            << " callback=" << integer_comparisons << '\n';
  return 0;
}

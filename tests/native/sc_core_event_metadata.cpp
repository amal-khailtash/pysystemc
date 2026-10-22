/** @file sc_core_event_metadata.cpp
 * @brief Compare event names and borrowed parents with Python metadata.
 * Events are destroyed before their owning module; copied names own storage.
 */
#include <iostream>
#include <string>
#include <systemc>
/** @brief Expose the native protected module constructor for the reference. */
struct Owner : sc_core::sc_module
{
  explicit Owner(sc_core::sc_module_name name) : sc_core::sc_module(name)
  {
  }
};
/** @brief Validate hierarchy metadata without starting the native scheduler. */
int sc_main(int, char**)
{
  using namespace sc_core;
  sc_event outside("outside");
  if (std::string(outside.name()) != "outside" || std::string(outside.basename()) != "outside" ||
      !outside.in_hierarchy() || outside.get_parent_object())
    return 1;
  sc_event generated;
  if (!generated.in_hierarchy() || std::string(generated.name()).find("event_") != 0 || generated.get_parent_object())
    return 4;
  if (sc_event::none.triggered()) return 5;
  sc_event_or_list or_source(outside);
  sc_event_and_list and_source(outside);
  sc_event_or_list or_combined(generated | or_source);
  sc_event_and_list and_combined(generated & and_source);
  sc_event_or_list or_direct(outside | generated);
  sc_event_and_list and_direct(outside & generated);
  if (or_combined.size() != 2 || and_combined.size() != 2 || or_source.size() != 1 || and_source.size() != 1 ||
      or_direct.size() != 2 || and_direct.size() != 2)
    return 6;
  {
    sc_event pending("pending");
    pending.notify(sc_time(1, SC_NS));
  }
  sc_module_name name("owner");
  Owner owner(name);
  std::string snapshot;
  {
    sc_event inside("inside");
    if (std::string(inside.name()) != "owner.inside" || std::string(inside.basename()) != "inside" ||
        !inside.in_hierarchy() || inside.get_parent_object() != &owner)
      return 2;
    snapshot = inside.name();
  }
  if (snapshot != "owner.inside" || std::string(owner.name()) != "owner") return 3;
  std::cout << "EVENT_METADATA_REFERENCE " << outside.name() << " " << outside.basename() << " " << snapshot << " "
            << owner.name() << "\n";
  return 0;
}

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

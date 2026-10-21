/** @file sc_core_name_registry.cpp
 * @brief Native registry reference; parent/event owners stay alive for all calls.
 * @details No callbacks or ownership transfers; run on the simulation thread.
 */
#include <stdexcept>
/** @brief Construct a native module through its public module-name constructor. */
struct registry_module : sc_core::sc_module
{
  explicit registry_module(sc_core::sc_module_name name) : sc_module(name)
  {
  }
};
#include <systemc>
/** @brief Compare full-name and parent/local-name registry overloads. */
int sc_main(int, char**)
{
  registry_module top("registry");
  sc_core::sc_event event("registry.event");
  for (const sc_core::sc_object* parent :
       {static_cast<const sc_core::sc_object*>(nullptr), static_cast<const sc_core::sc_object*>(&top)})
  {
    auto exists = [&](const char* key)
    { return parent ? sc_core::sc_hierarchical_name_exists(parent, key) : sc_core::sc_hierarchical_name_exists(key); };
    auto get = [&](const char* key)
    { return parent ? sc_core::sc_get_hierarchical_name(parent, key) : sc_core::sc_get_hierarchical_name(key); };
    auto add = [&](const char* key) {
      return parent ? sc_core::sc_register_hierarchical_name(parent, key) : sc_core::sc_register_hierarchical_name(key);
    };
    auto remove = [&](const char* key)
    {
      return parent ? sc_core::sc_unregister_hierarchical_name(parent, key)
                    : sc_core::sc_unregister_hierarchical_name(key);
    };
    const char* key = "external";
    std::string full = parent ? "registry.external" : "external";
    if (exists(key) || get(key) || !add(key) || add(key) || !exists(key)) return 2;
    std::string copied = get(key);
    if (copied != full || sc_core::sc_find_object(full.c_str()) || sc_core::sc_find_event(full.c_str()) ||
        !remove(key) || remove(key) || get(key) || copied != full)
      return 3;
    std::cout << "NAME_REGISTRY_REFERENCE " << full << " 1 0 1 1 0 0\n";
  }
  if (!sc_core::sc_hierarchical_name_exists("registry") || sc_core::sc_unregister_hierarchical_name("registry") ||
      !sc_core::sc_hierarchical_name_exists(&top, "event") || sc_core::sc_register_hierarchical_name(&top, "event") ||
      sc_core::sc_unregister_hierarchical_name(&top, "event"))
    return 4;
  std::cout << "NAME_REGISTRY_OBJECTS_REFERENCE 1 0 1 0 0\n";
  return 0;
}

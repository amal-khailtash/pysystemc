/** @file sc_core_object_lookup.cpp
 * @brief Native lookup/null/parent/top-level hierarchy reference.
 * @details All returned pointers are borrowed from a live module/context.
 */
#include <systemc>
/** @brief Own named objects and events for hierarchy lookup. */
struct model : sc_core::sc_module
{
  sc_core::sc_signal<int> signal{"signal"};
  sc_core::sc_event event{"event"};
  model(sc_core::sc_module_name name) : sc_module(name)
  {
    if (sc_core::sc_get_current_object() != this) throw std::runtime_error("construction context");
  }
};
/** @brief Verify present/missing lookup and explicit/default context views. */
int sc_main(int, char**)
{
  model top("lookup");
  auto* context = sc_core::sc_get_curr_simcontext();
  sc_core::sc_event root_event("root_event");
  if (sc_core::sc_get_current_object() || sc_core::sc_find_object("lookup.signal") != &top.signal ||
      sc_core::sc_find_event("lookup.event") != &top.event || sc_core::sc_find_object("missing") ||
      sc_core::sc_find_event("missing") || sc_core::sc_get_parent(&top.signal) != &top || sc_core::sc_get_parent(&top))
    return 2;
  for (bool explicit_context : {false, true})
  {
    const auto& objects =
      explicit_context ? sc_core::sc_get_top_level_objects(context) : sc_core::sc_get_top_level_objects();
    const auto& events =
      explicit_context ? sc_core::sc_get_top_level_events(context) : sc_core::sc_get_top_level_events();
    if (std::find(objects.begin(), objects.end(), &top) == objects.end() ||
        std::find(events.begin(), events.end(), &top.event) != events.end() ||
        std::find(events.begin(), events.end(), &root_event) == events.end())
      return 3;
  }
  sc_core::sc_start(1, sc_core::SC_NS);
  sc_core::sc_stop();
  std::cout << "OBJECT_LOOKUP_REFERENCE 1 1 1 1 1\n";
  return 0;
}

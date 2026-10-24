/** @file sc_core_object_public.cpp
 *  @brief Independent native reference for the sc_object declaration family.
 */
#include <systemc>
#include <type_traits>

#include "sc_core_object_probe.h"

/** @brief Verify native public operations and report observable parity records. */
int sc_main(int, char**)
{
  using namespace sc_core;
  using namespace sc_core_object_probe;
  static_assert(std::is_same_v<sc_object::phase_cb_mask, unsigned>);
  ObjectTreeProbe* tree = new ObjectTreeProbe("object_public");
  ObjectProbe& first = tree->first;
  ObjectProbe& second = tree->second;
  if (first.name() != std::string("object_public.first") || first.basename() != std::string("first") ||
      std::string(first.kind()) != "sc_object" || first.get_parent_object() != tree || first.get_parent() != tree ||
      tree->get_parent_object() != nullptr || first.simcontext() != sc_get_curr_simcontext())
    return 2;
  if (tree->get_child_objects().size() != 2 || tree->get_child_objects()[0] != &first ||
      tree->get_child_objects()[1] != &second || tree->get_child_events().size() != 1 ||
      tree->get_child_events()[0] != &tree->event)
    return 3;

  std::ostringstream printed_stream;
  first.print(printed_stream);
  std::ostringstream dumped_stream;
  first.dump(dumped_stream);
  first.trace(nullptr);
  if (printed_stream.str() != first.name() || dumped_stream.str() != "name = object_public.first\nkind = sc_object\n" ||
      printed(first) != printed_stream.str() || dumped(first) != dumped_stream.str())
    return 4;

  sc_attribute<int> attribute("public_value", 41), duplicate("public_value", 99), other("other_value", 7);
  if (first.num_attributes() != 0 || !first.add_attribute(attribute) || first.add_attribute(duplicate) ||
      !first.add_attribute(other) || first.num_attributes() != 2 || first.get_attribute("public_value") != &attribute ||
      const_attribute(first, "public_value") != &attribute || first.get_attribute("absent") ||
      const_attribute(first, "absent") || first.attr_cltn().size() != 2 || const_attributes(first).size() != 2 ||
      &first.attr_cltn() != &const_attributes(first))
    return 5;
  if (first.remove_attribute("public_value") != &attribute || first.remove_attribute("absent") ||
      first.num_attributes() != 1)
    return 6;
  first.remove_all_attributes();
  if (first.num_attributes() != 0 || first.attr_cltn().size() != 0) return 7;

  ObjectProbe default_object;
  ObjectProbe named_object("named_object");
  ObjectProbe copied_object(first);
  if (default_object.name() == nullptr || named_object.name() != std::string("named_object") ||
      copied_object.basename() != std::string("first_0") || copied_object.name() == first.name())
    return 8;
  ObjectProbe helper_parent("helper_parent");
  ObjectProbe helper_child("helper_child");
  sc_event helper_event("helper_event");
  sc_attribute<int> assignment_attribute("assignment", 5);
  if (!helper_parent.add_attribute(assignment_attribute)) return 9;
  helper_parent.add_child(&helper_child);
  helper_parent.add_event(&helper_event);
  if (helper_parent.get_child_objects().size() != 1 || helper_parent.get_child_objects()[0] != &helper_child ||
      helper_parent.get_child_events().size() != 1 || helper_parent.get_child_events()[0] != &helper_event ||
      &helper_parent.assign_from(helper_child) != &helper_parent ||
      helper_parent.name() != std::string("helper_parent") || helper_child.name() != std::string("helper_child") ||
      helper_parent.num_attributes() != 1 || helper_parent.get_attribute("assignment") != &assignment_attribute ||
      helper_parent.get_child_objects().size() != 1 || helper_parent.get_child_events().size() != 1 ||
      &helper_parent.assign_from(helper_parent) != &helper_parent ||
      helper_parent.remove_attribute("assignment") != &assignment_attribute ||
      !helper_parent.remove_child(&helper_child) || helper_parent.remove_child(&helper_child) ||
      !helper_parent.remove_event(&helper_event) || helper_parent.remove_event(&helper_event))
    return 10;
  const auto mask = static_cast<sc_object::phase_cb_mask>(SC_BEFORE_END_OF_ELABORATION);
  if (helper_parent.register_mask(mask) != mask || helper_parent.unregister_mask(mask) != 0) return 11;

  std::cout << "SC_OBJECT_PUBLIC_REFERENCE hierarchy=" << first.name() << " parent=" << tree->name()
            << " children=2 events=1 attributes=2,1,0 output='" << printed(first)
            << "' copy=" << copied_object.basename() << '\n';
  delete tree;
  if (sc_find_object("object_public") || sc_find_object("object_public.first")) return 12;
  return 0;
}

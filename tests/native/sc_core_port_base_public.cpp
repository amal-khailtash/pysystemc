/** @file sc_core_port_base_public.cpp
 *  @brief Independent native reference for the sc_port_base declaration family.
 */
#include <systemc>
#include <type_traits>

#include "sc_core_port_base_probe.h"

/** @brief Verify native public operations and report observable parity records. */
int sc_main(int, char**)
{
  using namespace sc_core;
  using namespace sc_core_port_base_probe;
  static_assert(std::is_same_v<sc_port_base::this_type, sc_port_base>);
  PortTreeProbe* tree = new PortTreeProbe("port_public");
  PortProbe& unnamed = tree->unnamed;
  PortProbe& default_policy = tree->default_policy;
  PortProbe& named = tree->named;
  PortProbe& named_default_policy = tree->named_default_policy;
  PortProbe& parent = tree->parent;
  PortProbe& child = tree->child;
  ProbeInterface& interface = tree->interface;
  if (unnamed.name() == nullptr || std::string(unnamed.kind()) != "sc_port_base" || default_policy.name() == nullptr ||
      default_policy.bind_count() != 0 || named.name() != std::string("port_public.named_port") ||
      std::string(named.kind()) != "sc_port_base" ||
      named_default_policy.name() != std::string("port_public.named_default_policy") || !named.has_binding_state() ||
      !parent.has_binding_state() || named.bind_count() != 0 || named.get_interface() || const_interface(named) ||
      std::string(named.get_interface_type().name()) != std::string(parent.get_interface_type().name()))
    return 2;

  bind_interface(named, interface);
  bind_parent(child, parent);
  if (named.bind_count() != 1 || parent.bind_count() != 0 || child.bind_count() != 1 ||
      !same_interface(named.get_interface(), &interface) || !same_interface(const_interface(named), &interface) ||
      child.get_interface() || const_interface(child) || child.get_parent_object() != tree)
    return 3;
  if (named.run_lifecycle_hooks() != "BESF" || child.run_lifecycle_hooks() != "BESF") return 4;
  const std::string diagnostic = named.report_error_text();
  if (diagnostic.find(named.name()) == std::string::npos || diagnostic.find("port probe") == std::string::npos)
    return 5;
  unnamed.probe_method_sensitivity();
  unnamed.probe_thread_sensitivity();

  std::cout << "SC_PORT_BASE_PUBLIC_REFERENCE named=" << named.name() << " kind=" << named.kind()
            << " binds=" << named.bind_count() << ',' << child.bind_count()
            << " interface=" << static_cast<bool>(named.get_interface())
            << " parent_interface=" << static_cast<bool>(child.get_interface())
            << " hooks=" << named.run_lifecycle_hooks() << " error="
            << (diagnostic.find(named.name()) != std::string::npos &&
                diagnostic.find("port probe") != std::string::npos)
            << '\n';
  delete tree;
  if (sc_find_object("port_public") || sc_find_object("port_public.named_port")) return 6;
  return 0;
}

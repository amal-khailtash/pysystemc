/** @file sc_core_ports_public.cpp
 *  @brief Native reference for the generic sc_port and sc_port_b families.
 */
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "sc_core_ports_probe.h"

namespace
{
using namespace sc_core_ports_probe;
using custom_port = sc_core::sc_port<ValueIf, 0, sc_core::SC_ZERO_OR_MORE_BOUND>;
using custom_port_b = PortBProbe<ValueIf>;
using owned_port = std::pair<sc_core::sc_port_base*, void (*)(sc_core::sc_port_base*)>;

static_assert(std::is_same<sc_core::sc_port_b<ValueIf>::base_type, sc_core::sc_port_base>::value,
              "sc_port_b base_type alias");
static_assert(std::is_same<sc_core::sc_port_b<ValueIf>::this_type, sc_core::sc_port_b<ValueIf>>::value,
              "sc_port_b this_type alias");
static_assert(std::is_same<sc_core::sc_port_b<ValueIf>::port_type, sc_core::sc_port_b<ValueIf>>::value,
              "sc_port_b port_type alias");
static_assert(!std::is_copy_constructible<custom_port>::value, "sc_port copy construction is disabled");
static_assert(!std::is_copy_assignable<custom_port>::value, "sc_port copy assignment is disabled");

/** @brief Reject a failed native check without release-disabled assertions. */
void require(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Verify every port overload for one live native interface specialization. */
template <class IF, class CHANNEL>
void check_specialization(const char* label, CHANNEL& channel, std::vector<owned_port>& ports)
{
  using port_type = sc_core::sc_port<IF>;
  using port_b = PortBProbe<IF>;
  using native_port_b = sc_core::sc_port_b<IF>;
  static_assert(std::is_same<typename native_port_b::base_type, sc_core::sc_port_base>::value,
                "sc_port_b base_type alias");
  static_assert(std::is_same<typename native_port_b::this_type, native_port_b>::value, "sc_port_b this_type alias");
  static_assert(std::is_same<typename native_port_b::port_type, native_port_b>::value, "sc_port_b port_type alias");
  const std::string prefix(label);
  const auto retain = [&ports](auto* port)
  {
    using port_type = typename std::remove_pointer<decltype(port)>::type;
    ports.emplace_back(port, [](sc_core::sc_port_base* base) { delete static_cast<port_type*>(base); });
    return port;
  };

  auto* default_port = retain(new port_type());
  auto* named_port = retain(new port_type((prefix + "_named").c_str()));
  default_port->bind(channel);
  (*named_port)(channel);
  auto* by_interface = retain(new port_type(channel));
  auto* named_by_interface = retain(new port_type((prefix + "_by_interface").c_str(), channel));
  auto* base_unnamed = retain(new port_b(1, sc_core::SC_ZERO_OR_MORE_BOUND));
  auto* base_parent = retain(new port_b((prefix + "_base_parent").c_str(), 1, sc_core::SC_ZERO_OR_MORE_BOUND));
  base_unnamed->bind(channel);
  base_parent->bind(channel);
  auto* from_base = retain(new port_type(*base_parent));
  auto* named_from_base = retain(new port_type((prefix + "_from_base").c_str(), *base_parent));
  auto* this_parent = retain(new port_type((prefix + "_this_parent").c_str()));
  this_parent->bind(channel);
  auto* from_this = retain(new port_type(*this_parent));
  auto* named_from_this = retain(new port_type((prefix + "_named_from_this").c_str(), *this_parent));
  auto* base_bind_child = retain(new port_b((prefix + "_base_bind_child").c_str(), 1, sc_core::SC_ZERO_OR_MORE_BOUND));
  auto* base_call_child = retain(new port_b((prefix + "_base_call_child").c_str(), 1, sc_core::SC_ZERO_OR_MORE_BOUND));
  base_bind_child->bind(*base_parent);
  (*base_call_child)(*base_parent);

  require(std::string(default_port->kind()) == "sc_port" && std::string(named_port->kind()) == "sc_port",
          "specialized port kind");
  port_type* directly_bound[] = {by_interface, named_by_interface, this_parent};
  for (auto* bound : directly_bound)
  {
    require(bound->bind_count() == 1 && bound->size() == 1, "specialized direct binding");
    require(interface_matches<IF>(bound->get_interface(0), channel), "specialized direct interface");
    require(interface_matches<IF>((*bound)[0], channel), "specialized subscript");
    require(has_interface_type<IF>(bound->get_interface_type()), "specialized type info");
  }
  require(base_parent->bind_count() == 1 && base_parent->size() == 1, "specialized sc_port_b direct binding");
  require(interface_matches<IF>(base_parent->get_interface(0), channel), "specialized sc_port_b interface");
  for (auto* child : {from_base, named_from_base, from_this, named_from_this})
  {
    require(child->bind_count() == 1 && child->size() == 0, "specialized parent binding");
  }
  require(base_unnamed->bind_count() == 1 && base_unnamed->size() == 1, "sc_port_b unnamed constructor");
  require(interface_matches<IF>(base_unnamed->get_interface(0), channel), "sc_port_b unnamed binding");
  require(base_bind_child->bind_count() == 1 && base_bind_child->size() == 0, "sc_port_b parent bind");
  require(base_call_child->bind_count() == 1 && base_call_child->size() == 0, "sc_port_b parent call");
  require(interface_matches<IF>(base_parent->base_interface(), channel), "specialized mutable base interface");
  require(interface_matches<IF>(base_parent->const_base_interface(), channel), "specialized const base interface");
  require(interface_matches<IF>(base_parent->get_interface(0), channel), "specialized indexed interface");
  require(interface_matches<IF>((*base_parent)[0], channel), "specialized mutable subscript");
  require(interface_matches<IF>(base_parent->interface_arrow(), channel), "specialized mutable arrow");
  require(interface_matches<IF>(base_parent->const_interface_at(0), channel), "specialized const indexed interface");
  require(interface_matches<IF>(base_parent->const_subscript(0), channel), "specialized const subscript");
  require(interface_matches<IF>(base_parent->const_interface_arrow(), channel), "specialized const arrow");
}

}  // namespace

/** @brief Compare generic and inventoried port constructors and accessors. */
/** @brief Compare generic/specialized port binding, access and destruction. */
int sc_main(int, char**)
{
  sc_core::sc_module_name module_name("ports_public");
  PortHost host(module_name);
  std::vector<owned_port> ports;
  const auto retain = [&ports](auto* port)
  {
    using port_type = typename std::remove_pointer<decltype(port)>::type;
    ports.emplace_back(port, [](sc_core::sc_port_base* base) { delete static_cast<port_type*>(base); });
    return port;
  };
  auto* unnamed = retain(new custom_port());
  auto* named = retain(new custom_port("named_port"));
  unnamed->bind(host.left);
  (*named)(host.right);
  auto* by_interface = retain(new custom_port(host.left));
  auto* named_by_interface = retain(new custom_port("named_interface_port", host.right));
  auto* base_parent = retain(new custom_port_b("base_parent", 0, sc_core::SC_ZERO_OR_MORE_BOUND));
  base_parent->bind(host.left);
  auto* base_child = retain(new custom_port(*base_parent));
  auto* named_base_child = retain(new custom_port("named_base_child", *base_parent));
  auto* this_parent = retain(new custom_port("this_parent"));
  this_parent->bind(host.right);
  auto* this_child = retain(new custom_port(*this_parent));
  auto* named_this_child = retain(new custom_port("named_this_child", *this_parent));

  require(std::string(unnamed->kind()) == "sc_port" && std::string(named->kind()) == "sc_port", "generic kind");
  require(by_interface->bind_count() == 1 && named_by_interface->bind_count() == 1, "generic interface constructors");
  require(base_parent->bind_count() == 1 && base_child->bind_count() == 1 && named_base_child->bind_count() == 1,
          "generic base parent constructors");
  require(this_parent->bind_count() == 1 && this_child->bind_count() == 1 && named_this_child->bind_count() == 1,
          "generic this parent constructors");
  require(base_child->size() == 0 && this_child->size() == 0, "generic unresolved parent size");
  require(interface_matches<ValueIf>(by_interface->get_interface(0), host.left), "generic first interface");
  require(interface_matches<ValueIf>(named_by_interface->get_interface(0), host.right), "generic second interface");
  require(interface_matches<ValueIf>(base_parent->get_interface(0), host.left), "generic base parent interface");
  require(interface_matches<ValueIf>(this_parent->get_interface(0), host.right), "generic this parent interface");

  auto* operators = retain(new custom_port("operators"));
  operators->bind(host.left);
  (*operators)(host.right);
  require(operators->size() == 2 && operators->bind_count() == 2, "generic operator bindings");
  require(interface_matches<ValueIf>(operators->get_interface(0), host.left), "generic indexed interface zero");
  require(interface_matches<ValueIf>(operators->get_interface(1), host.right), "generic indexed interface one");
  require(interface_matches<ValueIf>((*operators)[0], host.left), "generic mutable subscript zero");
  require(interface_matches<ValueIf>((*operators)[1], host.right), "generic mutable subscript one");
  require(interface_matches<ValueIf>(const_interface_at(*operators, 0), host.left),
          "generic const indexed interface zero");
  require(interface_matches<ValueIf>(const_interface_at(*operators, 1), host.right),
          "generic const indexed interface one");
  require(interface_matches<ValueIf>(const_subscript(*operators, 0), host.left), "generic const subscript zero");
  require(interface_matches<ValueIf>(const_subscript(*operators, 1), host.right), "generic const subscript one");
  require(interface_matches<ValueIf>(operators->get_interface(), host.left), "generic zero-index interface");
  require(read_port_value<ValueIf>(*operators) == 13, "generic arrow value");
  require(read_const_port_value<ValueIf>(*operators) == 13, "generic const arrow value");
  require(interface_matches<ValueIf>(const_arrow(*operators), host.left), "generic const arrow interface");
  require(has_interface_type<ValueIf>(operators->get_interface_type()), "generic interface type info");

  check_specialization<sc_core::sc_signal_in_if<bool>>("in_bool", host.bool_signal, ports);
  check_specialization<sc_core::sc_signal_in_if<sc_dt::sc_logic>>("in_logic", host.logic_signal, ports);
  check_specialization<sc_core::sc_signal_inout_if<bool>>("inout_bool", host.bool_signal, ports);
  check_specialization<sc_core::sc_signal_inout_if<sc_dt::sc_logic>>("inout_logic", host.logic_signal, ports);
  check_specialization<sc_core::sc_event_queue_if>("event_queue", host.event_queue, ports);

  auto* base_default = retain(new custom_port_b(1, sc_core::SC_ZERO_OR_MORE_BOUND));
  auto* base_named = retain(new custom_port_b("base_named", 1, sc_core::SC_ZERO_OR_MORE_BOUND));
  base_default->bind(host.left);
  (*base_named)(host.right);
  auto* base_bind_child = retain(new custom_port_b("base_bind_child", 1, sc_core::SC_ZERO_OR_MORE_BOUND));
  auto* base_call_child = retain(new custom_port_b("base_call_child", 1, sc_core::SC_ZERO_OR_MORE_BOUND));
  base_bind_child->bind(*base_default);
  (*base_call_child)(*base_named);
  require(base_bind_child->bind_count() == 1 && base_bind_child->size() == 0, "sc_port_b parent bind");
  require(base_call_child->bind_count() == 1 && base_call_child->size() == 0, "sc_port_b parent call");
  require(base_default->size() == 1 && base_named->size() == 1, "sc_port_b binding sizes");
  require(interface_matches<ValueIf>(base_default->get_interface(0), host.left), "sc_port_b indexed interface");
  require(interface_matches<ValueIf>(base_named->get_interface(0), host.right), "sc_port_b named interface");
  require(interface_matches<ValueIf>(base_default->interface_arrow(), host.left), "sc_port_b arrow interface");
  require(interface_matches<ValueIf>(base_default->const_interface_at(0), host.left),
          "sc_port_b const indexed interface");
  require(interface_matches<ValueIf>(base_default->const_subscript(0), host.left), "sc_port_b const subscript");
  require(interface_matches<ValueIf>(base_default->const_interface_arrow(), host.left), "sc_port_b const arrow");
  require(interface_matches<ValueIf>(base_default->base_interface(), host.left), "sc_port_b mutable base interface");
  require(interface_matches<ValueIf>(base_default->const_base_interface(), host.left),
          "sc_port_b const base interface");
  require(has_interface_type<ValueIf>(base_default->get_interface_type()), "sc_port_b interface type");

  auto* vbind_probe = retain(new custom_port_b("vbind_probe", 0, sc_core::SC_ZERO_OR_MORE_BOUND));
  require(vbind_probe->probe_vbind_interface(host.left) == 0, "protected interface vbind status");
  require(vbind_probe->bind_count() == 1 && vbind_probe->size() == 1, "protected interface vbind result");
  require(vbind_probe->probe_vbind_interface(host.bool_signal) == 2, "protected mismatched interface vbind status");
  require(vbind_probe->probe_vbind_port(*base_named) == 0, "protected base-port vbind status");
  require(vbind_probe->probe_vbind_port(*unnamed) == 0, "protected generic-port vbind status");
  require(vbind_probe->bind_count() == 3, "protected parent vbind count");
  auto* sensitivity_probe = retain(new custom_port_b("sensitivity_probe", 1, sc_core::SC_ZERO_OR_MORE_BOUND));
  sensitivity_probe->probe_method_sensitivity();
  sensitivity_probe->probe_thread_sensitivity();

  const std::size_t teardown_count = ports.size();
  for (auto it = ports.rbegin(); it != ports.rend(); ++it)
  {
    const std::string name(it->first->name());
    it->second(it->first);
    it->first = nullptr;
    require(sc_core::sc_find_object(name.c_str()) == nullptr, "port object teardown lookup");
  }
  require(host.left.value() == 13 && host.right.value() == 29, "host channel values");
  std::cout
    << "SC_PORT_FAMILY_REFERENCE generic=13,29 constructors=8x6 specializations=5 base_aliases=3 operators=2 teardown="
    << teardown_count << '\n';
  return 0;
}

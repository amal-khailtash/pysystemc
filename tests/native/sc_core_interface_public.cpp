/** @file sc_core_interface_public.cpp
 * @brief Compare public sc_interface virtual dispatch and access traits with Python.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <type_traits>

namespace
{
int destroyed = 0;

/** @brief Use the protected native base constructor without overriding its no-op method. */
class BareInterface : public virtual sc_core::sc_interface
{
 public:
  BareInterface() = default;
};

/** @brief Implement sc_interface with stable event storage for the native reference. */
class NativeInterface : public virtual sc_core::sc_interface
{
 public:
  NativeInterface() : event_("native_ready")
  {
  }

  ~NativeInterface() override
  {
    ++destroyed;
  }

  void register_port(sc_core::sc_port_base& port, const char* if_typename) override
  {
    ++register_count;
    port_name = port.name();
    interface_type = if_typename;
  }

  const sc_core::sc_event& default_event() const override
  {
    return event_;
  }

  int register_count = 0;
  std::string port_name;
  std::string interface_type;

 private:
  sc_core::sc_event event_;
};

/** @brief Provide a parent module whose port and interface die in safe order. */
class Owner : public sc_core::sc_module
{
 public:
  explicit Owner(sc_core::sc_module_name name) : sc_core::sc_module(name)
  {
  }
};
}  // namespace

/** @brief Exercise the public interface methods and C++ access/lifetime contract. */
int sc_main(int, char**)
{
  using namespace sc_core;
  bool dispatch_ok = false;
  bool base_default_ok = false;
  {
    sc_module_name name("interface_top");
    Owner owner(name);
    BareInterface base;
    sc_port<sc_interface> base_port("base_port");
    base_port.bind(base);
    NativeInterface interface;
    sc_port<sc_interface> port("interface_port");
    port.bind(interface);
    sc_start(SC_ZERO_TIME);

    const sc_event& event = port.get_interface()->default_event();
    const sc_event& fallback = base_port.get_interface()->default_event();
    dispatch_ok = interface.register_count == 1 && interface.port_name == "interface_top.interface_port" &&
                  !interface.interface_type.empty() && std::string(event.basename()) == "native_ready";
    base_default_ok = &fallback == &sc_event::none;
    sc_stop();
  }

  const bool traits_ok =
    std::is_default_constructible<BareInterface>::value && std::has_virtual_destructor<sc_interface>::value &&
    !std::is_default_constructible<sc_interface>::value && !std::is_copy_constructible<sc_interface>::value &&
    !std::is_copy_assignable<sc_interface>::value;
  const bool lifetime_ok = destroyed == 1;
  std::cout << "INTERFACE_REFERENCE " << dispatch_ok << " " << base_default_ok << " " << traits_ok << " " << lifetime_ok
            << " native_ready\n";
  return dispatch_ok && base_default_ok && traits_ok && lifetime_ok ? 0 : 1;
}

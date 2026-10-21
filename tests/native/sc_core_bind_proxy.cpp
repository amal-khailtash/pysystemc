/** @file sc_core_bind_proxy.cpp
 * @brief Native public bind-proxy constructor and nil-storage reference.
 * @details Proxies borrow channel/port addresses until module destruction.
 */
#include <systemc>
/** @brief Keep native port and interface storage alive during proxy access. */
struct model : sc_core::sc_module
{
  sc_core::sc_signal<int> signal{"signal"};
  sc_core::sc_port<sc_core::sc_signal_in_if<int>> port{"port"};
  model(sc_core::sc_module_name name) : sc_module(name)
  {
    port.bind(signal);
  }
};
/** @brief Check nil/default/interface/port native proxy states. */
int sc_main(int, char**)
{
  model top("top");
  sc_core::sc_bind_proxy empty, input(top.signal), output(top.port);
  const auto& nil = sc_core::SC_BIND_PROXY_NIL;
  if (nil.iface || nil.port || empty.iface || empty.port || input.iface != &top.signal || input.port || output.iface ||
      output.port != &top.port)
    return 2;
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  sc_core::sc_stop();
  std::cout << "BIND_PROXY_REFERENCE 1 1 1\n";
  return 0;
}

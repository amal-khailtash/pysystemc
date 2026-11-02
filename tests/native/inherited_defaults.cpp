/** @file inherited_defaults.cpp
 * @brief Native oracle for static defaults, virtual dispatch and template defaults.
 */
#include <iostream>

#include "inherited_defaults_probe.h"

/** @brief Retain default and explicit channels and bound ports through simulation. */
struct model : sc_core::sc_module
{
  sc_core::sc_signal<int> default_signal{"default_signal"};
  sc_core::sc_signal<int, sc_core::SC_ONE_WRITER> explicit_signal{"explicit_signal", 0};
  sc_core::sc_port<sc_core::sc_signal_in_if<int>> default_port{"default_port"};
  sc_core::sc_port<sc_core::sc_signal_in_if<int>, 1, sc_core::SC_ONE_OR_MORE_BOUND> explicit_port{"explicit_port"};
  /** @brief Borrow retained signals for the complete native port lifetime. */
  model(sc_core::sc_module_name name) : sc_module(name)
  {
    default_port.bind(default_signal);
    explicit_port.bind(explicit_signal);
  }
};

/** @brief Compare declaration defaults and elaborated public channel values. */
int sc_main(int, char**)
{
  inherited_defaults_probe::Derived derived;
  const inherited_defaults_probe::Base& base = derived;
  if (derived.value() != 229 || derived.Base::value() != 111 || base.value() != 211 || derived.value(11) != 211 ||
      inherited_defaults_probe::template_default_mask() != 3)
    return 2;
  model top("top");
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  if (top.default_port.size() != 1 || top.explicit_port.size() != 1 || top.default_port->read() != 0 ||
      top.explicit_port->read() != 0 || top.default_signal.read() != top.explicit_signal.read() ||
      sc_core::sc_time_stamp() != sc_core::SC_ZERO_TIME)
    return 3;
  sc_core::sc_stop();
  std::cout << "INHERITED_DEFAULTS_REFERENCE 229 111 211 211 3 1 1 0 0\n";
  return 0;
}

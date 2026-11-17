/** @file inherited_defaults_probe.h
 * @brief Native inheritance and public SystemC template-default reference types.
 * @details Instances own their storage and retain no Python callbacks or
 * references. Calls require ordinary SystemC simulation-thread confinement;
 * no extra GIL acquisition is required and value methods do not throw.
 */
#pragma once
#include <systemc>
#include <type_traits>

namespace inherited_defaults_probe
{
/** @brief Interface with a default selected from the static base declaration. */
struct Base : sc_core::sc_interface
{
  /** @brief Return the base implementation plus the caller's argument. */
  virtual int value(int argument = 11) const
  {
    return 100 + argument;
  }
};

/** @brief Native override providing a different declaration-local default. */
struct Derived : Base
{
  /** @brief Return the derived implementation plus the caller's argument. */
  int value(int argument = 29) const override
  {
    return 200 + argument;
  }
};

/** @brief Return independent masks for default signal and port type identity. */
inline int template_default_mask()
{
  return std::is_same_v<sc_core::sc_signal<int>, sc_core::sc_signal<int, sc_core::SC_ONE_WRITER>> |
         (std::is_same_v<sc_core::sc_port<sc_core::sc_signal_in_if<int>>, sc_core::sc_port<sc_core::sc_signal_in_if<int>, 1, sc_core::SC_ONE_OR_MORE_BOUND>> << 1);
}
}  // namespace inherited_defaults_probe

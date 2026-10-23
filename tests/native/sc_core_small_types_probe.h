/** @file sc_core_small_types_probe.h
 * @brief Type-specific native consumers for small public enum and alias contracts.
 */
#ifndef PYSC_SMALL_TYPES_PROBE_H
#define PYSC_SMALL_TYPES_PROBE_H
#include <systemc>
#include <type_traits>

namespace small_types_probe
{
/** @brief Preserve the exact selected native enum type at the consumer boundary. */
template <typename T>
int enum_value(T value)
{
  return static_cast<int>(value);
}
/** @brief Consume native report action bits without changing handler state. */
inline unsigned actions(sc_core::sc_actions value)
{
  return value;
}
/** @brief Borrow a native module through either historic typedef with GIL held. */
inline const char* behavior_name(const sc_core::sc_behavior& value)
{
  return value.name();
}
/** @brief Borrow a native channel module without taking ownership. */
inline const char* channel_name(const sc_core::sc_channel& value)
{
  return value.name();
}
static_assert(std::is_same<sc_core::sc_behavior, sc_core::sc_module>::value, "behavior alias");
static_assert(std::is_same<sc_core::sc_channel, sc_core::sc_module>::value, "channel alias");
static_assert(std::is_same<sc_core::sc_in_clk, sc_core::sc_in<bool>>::value, "clock input alias");
static_assert(std::is_same<sc_core::sc_inout_clk, sc_core::sc_inout<bool>>::value, "clock inout alias");
static_assert(std::is_same<sc_core::sc_out_clk, sc_core::sc_out<bool>>::value, "clock output alias");
static_assert(std::is_same<sc_core::sc_event_queue_port, sc_core::sc_port<sc_core::sc_event_queue_if>>::value,
              "event queue port alias");
}  // namespace small_types_probe
#endif

/** @file signal_values.cpp
 * @brief Exact native signal value conversions with borrowed reference results.
 * @details Calls hold the GIL and do not enter the scheduler. Virtual read
 * dispatch preserves the signal implementation's callback/lifetime contract.
 * Signals own the returned storage; keep each signal alive and treat the
 * reference as read-only. Native conversion and allocation errors propagate.
 */
#include <systemc>

namespace pysystemc_signals
{
/** @brief Invoke an unrestricted signal base's native const-reference conversion.
 * @tparam Value Native signal datatype, including arbitrary template widths.
 * @tparam Policy Native writer policy, selected from the actual signal type.
 */
template <class Value, sc_core::sc_writer_policy Policy>
struct value_dispatch
{
  /** @brief Borrow current storage through the native conversion operator.
   * @param signal Live native signal owning the converted value.
   * @return Read-only native value reference, invalidated by signal destruction
   * and reflecting later updates. cppyy copies fundamental scalars into Python.
   */
  static const Value& get(const sc_core::sc_signal_t<Value, Policy>& signal)
  {
    return static_cast<const Value&>(signal);
  }
};

/** @brief Exact input and bidirectional port conversions for any native datatype.
 * @tparam Value Native interface datatype, with unrestricted template widths.
 * @details Calls hold the GIL without entering the scheduler. The bound
 * interface owns results; retain ports and interfaces while borrowing values.
 * Native reports and conversion exceptions propagate to the caller.
 */
template <class Value>
struct port_value_dispatch
{
  /** @brief Borrow the bound input interface's current value.
   * @param port Live, bound input port.
   * @return Read-only interface storage; cppyy copies fundamental scalars.
   */
  static const Value& input(const sc_core::sc_in<Value>& port)
  {
    return static_cast<const Value&>(port);
  }

  /** @brief Borrow the bound bidirectional or output interface's current value.
   * @param port Live, bound bidirectional port, including derived output ports.
   * @return Read-only interface storage, reflecting subsequent native updates.
   */
  static const Value& inout(const sc_core::sc_inout<Value>& port)
  {
    return static_cast<const Value&>(port);
  }
};
}  // namespace pysystemc_signals

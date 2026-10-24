/** @file sc_core_sensitivity_probe.h
 * @brief Derived accessors for the protected module-owned sensitivity builders.
 * @details Builder references borrow their owning module and retain no Python
 * objects. Keep the module, process, port, interface, finder and event alive
 * through elaboration and simulation. Calls are synchronous; cppyy's normal
 * callback entry acquires the GIL when a registered Python method runs.
 */
#ifndef PYSYSTEMC_SC_CORE_SENSITIVITY_PROBE_H
#define PYSYSTEMC_SC_CORE_SENSITIVITY_PROBE_H

#include <sysc/kernel/sc_cthread_process.h>
#include <sysc/kernel/sc_method_process.h>
#include <sysc/kernel/sc_thread_process.h>

#include <systemc>
#include <type_traits>

namespace sc_core
{

/** @brief Expose protected static-sensitivity builders on a Python module.
 * @details This test adapter adds no ownership. The inherited ScModule retains
 * registered Python callbacks for their process lifetime; returned references
 * expire with this module. Native SystemC report and exception behavior applies.
 */
class SensitivityProbe : public ScModule
{
 public:
  /** @brief Construct a callback-owning module in its active naming scope.
   * @param name Native module name consumed by the SystemC hierarchy.
   */
  explicit SensitivityProbe(const sc_module_name& name) : ScModule(name)
  {
  }

  /** @brief Borrow the ordinary static sensitivity builder.
   * @return Reference valid until this module is destroyed.
   */
  sc_sensitive& ordinary_builder()
  {
    return sc_module::sensitive;
  }

  /** @brief Borrow the positive-edge static sensitivity builder.
   * @return Reference valid until this module is destroyed.
   */
  sc_sensitive_pos& positive_builder()
  {
    return sc_module::sensitive_pos;
  }

  /** @brief Borrow the negative-edge static sensitivity builder.
   * @return Reference valid until this module is destroyed.
   */
  sc_sensitive_neg& negative_builder()
  {
    return sc_module::sensitive_neg;
  }

  /** @brief Convert a registered method handle to the legacy typed handle.
   * @param handle Valid native method process handle from this kernel.
   * @return Non-owning method pointer for sensitivity overload selection, or nullptr for another process kind.
   */
  static sc_method_handle method_handle(const sc_process_handle& handle)
  {
    return dynamic_cast<sc_method_handle>(handle.get_process_object());
  }

  /** @brief Convert a registered thread handle to its legacy typed handle.
   * @param handle Valid native thread process handle from this kernel.
   * @return Non-owning thread pointer for sensitivity overload selection, or nullptr for another process kind.
   */
  static sc_thread_handle thread_handle(const sc_process_handle& handle)
  {
    return dynamic_cast<sc_thread_handle>(handle.get_process_object());
  }

  /** @brief Convert a registered clocked-thread handle to its typed handle.
   * @param handle Valid native clocked-thread process handle from this kernel.
   * @return Non-owning clocked-thread pointer for the two-argument helpers.
   */
  static sc_cthread_handle cthread_handle(const sc_process_handle& handle)
  {
    return dynamic_cast<sc_cthread_handle>(handle.get_process_object());
  }
};

#define PYSYSTEMC_CHECK_SENSITIVITY_ALIASES(builder)                                      \
  static_assert(std::is_same_v<builder::in_if_b_type, sc_signal_in_if<bool>>);            \
  static_assert(std::is_same_v<builder::in_if_l_type, sc_signal_in_if<sc_dt::sc_logic>>); \
  static_assert(std::is_same_v<builder::in_port_b_type, sc_in<bool>>);                    \
  static_assert(std::is_same_v<builder::in_port_l_type, sc_in<sc_dt::sc_logic>>);         \
  static_assert(std::is_same_v<builder::inout_port_b_type, sc_inout<bool>>);              \
  static_assert(std::is_same_v<builder::inout_port_l_type, sc_inout<sc_dt::sc_logic>>)

PYSYSTEMC_CHECK_SENSITIVITY_ALIASES(sc_sensitive);
PYSYSTEMC_CHECK_SENSITIVITY_ALIASES(sc_sensitive_pos);
PYSYSTEMC_CHECK_SENSITIVITY_ALIASES(sc_sensitive_neg);

#undef PYSYSTEMC_CHECK_SENSITIVITY_ALIASES

}  // namespace sc_core

#endif

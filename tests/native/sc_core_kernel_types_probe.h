/** @file sc_core_kernel_types_probe.h
 * @brief Native declaration probes for borrowed reset and report-handler types.
 */
#ifndef PYSYSTEMC_KERNEL_TYPES_PROBE_H
#define PYSYSTEMC_KERNEL_TYPES_PROBE_H
#include <sysc/kernel/sc_reset.h>

#include <systemc>
#include <type_traits>

namespace small_types_probe
{
static_assert(
  std::is_same_v<sc_core::sc_report_handler_proc, void (*)(const sc_core::sc_report&, const sc_core::sc_actions&)>);

/** @brief Test-owned reset view borrowing a boolean interface.
 * @details Only the publicly forwarded type is under test; this probe does not
 * register kernel reset targets or expose internal reset operations. The signal
 * must outlive this object and any borrowed base pointer.
 */
struct ResetView : sc_core::sc_reset
{
  /** @brief Construct the test-owned derived reset object. */
  explicit ResetView(const sc_core::sc_signal_in_if<bool>& source) : sc_reset(&source)
  {
  }
  /** @brief Return the borrowed opaque base pointer, with no ownership transfer. */
  const sc_core::sc_reset* borrowed() const
  {
    return this;
  }
};

/** @brief Compare borrowed handler tokens using their exact native pointer values. */
template <class Token>
bool same_handler_token(const Token& left, const Token& right)
{
  static_assert(std::is_same_v<decltype(left.value), sc_core::sc_report_handler_proc>);
  return left.value && left.value == right.value;
}

/** @brief Check that native handler transport retains the exact function-pointer type. */
inline bool handler_roundtrip()
{
  sc_core::sc_report_handler_proc handler = sc_core::sc_report_handler::get_handler();
  return handler && sc_core::sc_report_handler::set_handler(handler) == handler;
}
}  // namespace small_types_probe
#endif

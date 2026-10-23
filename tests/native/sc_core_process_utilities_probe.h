/** @file sc_core_process_utilities_probe.h
 * @brief Native reference access for thread stack configuration and handle order.
 */
#ifndef PYSC_PROCESS_UTILITIES_PROBE_H
#define PYSC_PROCESS_UTILITIES_PROBE_H
#include <systemc>

#include "sysc/kernel/sc_thread_process.h"
namespace process_utilities_probe
{
/** @brief Form a legal protected base-member pointer without casting a process to a fake derived instance. */
struct stack_probe : sc_core::sc_thread_process
{
  /** @brief Read the stack configuration of a borrowed real native thread. */
  static std::size_t size(sc_core::sc_process_handle handle)
  {
    auto* thread = static_cast<sc_core::sc_thread_handle>(handle);
    return thread->*(&stack_probe::m_stack_size);
  }
};
/** @brief Borrow the thread pointer for testing the original public setter signature. */
inline sc_core::sc_thread_handle thread(sc_core::sc_process_handle handle)
{
  return static_cast<sc_core::sc_thread_handle>(handle);
}
/** @brief Compare borrowed process handles using the exact native free operator. */
inline bool less(const sc_core::sc_process_handle& a, const sc_core::sc_process_handle& b)
{
  return sc_core::operator<(a, b);
}
}  // namespace process_utilities_probe
#endif

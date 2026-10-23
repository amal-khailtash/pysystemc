/** @file process_utilities.cpp
 * @brief Adapt thread utility signatures while preserving native process lifetime.
 */
#include <stdexcept>
#include <systemc>

namespace pysystemc_public
{
/** @brief Set a live native thread's stack size without a cppyy handle conversion.
 * @param handle Borrowed thread or clocked-thread handle, retained during this call.
 * @param size Native byte count; set before preparing/running the thread.
 * @details GIL remains held; no callback ownership or scheduler entry occurs.
 * @throws std::invalid_argument If the handle is empty or names a method process.
 */
void set_stack_size_handle(sc_core::sc_process_handle handle, std::size_t size)
{
  auto* thread = static_cast<sc_core::sc_thread_handle>(handle);
  if (!thread) throw std::invalid_argument("stack size requires a live thread handle");
  sc_core::sc_set_stack_size(thread, size);
}
/** @brief Set stack size through the original borrowed native thread pointer form.
 * @details The caller retains the thread and holds GIL; null pointers are rejected.
 * @throws std::invalid_argument If thread is null.
 */
void set_stack_size_pointer(sc_core::sc_thread_handle thread, std::size_t size)
{
  if (!thread) throw std::invalid_argument("stack size requires a live thread pointer");
  sc_core::sc_set_stack_size(thread, size);
}
}  // namespace pysystemc_public

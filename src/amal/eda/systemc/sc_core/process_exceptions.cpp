/** @file process_exceptions.cpp
 * @brief Typed native user-exception injection with safe Python scheduler entry.
 * @details Exception arguments are borrowed during injection; the kernel copies
 * them into its transient throw helper. Processes remain kernel-owned. Callers
 * release the GIL while another process handles the exception; its Python
 * callback reacquires it. Native errors propagate and reset/kill unwind remains
 * guarded. A user exception must be caught by the target before it terminates.
 */
#include <systemc>

namespace sc_core
{
/** @brief Expose a non-template callable for each arbitrary native exception type.
 * @tparam Exception Native copyable exception type understood by the target.
 */
template <class Exception>
struct python_throw_helper
{
  /** @brief Inject a copied exception into a process and optionally descendants.
   * @param handle Borrowed process handle retained through the call.
   * @param exception Borrowed native exception copied by SystemC.
   * @param descendants Native descendant policy.
   * @return False requests restoration of reset/kill unwind in Python cleanup.
   * @details Call with the GIL released; target callbacks acquire it themselves.
   */
  static bool invoke(sc_process_handle& handle, const Exception& exception,
                     sc_descendant_inclusion_info descendants = SC_NO_DESCENDANTS)
  {
    return python_guarded_operation([&] { handle.throw_it(exception, descendants); });
  }
};
}  // namespace sc_core

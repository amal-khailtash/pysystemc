/** @file interface_teardown.cpp
 * @brief Conservative native borrower gate for structural adapter teardown.
 * @details Call from Python with the GIL held after SC_STOPPED and final trace
 * reads. Objects are borrowed only during traversal. No callbacks, waits or
 * ownership transfers occur; a live port/export or suspended thread prevents adapter deletion.
 */
#include <systemc>

namespace pysystemc_interfaces
{
/** @brief Return whether this object hierarchy contains a live native borrower.
 * @details Checks every port/export, including native-created and multi-bound
 * ports. Conservative rejection avoids inspecting private binding storage.
 */
bool has_interface_borrowers(const sc_core::sc_object* object)
{
  if (dynamic_cast<const sc_core::sc_port_base*>(object) || dynamic_cast<const sc_core::sc_export_base*>(object))
    return true;
  sc_core::sc_process_handle process(const_cast<sc_core::sc_object*>(object));
  if (process.valid() && process.proc_kind() != sc_core::SC_METHOD_PROC_ && !process.terminated()) return true;
  for (const auto* child : object->get_child_objects())
    if (has_interface_borrowers(child)) return true;
  return false;
}

/** @brief Require destruction of all native ports/exports before adapter deletion.
 * @details Throws std::runtime_error before mutation when the kernel is not
 * stopped or any borrower remains. Caller must finish trace/reference access;
 * no native kernel reset or restart is performed.
 */
void require_interface_teardown()
{
  if (sc_core::sc_get_status() != sc_core::SC_STOPPED)
    throw std::runtime_error("interface teardown requires simulation stop");
  for (const auto* object : sc_core::sc_get_top_level_objects())
    if (has_interface_borrowers(object))
      throw std::runtime_error(
        "destroy all native ports/exports and terminate thread processes before interface teardown");
}
}  // namespace pysystemc_interfaces

/** @file report_legacy.cpp
 * @brief Public legacy report methods omitted by cppyy exception reflection.
 * @details Call on the simulation thread with the GIL held. No report ownership
 * transfers; native configuration persists in the singleton kernel. Native report
 * exceptions propagate; registered message types belong to native report storage.
 * A configured Python report handler retains its separately managed callback.
 */
#include <sysc/utils/sc_report.h>
namespace pysystemc
{
/** @brief Register a null diagnostic through the native legacy error path.
 * @details No message storage is borrowed. Call with the GIL held on the
 * simulation thread; the configured report handler executes synchronously.
 * Native registration reports and Python callback exceptions propagate once.
 */
void report_register_null_id(int id)
{
  sc_core::sc_report::register_id(id, nullptr);
}

/** @brief Register a native legacy report ID; propagate duplicate/invalid errors. */
void report_register_id(int id, const char* message)
{
  sc_core::sc_report::register_id(id, message);
}
/** @brief Read a library-owned legacy message, or native unknown-ID text. */
const char* report_get_message(int id)
{
  return sc_core::sc_report::get_message(id);
}
/** @brief Query native ID suppression state. */
bool report_is_suppressed(int id)
{
  return sc_core::sc_report::is_suppressed(id);
}
/** @brief Set native per-ID suppression, leaving absent IDs unchanged. */
void report_suppress_id(int id, bool suppress)
{
  sc_core::sc_report::suppress_id(id, suppress);
}
/** @brief Set native info-severity suppression or restore its default actions. */
void report_suppress_infos(bool suppress)
{
  sc_core::sc_report::suppress_infos(suppress);
}
/** @brief Set native warning-severity suppression or restore its default actions. */
void report_suppress_warnings(bool suppress)
{
  sc_core::sc_report::suppress_warnings(suppress);
}
/** @brief Set native legacy warning-to-error promotion. */
void report_make_warnings_errors(bool promote)
{
  sc_core::sc_report::make_warnings_errors(promote);
}
/** @brief Assign borrowed reports; destination owns copied fields and returned reference is borrowed. */
sc_core::sc_report& report_assign(sc_core::sc_report& destination, const sc_core::sc_report& source)
{
  return destination = source;
}
}  // namespace pysystemc

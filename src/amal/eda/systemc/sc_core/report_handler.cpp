/** @file report_handler.cpp
 * @brief Resolve report stop-limit overloads without enum/integer ambiguity.
 * @details Call on simulation thread with GIL held. Native policy owns limits;
 * no ownership transfers or retained callbacks. Native exceptions propagate.
 */
#include <sysc/utils/sc_report_handler.h>
namespace pysystemc
{
/** @brief Set/reset a severity-wide native stop limit. */
int report_stop_severity(sc_core::sc_severity severity, int limit)
{
  return sc_core::sc_report_handler::stop_after(severity, limit);
}
/** @brief Set/reset a message-wide native stop limit. */
int report_stop_message(const char* message, int limit)
{
  return sc_core::sc_report_handler::stop_after(message, limit);
}
/** @brief Set/reset a message/severity native stop limit. */
int report_stop_message_severity(const char* message, sc_core::sc_severity severity, int limit)
{
  return sc_core::sc_report_handler::stop_after(message, severity, limit);
}
}  // namespace pysystemc

namespace pysystemc
{
/** @brief Store a native report-handler pointer without Python callable conversion.
 * @details Pointer is borrowed and does not own or retain a Python callback.
 * Native handlers must remain loaded; a Python dispatcher additionally requires
 * its callback retention managed by set_report_handler. Use on simulation thread
 * with GIL held; construction/destruction never call the handler.
 */
struct report_handler_pointer
{
  sc_core::sc_report_handler_proc value;
};
/** @brief Return a borrowed native handler pointer token. */
report_handler_pointer report_get_handler()
{
  return {sc_core::sc_report_handler::get_handler()};
}
/** @brief Install a borrowed pointer token and return the previous token.
 * @details Does not release existing Python callback retention; native exceptions propagate.
 */
report_handler_pointer report_set_handler(const report_handler_pointer& pointer)
{
  return {sc_core::sc_report_handler::set_handler(pointer.value)};
}
/** @brief Restore default native handler for null selection, returning previous token. */
report_handler_pointer report_reset_handler()
{
  return {sc_core::sc_report_handler::set_handler(nullptr)};
}
}  // namespace pysystemc

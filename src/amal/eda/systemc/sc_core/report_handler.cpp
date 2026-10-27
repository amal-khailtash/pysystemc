/** @file report_handler.cpp
 * @brief Resolve report stop-limit overloads without enum/integer ambiguity.
 * @details Call on simulation thread with GIL held. Native policy owns limits;
 * no ownership transfers or retained callbacks. Native exceptions propagate.
 */
#include <sysc/utils/sc_report_handler.h>
namespace pysystemc
{
/** @brief Emit one message report without cppyy retrying after a thrown report.
 * @details String arguments are borrowed through this GIL-held call. Native
 * report policy owns copied diagnostics; Python handlers retain their own
 * callbacks and must not block. Report and callback exceptions propagate once.
 */
void report_message(sc_core::sc_severity severity, const char* message_type, const char* message, const char* file,
                    int line)
{
  sc_core::sc_report_handler::report(severity, message_type, message, file, line);
}

/** @brief Emit one message report with the native explicit verbosity signature.
 * @details Strings are borrowed, GIL remains held, and errors propagate without
 * retry. Report policy and callback ownership match report_message.
 */
void report_message_verbosity(sc_core::sc_severity severity, const char* message_type, const char* message,
                              int verbosity, const char* file, int line)
{
  sc_core::sc_report_handler::report(severity, message_type, message, verbosity, file, line);
}

/** @brief Emit one legacy ID report, preserving native promotion and metadata.
 * @details Strings are borrowed and GIL stays held. Native registered IDs own
 * their messages; policy and callback exceptions propagate without retry.
 */
void report_identifier(sc_core::sc_severity severity, int identifier, const char* message, const char* file, int line)
{
  sc_core::sc_report_handler::report(severity, identifier, message, file, line);
}

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

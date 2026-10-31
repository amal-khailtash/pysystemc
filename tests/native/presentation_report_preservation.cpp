/** @file presentation_report_preservation.cpp
 * @brief Native reference for report text, fields, actions and counts.
 * @details The handler copies report fields while the report is borrowed. It
 * retains no native report pointer or callback state beyond this process.
 */
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <vector>

namespace
{
/** @brief Own copied report fields and the action selected for the callback.
 * @details Each string owns a copy. No borrowed sc_report or native pointer
 * survives report-handler dispatch; the record is scoped to this test process.
 */
struct ReportRecord
{
  std::string message_type;
  std::string message;
  std::string file;
  int severity;
  int id;
  int line;
  unsigned actions;
};

std::vector<ReportRecord> reports;

/** @brief Copy scalar fields before SystemC releases the borrowed report.
 * @param report Report borrowed only for the synchronous handler invocation.
 * @param actions Native action mask selected for this message.
 * @details The handler runs on the caller's SystemC thread, uses no Python
 * callback or GIL, and retains only the copied values in @ref reports.
 * @throws std::bad_alloc if copying a report field cannot allocate storage.
 */
void capture(const sc_core::sc_report& report, const sc_core::sc_actions& actions)
{
  reports.push_back({report.get_msg_type(), report.get_msg(), report.get_file_name(),
                     static_cast<int>(report.get_severity()), report.get_id(), report.get_line_number(),
                     static_cast<unsigned>(actions)});
}

/** @brief Encode bytes as lowercase hex so Python and C++ output is comparable.
 * @param value Owned string bytes copied from a native report field.
 * @return Lowercase hexadecimal text representing the input bytes.
 * @details No native pointer is retained; this helper has no Python/GIL use.
 */
std::string hex(const std::string& value)
{
  std::ostringstream output;
  output << std::hex << std::setfill('0');
  for (unsigned char byte : value) output << std::setw(2) << static_cast<unsigned>(byte);
  return output.str();
}

/** @brief Emit the copied native record and its post-dispatch message count.
 * @param label Stable observation name shared with the Python model.
 * @param report Owned copy of the callback's scalar report fields.
 * @param count Native post-dispatch count for the report type.
 * @details Writes text synchronously and retains no borrowed report data.
 */
void emit(const char* label, const ReportRecord& report, int count)
{
  std::cout << "PRES_REPORT\t" << label << '\t' << report.severity << '\t' << report.id << '\t'
            << hex(report.message_type) << '\t' << hex(report.message) << '\t' << hex(report.file) << '\t'
            << report.line << '\t' << report.actions << '\t' << count << '\n';
}

/** @brief Emit native sc_report fields and the effective configured action.
 * @param report sc_report borrowed while the SC_THROW catch block is active.
 * @param count Native post-dispatch count for the report type.
 * @param action Action mask configured before the report call.
 * @details Copies all fields into the output stream before the exception borrow
 * ends; no pointer or report object survives this call.
 */
void emit_throw(const sc_core::sc_report& report, int count, unsigned action)
{
  std::cout << "PRES_THROW\t" << static_cast<int>(report.get_severity()) << '\t' << report.get_id() << '\t'
            << hex(report.get_msg_type()) << '\t' << hex(report.get_msg()) << '\t' << hex(report.get_file_name())
            << '\t' << report.get_line_number() << '\t' << action << '\t' << count << '\n';
}
}  // namespace

/** @brief Run native report-handler and SC_THROW reference calls.
 * @param argc Unused process argument count.
 * @param argv Unused process argument vector.
 * @return Zero after all native report assertions complete.
 * @details Calls execute synchronously on the kernel's caller thread. Handler
 * functions and reports remain owned by SystemC or this process; no Python
 * callback, GIL access, or simulation reset is involved.
 */
int sc_main(int argc, char** argv)
{
  (void)argc;
  (void)argv;
  using namespace sc_core;
  using handler = sc_report_handler;
  constexpr const char* message =
    "native diagnostic | sc_core::python_member_wait | pysystemc_vectors::member_0 | "
    "pysystemc_datatypes::fixed_number_bits";
  constexpr const char* file = "presentation-report-preservation.cpp";
  constexpr int line = 317;
  const char* labels[] = {"before", "after_help", "after_vector_error", "after_fixed_error"};
  const char* types[] = {"presentation/native_before", "presentation/native_after_help",
                         "presentation/native_after_vector_error", "presentation/native_after_fixed_error"};
  auto previous_handler = handler::set_handler(capture);
  for (int index = 0; index < 4; ++index)
  {
    const auto previous_action = handler::set_actions(types[index], SC_WARNING, SC_DO_NOTHING);
    handler::report(SC_WARNING, types[index], message, file, line);
    emit(labels[index], reports.back(), handler::get_count(types[index]));
    handler::set_actions(types[index], SC_WARNING, previous_action);
  }
  handler::set_handler(previous_handler);

  constexpr const char* throw_type = "presentation/native_throw";
  constexpr const char* throw_message = "native thrown diagnostic | sc_dt::python_fixed_constructor";
  const auto previous_action = handler::set_actions(throw_type, SC_ERROR, SC_THROW);
  try
  {
    handler::report(SC_ERROR, throw_type, throw_message, file, line + 1);
    return 2;
  }
  catch (const sc_report& report)
  {
    emit_throw(report, handler::get_count(throw_type), SC_THROW);
  }
  handler::set_actions(throw_type, SC_ERROR, previous_action);
  return 0;
}

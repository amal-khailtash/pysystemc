/** @file sc_core_report_action_defaults.cpp
 * @brief Native reference for defaulted report-action setter overloads.
 * @details Capture only copied scalar values while the callback report is borrowed.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <tuple>
#include <vector>

using namespace sc_core;
std::vector<std::tuple<std::string, int, unsigned>> records;

/** @brief Copy the report key, severity and resolved action mask. */
void capture(const sc_report& report, const sc_actions& actions)
{
  if (std::string(report.get_msg_type()).find("action/default/") == 0)
    records.emplace_back(report.get_msg_type(), int(report.get_severity()), actions);
}

/** @brief Print one setter's old value, post-reset value and reported fallback. */
void print_case(const char* label, unsigned seed_previous, unsigned default_previous, unsigned current)
{
  const auto& record = records.back();
  std::cout << "REPORT_ACTION_DEFAULT " << label << " 1 " << seed_previous << " " << default_previous << " " << current
            << " " << std::get<2>(record) << '\n';
}

/** @brief Compare severity, message-wide and message/severity default setters. */
int sc_main(int, char**)
{
  using h = sc_report_handler;
  const auto previous_handler = h::set_handler(capture);

  const auto severity_original = h::set_actions(SC_INFO, SC_LOG);
  const auto severity_default_previous = h::set_actions(SC_INFO);
  const auto severity_current = h::set_actions(SC_INFO, SC_UNSPECIFIED);
  h::set_actions(SC_INFO, severity_current);
  h::report(SC_INFO, "action/default/severity", "severity", "defaults.cpp", 11);
  print_case("severity", severity_original, severity_default_previous, severity_current);

  const auto warning_original = h::set_actions(SC_WARNING, SC_CACHE_REPORT);
  const auto message_seed_previous = h::set_actions("action/default/message", SC_LOG);
  const auto message_default_previous = h::set_actions("action/default/message");
  const auto message_current = h::set_actions("action/default/message", SC_UNSPECIFIED);
  h::set_actions("action/default/message", message_current);
  h::report(SC_WARNING, "action/default/message", "message", "defaults.cpp", 12);
  print_case("message", message_seed_previous, message_default_previous, message_current);

  const auto error_original = h::set_actions(SC_ERROR, SC_LOG);
  const auto specific_message_seed_previous = h::set_actions("action/default/specific", SC_CACHE_REPORT);
  const auto specific_seed_previous = h::set_actions("action/default/specific", SC_ERROR, SC_DISPLAY);
  const auto specific_default_previous = h::set_actions("action/default/specific", SC_ERROR);
  const auto specific_current = h::set_actions("action/default/specific", SC_ERROR, SC_UNSPECIFIED);
  h::set_actions("action/default/specific", SC_ERROR, specific_current);
  h::report(SC_ERROR, "action/default/specific", "specific", "defaults.cpp", 13);
  print_case("specific", specific_seed_previous, specific_default_previous, specific_current);

  if (records.size() != 3) return 2;
  h::set_handler(previous_handler);
  h::set_actions(SC_INFO, severity_original);
  h::set_actions(SC_WARNING, warning_original);
  h::set_actions("action/default/message", message_seed_previous);
  h::set_actions(SC_ERROR, error_original);
  h::set_actions("action/default/specific", specific_message_seed_previous);
  h::set_actions("action/default/specific", SC_ERROR, specific_seed_previous);
  std::cout << "REPORT_ACTION_DEFAULT_CASES " << records.size() << '\n';
  return 0;
}

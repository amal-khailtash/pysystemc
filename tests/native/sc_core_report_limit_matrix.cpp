/** @file sc_core_report_limit_matrix.cpp
 * @brief Native reference rows for report stop-after thresholds.
 * @details A retained native handler records SC_STOP actions and never stops
 * the kernel. Report counters are reset between cases with initialize(); the
 * SystemC kernel itself is never reset.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <tuple>
#include <vector>

using namespace sc_core;
std::vector<std::tuple<std::string, int, unsigned>> records;

/** @brief Retain only report type, severity and actions for matrix reports. */
void capture(const sc_report& report, const sc_actions& actions)
{
  if (std::string(report.get_msg_type()).find("limit/") == 0)
    records.emplace_back(report.get_msg_type(), int(report.get_severity()), actions);
}

/** @brief Install a non-stopping handler and emit all threshold boundaries. */
int sc_main(int, char**)
{
  using h = sc_report_handler;
  auto previous_handler = h::set_handler(capture);
  const char* labels[] = {"omitted", "minus_one", "zero", "one", "two"};
  const int limits[] = {-1, -1, 0, 1, 2};
  const char* scopes[] = {"severity", "message", "message_severity"};
  const sc_severity severity = SC_INFO;

  for (int scope = 0; scope < 3; ++scope)
  {
    const std::string message = std::string("limit/") + scopes[scope];
    int expected_previous = -1;
    for (int option = 0; option < 5; ++option)
    {
      h::initialize();  // Reset report counts only; the singleton kernel stays intact.
      int previous = -2;
      if (scope == 0)
        previous = option == 0 ? h::stop_after(severity) : h::stop_after(severity, limits[option]);
      else if (scope == 1)
        previous = option == 0 ? h::stop_after(message.c_str()) : h::stop_after(message.c_str(), limits[option]);
      else
        previous = option == 0 ? h::stop_after(message.c_str(), severity)
                               : h::stop_after(message.c_str(), severity, limits[option]);

      if (previous != expected_previous) return 10 + scope;
      const int configured = limits[option];
      const std::string& case_message = message;
      for (int index = 1; index <= 3; ++index)
      {
        h::report(severity, case_message.c_str(), "threshold", "report_limit_matrix.cpp", index);
        const auto& record = records.back();
        if (std::get<0>(record) != case_message || std::get<1>(record) != int(severity)) return 20;
        const unsigned actions = std::get<2>(record);
        const int count = h::get_count(case_message.c_str(), severity);
        const int severity_count = h::get_count(severity);
        const int message_count = h::get_count(message.c_str());
        std::cout << "REPORT_LIMIT " << scopes[scope] << ' ' << labels[option] << ' ' << previous << ' ' << configured
                  << ' ' << index << ' ' << actions << ' ' << count << ' ' << message_count << ' ' << severity_count
                  << '\n';
      }
      expected_previous = configured < 0 ? -1 : configured;
    }
    h::initialize();
    const int cleared = scope == 0   ? h::stop_after(severity)
                        : scope == 1 ? h::stop_after(message.c_str())
                                     : h::stop_after(message.c_str(), severity);
    if (cleared != 2) return 30 + scope;
  }

  h::initialize();
  const std::string precedence_message = "limit/precedence";
  if (h::stop_after(severity, 2) != -1 || h::stop_after(precedence_message.c_str(), 3) != -1 ||
      h::stop_after(precedence_message.c_str(), severity, 1) != -1)
    return 40;
  h::report(severity, precedence_message.c_str(), "combined", "report_limit_matrix.cpp", 40);
  if (!(std::get<2>(records.back()) & SC_STOP)) return 41;
  std::cout << "REPORT_PRECEDENCE message_severity 1 " << std::get<2>(records.back()) << ' '
            << h::get_count(precedence_message.c_str(), severity) << ' ' << h::get_count(precedence_message.c_str())
            << ' ' << h::get_count(severity) << '\n';
  if (h::stop_after(precedence_message.c_str(), severity) != 1) return 42;

  h::initialize();
  for (int index = 1; index <= 3; ++index)
  {
    h::report(severity, precedence_message.c_str(), "message", "report_limit_matrix.cpp", 40 + index);
    const auto actions = std::get<2>(records.back());
    const bool expected_stop = index >= 3;
    if (bool(actions & SC_STOP) != expected_stop) return 43;
    std::cout << "REPORT_PRECEDENCE message " << index << ' ' << actions << ' '
              << h::get_count(precedence_message.c_str(), severity) << ' ' << h::get_count(precedence_message.c_str())
              << ' ' << h::get_count(severity) << '\n';
  }
  if (h::stop_after(precedence_message.c_str()) != 3) return 44;

  h::initialize();
  for (int index = 1; index <= 2; ++index)
  {
    h::report(severity, precedence_message.c_str(), "severity", "report_limit_matrix.cpp", 50 + index);
    const auto actions = std::get<2>(records.back());
    const bool expected_stop = index >= 2;
    if (bool(actions & SC_STOP) != expected_stop) return 45;
    std::cout << "REPORT_PRECEDENCE severity " << index << ' ' << actions << ' '
              << h::get_count(precedence_message.c_str(), severity) << ' ' << h::get_count(precedence_message.c_str())
              << ' ' << h::get_count(severity) << '\n';
  }
  if (h::stop_after(severity) != 2) return 46;

  h::set_handler(previous_handler);
  return 0;
}

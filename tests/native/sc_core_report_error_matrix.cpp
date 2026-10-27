/** @file sc_core_report_error_matrix.cpp
 * @brief Native report action precedence and thrown error reference.
 * @details Callback values are copied while reports are borrowed. A thrown
 * report and cached copy are inspected before their owning scopes end.
 */
#include <functional>
#include <iostream>
#include <string>
#include <systemc>
#include <tuple>
#include <vector>

using namespace sc_core;
std::vector<std::tuple<std::string, std::string, unsigned, int>> records;
int callback_calls = 0;

/** @brief Capture primitive report values for policy parity checks. */
void capture(const sc_report& report, const sc_actions& actions)
{
  if (std::string(report.get_msg_type()).find("matrix/") == 0)
    records.emplace_back(report.get_msg_type(), report.get_msg(), actions, int(report.get_severity()));
}

/** @brief Throw a native callback exception after recording one report dispatch. */
void fail_once(const sc_report&, const sc_actions&)
{
  ++callback_calls;
  throw std::runtime_error("report_callback_matrix_marker");
}

/** @brief Check a throwing report overload and its cached report copy. */
int check_thrown_report(const char* label, const char* message_type, int report_id, int verbosity, int line,
                        int expected_count, const std::function<void()>& invoke)
{
  using h = sc_report_handler;
  if (h::set_actions(message_type, SC_ERROR, SC_THROW | SC_CACHE_REPORT) != SC_UNSPECIFIED) return 20;
  try
  {
    invoke();
    return 21;
  }
  catch (const sc_report& report)
  {
    if (report.get_severity() != SC_ERROR || std::string(report.get_msg_type()) != message_type ||
        report.get_id() != report_id || std::string(report.get_msg()) != label ||
        std::string(report.get_file_name()) != "error_matrix.cpp" || report.get_line_number() != line ||
        report.get_verbosity() != verbosity)
      return 22;
  }
  if (h::get_count(message_type) != 1 || h::get_count(message_type, SC_ERROR) != 1 ||
      h::get_count(SC_ERROR) != expected_count)
    return 23;
  auto* cached = h::get_cached_report();
  if (!cached || std::string(cached->get_msg()) != label) return 24;
  sc_report copy(*cached);
  h::clear_cached_report();
  if (h::get_cached_report() || std::string(copy.get_msg()) != label) return 25;
  std::cout << "REPORT_ERROR_THROW " << label << ' ' << message_type << ' ' << report_id << ' ' << verbosity << ' '
            << h::get_count(SC_ERROR) << '\n';
  return 0;
}

/** @brief Assert report action precedence, global masks, counters and throw caching. */
int sc_main(int, char**)
{
  using h = sc_report_handler;
  h::initialize();
  auto old_severity_actions = h::set_actions(SC_WARNING, SC_DISPLAY | SC_LOG);
  auto old_message_actions = h::set_actions("matrix/policy", SC_LOG);
  auto old_specific_actions = h::set_actions("matrix/policy", SC_WARNING, SC_CACHE_REPORT);
  if (old_severity_actions != SC_DEFAULT_WARNING_ACTIONS || old_message_actions != SC_UNSPECIFIED ||
      old_specific_actions != SC_UNSPECIFIED)
    return 2;
  auto previous = h::set_handler(capture);
  h::report(SC_WARNING, "matrix/policy", "specific", "error_matrix.cpp", 31);
  if (!h::get_cached_report() || std::string(h::get_cached_report()->get_msg()) != "specific") return 3;
  h::report(SC_INFO, "matrix/policy", "message", "error_matrix.cpp", 32);
  if (h::get_count("matrix/policy") != 2 || h::get_count("matrix/policy", SC_WARNING) != 1 ||
      h::get_count(SC_WARNING) != 1 || h::get_count(SC_INFO) != 1)
    return 4;

  auto old_suppress = h::suppress(SC_DISPLAY | SC_LOG);
  auto old_force = h::force(SC_THROW);
  if (old_suppress != SC_UNSPECIFIED || old_force != SC_UNSPECIFIED) return 5;
  h::report(SC_INFO, "matrix/policy", "masked", "error_matrix.cpp", 33);
  auto suppress_mask = h::suppress();
  auto force_mask = h::force();
  if (suppress_mask != (SC_DISPLAY | SC_LOG) || force_mask != SC_THROW || h::get_count("matrix/policy") != 3 ||
      std::get<2>(records.back()) != SC_THROW)
    return 6;
  h::clear_cached_report();
  if (h::get_cached_report()) return 7;
  if (h::stop_after("matrix/zero", SC_ERROR, 0) != -1) return 26;
  h::set_handler(capture);
  h::report(SC_ERROR, "matrix/zero", "zero", "error_matrix.cpp", 34);
  h::report(SC_ERROR, "matrix/zero", "zero", "error_matrix.cpp", 35);
  h::set_handler(previous);
  if (h::stop_after("matrix/zero", SC_ERROR) != 0 || h::get_count("matrix/zero") != 2 ||
      h::get_count("matrix/zero", SC_ERROR) != 2 || (std::get<2>(records[records.size() - 1]) & SC_STOP) ||
      (std::get<2>(records[records.size() - 2]) & SC_STOP))
    return 27;
  std::cout << "REPORT_STOP_ZERO " << h::get_count("matrix/zero") << " 0\n";
  h::set_handler(previous);
  h::set_actions(SC_WARNING, old_severity_actions);

  if (check_thrown_report("message", "matrix/error_message", -1, SC_MEDIUM, 41, 3,
                          []()
                          { h::report(SC_ERROR, "matrix/error_message", "message", "error_matrix.cpp", 41); }) != 0)
    return 8;
  if (check_thrown_report(
        "verbosity", "matrix/error_verbosity", -1, SC_HIGH, 42, 4,
        []() { h::report(SC_ERROR, "matrix/error_verbosity", "verbosity", SC_HIGH, "error_matrix.cpp", 42); }) != 0)
    return 9;
  sc_report::register_id(61011, "matrix/error_legacy");
  if (check_thrown_report("legacy", "matrix/error_legacy", 61011, SC_MEDIUM, 43, 5,
                          []() { h::report(SC_ERROR, 61011, "legacy", "error_matrix.cpp", 43); }) != 0)
    return 10;

  if (h::set_actions("matrix/callback", SC_ERROR, SC_CACHE_REPORT) != SC_UNSPECIFIED) return 11;
  h::set_handler(fail_once);
  try
  {
    h::report(SC_ERROR, "matrix/callback", "callback", "error_matrix.cpp", 44);
    return 12;
  }
  catch (const std::runtime_error& error)
  {
    if (std::string(error.what()) != "report_callback_matrix_marker") return 13;
  }
  h::set_handler(previous);
  if (callback_calls != 1 || h::get_count("matrix/callback") != 1 || h::get_count("matrix/callback", SC_ERROR) != 1 ||
      h::get_count(SC_ERROR) != 6)
    return 14;
  std::cout << "REPORT_CALLBACK_ERROR matrix/callback callback " << SC_CACHE_REPORT << ' ' << callback_calls << '\n';

  h::set_handler(capture);
  h::report(SC_ERROR, "matrix/error_action", "action", "error_matrix.cpp", 45);
  h::set_handler(previous);
  if (std::get<2>(records.back()) != SC_DEFAULT_ERROR_ACTIONS) return 15;

  for (const auto& row : records)
    std::cout << "REPORT_ERROR_ACTION " << std::get<0>(row) << ' ' << std::get<1>(row) << ' ' << std::get<2>(row) << ' '
              << std::get<3>(row) << '\n';
  std::cout << "REPORT_ERROR_COUNTS " << h::get_count("matrix/policy") << ' ' << h::get_count(SC_WARNING) << ' '
            << h::get_count(SC_INFO) << ' ' << h::get_count(SC_ERROR) << '\n';
  return 0;
}

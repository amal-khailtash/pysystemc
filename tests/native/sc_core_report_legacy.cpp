/** @file sc_core_report_legacy.cpp
 * @brief Complete legacy report control, assignment and destructor reference.
 * @details Copies own their values; handlers borrow reports for callback duration.
 */
#include <systemc>
#include <tuple>
#include <vector>
std::vector<sc_core::sc_report> owned;
std::vector<std::tuple<int, int, unsigned>> records;
/** @brief Retain action/severity and copy only tested report IDs. */
void capture(const sc_core::sc_report& report, const sc_core::sc_actions& actions)
{
  std::string type = report.get_msg_type();
  if (type.find("legacy/") == 0 || type == "unknown id")
  {
    owned.push_back(report);
    records.emplace_back(report.get_id(), int(report.get_severity()), actions);
  }
}
/** @brief Require invalid registration to throw its native report. */
void rejected(int id, const char* message)
{
  try
  {
    sc_core::sc_report::register_id(id, message);
    throw std::runtime_error("invalid accepted");
  }
  catch (const sc_core::sc_report&)
  {
  }
}
/** @brief Exercise every public legacy report configuration and restore defaults. */
int sc_main(int, char**)
{
  using namespace sc_core;
  sc_report::register_id(61001, "legacy/first");
  sc_report::register_id(61002, "legacy/second");
  sc_report::register_id(61001, "legacy/first");
  if (std::string(sc_report::get_message(61001)) != "legacy/first" ||
      std::string(sc_report::get_message(61999)) != "unknown id" || sc_report::is_suppressed(61999))
    return 2;
  sc_report::suppress_id(61999, true);
  rejected(-1, "bad");
  rejected(61001, "conflict");
  rejected(61003, nullptr);
  auto previous = sc_report_handler::set_handler(capture);
  sc_report::suppress_id(61001, true);
  sc_report_handler::report(SC_INFO, 61001, "suppressed", "legacy.cpp", 9);
  sc_report::suppress_id(61001, false);
  sc_report::suppress_infos(true);
  sc_report_handler::report(SC_INFO, 61001, "info", "legacy.cpp", 9);
  sc_report::suppress_infos(false);
  sc_report_handler::report(SC_INFO, 61001, "normal", "legacy.cpp", 9);
  sc_report::suppress_warnings(true);
  sc_report_handler::report(SC_WARNING, 61002, "warning", "legacy.cpp", 9);
  sc_report::suppress_warnings(false);
  sc_report::make_warnings_errors(true);
  sc_report_handler::report(SC_WARNING, 61002, "promoted", "legacy.cpp", 9);
  sc_report::make_warnings_errors(false);
  sc_report_handler::report(SC_WARNING, 61002, "normal", "legacy.cpp", 9);
  sc_report_handler::report(SC_INFO, 61999, "missing", "legacy.cpp", 9);
  sc_report_handler::set_handler(previous);
  owned[0] = owned[4];
  owned[0] = owned[0];
  if (std::string(owned[0].get_msg()) != "promoted" || owned[0].get_severity() != SC_ERROR) return 3;
  auto* copy = new sc_report(owned[0]);
  delete copy;
  for (auto& row : records)
    std::cout << "REPORT_LEGACY_REFERENCE " << std::get<0>(row) << " " << std::get<1>(row) << " " << std::get<2>(row)
              << "\n";
  std::cout << "REPORT_ASSIGN_REFERENCE promoted 2\n";
  return 0;
}

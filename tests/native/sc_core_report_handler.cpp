/** @file sc_core_report_handler.cpp
 * @brief Native complete report-handler overload/policy/cache/log reference.
 * @details Reports borrowed during handler, copies destroyed before release.
 */
#include <fstream>
#include <systemc>
#include <tuple>
#include <vector>
using namespace sc_core;
std::vector<std::tuple<std::string, std::string, unsigned, int>> records;
/** @brief Record primitive callback values without retaining report pointers. */
void capture(const sc_report& report, const sc_actions& actions)
{
  if (std::string(report.get_msg_type()).find("handler/") == 0)
    records.emplace_back(report.get_msg_type(), report.get_msg(), actions, report.get_verbosity());
}
/** @brief Exercise all handler policy, cache, count, limit, log and lifecycle methods. */
int sc_main(int, char**)
{
  using h = sc_report_handler;
  auto previous = h::set_handler(capture);
  if (!h::get_handler()) return 2;
  h::initialize();
  auto old = h::set_actions(SC_WARNING, SC_DISPLAY);
  h::set_actions("handler/policy", SC_LOG);
  h::set_actions("handler/policy", SC_WARNING, SC_CACHE_REPORT);
  h::report(SC_WARNING, "handler/policy", "specific", "handler.cpp", 12);
  {
    sc_report copy(*h::get_cached_report());
    h::clear_cached_report();
    if (h::get_cached_report() || std::string(copy.get_msg()) != "specific") return 3;
    h::default_handler(copy, SC_DO_NOTHING);
  }
  h::set_actions("handler/policy", SC_WARNING, SC_UNSPECIFIED);
  h::report(SC_WARNING, "handler/policy", "type", "handler.cpp", 12);
  h::set_actions("handler/policy", SC_UNSPECIFIED);
  h::report(SC_WARNING, "handler/policy", "severity", "handler.cpp", 12);
  h::suppress(SC_DISPLAY);
  h::force(SC_LOG);
  h::report(SC_WARNING, "handler/policy", "masks", "handler.cpp", 12);
  h::suppress();
  h::force();
  h::set_actions(SC_WARNING, old);
  if (h::get_count("handler/policy") != 4 || h::get_count("handler/policy", SC_WARNING) != 4 ||
      h::get_count(SC_WARNING) != 4)
    return 4;
  auto verbosity = h::set_verbosity_level(SC_LOW);
  h::report(SC_INFO, "handler/verbosity", "hidden", "handler.cpp", 12);
  h::report(SC_INFO, "handler/verbosity", "hidden", SC_HIGH, "handler.cpp", 12);
  h::report(SC_INFO, "handler/verbosity", "visible", SC_LOW, "handler.cpp", 12);
  if (h::get_count("handler/verbosity") != 1 || h::get_verbosity_level() != SC_LOW) return 5;
  h::set_verbosity_level(verbosity);
  for (int scope = 0; scope < 3; ++scope)
  {
    if (scope == 0)
      h::stop_after(SC_INFO, 2);
    else if (scope == 1)
      h::stop_after("handler/limit", 2);
    else
      h::stop_after("handler/limit", SC_INFO, 2);
    h::report(SC_INFO, "handler/limit", "limit", "handler.cpp", 12);
    h::report(SC_INFO, "handler/limit", "limit", "handler.cpp", 12);
    if (!(std::get<2>(records.back()) & SC_STOP)) return 6;
    if (scope == 0)
      h::stop_after(SC_INFO);
    else if (scope == 1)
      h::stop_after("handler/limit");
    else
      h::stop_after("handler/limit", SC_INFO);
  }
  auto caught = h::get_catch_actions();
  h::set_catch_actions(SC_DO_NOTHING);
  if (h::get_catch_actions() != SC_DO_NOTHING) return 7;
  h::set_catch_actions(caught);
  auto action = h::get_new_action_id();
  h::set_handler(previous);
  if (!h::set_log_file_name("handler.log") || h::set_log_file_name("other.log")) return 8;
  h::set_actions("handler/log", SC_LOG);
  h::report(SC_INFO, "handler/log", "written", "handler.cpp", 12);
  if (sc_report_close_default_log() || h::get_log_file_name() || !sc_report_close_default_log()) return 9;
  auto process = sc_spawn(
    []()
    {
      h::set_actions("handler/local", SC_CACHE_REPORT);
      auto old = h::set_handler(capture);
      h::report(SC_WARNING, "handler/local", "local", "handler.cpp", 12);
      {
        sc_report retained(*h::get_cached_report());
        if (std::string(retained.get_process_name()) != "cache_worker") throw std::runtime_error("process cache");
        h::clear_cached_report();
        if (h::get_cached_report() || std::string(retained.get_msg()) != "local")
          throw std::runtime_error("clear cache");
      }
      h::set_handler(old);
    },
    "cache_worker");
  sc_start(1, SC_NS);
  if (!process.terminated() || h::get_cached_report()) return 12;
  h::set_handler(capture);
  h::set_actions("handler/throw", SC_CACHE_REPORT);
  h::report(SC_WARNING, "handler/throw", "specific", "handler.cpp", 12);
  h::set_handler(previous);
  {
    sc_report value(*h::get_cached_report());
    h::clear_cached_report();
    try
    {
      h::default_handler(value, SC_THROW);
      return 13;
    }
    catch (const sc_report&)
    {
    }
    h::default_handler(value, SC_STOP);
  }
  h::initialize();
  if (h::get_count("handler/policy") != 0) return 10;
  h::release();
  h::initialize();
  if (h::get_count("handler/policy") != 0) return 11;
  records.resize(11);
  for (auto& row : records)
    std::cout << "HANDLER_REFERENCE " << std::get<0>(row) << " " << std::get<1>(row) << " " << std::get<2>(row) << " "
              << std::get<3>(row) << "\n";
  std::cout << "HANDLER_FINAL_REFERENCE " << action << " " << records.size() << "\n";
  return 0;
}

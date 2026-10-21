/** @file sc_core_default_actions.cpp
 * @brief Native default-report action ordering and terminal abort reference.
 * @details Copies own fields; each invocation has fresh kernel and log state.
 */
#include <sys/resource.h>

#include <csignal>
#include <fstream>
#include <systemc>
#include <vector>
std::vector<sc_core::sc_report> reports;
/** @brief Copy a borrowed report for independent default-handler invocation. */
void capture(const sc_core::sc_report& report, const sc_core::sc_actions&)
{
  reports.push_back(report);
}
/** @brief Run one default action, including terminal abort, with no core dump. */
int sc_main(int argc, char** argv)
{
  using namespace sc_core;
  rlimit limit{0, 0};
  setrlimit(RLIMIT_CORE, &limit);
  std::signal(SIGABRT, SIG_DFL);
  if (argc != 2) return 2;
  std::string mode = argv[1];
  auto previous = sc_report_handler::set_handler(capture);
  sc_report_handler::report(SC_WARNING, "actions/test", "message", "actions.cpp", 17);
  sc_report_handler::set_handler(previous);
  sc_actions actions = SC_DO_NOTHING;
  if (mode == "display")
    actions = SC_DISPLAY;
  else if (mode == "log")
    actions = SC_LOG;
  else if (mode == "interrupt")
    actions = SC_INTERRUPT;
  else if (mode == "throw")
    actions = SC_THROW;
  else if (mode == "stop")
    actions = SC_STOP;
  else if (mode == "stop_throw")
    actions = SC_STOP | SC_THROW;
  else if (mode == "abort")
    actions = SC_ABORT;
  else if (mode == "combined")
    actions = SC_DISPLAY | SC_LOG | SC_INTERRUPT | SC_THROW;
  if (mode == "log" || mode == "combined") sc_report_handler::set_log_file_name("actions.log");
  std::cout << "DEFAULT_ACTION_ENTER " << mode << std::endl;
  try
  {
    sc_report_handler::default_handler(reports[0], actions);
    if (mode == "throw" || mode == "combined" || mode == "abort" || mode == "stop_throw") return 3;
  }
  catch (const sc_report&)
  {
    if (mode != "throw" && mode != "combined" && mode != "stop_throw") return 4;
    std::cout << "DEFAULT_ACTION_THROW " << mode << "\n";
  }
  if (mode == "log" || mode == "combined")
  {
    if (sc_report_close_default_log()) return 5;
    std::ifstream input("actions.log");
    std::string contents((std::istreambuf_iterator<char>(input)), {});
    if (contents.find("Warning: actions/test: message\nIn file: actions.cpp:17") == std::string::npos) return 6;
  }
  if ((mode == "stop" || mode == "stop_throw") && sc_get_status() != SC_STOPPED) return 7;
  std::cout << "DEFAULT_ACTION_RETURN " << mode << "\n";
  return 0;
}

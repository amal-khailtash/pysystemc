/** @file sc_core_kernel_report_boundaries.cpp
 * @brief Native references for recoverable wait, event-list and stop reports.
 * @details The custom handler copies report fields during the callback and
 * delegates configured throws to SystemC's default handler. Every independent
 * scenario runs in a separate process so singleton kernel state stays intact.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

using namespace sc_core;

namespace
{
struct CapturedReport
{
  std::string label;
  int id;
  int severity;
  std::string message_type;
  std::string message;
  std::uint64_t time;
  int count;
};

std::string scenario;
std::string current_label;
std::vector<CapturedReport> reports;
std::vector<std::string> recovered;

/** @brief Copy native report metadata before the default handler throws.
 * @param report Borrowed report, copied before this callback returns.
 * @param actions Handler actions selected by the native report policy.
 * @details The callback runs on the simulation thread or sc_main thread; it
 * acquires no Python state and retains no borrowed report references.
 */
void capture_report(const sc_report& report, const sc_actions& actions)
{
  if (!current_label.empty())
    reports.push_back({current_label, report.get_id(), static_cast<int>(report.get_severity()), report.get_msg_type(),
                       report.get_msg(), report.get_time().value(),
                       sc_report_handler::get_count(report.get_msg_type())});
  sc_report_handler::default_handler(report, actions);
}

/** @brief Require one report, validate its native metadata, and record recovery.
 * @param label Stable operation name shared with the Python model.
 * @param id Expected native report number.
 * @param severity Expected SystemC report severity.
 * @param operation Public API operation expected to throw through SC_THROW.
 */
void attempt(const std::string& label, int id, sc_severity severity, const std::function<void()>& operation)
{
  current_label = label;
  const auto before = reports.size();
  try
  {
    operation();
  }
  catch (const sc_report& report)
  {
    if (reports.size() != before + 1 || report.get_id() != id || report.get_severity() != severity ||
        report.get_msg_type() != reports.back().message_type || report.get_time().value() != sc_time_stamp().value())
      throw std::runtime_error("native report metadata mismatch: " + label);
    recovered.push_back(label);
    return;
  }
  throw std::runtime_error("configured native report did not throw: " + label);
}

/** @brief Print an error message with the same whitespace normalization as Python. */
std::string normalized(const std::string& message)
{
  std::istringstream input(message);
  std::string word, result;
  while (input >> word)
  {
    if (!result.empty()) result += ' ';
    result += word;
  }
  return result;
}

/** @brief Emit reports and callback recovery records in observed order. */
void print_records()
{
  for (const auto& report : reports)
  {
    std::cout << "KERNEL_BOUNDARY_REPORT " << scenario << '|' << report.label << '|' << report.id << '|'
              << report.severity << '|' << report.message_type << '|' << normalized(report.message) << '|'
              << report.time << '|' << report.count << '\n';
    const auto found = std::find(recovered.begin(), recovered.end(), report.label);
    if (found != recovered.end()) std::cout << "KERNEL_BOUNDARY_RECOVERED " << scenario << '|' << report.label << '\n';
  }
  std::cout << "KERNEL_BOUNDARY_OK " << scenario << " reports=" << reports.size() << '\n';
}

/** @brief Run invalid waits in one native thread and prove both recoveries. */
int run_wait_n()
{
  sc_report_handler::set_actions(SC_ID_WAIT_N_INVALID_, SC_ERROR, SC_THROW);
  bool finished = false;
  auto process = sc_spawn(
    [&]
    {
      attempt("wait_zero", 525, SC_ERROR, [] { wait(0); });
      attempt("wait_negative", 525, SC_ERROR, [] { wait(-1); });
      finished = true;
    },
    "invalid_wait_n");
  sc_start(1, SC_NS);
  if (!process.terminated() || !finished || reports.size() != 2 ||
      sc_report_handler::get_count(SC_ID_WAIT_N_INVALID_) != 2)
    throw std::runtime_error("invalid wait callback did not recover");
  return 0;
}

/** @brief Exercise empty OR/AND lists from both wait and next_trigger. */
struct EmptyListModel : sc_module
{
  sc_event kick{"empty_list_kick"};
  sc_event_or_list or_list;
  sc_event_and_list and_list;
  bool wait_done = false;
  bool trigger_done = false;

  SC_HAS_PROCESS(EmptyListModel);

  /** @brief Register the thread and dynamically sensitive method. */
  explicit EmptyListModel(sc_module_name name) : sc_module(name)
  {
    SC_THREAD(wait_empty_lists);
    SC_METHOD(trigger_empty_lists);
    sensitive << kick;
    dont_initialize();
  }

  /** @brief Catch both empty-list wait reports and continue the thread. */
  void wait_empty_lists()
  {
    attempt("wait_empty_or", 565, SC_ERROR, [&] { wait(or_list); });
    attempt("wait_empty_and", 565, SC_ERROR, [&] { wait(and_list); });
    wait_done = true;
  }

  /** @brief Catch both empty-list next_trigger reports and return normally. */
  void trigger_empty_lists()
  {
    attempt("next_trigger_empty_or", 565, SC_ERROR, [&] { next_trigger(or_list); });
    attempt("next_trigger_empty_and", 565, SC_ERROR, [&] { next_trigger(and_list); });
    trigger_done = true;
  }
};

/** @brief Start the empty-list model and require all four recovery paths. */
int run_empty_lists()
{
  sc_report_handler::set_actions(SC_ID_EVENT_LIST_FAILED_, SC_ERROR, SC_THROW);
  EmptyListModel model("empty_list_boundary");
  model.kick.notify(SC_ZERO_TIME);
  sc_start(1, SC_NS);
  if (!model.wait_done || !model.trigger_done || reports.size() != 4 ||
      sc_report_handler::get_count(SC_ID_EVENT_LIST_FAILED_) != 4)
    throw std::runtime_error("empty-list callbacks did not recover");
  return 0;
}

/** @brief Require one duplicate-stop warning and recover from a later start. */
int run_stop_start()
{
  sc_report_handler::set_actions(SC_ID_SIMULATION_STOP_CALLED_TWICE_, SC_WARNING, SC_THROW);
  sc_report_handler::set_actions(SC_ID_SIMULATION_START_AFTER_STOP_, SC_ERROR, SC_THROW);
  // This first call models the initial public stop request. The next call
  // exercises the native duplicate-stop warning, then a third tests suppression.
  sc_stop();
  attempt("duplicate_stop", 545, SC_WARNING, [] { sc_stop(); });
  const int warning_count = sc_report_handler::get_count(SC_ID_SIMULATION_STOP_CALLED_TWICE_);
  if (warning_count != 1) throw std::runtime_error("duplicate-stop warning count");
  sc_stop();
  if (sc_report_handler::get_count(SC_ID_SIMULATION_STOP_CALLED_TWICE_) != warning_count)
    throw std::runtime_error("duplicate-stop warning repeated");
  attempt("start_after_stop", 546, SC_ERROR, [] { sc_start(SC_ZERO_TIME); });
  if (sc_report_handler::get_count(SC_ID_SIMULATION_START_AFTER_STOP_) != 1)
    throw std::runtime_error("start-after-stop error count");
  if (reports.size() != 2 || recovered.size() != 2) throw std::runtime_error("stop/start report recovery count");
  return 0;
}
}  // namespace

/** @brief Run one fresh-process report boundary selected by the native argument. */
int sc_main(int argc, char** argv)
{
  if (argc != 2) throw std::runtime_error("expected one scenario argument");
  scenario = argv[1];
  const auto previous = sc_report_handler::set_handler(&capture_report);
  int result = 0;
  if (scenario == "wait_n")
    result = run_wait_n();
  else if (scenario == "empty_lists")
    result = run_empty_lists();
  else if (scenario == "stop_start")
    result = run_stop_start();
  else
    throw std::runtime_error("unknown kernel report boundary scenario");
  sc_report_handler::set_handler(previous);
  print_records();
  return result;
}

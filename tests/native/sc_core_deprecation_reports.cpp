/** @file sc_core_deprecation_reports.cpp
 * @brief Native references for selected IEEE 1666 deprecation report sites.
 * @details Each scenario runs in a fresh process so SystemC's per-site
 * warn-once flags match the Python model. The report handler copies metadata
 * during the callback and retains no borrowed report data.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

using namespace sc_core;

namespace sc_core
{
SC_API void sc_cycle(const sc_time& duration);
}

namespace
{
std::string current_label;
std::vector<std::string> records;

/** @brief Normalize message whitespace for Python/native parity. */
std::string normalized(const std::string& message)
{
  std::istringstream words(message);
  std::string word, result;
  while (words >> word)
  {
    if (!result.empty()) result += ' ';
    result += word;
  }
  return result;
}

/** @brief Keep source filenames portable while preserving the source site. */
std::string basename(const char* filename)
{
  const std::string path = filename ? filename : "";
  const auto separator = path.find_last_of("/\\");
  return separator == std::string::npos ? path : path.substr(separator + 1);
}

/** @brief Copy exact report metadata and current kernel state. */
void capture(const sc_report& report, const sc_actions& actions)
{
  const std::string message_type = report.get_msg_type();
  if (message_type != SC_ID_IEEE_1666_DEPRECATION_ && message_type != SC_ID_NOT_IMPLEMENTED_) return;
  if (current_label.empty()) throw std::runtime_error("unlabelled deprecation report");
  records.push_back(
    "DEPRECATION_REPORT " + current_label + "|" + std::to_string(report.get_id()) + "|" +
    std::to_string(static_cast<int>(report.get_severity())) + "|" + message_type + "|" + normalized(report.get_msg()) +
    "|" + basename(report.get_file_name()) + "|" + std::to_string(report.get_line_number()) + "|" +
    std::to_string(report.get_time().value()) + "|" + std::to_string(report.get_verbosity()) + "|" +
    std::to_string(sc_report_handler::get_count(message_type.c_str())) + "|" + std::to_string(sc_delta_count()) + "|" +
    std::to_string(static_cast<int>(sc_get_status())) + "|" + std::to_string(static_cast<unsigned>(actions)));
}

/** @brief Capture handler state and enable deterministic observation of INFO reports. */
sc_report_handler_proc set_capture()
{
  const auto previous = sc_report_handler::set_handler(capture);
  sc_report_handler::set_actions(SC_ID_IEEE_1666_DEPRECATION_, SC_INFO, SC_DISPLAY);
  sc_report_handler::set_actions(SC_ID_NOT_IMPLEMENTED_, SC_WARNING, SC_DISPLAY);
  return previous;
}

/** @brief Emit copied records and final pre-summary report/kernel counts. */
void print_final(const std::string& scenario, std::size_t expected_reports, sc_report_handler_proc previous)
{
  const bool deprecations = scenario != "random_seed";
  const auto type = deprecations ? SC_ID_IEEE_1666_DEPRECATION_ : SC_ID_NOT_IMPLEMENTED_;
  const auto count = sc_report_handler::get_count(type);
  if (count != static_cast<int>(expected_reports) || records.size() != expected_reports)
    throw std::runtime_error("deprecation report count mismatch");
  records.push_back("DEPRECATION_FINAL " + scenario + "|" + std::to_string(count) + "|" +
                    std::to_string(sc_time_stamp().value()) + "|" + std::to_string(sc_delta_count()) + "|" +
                    std::to_string(static_cast<int>(sc_get_status())));
  sc_report_handler::set_handler(previous);
  for (const auto& record : records) std::cout << record << '\n';
  std::cout << "DEPRECATION_OK " << scenario << " reports=" << expected_reports << '\n';
}

/** @brief Exercise four deprecated sc_simcontext members and four global APIs. */
void run_simcontext()
{
  sc_module_name name("top");
  struct PlainModel : sc_module
  {
    explicit PlainModel(sc_module_name module_name) : sc_module(module_name)
    {
    }
  } top(name);
  auto* context = sc_get_curr_simcontext();
  current_label = "find_object";
  if (!context->find_object("top")) throw std::runtime_error("deprecated find_object result");
  current_label = "get_child_objects";
  if (context->get_child_objects().empty()) throw std::runtime_error("deprecated child list result");
  current_label = "delta_count";
  if (context->delta_count() != sc_delta_count()) throw std::runtime_error("deprecated delta count result");
  current_label = "is_running";
  if (context->is_running() != sc_is_running()) throw std::runtime_error("deprecated running result");
  current_label = "initialize";
  sc_initialize();
  current_label = "cycle";
  sc_core::sc_cycle(sc_time(1, SC_NS));
  current_label = "simulation_time";
  if (std::fabs(sc_simulation_time() - sc_time_stamp().to_default_time_units()) > 1e-12)
    throw std::runtime_error("deprecated simulation-time result");
}

/** @brief Call deprecated current-process, CTHREAD wait, event-list wait and timed_out sites. */
struct CthreadModel : sc_module
{
  sc_clock clock;
  sc_in<bool> clock_input;
  sc_event first, second;
  bool finished = false;
  SC_HAS_PROCESS(CthreadModel);

  /** @brief Register the clocked worker and event driver. */
  explicit CthreadModel(sc_module_name name)
    : sc_module(name), clock("clock", sc_time(2, SC_NS)), clock_input("clock_input")
  {
    clock_input(clock);
    SC_CTHREAD(worker, clock_input.pos());
    SC_THREAD(driver);
  }

  /** @brief Visit each deprecated operation once while the native process remains valid. */
  void worker()
  {
    current_label = "current_process_handle";
    auto* process = sc_get_curr_process_handle();
    if (!process || process != sc_get_current_process_b()) throw std::runtime_error("current process result");
    current_label = "cthread_general_wait";
    wait(first);
    current_label = "cthread_event_list_wait";
    sc_event_or_list events = first | second;
    wait(events);
    current_label = "timed_out";
    (void)timed_out();
    finished = true;
  }

  /** @brief Notify wait events between clock edges to exercise CTHREAD waits. */
  void driver()
  {
    wait(1, SC_NS);
    first.notify(SC_ZERO_TIME);
    wait(2, SC_NS);
    second.notify(SC_ZERO_TIME);
  }
};

/** @brief Exercise positional stream binding and repeated module call binding. */
struct BindingChild : sc_module
{
  sc_in<bool> first, second;

  explicit BindingChild(sc_module_name name) : sc_module(name), first("first"), second("second")
  {
  }
};

/** @brief Own separate children so each deprecated binding branch has one clean trigger. */
struct BindingTop : sc_module
{
  sc_signal<bool> signal;
  BindingChild positional;
  BindingChild repeated;

  explicit BindingTop(sc_module_name name)
    : sc_module(name), signal("signal"), positional("positional"), repeated("repeated")
  {
    current_label = "positional_shift";
    positional << signal;
    current_label = "repeat_first";
    repeated(signal);
    current_label = "repeat_second";
    repeated(signal);
  }
};

/** @brief Call one native operation in a labelled report context. */
template <class Operation>
void labeled(const char* label, Operation&& operation)
{
  current_label = label;
  operation();
}

/** @brief Execute a process-context scenario and require native lifecycle completion. */
void run_cthread()
{
  CthreadModel model("cthread_model");
  sc_start(sc_time(6, SC_NS));
  if (!model.finished || sc_time_stamp() != sc_time(6, SC_NS) || sc_get_status() != SC_PAUSED)
    throw std::runtime_error("CTHREAD deprecation scenario state");
}

/** @brief Trigger each distinct module binding report and finish elaboration. */
void run_module()
{
  BindingTop model("binding_top");
  sc_start(SC_ZERO_TIME);
  if (sc_get_status() != SC_PAUSED || sc_time_stamp() != SC_ZERO_TIME)
    throw std::runtime_error("module binding scenario state");
}

/** @brief Safely characterize the native unimplemented seed warning. */
void run_random_seed()
{
  labeled("random_seed", [] { sc_set_random_seed(123); });
}
}  // namespace

/** @brief Run one fresh-process audit scenario selected by argv. */
int sc_main(int argc, char** argv)
{
  if (argc != 2) throw std::runtime_error("expected one deprecation scenario");
  const std::string scenario = argv[1];
  const auto previous = set_capture();
  std::size_t expected_reports = 0;
  if (scenario == "simcontext")
  {
    run_simcontext();
    expected_reports = 7;
  }
  else if (scenario == "cthread")
  {
    run_cthread();
    expected_reports = 4;
  }
  else if (scenario == "module")
  {
    run_module();
    expected_reports = 2;
  }
  else if (scenario == "random_seed")
  {
    run_random_seed();
    expected_reports = 1;
  }
  else
    throw std::runtime_error("unknown deprecation scenario: " + scenario);
  print_final(scenario, expected_reports, previous);
  return 0;
}

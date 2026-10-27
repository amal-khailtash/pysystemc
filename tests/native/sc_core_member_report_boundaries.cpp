/** @file sc_core_member_report_boundaries.cpp
 * @brief Native references for invalid member scheduling and stop-mode reports.
 * @details Each scenario executes in a fresh process because SystemC keeps
 * singleton kernel state. Reports are copied while caught; no callbacks or
 * Python/GIL state are retained by this model.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

using namespace sc_core;

namespace
{
std::vector<std::string> records;

/** @brief Expose the native primitive-channel member overloads to the model. */
struct Channel : sc_prim_channel
{
  using sc_prim_channel::next_trigger;

  explicit Channel(const char* name) : sc_prim_channel(name)
  {
  }
};

/** @brief Normalize report whitespace for exact Python/native output comparison. */
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

/** @brief Require one native report and record exact metadata and kernel state.
 * @param route Member owner or public function route under test.
 * @param label Stable operation label shared with the Python reference.
 * @param report_id Expected native report number.
 * @param message_type Expected registered message type.
 * @param operation Native operation that must throw under SC_THROW.
 */
template <class Operation>
void attempt(const std::string& route, const std::string& label, int report_id, const char* message_type,
             Operation&& operation, sc_status expected_status = SC_RUNNING)
{
  const int before = sc_report_handler::get_count(message_type);
  try
  {
    operation();
  }
  catch (const sc_report& report)
  {
    const int count = sc_report_handler::get_count(message_type) - before;
    const auto time = report.get_time().value();
    const auto delta = sc_delta_count();
    const auto status = static_cast<int>(sc_get_status());
    if (report.get_id() != report_id || report.get_severity() != SC_ERROR ||
        std::string(report.get_msg_type()) != message_type || time != sc_time_stamp().value() || count != 1 ||
        status != static_cast<int>(expected_status))
      throw std::runtime_error("native report metadata mismatch: " + route + "/" + label);
    records.push_back("MEMBER_BOUNDARY_REPORT " + route + "|" + label + "|" + std::to_string(report.get_id()) + "|" +
                      std::to_string(static_cast<int>(report.get_severity())) + "|" + report.get_msg_type() + "|" +
                      normalized(report.get_msg()) + "|" + std::to_string(time) + "|" + std::to_string(delta) + "|" +
                      std::to_string(status) + "|" + std::to_string(sc_report_handler::get_count(message_type)));
    records.push_back("MEMBER_BOUNDARY_RECOVERED " + route + "|" + label);
    return;
  }
  throw std::runtime_error("configured native report did not throw: " + route + "/" + label);
}

/** @brief Own both member routes and their events throughout native simulation. */
struct Top : sc_module
{
  Channel channel{"member_boundary_channel"};
  sc_event first{"first"}, second{"second"};
  sc_process_handle handle;
  bool use_channel;
  using sc_module::next_trigger;

  SC_HAS_PROCESS(Top);

  /** @brief Register one thread that invokes each invalid member overload. */
  Top(sc_module_name name, bool channel_route) : sc_module(name), use_channel(channel_route)
  {
    SC_THREAD(run);
    handle = sc_get_last_created_process_handle();
  }

  /** @brief Invoke all twelve native next_trigger signatures on the selected owner. */
  template <class Owner>
  void trigger_forms(Owner& owner, const std::string& route)
  {
    sc_event_or_list or_list = first | second;
    sc_event_and_list and_list = first & second;
    sc_time duration(3, SC_NS);
    attempt(route, "none", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, [&] { owner.next_trigger(); });
    attempt(route, "event", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, [&] { owner.next_trigger(first); });
    attempt(route, "or_list", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, [&] { owner.next_trigger(or_list); });
    attempt(route, "and_list", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, [&] { owner.next_trigger(and_list); });
    attempt(route, "time", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, [&] { owner.next_trigger(duration); });
    attempt(route, "units", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, [&] { owner.next_trigger(3.0, SC_NS); });
    attempt(route, "time_event", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, [&] { owner.next_trigger(duration, first); });
    attempt(route, "units_event", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, [&] { owner.next_trigger(3.0, SC_NS, first); });
    attempt(route, "time_or", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, [&] { owner.next_trigger(duration, or_list); });
    attempt(route, "units_or", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, [&] { owner.next_trigger(3.0, SC_NS, or_list); });
    attempt(route, "time_and", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, [&] { owner.next_trigger(duration, and_list); });
    attempt(route, "units_and", 520, SC_ID_NEXT_TRIGGER_NOT_ALLOWED_,
            [&] { owner.next_trigger(3.0, SC_NS, and_list); });
  }

  /** @brief Recover every report through module/channel members and module halt paths. */
  void run()
  {
    if (use_channel)
      trigger_forms(channel, "channel");
    else
    {
      trigger_forms(*this, "module");
      attempt("module_halt", "halt", 522, SC_ID_HALT_NOT_ALLOWED_, [&] { halt(); });
      attempt("free_halt", "halt", 522, SC_ID_HALT_NOT_ALLOWED_, [&] { sc_core::halt(); });
    }
  }
};

/** @brief Run one independent owner route and assert exact completion state. */
int run_member_route(bool channel_route)
{
  const int expected_520 = 12;
  const int expected_522 = channel_route ? 0 : 2;
  sc_report_handler::set_actions(SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, SC_ERROR, SC_THROW);
  sc_report_handler::set_actions(SC_ID_HALT_NOT_ALLOWED_, SC_ERROR, SC_THROW);
  Top top("member_boundary", channel_route);
  sc_start(1, SC_NS);
  if (!top.handle.terminated() || sc_report_handler::get_count(SC_ID_NEXT_TRIGGER_NOT_ALLOWED_) != expected_520 ||
      sc_report_handler::get_count(SC_ID_HALT_NOT_ALLOWED_) != expected_522 || sc_time_stamp() != sc_time(1, SC_NS) ||
      sc_get_status() != SC_PAUSED)
    throw std::runtime_error("member boundary final state mismatch");
  records.push_back("MEMBER_BOUNDARY_FINAL " + std::string(channel_route ? "channel" : "module") + "|" +
                    std::to_string(expected_520) + "|" + std::to_string(expected_522) + "|" +
                    std::to_string(sc_time_stamp().value()) + "|" + std::to_string(sc_delta_count()) + "|" +
                    std::to_string(static_cast<int>(sc_get_status())) + "|1");
  return 0;
}

/** @brief Require one exact recoverable W547 and unchanged stop-mode state. */
int run_stop_mode()
{
  const auto original = sc_get_stop_mode();
  if (original != SC_STOP_FINISH_DELTA) throw std::runtime_error("initial stop mode mismatch");
  sc_report_handler::set_actions(SC_ID_STOP_MODE_AFTER_START_, SC_ERROR, SC_THROW);
  sc_start(1, SC_NS);
  attempt(
    "stop_mode", "set_after_start", 547, SC_ID_STOP_MODE_AFTER_START_, [] { sc_set_stop_mode(SC_STOP_IMMEDIATE); },
    SC_PAUSED);
  if (sc_report_handler::get_count(SC_ID_STOP_MODE_AFTER_START_) != 1 || sc_get_stop_mode() != original ||
      sc_time_stamp() != sc_time(1, SC_NS) || sc_get_status() != SC_PAUSED)
    throw std::runtime_error("stop-mode boundary final state mismatch");
  records.push_back("MEMBER_BOUNDARY_FINAL stop_mode|0|1|" + std::to_string(sc_time_stamp().value()) + "|" +
                    std::to_string(sc_delta_count()) + "|" + std::to_string(static_cast<int>(sc_get_status())) + "|" +
                    std::to_string(static_cast<int>(sc_get_stop_mode())));
  return 0;
}
}  // namespace

/** @brief Execute one recoverable member or stop-mode boundary scenario. */
int sc_main(int argc, char** argv)
{
  if (argc != 2) throw std::runtime_error("expected one boundary scenario");
  const std::string scenario = argv[1];
  int result = 0;
  if (scenario == "module")
    result = run_member_route(false);
  else if (scenario == "channel")
    result = run_member_route(true);
  else if (scenario == "stop_mode")
    result = run_stop_mode();
  else
    throw std::runtime_error("unknown boundary scenario: " + scenario);
  for (const auto& record : records) std::cout << record << '\n';
  std::cout << "MEMBER_BOUNDARY_OK " << scenario << " reports=" << records.size() / 2 << '\n';
  return result;
}

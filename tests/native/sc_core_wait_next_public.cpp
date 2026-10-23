/** @file sc_core_wait_next_public.cpp
 * @brief Native references for free wait and dynamic method-trigger overloads.
 * @details The model owns every event, event list, clock and callback target for
 * the full simulation. Each wait uses the explicit current simulation context
 * where that overload accepts one, and records native callback order, time and
 * delta count for comparison with the guarded Python adapters.
 */
#include <array>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <systemc>
#include <utility>
#include <vector>

/** @brief Exercise the free wait and next_trigger overload families. */
struct WaitNextPublic : sc_core::sc_module
{
  sc_core::sc_in<bool> clock_port;
  sc_core::sc_event event_a{"event_a"};
  sc_core::sc_event event_b{"event_b"};
  sc_core::sc_event static_event{"static_event"};
  sc_core::sc_event cleanup_event{"cleanup_event"};
  sc_core::sc_event_or_list or_list;
  sc_core::sc_event_and_list and_list;
  std::vector<std::string> wait_records;
  std::vector<std::string> trigger_records;
  std::vector<std::string> error_records;
  std::vector<std::string> cleanup_records;
  std::vector<bool> cleanup_unwinds;
  sc_core::sc_process_handle cleanup_handle;
  std::array<unsigned, 36> trigger_invocations{};

  SC_HAS_PROCESS(WaitNextPublic);

  /** @brief Build lists and register every native process reference.
   * @param name Native module name, borrowed for registration.
   * @param clock Clock borrowed for the module lifetime.
   */
  WaitNextPublic(sc_core::sc_module_name name, sc_core::sc_clock& clock) : sc_module(name), clock_port("clock_port")
  {
    clock_port(clock);
    or_list |= event_a;
    or_list |= event_b;
    and_list &= event_a;
    and_list &= event_b;
    SC_CTHREAD(wait_no_arguments, clock_port.pos());
    SC_CTHREAD(wait_cycles, clock_port.pos());
    SC_CTHREAD(wait_no_arguments_explicit, clock_port.pos());
    SC_CTHREAD(wait_cycles_explicit, clock_port.pos());
    SC_THREAD(wait_time_object_default);
    SC_THREAD(wait_time_object_explicit);
    SC_THREAD(wait_time_number_default);
    SC_THREAD(wait_time_number_explicit);
    SC_THREAD(wait_event_default);
    SC_THREAD(wait_event_explicit);
    SC_THREAD(wait_time_event_default);
    SC_THREAD(wait_time_event_explicit);
    SC_THREAD(wait_number_event_default);
    SC_THREAD(wait_number_event_explicit);
    SC_THREAD(wait_or_list_default);
    SC_THREAD(wait_or_list_explicit);
    SC_THREAD(wait_time_or_list_default);
    SC_THREAD(wait_time_or_list_explicit);
    SC_THREAD(wait_number_or_list_default);
    SC_THREAD(wait_number_or_list_explicit);
    SC_THREAD(wait_and_list_default);
    SC_THREAD(wait_and_list_explicit);
    SC_THREAD(wait_time_and_list_default);
    SC_THREAD(wait_time_and_list_explicit);
    SC_THREAD(wait_number_and_list_default);
    SC_THREAD(wait_number_and_list_explicit);
    SC_THREAD(wait_or_expression_default);
    SC_THREAD(wait_or_expression_explicit);
    SC_THREAD(wait_time_or_expression_default);
    SC_THREAD(wait_time_or_expression_explicit);
    SC_THREAD(wait_number_or_expression_default);
    SC_THREAD(wait_number_or_expression_explicit);
    SC_THREAD(wait_and_expression_default);
    SC_THREAD(wait_and_expression_explicit);
    SC_THREAD(wait_time_and_expression_default);
    SC_THREAD(wait_time_and_expression_explicit);
    SC_THREAD(wait_number_and_expression_default);
    SC_THREAD(wait_number_and_expression_explicit);
#define REGISTER_TRIGGER(NAME) \
  SC_METHOD(trigger_##NAME);   \
  sensitive << static_event;
    REGISTER_TRIGGER(default_context)
    REGISTER_TRIGGER(explicit_context)
    REGISTER_TRIGGER(event_default)
    REGISTER_TRIGGER(event_explicit)
    REGISTER_TRIGGER(or_list_default)
    REGISTER_TRIGGER(or_list_explicit)
    REGISTER_TRIGGER(and_list_default)
    REGISTER_TRIGGER(and_list_explicit)
    REGISTER_TRIGGER(time_object_default)
    REGISTER_TRIGGER(time_object_explicit)
    REGISTER_TRIGGER(time_number_default)
    REGISTER_TRIGGER(time_number_explicit)
    REGISTER_TRIGGER(time_event_default)
    REGISTER_TRIGGER(time_event_explicit)
    REGISTER_TRIGGER(number_event_default)
    REGISTER_TRIGGER(number_event_explicit)
    REGISTER_TRIGGER(time_or_list_default)
    REGISTER_TRIGGER(time_or_list_explicit)
    REGISTER_TRIGGER(number_or_list_default)
    REGISTER_TRIGGER(number_or_list_explicit)
    REGISTER_TRIGGER(time_and_list_default)
    REGISTER_TRIGGER(time_and_list_explicit)
    REGISTER_TRIGGER(number_and_list_default)
    REGISTER_TRIGGER(number_and_list_explicit)
    REGISTER_TRIGGER(or_expression_default)
    REGISTER_TRIGGER(or_expression_explicit)
    REGISTER_TRIGGER(and_expression_default)
    REGISTER_TRIGGER(and_expression_explicit)
    REGISTER_TRIGGER(time_or_expression_default)
    REGISTER_TRIGGER(time_or_expression_explicit)
    REGISTER_TRIGGER(number_or_expression_default)
    REGISTER_TRIGGER(number_or_expression_explicit)
    REGISTER_TRIGGER(time_and_expression_default)
    REGISTER_TRIGGER(time_and_expression_explicit)
    REGISTER_TRIGGER(number_and_expression_default)
    REGISTER_TRIGGER(number_and_expression_explicit)
#undef REGISTER_TRIGGER
    SC_THREAD(drive);
    SC_THREAD(explicit_wait_cleanup);
    cleanup_handle = sc_core::sc_get_last_created_process_handle();
    SC_THREAD(control_cleanup);
    SC_METHOD(invalid_wait_method);
    SC_THREAD(invalid_next_trigger_thread);
    SC_THREAD(stopper);
  }

  /** @brief Append one completion record with exact kernel time and delta. */
  void record(std::vector<std::string>& records, const std::string& label)
  {
    records.push_back(label + "@" + std::to_string(sc_core::sc_time_stamp().value()) + "/" +
                      std::to_string(sc_core::sc_delta_count()));
  }

  /** @brief Wait for one native clock edge using the default process context. */
  void wait_no_arguments()
  {
    sc_core::wait();
    record(wait_records, "wait_no_arguments");
  }

  /** @brief Wait for one native clock edge with an explicit simulation context. */
  void wait_no_arguments_explicit()
  {
    sc_core::wait(sc_core::sc_get_curr_simcontext());
    record(wait_records, "wait_no_arguments_explicit");
  }

  /** @brief Wait two native clock edges through the integer overload. */
  void wait_cycles()
  {
    sc_core::wait(2);
    record(wait_records, "wait_cycles");
  }

  /** @brief Wait two native clock edges with an explicit simulation context. */
  void wait_cycles_explicit()
  {
    sc_core::wait(2, sc_core::sc_get_curr_simcontext());
    record(wait_records, "wait_cycles_explicit");
  }

  /** @brief Dispatch a wait with its default or explicitly supplied context. */
  template <typename... Arguments>
  void call_wait(bool explicit_context, Arguments&&... arguments)
  {
    if (explicit_context)
      sc_core::wait(std::forward<Arguments>(arguments)..., sc_core::sc_get_curr_simcontext());
    else
      sc_core::wait(std::forward<Arguments>(arguments)...);
  }

  /** @brief Run one timed or event-based native wait and record its resumption. */
  void run_wait(unsigned form, bool explicit_context)
  {
    using namespace sc_core;
    switch (form)
    {
    case 0:
      call_wait(explicit_context, sc_time(3, SC_NS));
      break;
    case 1:
      call_wait(explicit_context, 4, SC_NS);
      break;
    case 2:
      call_wait(explicit_context, event_a);
      break;
    case 3:
      call_wait(explicit_context, sc_time(5, SC_NS), event_a);
      break;
    case 4:
      call_wait(explicit_context, 5, SC_NS, event_b);
      break;
    case 5:
      call_wait(explicit_context, or_list);
      break;
    case 6:
      call_wait(explicit_context, sc_time(5, SC_NS), or_list);
      break;
    case 7:
      call_wait(explicit_context, 5, SC_NS, or_list);
      break;
    case 8:
      call_wait(explicit_context, and_list);
      break;
    case 9:
      call_wait(explicit_context, sc_time(5, SC_NS), and_list);
      break;
    case 10:
      call_wait(explicit_context, 5, SC_NS, and_list);
      break;
    case 11:
      call_wait(explicit_context, event_a | event_b);
      break;
    case 12:
      call_wait(explicit_context, sc_time(5, SC_NS), event_a | event_b);
      break;
    case 13:
      call_wait(explicit_context, 5, SC_NS, event_a | event_b);
      break;
    case 14:
      call_wait(explicit_context, event_a & event_b);
      break;
    case 15:
      call_wait(explicit_context, sc_time(5, SC_NS), event_a & event_b);
      break;
    case 16:
      call_wait(explicit_context, 5, SC_NS, event_a & event_b);
      break;
    default:
      throw std::logic_error("invalid wait form");
    }
    static const std::array<const char*, 17> names = {"time_object",         "time_number",          "event",
                                                      "time_event",          "number_event",         "or_list",
                                                      "time_or_list",        "number_or_list",       "and_list",
                                                      "time_and_list",       "number_and_list",      "or_expression",
                                                      "time_or_expression",  "number_or_expression", "and_expression",
                                                      "time_and_expression", "number_and_expression"};
    record(wait_records, std::string("wait_") + names.at(form) + (explicit_context ? "_explicit" : "_default"));
  }

#define DEFINE_WAIT(NAME, INDEX) \
  void wait_##NAME##_default()   \
  {                              \
    run_wait(INDEX, false);      \
  }                              \
  void wait_##NAME##_explicit()  \
  {                              \
    run_wait(INDEX, true);       \
  }
  DEFINE_WAIT(time_object, 0)
  DEFINE_WAIT(time_number, 1)
  DEFINE_WAIT(event, 2)
  DEFINE_WAIT(time_event, 3)
  DEFINE_WAIT(number_event, 4)
  DEFINE_WAIT(or_list, 5)
  DEFINE_WAIT(time_or_list, 6)
  DEFINE_WAIT(number_or_list, 7)
  DEFINE_WAIT(and_list, 8)
  DEFINE_WAIT(time_and_list, 9)
  DEFINE_WAIT(number_and_list, 10)
  DEFINE_WAIT(or_expression, 11)
  DEFINE_WAIT(time_or_expression, 12)
  DEFINE_WAIT(number_or_expression, 13)
  DEFINE_WAIT(and_expression, 14)
  DEFINE_WAIT(time_and_expression, 15)
  DEFINE_WAIT(number_and_expression, 16)
#undef DEFINE_WAIT

  /** @brief Dispatch next_trigger with its default or explicit context. */
  template <typename... Arguments>
  void call_trigger(bool explicit_context, Arguments&&... arguments)
  {
    if (explicit_context)
      sc_core::next_trigger(std::forward<Arguments>(arguments)..., sc_core::sc_get_curr_simcontext());
    else
      sc_core::next_trigger(std::forward<Arguments>(arguments)...);
  }

  /** @brief Set one dynamic trigger overload and verify later static restoration. */
  void run_trigger(unsigned slot)
  {
    static const std::array<const char*, 36> names = {"default_context",
                                                      "explicit_context",
                                                      "event_default",
                                                      "event_explicit",
                                                      "or_list_default",
                                                      "or_list_explicit",
                                                      "and_list_default",
                                                      "and_list_explicit",
                                                      "time_object_default",
                                                      "time_object_explicit",
                                                      "time_number_default",
                                                      "time_number_explicit",
                                                      "time_event_default",
                                                      "time_event_explicit",
                                                      "number_event_default",
                                                      "number_event_explicit",
                                                      "time_or_list_default",
                                                      "time_or_list_explicit",
                                                      "number_or_list_default",
                                                      "number_or_list_explicit",
                                                      "time_and_list_default",
                                                      "time_and_list_explicit",
                                                      "number_and_list_default",
                                                      "number_and_list_explicit",
                                                      "or_expression_default",
                                                      "or_expression_explicit",
                                                      "and_expression_default",
                                                      "and_expression_explicit",
                                                      "time_or_expression_default",
                                                      "time_or_expression_explicit",
                                                      "number_or_expression_default",
                                                      "number_or_expression_explicit",
                                                      "time_and_expression_default",
                                                      "time_and_expression_explicit",
                                                      "number_and_expression_default",
                                                      "number_and_expression_explicit"};
    const unsigned invocation = trigger_invocations.at(slot)++;
    const bool no_arguments = slot < 2;
    const bool explicit_context = no_arguments ? slot == 1 : slot % 2 == 1;
    if (invocation == 0)
    {
      if (no_arguments)
        call_trigger(explicit_context);
      else
      {
        switch ((slot - 2) / 2)
        {
        case 0:
          call_trigger(explicit_context, event_a);
          break;
        case 1:
          call_trigger(explicit_context, or_list);
          break;
        case 2:
          call_trigger(explicit_context, and_list);
          break;
        case 3:
          call_trigger(explicit_context, sc_core::sc_time(4, sc_core::SC_NS));
          break;
        case 4:
          call_trigger(explicit_context, 5, sc_core::SC_NS);
          break;
        case 5:
          call_trigger(explicit_context, sc_core::sc_time(5, sc_core::SC_NS), event_a);
          break;
        case 6:
          call_trigger(explicit_context, 6, sc_core::SC_NS, event_b);
          break;
        case 7:
          call_trigger(explicit_context, sc_core::sc_time(6, sc_core::SC_NS), or_list);
          break;
        case 8:
          call_trigger(explicit_context, 6, sc_core::SC_NS, or_list);
          break;
        case 9:
          call_trigger(explicit_context, sc_core::sc_time(6, sc_core::SC_NS), and_list);
          break;
        case 10:
          call_trigger(explicit_context, 6, sc_core::SC_NS, and_list);
          break;
        case 11:
          call_trigger(explicit_context, event_a | event_b);
          break;
        case 12:
          call_trigger(explicit_context, event_a & event_b);
          break;
        case 13:
          call_trigger(explicit_context, sc_core::sc_time(6, sc_core::SC_NS), event_a | event_b);
          break;
        case 14:
          call_trigger(explicit_context, 6, sc_core::SC_NS, event_a | event_b);
          break;
        case 15:
          call_trigger(explicit_context, sc_core::sc_time(6, sc_core::SC_NS), event_a & event_b);
          break;
        case 16:
          call_trigger(explicit_context, 6, sc_core::SC_NS, event_a & event_b);
          break;
        default:
          throw std::logic_error("invalid next_trigger form");
        }
      }
      return;
    }
    if (invocation == 1)
    {
      record(trigger_records, std::string("trigger_") + names.at(slot) + (no_arguments ? "_static" : "_dynamic"));
      if (!no_arguments) call_trigger(explicit_context);
      return;
    }
    if (invocation == 2)
    {
      record(trigger_records, std::string("trigger_") + names.at(slot) + "_restored");
      return;
    }
    throw std::logic_error("unexpected extra next_trigger invocation");
  }

#define DEFINE_TRIGGER(NAME, INDEX) \
  void trigger_##NAME()             \
  {                                 \
    run_trigger(INDEX);             \
  }
  DEFINE_TRIGGER(default_context, 0)
  DEFINE_TRIGGER(explicit_context, 1)
  DEFINE_TRIGGER(event_default, 2)
  DEFINE_TRIGGER(event_explicit, 3)
  DEFINE_TRIGGER(or_list_default, 4)
  DEFINE_TRIGGER(or_list_explicit, 5)
  DEFINE_TRIGGER(and_list_default, 6)
  DEFINE_TRIGGER(and_list_explicit, 7)
  DEFINE_TRIGGER(time_object_default, 8)
  DEFINE_TRIGGER(time_object_explicit, 9)
  DEFINE_TRIGGER(time_number_default, 10)
  DEFINE_TRIGGER(time_number_explicit, 11)
  DEFINE_TRIGGER(time_event_default, 12)
  DEFINE_TRIGGER(time_event_explicit, 13)
  DEFINE_TRIGGER(number_event_default, 14)
  DEFINE_TRIGGER(number_event_explicit, 15)
  DEFINE_TRIGGER(time_or_list_default, 16)
  DEFINE_TRIGGER(time_or_list_explicit, 17)
  DEFINE_TRIGGER(number_or_list_default, 18)
  DEFINE_TRIGGER(number_or_list_explicit, 19)
  DEFINE_TRIGGER(time_and_list_default, 20)
  DEFINE_TRIGGER(time_and_list_explicit, 21)
  DEFINE_TRIGGER(number_and_list_default, 22)
  DEFINE_TRIGGER(number_and_list_explicit, 23)
  DEFINE_TRIGGER(or_expression_default, 24)
  DEFINE_TRIGGER(or_expression_explicit, 25)
  DEFINE_TRIGGER(and_expression_default, 26)
  DEFINE_TRIGGER(and_expression_explicit, 27)
  DEFINE_TRIGGER(time_or_expression_default, 28)
  DEFINE_TRIGGER(time_or_expression_explicit, 29)
  DEFINE_TRIGGER(number_or_expression_default, 30)
  DEFINE_TRIGGER(number_or_expression_explicit, 31)
  DEFINE_TRIGGER(time_and_expression_default, 32)
  DEFINE_TRIGGER(time_and_expression_explicit, 33)
  DEFINE_TRIGGER(number_and_expression_default, 34)
  DEFINE_TRIGGER(number_and_expression_explicit, 35)
#undef DEFINE_TRIGGER

  /** @brief Notify event waiters in order and then restore static sensitivity. */
  void drive()
  {
    using namespace sc_core;
    wait(2, SC_NS);
    event_a.notify();
    wait(1, SC_NS);
    event_b.notify();
    wait(17, SC_NS);
    static_event.notify();
  }

  /** @brief Preserve cleanup time and native unwind reason for explicit wait. */
  void explicit_wait_cleanup()
  {
    try
    {
      sc_core::wait(cleanup_event, sc_core::sc_get_curr_simcontext());
    }
    catch (const sc_core::sc_unwind_exception& unwind)
    {
      cleanup_unwinds.push_back(unwind.is_reset());
      cleanup_records.push_back(std::string(unwind.is_reset() ? "reset@" : "kill@") +
                                std::to_string(sc_core::sc_time_stamp().value()) + "/" +
                                std::to_string(sc_core::sc_delta_count()));
      throw;
    }
  }

  /** @brief Reset then kill the native process blocked on an explicit-context wait. */
  void control_cleanup()
  {
    using namespace sc_core;
    wait(1, SC_NS);
    cleanup_handle.reset();
    wait(1, SC_NS);
    cleanup_handle.kill();
  }

  /** @brief Record a caught native report from an invalid method wait. */
  void invalid_wait_method()
  {
    try
    {
      sc_core::wait(1, sc_core::SC_NS);
    }
    catch (const sc_core::sc_report& report)
    {
      record_error("method_wait", report);
      return;
    }
    throw std::logic_error("wait from an SC_METHOD unexpectedly succeeded");
  }

  /** @brief Record a caught native report from an invalid thread next_trigger. */
  void invalid_next_trigger_thread()
  {
    try
    {
      sc_core::next_trigger(sc_core::sc_get_curr_simcontext());
    }
    catch (const sc_core::sc_report& report)
    {
      record_error("thread_next_trigger", report);
      return;
    }
    throw std::logic_error("next_trigger from an SC_THREAD unexpectedly succeeded");
  }

  /** @brief Normalize and retain scalar fields from one caught report. */
  void record_error(const char* label, const sc_core::sc_report& report)
  {
    std::istringstream words(report.get_msg());
    std::ostringstream message;
    std::string word;
    bool first = true;
    while (words >> word)
    {
      if (!first) message << ' ';
      message << word;
      first = false;
    }
    std::ostringstream row;
    row << "WAIT_NEXT_ERROR_REFERENCE|" << label << '|' << report.get_id() << '|'
        << static_cast<int>(report.get_severity()) << '|' << report.get_msg_type() << '|' << message.str();
    error_records.push_back(row.str());
  }

  /** @brief Stop after static-sensitivity restoration callbacks complete. */
  void stopper()
  {
    sc_core::wait(21, sc_core::SC_NS, sc_core::sc_get_curr_simcontext());
    sc_core::sc_stop();
  }
};

/** @brief Run native overload references and verify expected resumption times. */
int sc_main(int, char**)
{
  using namespace sc_core;
  sc_report_handler::set_actions(SC_ID_WAIT_NOT_ALLOWED_, SC_ERROR, SC_THROW);
  sc_report_handler::set_actions(SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, SC_ERROR, SC_THROW);
  sc_clock clock("wait_trigger_clock", sc_time(2, SC_NS));
  WaitNextPublic model("wait_next_public", clock);
  sc_start();

  std::map<std::string, sc_dt::uint64> expected_wait_times = {{"wait_no_arguments", 2000},
                                                              {"wait_no_arguments_explicit", 2000},
                                                              {"wait_cycles", 4000},
                                                              {"wait_cycles_explicit", 4000}};
  const std::array<std::pair<const char*, sc_dt::uint64>, 17> base_wait_times = {{{"time_object", 3000},
                                                                                  {"time_number", 4000},
                                                                                  {"event", 2000},
                                                                                  {"time_event", 2000},
                                                                                  {"number_event", 3000},
                                                                                  {"or_list", 2000},
                                                                                  {"time_or_list", 2000},
                                                                                  {"number_or_list", 2000},
                                                                                  {"and_list", 3000},
                                                                                  {"time_and_list", 3000},
                                                                                  {"number_and_list", 3000},
                                                                                  {"or_expression", 2000},
                                                                                  {"time_or_expression", 2000},
                                                                                  {"number_or_expression", 2000},
                                                                                  {"and_expression", 3000},
                                                                                  {"time_and_expression", 3000},
                                                                                  {"number_and_expression", 3000}}};
  for (const auto& [name, time] : base_wait_times)
  {
    expected_wait_times.emplace(std::string("wait_") + name + "_default", time);
    expected_wait_times.emplace(std::string("wait_") + name + "_explicit", time);
  }
  if (model.wait_records.size() != expected_wait_times.size()) return 1;
  std::map<std::string, unsigned> observed_waits;
  for (const auto& record : model.wait_records)
  {
    const auto separator = record.find('@');
    const auto delta_separator = record.find('/', separator);
    if (separator == std::string::npos || delta_separator == std::string::npos) return 2;
    const auto name = record.substr(0, separator);
    const auto time = std::stoull(record.substr(separator + 1, delta_separator - separator - 1));
    const auto expected = expected_wait_times.find(name);
    if (expected == expected_wait_times.end() || expected->second != time) return 3;
    ++observed_waits[name];
  }
  if (observed_waits.size() != expected_wait_times.size()) return 3;
  if (model.trigger_records.size() != 70) return 4;
  if (model.cleanup_records.size() != 2 || model.cleanup_unwinds != std::vector<bool>{true, false} ||
      model.cleanup_records[0].find("reset@1000/") != 0 || model.cleanup_records[1].find("kill@2000/") != 0)
    return 5;
  if (model.error_records.size() != 2 || model.error_records[0].find("|method_wait|519|") == std::string::npos ||
      model.error_records[1].find("|thread_next_trigger|520|") == std::string::npos)
    return 6;

  std::cout << "WAIT_NEXT_WAIT_REFERENCE";
  for (const auto& record : model.wait_records) std::cout << " " << record;
  std::cout << "\nWAIT_NEXT_TRIGGER_REFERENCE";
  for (const auto& record : model.trigger_records) std::cout << " " << record;
  std::cout << "\nWAIT_NEXT_CLEANUP_REFERENCE";
  for (const auto& record : model.cleanup_records) std::cout << " " << record;
  std::cout << "\n";
  for (const auto& record : model.error_records) std::cout << record << "\n";
  return 0;
}

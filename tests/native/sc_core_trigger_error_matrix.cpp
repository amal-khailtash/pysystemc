/** @file sc_core_trigger_error_matrix.cpp
 * @brief Independent next_trigger restriction reports for every overload.
 * @details Events and lists remain owned until simulation ends. No Python/GIL
 * is involved; caught reports are copied into strings before their scope ends.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <utility>
#include <vector>
using namespace sc_core;

/** @brief Own error observations independently of native report storage. */
std::vector<std::string> records;

/** @brief Invoke one native overload and require a single recoverable E520.
 * @details Arguments remain borrowed through this non-blocking call. The
 * template forwards both list references and native single-use expressions.
 */
template <class... Arguments>
void attempt(const std::string& name, bool explicit_context, Arguments&&... arguments)
{
  int before = sc_report_handler::get_count(SC_ID_NEXT_TRIGGER_NOT_ALLOWED_);
  try
  {
    if (explicit_context)
      next_trigger(std::forward<Arguments>(arguments)..., sc_get_curr_simcontext());
    else
      next_trigger(std::forward<Arguments>(arguments)...);
  }
  catch (const sc_report& report)
  {
    int count = sc_report_handler::get_count(SC_ID_NEXT_TRIGGER_NOT_ALLOWED_) - before;
    if (report.get_id() != 520 || report.get_severity() != SC_ERROR || count != 1 || sc_time_stamp() != SC_ZERO_TIME ||
        sc_delta_count() != 0)
      throw std::runtime_error("native report metadata");
    std::istringstream words(report.get_msg());
    std::string word, message;
    while (words >> word)
    {
      if (!message.empty()) message += " ";
      message += word;
    }
    records.push_back("TRIGGER_ERROR " + name + (explicit_context ? "_explicit" : "_default") + "|" +
                      std::to_string(report.get_id()) + "|" + std::to_string(report.get_severity()) + "|" +
                      report.get_msg_type() + "|" + message + "|" + std::to_string(sc_time_stamp().value()) + "|" +
                      std::to_string(sc_delta_count()) + "|" + std::to_string(count));
    return;
  }
  throw std::runtime_error("invalid thread next_trigger returned");
}

/** @brief Run all forms in a native thread and assert terminal/count state. */
int sc_main(int, char**)
{
  sc_event first("first"), second("second");
  sc_event_or_list or_list = first | second;
  sc_event_and_list and_list = first & second;
  sc_time duration(1, SC_NS);
  auto previous = sc_report_handler::set_actions(SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, SC_ERROR, SC_THROW);
  auto handle = sc_spawn(
    [&]
    {
      for (bool explicit_context : {false, true})
      {
        attempt("none", explicit_context);
        attempt("event", explicit_context, first);
        attempt("or_list", explicit_context, or_list);
        attempt("and_list", explicit_context, and_list);
        attempt("time", explicit_context, duration);
        attempt("units", explicit_context, 1, SC_NS);
        attempt("time_event", explicit_context, duration, first);
        attempt("units_event", explicit_context, 1, SC_NS, first);
        attempt("time_or", explicit_context, duration, or_list);
        attempt("units_or", explicit_context, 1, SC_NS, or_list);
        attempt("time_and", explicit_context, duration, and_list);
        attempt("units_and", explicit_context, 1, SC_NS, and_list);
        attempt("or_expr", explicit_context, first | second);
        attempt("and_expr", explicit_context, first & second);
        attempt("time_or_expr", explicit_context, duration, first | second);
        attempt("units_or_expr", explicit_context, 1, SC_NS, first | second);
        attempt("time_and_expr", explicit_context, duration, first & second);
        attempt("units_and_expr", explicit_context, 1, SC_NS, first & second);
      }
    },
    "trigger_errors");
  sc_start(1, SC_NS);
  if (!handle.terminated() || records.size() != 36 ||
      sc_report_handler::get_count(SC_ID_NEXT_TRIGGER_NOT_ALLOWED_) != 36)
    throw std::runtime_error("native final state");
  sc_report_handler::set_actions(SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, SC_ERROR, previous);
  sc_stop();
  for (const auto& record : records) std::cout << record << '\n';
  return 0;
}

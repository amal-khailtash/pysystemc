/** @file sc_core_small_gaps_public.cpp
 * @brief Native reference for event copying, exception translation and kernel flags.
 */
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "sc_core_small_gaps_probe.h"
using namespace sc_core;
using namespace small_gaps_probe;
std::vector<std::tuple<int, std::string, std::string>> reports;
/** @brief Copy native warning diagnostics without retaining report pointers. */
void capture(const sc_report& report, const sc_actions&)
{
  reports.emplace_back(report.get_id(), report.get_msg_type(), report.get_msg());
}
/** @brief Own event lists, callback lifetimes and the native timed control thread. */
struct Model : sc_module
{
  SC_HAS_PROCESS(Model);
  sc_event a{"a"}, b{"b"}, c{"c"};
  sc_event_or_list extra_or{c}, or_event{(a | b) | c}, or_list{(a | b) | extra_or};
  sc_event_and_list extra_and{c}, and_event{(a & b) & c}, and_list{(a & b) & extra_and};
  sc_process_handle handle;
  std::vector<std::pair<std::string, unsigned long long>> records;
  /** @brief Register matching event and timeout processes. */
  explicit Model(sc_module_name name) : sc_module(name)
  {
    SC_THREAD(wait_or_event);
    SC_THREAD(wait_or_list);
    SC_THREAD(wait_and_event);
    SC_THREAD(wait_and_list);
    SC_THREAD(timed);
    handle = sc_get_last_created_process_handle();
    SC_THREAD(drive);
  }
  /** @brief Wait for event-appended disjunction. */
  void wait_or_event()
  {
    wait(or_event);
    records.emplace_back("or_event", sc_time_stamp().value());
  }
  /** @brief Wait for list-appended disjunction. */
  void wait_or_list()
  {
    wait(or_list);
    records.emplace_back("or_list", sc_time_stamp().value());
  }
  /** @brief Wait for event-appended conjunction. */
  void wait_and_event()
  {
    wait(and_event);
    records.emplace_back("and_event", sc_time_stamp().value());
  }
  /** @brief Wait for list-appended conjunction. */
  void wait_and_list()
  {
    wait(and_list);
    records.emplace_back("and_list", sc_time_stamp().value());
  }
  /** @brief Complete the timeout after temporarily disabling its thread. */
  void timed()
  {
    wait(4, SC_NS);
    records.emplace_back("timed", sc_time_stamp().value());
  }
  /** @brief Toggle the actual kernel flag and retain strict-policy diagnostic. */
  void drive()
  {
    sc_simcontext* context = sc_get_curr_simcontext();
    sc_process_b* process = sc_get_current_process_b();
    sc_assert(context == simcontext() && process);
    std::cout << "SMALL_CONTEXT_PROCESS 1 1\n";
    wait(1, SC_NS);
    try
    {
      handle.disable();
      sc_assert(false);
    }
    catch (const sc_report& report)
    {
      sc_assert(report.get_id() == 559);
      std::cout << "SMALL_CORNER_REPORT|" << report.get_id() << '|' << report.get_msg() << '\n';
    }
    sc_allow_process_control_corners = true;
    handle.disable();
    a.notify();
    wait(1, SC_NS);
    b.notify();
    wait(1, SC_NS);
    c.notify();
    handle.enable();
    sc_allow_process_control_corners = false;
    wait(2, SC_NS);
    sc_stop();
  }
};
/** @brief Verify report ownership, copied lists and exact scheduler outcomes. */
int sc_main(int, char**)
{
  sc_assert((std::is_same<sc_exception, std::exception>::value));
  for (int i = 0; i < 4; ++i)
  {
    std::unique_ptr<sc_report> report(exception(i));
    sc_assert(report);
    std::cout << "SMALL_EXCEPTION|" << i << '|' << report->get_id() << '|' << report->get_msg_type() << '|'
              << report->get_msg() << '\n';
  }
  auto old = sc_report_handler::set_actions(SC_ERROR, SC_DO_NOTHING);
  sc_assert(!exception(0));
  sc_report_handler::set_actions(SC_ERROR, old);
  std::cout << "SMALL_EXCEPTION_SUPPRESSED 1\n";
  sc_assert(std::string(SC_ID_REGISTER_ID_FAILED_) == "register_id failed");
  try
  {
    sc_report::register_id(-1, "invalid registration");
    sc_assert(false);
  }
  catch (const sc_report& report)
  {
    sc_assert(report.get_id() == 800);
    std::cout << "SMALL_REGISTER_REPORT|" << report.get_id() << '|' << report.get_msg_type() << '|' << report.get_msg()
              << '\n';
  }
  sc_report_handler::set_handler(capture);
  sc_event checked("checked name");
  sc_signal<bool> checked_object("checked object");
  sc_enable_name_checking = false;
  sc_signal<bool> unchecked_object("unchecked object");
  sc_enable_name_checking = true;
  sc_set_random_seed(123);
  sc_report_handler::set_handler(nullptr);
  sc_assert(reports.size() == 2);
  for (const auto& row : reports)
    std::cout << "SMALL_CONFIG_REPORT|" << std::get<0>(row) << '|' << std::get<1>(row) << '|' << std::get<2>(row)
              << '\n';
  std::cout << "SMALL_NAMES|" << checked_object.name() << '|' << unchecked_object.name() << '\n';
  {
    auto event = std::make_unique<sc_event>("temporary_owned");
    event->notify(9, SC_NS);
  }
  sc_assert(!sc_find_event("temporary_owned"));
  std::cout << "SMALL_EVENT_DESTRUCTION 1\n";
  Model model("small_gaps");
  for (const sc_event_list* list :
       {static_cast<const sc_event_list*>(&model.or_event), static_cast<const sc_event_list*>(&model.or_list),
        static_cast<const sc_event_list*>(&model.and_event), static_cast<const sc_event_list*>(&model.and_list)})
  {
    List copied(*list);
    sc_assert(copied.size() == 3 && list->size() == 3);
  }
  std::cout << "SMALL_EVENT_LIST_COPY 3 3 3 3\n";
  sc_start();
  std::sort(model.records.begin(), model.records.end());
  sc_assert(model.records.size() == 5);
  for (const auto& row : model.records) std::cout << "SMALL_EVENT_RECORD " << row.first << ' ' << row.second << '\n';
  return 0;
}

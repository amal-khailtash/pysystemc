/** @file sc_core_monitor_join_public.cpp
 * @brief Native references for process monitor and join declarations.
 * @details Native monitor probes own no Python references or callbacks. Joins
 * retain their native members through process termination; the late join
 * records SystemC's count when a terminated handle is added.
 */
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

namespace monitor_join_public_probe
{
int destroyed = 0;

/** @brief Derived native monitor used to check virtual base destruction. */
struct DerivedMonitor : sc_core::sc_process_monitor
{
  ~DerivedMonitor() override
  {
    ++destroyed;
  }
};

/** @brief Check the default callback and destruction through the public base. */
int monitor_contract()
{
  sc_core::sc_process_monitor base;
  base.signal(nullptr, sc_core::sc_process_monitor::spm_exit);
  sc_core::sc_process_monitor* derived = new DerivedMonitor;
  delete derived;
  return destroyed;
}
}  // namespace monitor_join_public_probe

/** @brief Run dynamic and clocked workers monitored by native sc_join. */
struct MonitorJoin : sc_core::sc_module
{
  sc_core::sc_join dynamic_join;
  sc_core::sc_join clocked_join;
  sc_core::sc_join late_join;
  sc_core::sc_join dynamic_control_join;
  sc_core::sc_join clocked_control_join;
  sc_core::sc_process_handle dynamic_fast_handle;
  sc_core::sc_process_handle method_handle;
  sc_core::sc_process_handle dynamic_control_handle;
  sc_core::sc_process_handle clocked_control_handle;
  sc_core::sc_clock clock;
  sc_core::sc_event method_event;
  sc_core::sc_in<bool> clock_port;
  std::vector<std::string> records;
  std::vector<std::string> cleanup_records;

  SC_HAS_PROCESS(MonitorJoin);

  explicit MonitorJoin(sc_core::sc_module_name name)
    : sc_module(name)
    , clock("join_clock", sc_core::sc_time(2, sc_core::SC_NS))
    , method_event("method_event")
    , clock_port("clock_port")
  {
    clock_port(clock);
    SC_METHOD(method_target);
    method_handle = sc_core::sc_get_last_created_process_handle();
    sensitive << method_event;
    dont_initialize();
    SC_THREAD(dynamic_fast);
    dynamic_fast_handle = sc_core::sc_get_last_created_process_handle();
    dynamic_join.add_process(dynamic_fast_handle);
    SC_THREAD(dynamic_slow);
    dynamic_join.add_process(sc_core::sc_get_last_created_process_handle());
    SC_THREAD(dynamic_waiter);
    SC_THREAD(late_monitor);
    SC_CTHREAD(clocked_fast, clock_port.pos());
    clocked_join.add_process(sc_core::sc_get_last_created_process_handle());
    SC_CTHREAD(clocked_slow, clock_port.pos());
    clocked_join.add_process(sc_core::sc_get_last_created_process_handle());
    SC_CTHREAD(clocked_waiter, clock_port.pos());
    SC_THREAD(dynamic_control_fast);
    dynamic_control_join.add_process(sc_core::sc_get_last_created_process_handle());
    SC_THREAD(dynamic_control_slow);
    dynamic_control_join.add_process(sc_core::sc_get_last_created_process_handle());
    SC_THREAD(dynamic_control_waiter);
    dynamic_control_handle = sc_core::sc_get_last_created_process_handle();
    SC_CTHREAD(clocked_control_fast, clock_port.pos());
    clocked_control_join.add_process(sc_core::sc_get_last_created_process_handle());
    SC_CTHREAD(clocked_control_slow, clock_port.pos());
    clocked_control_join.add_process(sc_core::sc_get_last_created_process_handle());
    SC_CTHREAD(clocked_control_waiter, clock_port.pos());
    clocked_control_handle = sc_core::sc_get_last_created_process_handle();
    SC_THREAD(control_waiters);
    SC_THREAD(stopper);
  }

  /** @brief Provide a valid method process handle that sc_join must reject. */
  void method_target()
  {
  }

  /** @brief End the first dynamic thread after one nanosecond. */
  void dynamic_fast()
  {
    sc_core::wait(1, sc_core::SC_NS);
  }

  /** @brief End the second dynamic thread after three nanoseconds. */
  void dynamic_slow()
  {
    sc_core::wait(3, sc_core::SC_NS);
  }

  /** @brief Wait for both dynamic workers and record the final count. */
  void dynamic_waiter()
  {
    records.emplace_back("dynamic_start=" + std::to_string(dynamic_join.process_count()) + "@" +
                         std::to_string(sc_core::sc_time_stamp().value()));
    dynamic_join.wait();
    records.emplace_back("dynamic_done=" + std::to_string(dynamic_join.process_count()) + "@" +
                         std::to_string(sc_core::sc_time_stamp().value()));
  }

  /** @brief Add a monitor after its target thread has terminated. */
  void late_monitor()
  {
    sc_core::wait(2, sc_core::SC_NS);
    late_join.add_process(dynamic_fast_handle);
    records.emplace_back("late_add=" + std::to_string(late_join.process_count()) + "@" +
                         std::to_string(sc_core::sc_time_stamp().value()));
  }

  /** @brief End the first clocked thread at the next positive edge. */
  void clocked_fast()
  {
    sc_core::wait();
  }

  /** @brief End the second clocked thread after two positive edges. */
  void clocked_slow()
  {
    sc_core::wait(2);
  }

  /** @brief Wait on clock cycles until both monitored CTHREADs exit. */
  void clocked_waiter()
  {
    records.emplace_back("clocked_start=" + std::to_string(clocked_join.process_count()) + "@" +
                         std::to_string(sc_core::sc_time_stamp().value()));
    clocked_join.wait_clocked();
    records.emplace_back("clocked_done=" + std::to_string(clocked_join.process_count()) + "@" +
                         std::to_string(sc_core::sc_time_stamp().value()));
  }

  /** @brief Keep the dynamic control join alive until six nanoseconds. */
  void dynamic_control_fast()
  {
    sc_core::wait(6, sc_core::SC_NS);
  }

  /** @brief Keep the dynamic control join alive until eight nanoseconds. */
  void dynamic_control_slow()
  {
    sc_core::wait(8, sc_core::SC_NS);
  }

  /** @brief Capture native reset and kill unwinds from a dynamic join wait. */
  void dynamic_control_waiter()
  {
    cleanup_records.emplace_back("dynamic_start@" + std::to_string(sc_core::sc_time_stamp().value()));
    try
    {
      dynamic_control_join.wait();
    }
    catch (const sc_core::sc_unwind_exception&)
    {
      cleanup_records.emplace_back("dynamic_cleanup@" + std::to_string(sc_core::sc_time_stamp().value()));
      throw;
    }
  }

  /** @brief Remain monitored for three positive clock edges. */
  void clocked_control_fast()
  {
    sc_core::wait(3);
  }

  /** @brief Remain monitored for four positive clock edges. */
  void clocked_control_slow()
  {
    sc_core::wait(4);
  }

  /** @brief Capture native reset and kill unwinds from a clocked join wait. */
  void clocked_control_waiter()
  {
    cleanup_records.emplace_back("clocked_start@" + std::to_string(sc_core::sc_time_stamp().value()));
    try
    {
      clocked_control_join.wait_clocked();
    }
    catch (const sc_core::sc_unwind_exception&)
    {
      cleanup_records.emplace_back("clocked_cleanup@" + std::to_string(sc_core::sc_time_stamp().value()));
      throw;
    }
  }

  /** @brief Reset and kill blocked join waiters, then verify worker completion. */
  void control_waiters()
  {
    sc_core::wait(500, sc_core::SC_PS);
    dynamic_control_handle.reset();
    clocked_control_handle.reset();
    sc_core::wait(500, sc_core::SC_PS);
    dynamic_control_handle.kill();
    sc_core::wait(1500, sc_core::SC_PS);
    clocked_control_handle.kill();
    sc_core::wait(8, sc_core::SC_NS);
    if (dynamic_control_join.process_count() != 0 || clocked_control_join.process_count() != 0)
      SC_REPORT_FATAL("monitor_join", "monitored workers did not finish");
    sc_core::sc_stop();
  }

  /** @brief Stop simulation after both joins and the late registration. */
  void stopper()
  {
    sc_core::wait(11, sc_core::SC_NS);
  }
};

/** @brief Verify native monitor and join results against their public contract. */
int sc_main(int, char**)
{
  const int destroyed = monitor_join_public_probe::monitor_contract();
  if (sc_core::sc_process_monitor::spm_exit != 0 || destroyed != 1) return 1;
  std::cout << "MONITOR_PUBLIC_REFERENCE " << sc_core::sc_process_monitor::spm_exit << " " << destroyed << "\n";

  MonitorJoin model("monitor_join");
  sc_core::sc_join rejection_join;
  const sc_core::sc_actions previous_actions =
    sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  auto verify_rejection = [&](const char* label, sc_core::sc_process_handle handle)
  {
    try
    {
      rejection_join.add_process(handle);
    }
    catch (const sc_core::sc_report& report)
    {
      if (report.get_id() != 561 || report.get_severity() != sc_core::SC_ERROR ||
          std::string(report.get_msg_type()) != sc_core::SC_ID_JOIN_ON_METHOD_HANDLE_ ||
          rejection_join.process_count() != 0)
        throw std::runtime_error("unexpected sc_join handle report");
      std::cout << "JOIN_REJECTION_REFERENCE\t" << label << '\t' << report.get_id() << '\t'
                << static_cast<int>(report.get_severity()) << '\t' << report.get_msg_type() << '\t' << report.get_msg()
                << '\t' << rejection_join.process_count() << '\n';
      return;
    }
    throw std::runtime_error("sc_join accepted a rejected handle");
  };
  verify_rejection("method", model.method_handle);
  verify_rejection("empty", sc_core::sc_process_handle());
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous_actions);

  sc_core::sc_start();
  std::cout << "JOIN_PUBLIC_REFERENCE";
  for (const auto& record : model.records) std::cout << " " << record;
  std::cout << "\n";
  std::cout << "JOIN_CLEANUP_REFERENCE";
  for (const auto& record : model.cleanup_records) std::cout << " " << record;
  std::cout << "\n";
  return 0;
}

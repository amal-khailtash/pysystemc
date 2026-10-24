/** @file sc_core_self_suspend.cpp
 * @brief Native self-suspension reference for resume, reset and kill.
 * Native callbacks own their captures for process lifetime. Cleanup records are
 * written before rethrowing scheduler unwind; no Python references or GIL state
 * participate in this executable.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <utility>
#include <vector>

/** @brief Own the self-suspended worker and a controller process. */
struct Model : sc_core::sc_module
{
  std::string mode;
  std::vector<std::string> records;
  std::vector<std::string> event_records;
  sc_core::sc_process_handle worker;
  sc_core::sc_process_handle controller;
  sc_core::sc_process_handle reset_watcher;
  sc_core::sc_process_handle termination_watcher;
  const sc_core::sc_event* reset_event = nullptr;
  const sc_core::sc_event* termination_event = nullptr;

  /** @brief Spawn worker before its controller to make the initial entry deterministic. */
  Model(sc_core::sc_module_name name, std::string selected_mode) : sc_module(name), mode(std::move(selected_mode))
  {
    worker = sc_core::sc_spawn([this] { run_worker(); }, "worker");
    reset_event = &worker.reset_event();
    termination_event = &worker.terminated_event();
    if (mode == "reset") reset_watcher = sc_core::sc_spawn([this] { watch_reset(); }, "reset_watcher");
    termination_watcher = sc_core::sc_spawn([this] { watch_termination(); }, "termination_watcher");
    controller = sc_core::sc_spawn([this] { control_worker(); }, "controller");
  }

  /** @brief Wait on the retained process reset event and record actual dispatch time. */
  void watch_reset()
  {
    sc_core::wait(*reset_event);
    event_records.push_back("reset " + sc_core::sc_time_stamp().to_string());
  }

  /** @brief Wait on the retained process termination event and record actual dispatch time. */
  void watch_termination()
  {
    sc_core::wait(*termination_event);
    event_records.push_back("terminated " + sc_core::sc_time_stamp().to_string());
  }

  /** @brief Suspend the current process, recording normal return or unwind cleanup. */
  void run_worker()
  {
    using namespace sc_core;
    records.push_back("entry " + sc_time_stamp().to_string());
    try
    {
      sc_get_current_process_handle().suspend();
      records.push_back("resume " + sc_time_stamp().to_string());
    }
    catch (const sc_unwind_exception& unwind)
    {
      if (unwind.is_reset() != (mode == "reset")) throw std::runtime_error("unexpected self-suspend unwind");
      records.push_back("cleanup " + sc_time_stamp().to_string());
      throw;
    }
    records.push_back("cleanup " + sc_time_stamp().to_string());
  }

  /** @brief Resume, reset then resume, or kill the suspended worker at fixed times. */
  void control_worker()
  {
    using namespace sc_core;
    wait(1, SC_NS);
    if (mode == "resume")
      worker.resume();
    else if (mode == "reset")
    {
      worker.reset();
      wait(1, SC_NS);
      worker.resume();
    }
    else
      worker.kill();
  }

  /** @brief Assert lifecycle completion and exact entries, resumes and cleanup times. */
  void verify() const
  {
    std::vector<std::string> expected;
    if (mode == "resume")
      expected = {"entry 0 s", "resume 1 ns", "cleanup 1 ns"};
    else if (mode == "reset")
      expected = {"entry 0 s", "cleanup 1 ns", "entry 1 ns", "resume 2 ns", "cleanup 2 ns"};
    else
      expected = {"entry 0 s", "cleanup 1 ns"};
    std::vector<std::string> expected_events;
    if (mode == "reset")
      expected_events = {"reset 1 ns", "terminated 2 ns"};
    else if (mode == "resume")
      expected_events = {"terminated 1 ns"};
    else
      expected_events = {"terminated 1 ns"};
    if (!worker.terminated() || !controller.terminated() || !termination_watcher.terminated() ||
        (mode == "reset" && !reset_watcher.terminated()) || records != expected || event_records != expected_events)
      throw std::runtime_error("self-suspend lifecycle mismatch");
    for (const auto& record : records) std::cout << "SELF_SUSPEND_REFERENCE " << mode << " " << record << '\n';
    for (const auto& record : event_records)
      std::cout << "SELF_SUSPEND_EVENT_REFERENCE " << mode << " " << record << '\n';
  }
};

/** @brief Run one independent self-suspend case in its own native kernel. */
int sc_main(int argc, char** argv)
{
  if (argc != 2) throw std::runtime_error("expected resume, reset or kill mode");
  const std::string mode = argv[1];
  if (mode != "resume" && mode != "reset" && mode != "kill") throw std::runtime_error("unsupported mode");
  Model model("self_suspend", mode);
  sc_core::sc_start(4, sc_core::SC_NS);
  model.verify();
  sc_core::sc_stop();
  return 0;
}

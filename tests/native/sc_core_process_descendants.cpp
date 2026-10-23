/** @file sc_core_process_descendants.cpp
 * @brief Native process-tree reference for descendant controls and teardown.
 * Native callbacks own their process state. Each kill records cleanup before
 * rethrowing scheduler unwind; no Python callback or GIL state is involved.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <utility>
#include <vector>

/** @brief Own a root thread, its dynamically spawned child and a controller. */
struct Model : sc_core::sc_module
{
  sc_core::sc_event event{"shared_event"};
  sc_core::sc_process_handle root;
  sc_core::sc_process_handle child;
  sc_core::sc_process_handle controller;
  std::vector<std::string> root_records;
  std::vector<std::string> child_records;
  std::string mode;
  sc_core::sc_descendant_inclusion_info policy;

  /** @brief Spawn root and controller with the chosen descendant policy. */
  Model(sc_core::sc_module_name name, std::string selected_mode)
    : sc_module(name)
    , mode(std::move(selected_mode))
    , policy(mode == "no_descendants" ? sc_core::SC_NO_DESCENDANTS : sc_core::SC_INCLUDE_DESCENDANTS)
  {
    root = sc_core::sc_spawn([this] { run_root(); }, "root");
    controller = sc_core::sc_spawn([this] { control_tree(); }, "controller");
  }

  /** @brief Spawn the child and record shared-event activations and cleanup. */
  void run_root()
  {
    using namespace sc_core;
    unsigned activation = 0;
    root_records.push_back("start " + sc_time_stamp().to_string());
    child = sc_spawn([this] { run_child(); }, "child");
    try
    {
      while (true)
      {
        wait(event);
        root_records.push_back("event " + std::to_string(++activation) + " " + sc_time_stamp().to_string());
      }
    }
    catch (const sc_unwind_exception&)
    {
      root_records.push_back("cleanup " + sc_time_stamp().to_string());
      throw;
    }
  }

  /** @brief Record child event activations and cleanup on its eventual kill. */
  void run_child()
  {
    using namespace sc_core;
    unsigned activation = 0;
    child_records.push_back("start " + sc_time_stamp().to_string());
    try
    {
      while (true)
      {
        wait(event);
        child_records.push_back("event " + std::to_string(++activation) + " " + sc_time_stamp().to_string());
      }
    }
    catch (const sc_unwind_exception&)
    {
      child_records.push_back("cleanup " + sc_time_stamp().to_string());
      throw;
    }
  }

  /** @brief Disable, enable, suspend, resume and kill according to descendant policy. */
  void control_tree()
  {
    using namespace sc_core;
    wait(1, SC_NS);
    root.disable(policy);
    event.notify(SC_ZERO_TIME);
    wait(1, SC_NS);
    root.enable(policy);
    event.notify(SC_ZERO_TIME);
    wait(1, SC_NS);
    root.suspend(policy);
    event.notify(SC_ZERO_TIME);
    wait(1, SC_NS);
    root.resume(policy);
    wait(1, SC_NS);
    root.kill(policy);
    if (mode == "no_descendants")
    {
      if (child.terminated()) throw std::runtime_error("NO_DESCENDANTS killed the child");
      child.kill(SC_NO_DESCENDANTS);
    }
  }

  /** @brief Verify process completion and exact root/child counts, times and cleanup. */
  void verify() const
  {
    const std::vector<std::string> expected_root{"start 0 s", "event 1 2 ns", "event 2 4 ns", "cleanup 5 ns"};
    const std::vector<std::string> expected_child =
      mode == "no_descendants"
        ? std::vector<std::string>{"start 0 s", "event 1 1 ns", "event 2 2 ns", "event 3 3 ns", "cleanup 5 ns"}
        : std::vector<std::string>{"start 0 s", "event 1 2 ns", "event 2 4 ns", "cleanup 5 ns"};
    if (!root.terminated() || !child.terminated() || !controller.terminated() || root_records != expected_root ||
        child_records != expected_child)
      throw std::runtime_error("process descendant lifecycle mismatch");
    for (const auto& record : root_records) std::cout << "DESCENDANT_REFERENCE " << mode << " root " << record << '\n';
    for (const auto& record : child_records)
      std::cout << "DESCENDANT_REFERENCE " << mode << " child " << record << '\n';
  }
};

/** @brief Run one isolated process-tree policy case in the native kernel. */
int sc_main(int argc, char** argv)
{
  if (argc != 2) throw std::runtime_error("expected descendant mode");
  const std::string mode = argv[1];
  if (mode != "no_descendants" && mode != "include_descendants") throw std::runtime_error("unsupported mode");
  Model model("process_descendants", mode);
  sc_core::sc_start(6, sc_core::SC_NS);
  model.verify();
  sc_core::sc_stop();
  return 0;
}

/** @file sc_core_start_public.cpp
 * @brief Native reference for the public SystemC sc_start overload family.
 * @details The kernel owns the scheduled events and callback module until process exit.
 */
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <utility>
#include <vector>

namespace
{

/** @brief Record and schedule callbacks for one finite start scenario. */
struct start_model : sc_core::sc_module
{
  sc_core::sc_event tick;
  sc_core::sc_event boundary;
  std::string scenario;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> callbacks;

  SC_HAS_PROCESS(start_model);

  /** @brief Register a retained native method callback for timed events.
   * @param name Native module name owned by the kernel during this interpreter.
   * @param selected Scenario name supplied to sc_main.
   */
  start_model(sc_core::sc_module_name name, std::string selected) : sc_module(name), scenario(std::move(selected))
  {
    SC_METHOD(record_first);
    sensitive << tick;
    dont_initialize();
    SC_METHOD(record_boundary);
    sensitive << boundary;
    dont_initialize();
  }

  /** @brief Save the first callback and schedule a separate boundary event.
   * @details sc_start's caller owns simulation control; this process never exits the kernel itself.
   */
  void record_first()
  {
    const auto now = sc_core::sc_time_stamp().value();
    callbacks.emplace_back(now, sc_core::sc_delta_count());
    if (scenario == "error")
    {
      throw std::runtime_error("callback thrown");
    }
    else if (now == sc_core::sc_time(1, sc_core::SC_NS).value() &&
             (scenario == "time_run" || scenario == "int_run" || scenario == "double_run" || scenario == "noarg"))
    {
      boundary.notify(sc_core::sc_time(2, sc_core::SC_NS));
    }
  }

  /** @brief Save the exact boundary callback and stop the bounded noarg scenario. */
  void record_boundary()
  {
    const auto now = sc_core::sc_time_stamp().value();
    callbacks.emplace_back(now, sc_core::sc_delta_count());
    if (scenario == "noarg")
    {
      sc_core::sc_stop();
    }
  }
};

/** @brief Test a string prefix without requiring C++20 library support. */
bool starts_with(const std::string& value, const std::string& prefix)
{
  return value.rfind(prefix, 0) == 0;
}

/** @brief Test a string suffix without requiring C++20 library support. */
bool ends_with(const std::string& value, const std::string& suffix)
{
  return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/** @brief Print normalized callback and final state rows for comparison. */
void print_records(const start_model& model)
{
  for (std::size_t index = 0; index < model.callbacks.size(); ++index)
  {
    std::cout << "START_CALLBACK\t" << index << '\t' << model.callbacks[index].first << '\t'
              << model.callbacks[index].second << '\n';
  }
  std::cout << "START_FINAL\t" << sc_core::sc_time_stamp().value() << '\t' << sc_core::sc_delta_count() << '\t'
            << static_cast<int>(sc_core::sc_get_status()) << '\n';
}

}  // namespace

/** @brief Run one native overload/policy scenario as a Python comparison reference.
 * @param argc Argument count; argv[1] selects the overload scenario.
 * @param argv Null-terminated process arguments owned by the runner.
 * @return Zero after a completed scenario; nonzero for invalid setup.
 * @details The isolated error scenario catches and prints a callback's native report fields.
 */
int sc_main(int argc, char** argv)
{
  using namespace sc_core;
  if (argc != 2) return 2;
  const std::string scenario(argv[1]);
  start_model model("start_model", scenario);
  model.tick.notify(sc_time(1, SC_NS));

  if (scenario == "noarg")
  {
    sc_start();
  }
  else if (scenario == "error")
  {
    sc_report_handler::set_actions(SC_ERROR, SC_THROW);
    try
    {
      sc_start(3.0, SC_NS, SC_RUN_TO_TIME);
    }
    catch (const sc_report& report)
    {
      if (model.callbacks.size() != 1 || model.callbacks[0].first != sc_time(1, SC_NS).value()) return 4;
      if (report.get_id() != 549 || std::string(report.get_msg_type()) != "uncaught exception" ||
          std::string(report.get_msg()).find("callback thrown") == std::string::npos)
        return 5;
      std::cout << "START_ERROR\t" << report.get_id() << '\t' << report.get_msg_type() << '\t'
                << static_cast<int>(report.get_severity()) << '\t' << model.callbacks[0].first << '\t'
                << model.callbacks[0].second << '\n';
      return 0;
    }
    return 3;
  }
  else
  {
    const auto policy = ends_with(scenario, "_run") ? SC_RUN_TO_TIME : SC_EXIT_ON_STARVATION;
    if (starts_with(scenario, "time_"))
      sc_start(sc_time(3, SC_NS), policy);
    else if (starts_with(scenario, "int_"))
      sc_start(3, SC_NS, policy);
    else if (starts_with(scenario, "double_"))
      sc_start(3.0, SC_NS, policy);
    else
      return 2;
    if (policy == SC_RUN_TO_TIME) sc_start(SC_ZERO_TIME);
  }

  print_records(model);
  return 0;
}

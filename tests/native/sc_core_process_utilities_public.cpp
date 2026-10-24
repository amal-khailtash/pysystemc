/** @file sc_core_process_utilities_public.cpp @brief Native thread utility reference. */
#include <algorithm>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include "sc_core_process_utilities_probe.h"
using namespace sc_core;
using record = std::tuple<std::string, std::string, sc_dt::uint64>;
std::vector<record> records;
/** @brief Record finally-equivalent native stack cleanup for halt/reset/kill. */
struct cleanup
{
  std::string label;
  ~cleanup()
  {
    records.emplace_back(label, "cleanup", sc_time_stamp().value());
  }
};
/** @brief Retain native halted threads and their independent controller. */
struct Model : sc_module
{
  sc_clock clock;
  sc_in<bool> clock_input;
  sc_process_handle first, second, reset_handle, kill_handle;
  SC_HAS_PROCESS(Model);
  Model(sc_module_name name) : sc_module(name), clock("clock", sc_time(2, SC_NS)), clock_input("clock_input")
  {
    clock_input(clock);
    SC_CTHREAD(default_halt, clock_input.pos());
    first = sc_get_last_created_process_handle();
    SC_CTHREAD(explicit_halt, clock_input.pos());
    second = sc_get_last_created_process_handle();
    SC_CTHREAD(reset_halt, clock_input.pos());
    reset_handle = sc_get_last_created_process_handle();
    SC_CTHREAD(kill_halt, clock_input.pos());
    kill_handle = sc_get_last_created_process_handle();
    SC_THREAD(drive);
    sc_set_stack_size(static_cast<sc_thread_handle>(first), 2 * 1024 * 1024);
    sc_set_stack_size(static_cast<sc_thread_handle>(second), 3 * 1024 * 1024);
    sc_assert(process_utilities_probe::stack_probe::size(first) == 2 * 1024 * 1024);
    sc_assert(process_utilities_probe::stack_probe::size(second) == 3 * 1024 * 1024);
  }
  /** @brief Use native halt with default or explicit context. */
  void halted(const char* label, bool explicit_context = false)
  {
    records.emplace_back(label, "start", sc_time_stamp().value());
    cleanup scope{label};
    if (explicit_context)
      sc_core::halt(sc_get_curr_simcontext());
    else
      sc_core::halt();
    sc_assert(false);
  }
  void default_halt()
  {
    halted("default");
  }
  void explicit_halt()
  {
    halted("explicit", true);
  }
  void reset_halt()
  {
    halted("reset");
  }
  void kill_halt()
  {
    halted("kill");
  }
  /** @brief Reset and kill suspended halts, then stop after terminal events. */
  void drive()
  {
    sc_assert(sc_get_status() == SC_RUNNING);
    wait(0.5, SC_NS);
    reset_handle.reset();
    wait(0.5, SC_NS);
    kill_handle.kill();
    wait(4, SC_NS);
    sc_assert(first.terminated() && second.terminated() && reset_handle.terminated() && kill_handle.terminated());
    sc_stop();
  }
};
/** @brief Compare exact native handle predicates, stack size and halt lifecycle. */
int sc_main(int argc, char**)
{
  if (argc > 1)
  {
    sc_report_handler::set_actions(SC_ERROR, SC_THROW);
    try
    {
      sc_core::halt();
    }
    catch (const sc_report& report)
    {
      std::cout << "HALT_ERROR\t" << report.get_id() << '\t' << int(report.get_severity()) << '\t'
                << report.get_msg_type() << '\n';
      return 0;
    }
    return 1;
  }
  sc_assert(sc_get_status() == SC_ELABORATION);
  Model model("utilities");
  sc_process_handle copied(model.first), empty, another_empty;
  sc_assert(model.first == copied && !(model.first != copied));
  sc_assert(model.first != model.second && !(model.first == model.second));
  sc_assert(!(empty == another_empty) && empty != another_empty);
  sc_assert(!(model.first < model.first) && !(empty < another_empty));
  sc_assert((model.first < model.second) != (model.second < model.first));
  std::cout << "PROCESS_COMPARE 1 1 1 1 1\nSTACK_CONFIG 2097152 3145728\n";
  sc_start();
  sc_assert(sc_get_status() == SC_STOPPED && sc_time_stamp() == sc_time(5, SC_NS));
  std::sort(records.begin(), records.end());
  for (const auto& r : records)
    std::cout << "HALT_RECORD " << std::get<0>(r) << ' ' << std::get<1>(r) << ' ' << std::get<2>(r) << '\n';
  sc_assert(model.first == copied && model.first != model.second);
  std::cout << "PROCESS_FINAL " << sc_time_stamp().value() << ' ' << int(sc_get_status()) << '\n';
  return 0;
}

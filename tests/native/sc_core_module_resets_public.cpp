/** @file sc_core_module_resets_public.cpp
 *  @brief Native reference for all eight sc_module reset signal overloads.
 *  @details Each independent clocked process borrows one reset source and
 *  records stack cleanup through a native RAII object when reset restarts it.
 */
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <systemc>
#include <vector>

using namespace sc_core;

/** @brief Native SystemC reference with one CTHREAD per reset overload. */
class ModuleResetReference : public sc_module
{
 public:
  sc_clock clock{"clock", sc_time(2, SC_NS)};
  sc_in<bool> clock_port{"clock_port"};
  std::array<sc_signal<bool>, 8> sources{
    sc_signal<bool>("reset_source_0", false), sc_signal<bool>("reset_source_1", false),
    sc_signal<bool>("reset_source_2", false), sc_signal<bool>("reset_source_3", false),
    sc_signal<bool>("reset_source_4", false), sc_signal<bool>("reset_source_5", false),
    sc_signal<bool>("reset_source_6", false), sc_signal<bool>("reset_source_7", false)};
  sc_in<bool> sync_input{"reset_target_0"};
  sc_inout<bool> sync_inout{"reset_target_1"};
  sc_out<bool> sync_output{"reset_target_2"};
  sc_in<bool> async_input{"reset_target_4"};
  sc_inout<bool> async_inout{"reset_target_5"};
  sc_out<bool> async_output{"reset_target_6"};
  std::array<std::vector<std::uint64_t>, 8> starts;
  std::array<std::vector<std::uint64_t>, 8> cleanups;

  SC_HAS_PROCESS(ModuleResetReference);

  /** @brief Construct sources, processes and exact reset registrations.
   *  @param name Native module hierarchy name retained through elaboration.
   */
  explicit ModuleResetReference(sc_module_name name) : sc_module(name)
  {
    clock_port(clock);
    sync_input(sources[0]);
    sync_inout(sources[1]);
    sync_output(sources[2]);
    async_input(sources[4]);
    async_inout(sources[5]);
    async_output(sources[6]);

    SC_CTHREAD(sync_input_worker, clock_port.pos());
    reset_signal_is(static_cast<const sc_in<bool>&>(sync_input), true);
    SC_CTHREAD(sync_inout_worker, clock_port.pos());
    reset_signal_is(static_cast<const sc_inout<bool>&>(sync_inout), true);
    SC_CTHREAD(sync_output_worker, clock_port.pos());
    reset_signal_is(static_cast<const sc_out<bool>&>(sync_output), true);
    SC_CTHREAD(sync_interface_worker, clock_port.pos());
    reset_signal_is(static_cast<const sc_signal_in_if<bool>&>(sources[3]), true);

    SC_CTHREAD(async_input_worker, clock_port.pos());
    async_reset_signal_is(static_cast<const sc_in<bool>&>(async_input), true);
    SC_CTHREAD(async_inout_worker, clock_port.pos());
    async_reset_signal_is(static_cast<const sc_inout<bool>&>(async_inout), true);
    SC_CTHREAD(async_output_worker, clock_port.pos());
    async_reset_signal_is(static_cast<const sc_out<bool>&>(async_output), true);
    SC_CTHREAD(async_interface_worker, clock_port.pos());
    async_reset_signal_is(static_cast<const sc_signal_in_if<bool>&>(sources[7]), true);

    SC_THREAD(reset_driver);
  }

  /** @brief Record process entry and keep an RAII cleanup marker on its stack. */
  void run_row(std::size_t index)
  {
    starts[index].push_back(sc_time_stamp().value());
    struct Cleanup
    {
      ModuleResetReference& owner;
      std::size_t index;
      ~Cleanup()
      {
        owner.cleanups[index].push_back(sc_time_stamp().value());
      }
    } cleanup{*this, index};
    while (true) wait();
  }

  /** @brief Pulse each reset source once, separated from clock edges. */
  void reset_driver()
  {
    wait(3, SC_NS);
    for (std::size_t index = 0; index < sources.size(); ++index)
    {
      sources[index].write(true);
      wait(index < 4 ? 2 : 0.5, SC_NS);
      sources[index].write(false);
      if (index + 1 < sources.size()) wait(index < 4 ? 2 : 3.5, SC_NS);
    }
  }

  void sync_input_worker()
  {
    run_row(0);
  }
  void sync_inout_worker()
  {
    run_row(1);
  }
  void sync_output_worker()
  {
    run_row(2);
  }
  void sync_interface_worker()
  {
    run_row(3);
  }
  void async_input_worker()
  {
    run_row(4);
  }
  void async_inout_worker()
  {
    run_row(5);
  }
  void async_output_worker()
  {
    run_row(6);
  }
  void async_interface_worker()
  {
    run_row(7);
  }
};

/** @brief Run one isolated kernel and print starts/cleanup for exact signatures. */
int sc_main(int, char**)
{
  ModuleResetReference model("module_resets");
  sc_start(40, SC_NS);
  constexpr std::array<const char*, 8> labels{
    "sync|sc_in|reset_signal_is|const sc_core::sc_in<bool>&,bool",
    "sync|sc_inout|reset_signal_is|const sc_core::sc_inout<bool>&,bool",
    "sync|sc_out|reset_signal_is|const sc_core::sc_out<bool>&,bool",
    "sync|sc_signal_in_if|reset_signal_is|const sc_core::sc_signal_in_if<bool>&,bool",
    "async|sc_in|async_reset_signal_is|const sc_core::sc_in<bool>&,bool",
    "async|sc_inout|async_reset_signal_is|const sc_core::sc_inout<bool>&,bool",
    "async|sc_out|async_reset_signal_is|const sc_core::sc_out<bool>&,bool",
    "async|sc_signal_in_if|async_reset_signal_is|const sc_core::sc_signal_in_if<bool>&,bool",
  };
  for (std::size_t index = 0; index < labels.size(); ++index)
  {
    if (model.starts[index].size() != 2 || model.cleanups[index].size() != 1 || model.starts[index][0] != 0 ||
        model.cleanups[index][0] <= model.starts[index][0])
      return 2;
    std::cout << "MODULE_RESET|" << labels[index] << "|starts=";
    for (std::size_t item = 0; item < model.starts[index].size(); ++item)
    {
      if (item) std::cout << ',';
      std::cout << model.starts[index][item];
    }
    std::cout << "|cleanups=";
    for (std::size_t item = 0; item < model.cleanups[index].size(); ++item)
    {
      if (item) std::cout << ',';
      std::cout << model.cleanups[index][item];
    }
    std::cout << '\n';
  }
  return 0;
}

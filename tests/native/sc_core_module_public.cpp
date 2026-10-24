/** @file sc_core_module_public.cpp
 *  @brief Native reference for public sc_module members and lifecycle behavior.
 *  @details This executable owns its kernel state and uses only native processes.
 */
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "sc_core_module_probe.h"
#include "sc_core_process_utilities_probe.h"

using namespace sc_core;
using sc_core_module_probe::ModuleProbe;

/** @brief Native reference module exercising process-host members and lifecycle. */
class ModuleReference : public ModuleProbe
{
 public:
  sc_event seed{"seed"};
  sc_event event_a{"event_a"};
  sc_event event_b{"event_b"};
  sc_event_or_list either;
  sc_event_and_list both;
  sc_signal<bool> edge_bool{"edge_bool", false};
  sc_signal<sc_dt::sc_logic> edge_logic{"edge_logic", sc_dt::SC_LOGIC_0};
  sc_clock clock{"clock", sc_time(2, SC_NS)};
  sc_in<bool> clock_port;
  sc_in<sc_dt::sc_logic> logic_port;
  ModuleProbe* child = nullptr;
  std::unique_ptr<sc_core_module_probe::SixPortBindingModule> binding_child;
  std::vector<std::string> waits;
  std::vector<std::string> triggers;
  std::vector<std::string> edges;
  std::vector<std::string> lifecycle;
  int trigger_step = 0;
  int sensitivity_hits = 0;
  bool child_found = false;

  SC_HAS_PROCESS(ModuleReference);

  /** @brief Register the native callback references and one nested module.
   *  @param name Native scope object retained by the caller through elaboration.
   */
  explicit ModuleReference(sc_module_name name) : ModuleProbe(name), clock_port("clock_port"), logic_port("logic_port")
  {
    either |= event_a;
    either |= event_b;
    both &= event_a;
    both &= event_b;
    clock_port(clock);
    logic_port(edge_logic);
    {
      sc_module_name child_name("child");
      child = new ModuleProbe(child_name);
    }
    {
      sc_module_name binding_name("bindings");
      binding_child.reset(new sc_core_module_probe::SixPortBindingModule(binding_name));
    }
    sc_core_module_probe::positional_binding_matrix(*binding_child, edge_bool, clock_port);

    SC_METHOD(dynamic_trigger);
    sensitive << seed;
    dont_initialize();
    SC_THREAD(wait_matrix);
    sensitive << seed;
    SC_METHOD(sensitivity_probe);
    sc_core_module_probe::ModuleProbe::sensitivity_field_operators(sc_get_last_created_process_handle(), seed,
                                                                   edge_bool, clock_port, clock_port.pos(), edge_bool,
                                                                   edge_logic, clock_port, logic_port);
    SC_CTHREAD(edge_matrix, clock_port.pos());
    stack_size(2 * 1024 * 1024);
    sc_assert(process_utilities_probe::stack_probe::size(sc_get_last_created_process_handle()) == 2 * 1024 * 1024);
    SC_THREAD(event_driver);
  }

  /** @brief Destroy the owned nested module after the simulation ends. */
  ~ModuleReference() override
  {
    delete child;
  }

  /** @brief Record each native phase callback in kernel order. */
  void before_end_of_elaboration() override
  {
    lifecycle.emplace_back("before");
  }
  /** @brief Record elaboration completion. */
  void end_of_elaboration() override
  {
    lifecycle.emplace_back("elaboration");
  }
  /** @brief Record simulation start. */
  void start_of_simulation() override
  {
    lifecycle.emplace_back("start");
  }
  /** @brief Record simulation completion. */
  void end_of_simulation() override
  {
    lifecycle.emplace_back("end");
  }

  /** @brief Re-arm the method with each of the twelve native trigger members. */
  void dynamic_trigger()
  {
    if (trigger_step > 0)
      triggers.emplace_back(std::to_string(trigger_step - 1) + "@" + std::to_string(sc_time_stamp().value()));
    if (trigger_step == 12) return;
    sc_core_module_probe::invoke_next_trigger_member(*this, trigger_step, event_a, either, both);
    ++trigger_step;
  }

  /** @brief Invoke all twelve non-clocked wait overloads on the native module. */
  void wait_matrix()
  {
    sc_core_module_probe::exercise_wait_members(*this, event_a, either, both,
                                                [this](int index)
                                                {
                                                  waits.emplace_back(std::to_string(index) + "@" +
                                                                     std::to_string(sc_time_stamp().value()) + "/" +
                                                                     (timed_out() ? "1" : "0"));
                                                });
  }

  /** @brief Observe native default and edge sensitivity builder registrations. */
  void sensitivity_probe()
  {
    ++sensitivity_hits;
  }

  /** @brief Exercise stack size, wait(int), edge overloads and halt in CTHREAD context. */
  void edge_matrix()
  {
    wait(1);
    waits.emplace_back("12@" + std::to_string(sc_time_stamp().value()) + "/0");
    at_posedge(edge_bool);
    edges.emplace_back("bool_pos@" + std::to_string(sc_time_stamp().value()));
    at_negedge(edge_bool);
    edges.emplace_back("bool_neg@" + std::to_string(sc_time_stamp().value()));
    at_posedge(edge_logic);
    edges.emplace_back("logic_pos@" + std::to_string(sc_time_stamp().value()));
    at_negedge(edge_logic);
    edges.emplace_back("logic_neg@" + std::to_string(sc_time_stamp().value()));
    halt();
  }

  /** @brief Produce recurring event and edge changes for bounded process tests. */
  void event_driver()
  {
    seed.notify(SC_ZERO_TIME);
    for (int tick = 0; tick < 14; ++tick)
    {
      wait(1, SC_NS);
      if (tick == 0) seed.notify(SC_ZERO_TIME);
      event_a.notify(SC_ZERO_TIME);
      event_b.notify(SC_ZERO_TIME);
      const bool value = (tick / 2 % 2) == 0;
      edge_bool.write(value);
      edge_logic.write(value ? sc_dt::SC_LOGIC_1 : sc_dt::SC_LOGIC_0);
    }
    wait(6, SC_NS);
    sc_stop();
  }

  /** @brief Capture native hierarchy name, generated names and direct child membership. */
  std::string hierarchy()
  {
    child_found = false;
    for (auto* object : get_child_objects())
      if (object == child) child_found = true;
    std::ostringstream output;
    output << name() << ' ' << child->name() << ' ' << gen_unique_name("generated", true) << ' '
           << gen_unique_name("generated", true) << ' ' << (context() != nullptr) << ' ' << child_found;
    return output.str();
  }
};

/** @brief Run an isolated native kernel and print comparable member records. */
int sc_main(int, char**)
{
  auto* legacy = new sc_core_module_probe::LegacyModule("legacy");
  const std::string legacy_name = legacy->name();
  delete legacy;
  auto* name = new sc_module_name("module_top");
  auto* model = new ModuleReference(*name);
  const std::string hierarchy = model->hierarchy();
  const auto context = model->context();
  if (context == nullptr || !model->child_found) return 1;
  model->finish_module();
  sc_start();
  delete name;

  std::cout << "MODULE_HIERARCHY " << hierarchy << '\n';
  std::cout << "MODULE_BINDINGS " << model->binding_child->bound_count() << ' ' << model->binding_child->values()
            << '\n';
  std::cout << "MODULE_SENSITIVITY " << model->sensitivity_hits << '\n';
  std::cout << "MODULE_STACK 2097152\n";
  for (const auto& item : model->waits) std::cout << "MODULE_WAIT " << item << '\n';
  for (const auto& item : model->triggers) std::cout << "MODULE_TRIGGER " << item << '\n';
  for (const auto& item : model->edges) std::cout << "MODULE_EDGE " << item << '\n';
  std::cout << "MODULE_LIFECYCLE";
  for (const auto& phase : model->lifecycle) std::cout << ' ' << phase;
  std::cout << "\nMODULE_CONTEXT 1\n";
  delete model;

  std::cout << "MODULE_DESTRUCTOR " << legacy_name << ' ' << (sc_find_object(legacy_name.c_str()) == nullptr) << ' '
            << sc_core_module_probe::LegacyModule::destructions << '\n';
  return 0;
}

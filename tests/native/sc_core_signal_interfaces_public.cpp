/** @file sc_core_signal_interfaces_public.cpp
 * @brief Independent native reference for typed signal interfaces and edges.
 */
#include <iostream>
#include <memory>

#include "sc_core_signal_interfaces_probe.h"
using namespace sc_core;
using namespace sc_dt;
using namespace signal_interfaces_probe;
/** @brief Own native interface implementations and kernel process callbacks. */
struct Model : sc_module
{
  SC_HAS_PROCESS(Model);
  Interface<bool> boolean{"boolean"};
  Interface<sc_logic> logic{"logic"};
  Interface<sc_biguint<137>> wide{"wide"};
  Channel channel{"channel"};
  int observations = 0;
  /** @brief Bind native static sensitivity to the interface-owned events. */
  explicit Model(sc_module_name name) : sc_module(name)
  {
    SC_THREAD(drive);
    SC_METHOD(observe);
    sensitive << boolean.input().value_changed_event() << logic.input().value_changed_event()
              << wide.input().value_changed_event() << channel.default_event();
    dont_initialize();
  }
  /** @brief Copy values through output and combined virtual interfaces. */
  void drive()
  {
    const char* states[] = {"1", "X", "0"};
    for (int i = 1; i <= 3; ++i)
    {
      wait(1, SC_NS);
      const sc_biguint<137> sample = (sc_biguint<137>(1) << 136) + i;
      if (i == 2)
      {
        boolean.inout().write(false);
        logic.inout().write(sc_logic(states[i - 1][0]));
        wide.inout().write(sample);
      }
      else
      {
        boolean.output().write(true);
        logic.output().write(sc_logic(states[i - 1][0]));
        wide.output().write(sample);
      }
      channel.change();
    }
    wait(1, SC_NS);
    sc_stop();
  }
  /** @brief Print equivalent callback time, delta, value and edge records. */
  void observe()
  {
    sc_assert(boolean.aliases() && logic.aliases() && wide.aliases());
    sc_assert(boolean.input().event() && logic.input().event() && wide.input().event() && channel.event());
    ++observations;
    std::cout << "SIGNAL_INTERFACE_RECORD " << sc_time_stamp().value() << ' ' << sc_delta_count() << ' '
              << boolean.input().read() << ' ' << boolean.input().posedge() << ' ' << boolean.input().negedge() << ' '
              << logic.input().read() << ' ' << logic.input().posedge() << ' ' << logic.input().negedge() << ' '
              << wide.input().read().to_string() << '\n';
  }
};
/** @brief Assert channel virtual destruction and execute the native comparison. */
int sc_main(int, char**)
{
  {
    std::unique_ptr<sc_signal_channel> owned(owned_channel());
    sc_assert(std::string(owned->kind()) == "sc_signal_channel");
    sc_assert(&owned->default_event() == &owned->value_changed_event());
  }
  sc_assert(destructions() == 1);
  std::cout << "SIGNAL_INTERFACE_DESTRUCTION 1\n";
  Model model("interfaces");
  sc_assert(model.boolean.output().get_writer_policy() == SC_DEFAULT_WRITER_POLICY);
  sc_assert(model.logic.output().get_writer_policy() == SC_DEFAULT_WRITER_POLICY);
  sc_assert(model.wide.output().get_writer_policy() == SC_DEFAULT_WRITER_POLICY);
  sc_start();
  sc_assert(model.observations == 3 && sc_time_stamp() == sc_time(4, SC_NS));
  sc_assert(!model.boolean.event() && !model.logic.event() && !model.wide.event() && !model.channel.event());
  std::cout << "SIGNAL_INTERFACE_FINAL 4000 3 1\n";
  return 0;
}

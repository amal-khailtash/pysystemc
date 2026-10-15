/**
 * @file method_sensitivity.cpp
 * @brief Native reference for static method sensitivity and initialization.
 * @details The module owns its channels and event; callbacks run on the kernel thread without Python or GIL use.
 */
#include <iostream>
#include <systemc>

using namespace sc_core;

/** @brief Own sensitivity targets until the stopped kernel exits. */
struct Model : sc_module
{
  sc_signal<bool> signal{"signal"};
  sc_in<bool> port{"port"};
  sc_event event{"event"};
  SC_HAS_PROCESS(Model);

  /** @brief Register native methods with the same sensitivity as the Python model. */
  Model(sc_module_name name) : sc_module(name)
  {
    port(signal);
    SC_METHOD(interface_tick);
    sensitive << signal;
    dont_initialize();
    SC_METHOD(port_tick);
    sensitive << port;
    dont_initialize();
    SC_METHOD(edge_tick);
    sensitive << port.pos();
    dont_initialize();
    SC_METHOD(event_tick);
    sensitive << event;
    dont_initialize();
    SC_METHOD(initial_tick);
  }

  /** @brief Emit an observable invocation with native time and value. */
  void record(const char* kind)
  {
    std::cout << "RECORD " << kind << "|" << sc_time_stamp() << "|" << signal.read() << '\n';
  }
  /** @brief Observe the interface default event. */
  void interface_tick() { record("interface"); }
  /** @brief Observe the bound port default event. */
  void port_tick() { record("port"); }
  /** @brief Observe the positive edge finder. */
  void edge_tick() { record("edge"); }
  /** @brief Observe the explicit event. */
  void event_tick() { record("event"); }
  /** @brief Observe default method initialization. */
  void initial_tick() { record("initial"); }
};

/** @brief Drive the reference through the same bounded starts as Python. */
int sc_main(int, char**)
{
  Model model("sensitivity");
  sc_start(1, SC_NS);
  model.signal.write(true);
  model.event.notify(1, SC_NS);
  sc_start(2, SC_NS);
  model.signal.write(false);
  sc_start(1, SC_NS);
  sc_stop();
  return 0;
}

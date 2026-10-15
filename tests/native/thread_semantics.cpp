/**
 * @file thread_semantics.cpp
 * @brief Reference pthread waits, clocked threads and pause/resume.
 * @details Module-owned events and ports outlive processes; no Python references or GIL are involved.
 */
#include <iostream>
#include <systemc>
using namespace sc_core;

/** @brief Own native wait targets and finite processes. */
struct Model : sc_module
{
  sc_event first{"first"}, second{"second"};
  sc_clock clock{"clock", sc_time(2, SC_NS)};
  sc_in<bool> port{"clock_port"};
  SC_HAS_PROCESS(Model);
  /** @brief Register equivalent native thread and clocked-thread processes. */
  Model(sc_module_name name) : sc_module(name)
  {
    port(clock);
    SC_THREAD(events);
    SC_THREAD(drive);
    SC_CTHREAD(clocked, port.pos());
  }
  /** @brief Record native wake time. */
  void record(const char* label) { std::cout << "THREAD_RECORD " << label << "|" << sc_time_stamp() << '\n'; }
  /** @brief Wait for OR, AND and timeout, then pause. */
  void events()
  {
    wait(first | second);
    record("or");
    wait(first & second);
    record("and");
    wait(sc_time(2, SC_NS), first);
    record("timeout");
    sc_pause();
  }
  /** @brief Notify events at distinct times. */
  void drive()
  {
    wait(1, SC_NS);
    first.notify();
    wait(1, SC_NS);
    first.notify();
    wait(1, SC_NS);
    second.notify();
  }
  /** @brief Wait one and then two clock cycles. */
  void clocked()
  {
    record("clock_start");
    wait();
    record("clock_one");
    wait(2);
    record("clock_three");
  }
};
/** @brief Start, resume and stop the native simulation. */
int sc_main(int, char**)
{
  Model model("threads");
  sc_start(10, SC_NS);
  if (sc_time_stamp() != sc_time(5, SC_NS) || sc_get_status() != SC_PAUSED) return 1;
  sc_start(2, SC_NS);
  if (sc_time_stamp() != sc_time(7, SC_NS)) return 2;
  sc_stop();
  return 0;
}

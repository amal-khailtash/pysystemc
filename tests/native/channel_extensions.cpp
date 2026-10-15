/**
 * @file
 * @brief Native reference for queued notifications and resolved contention.
 * @details Module owns channels until simulation ends. No Python references or GIL state are used.
 */
#include <systemc>
#include <vector>

struct Model : sc_core::sc_module
{
  sc_core::sc_event_queue queue;
  sc_core::sc_signal_resolved resolved;
  std::vector<sc_core::sc_time> seen;
  SC_HAS_PROCESS(Model);
  explicit Model(sc_core::sc_module_name name) : sc_module(name), queue("queue"), resolved("resolved")
  {
    SC_METHOD(observe);
    sensitive << queue.default_event();
    dont_initialize();
    SC_THREAD(zero);
    SC_THREAD(one);
  }
  /** @brief Record one queued notification without suspending. */
  void observe() { seen.push_back(sc_core::sc_time_stamp()); }
  /** @brief Drive zero from an independent native process. */
  void zero() { resolved.write(sc_dt::SC_LOGIC_0); }
  /** @brief Drive one from an independent native process. */
  void one() { resolved.write(sc_dt::SC_LOGIC_1); }
};

int sc_main(int, char**)
{
  using namespace sc_core;
  Model model("extensions");
  model.queue.notify(1, SC_NS);
  model.queue.notify(2, SC_NS);
  model.queue.notify(2, SC_NS);
  sc_start(3, SC_NS);
  sc_assert(model.seen == std::vector<sc_time>({sc_time(1, SC_NS), sc_time(2, SC_NS), sc_time(2, SC_NS)}));
  sc_assert(model.resolved.read() == sc_dt::SC_LOGIC_X);
  model.queue.notify(1, SC_NS);
  model.queue.cancel_all();
  sc_start(2, SC_NS);
  sc_assert(model.seen.size() == 3);
  sc_stop();
  std::cout << "CHANNEL_EXTENSIONS_OK\n";
  return 0;
}

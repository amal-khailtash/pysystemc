/** @file sc_core_event_notification.cpp
 * @brief Native notification priority, cancellation and triggered reference.
 * The module owns the event for the complete process lifetime.
 */
#include <iostream>
#include <systemc>
#include <vector>
/** @brief Own observer/driver and retained event storage. */
struct Events : sc_core::sc_module
{
  SC_HAS_PROCESS(Events);
  sc_core::sc_event event;
  std::vector<sc_dt::uint64> records;
  bool valid = true;
  int errors = 0;
  /** @brief Register native processes before starting simulation. */
  explicit Events(sc_core::sc_module_name name) : sc_module(name), event("event")
  {
    SC_METHOD(observer);
    sensitive << event;
    dont_initialize();
    SC_THREAD(drive);
  }
  /** @brief Verify event trigger state while its observer executes. */
  void observer()
  {
    if (!event.triggered()) valid = false;
    records.push_back(sc_core::sc_time_stamp().value());
  }
  /** @brief Exercise native notification overloads, priorities and cancellation. */
  void drive()
  {
    using namespace sc_core;
    wait(1, SC_NS);
    event.notify();
    wait(1, SC_NS);
    event.notify(sc_time(1, SC_NS));
    wait(2, SC_NS);
    event.notify(1, SC_NS);
    wait(2, SC_NS);
    event.notify(5, SC_NS);
    event.notify(2, SC_NS);
    event.notify(3, SC_NS);
    wait(3, SC_NS);
    event.notify(1, SC_NS);
    event.cancel();
    event.cancel();
    wait(2, SC_NS);
    if (event.triggered()) valid = false;
    event.notify(SC_ZERO_TIME);
    wait(1, SC_NS);
    if (event.triggered()) valid = false;
    for (int mode = 0; mode < 3; ++mode)
    {
      auto notify = [&]()
      {
        if (mode == 0)
          event.notify_delayed();
        else if (mode == 1)
          event.notify_delayed(sc_time(1, SC_NS));
        else
          event.notify_delayed(1, SC_NS);
      };
      notify();
      try
      {
        notify();
        valid = false;
      }
      catch (const sc_report& report)
      {
        if (std::string(report.get_msg_type()) != SC_ID_NOTIFY_DELAYED_) valid = false;
        ++errors;
      }
      wait(2, SC_NS);
    }
    sc_stop();
  }
};
/** @brief Run and check every callback time rather than only kernel exit. */
int sc_main(int, char**)
{
  Events model("events");
  sc_core::sc_start();
  if (model.errors != 3 || !model.valid ||
      model.records != std::vector<sc_dt::uint64>{1000, 3000, 5000, 8000, 11000, 12000, 15000, 17000})
    return 1;
  std::cout << "EVENT_NOTIFY_REFERENCE";
  for (auto time : model.records) std::cout << " " << time;
  std::cout << "\n";
  std::cout << "EVENT_DELAYED_REFERENCE " << model.errors << "\n";
  return 0;
}

/** @file sc_core_event_destruction.cpp
 * @brief Dynamic thread/method timeout recovery after native event destruction.
 * The module owns the heap event until the driver deletes it. Dynamic waiters
 * retain timeout registrations; no deleted event pointer is accessed afterward.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <vector>
/** @brief Own native waiters and explicit event destruction. */
struct Model : sc_core::sc_module
{
  SC_HAS_PROCESS(Model);
  sc_core::sc_event* event;
  bool started = false;
  bool valid = true;
  std::vector<std::string> records;
  /** @brief Register callbacks while the event is alive. */
  explicit Model(sc_core::sc_module_name name) : sc_module(name), event(new sc_core::sc_event("target"))
  {
    SC_THREAD(thread);
    SC_METHOD(method);
    SC_THREAD(driver);
  }
  /** @brief Release the event if simulation terminates before the driver. */
  ~Model()
  {
    delete event;
  }
  /** @brief Record thread timeout after dynamic event detachment. */
  void thread()
  {
    sc_core::wait(sc_core::sc_time(3, sc_core::SC_NS), *event);
    records.push_back("thread:" + std::to_string(sc_core::sc_time_stamp().value()));
  }
  /** @brief Register and observe a method timeout. */
  void method()
  {
    if (!started)
    {
      started = true;
      sc_core::next_trigger(sc_core::sc_time(4, sc_core::SC_NS), *event);
    }
    else
      records.push_back("method:" + std::to_string(sc_core::sc_time_stamp().value()));
  }
  /** @brief Delete the event with pending delivery and two dynamic waiters. */
  void driver()
  {
    sc_core::wait(1, sc_core::SC_NS);
    event->notify(1, sc_core::SC_NS);
    delete event;
    event = nullptr;
    if (sc_core::sc_find_event("destroy.target")) valid = false;
    sc_core::wait(4, sc_core::SC_NS);
    sc_core::sc_stop();
  }
};
/** @brief Assert exact recovery records before reporting reference output. */
int sc_main(int, char**)
{
  Model model("destroy");
  sc_core::sc_start();
  if (!model.valid || model.records != std::vector<std::string>{"thread:3000", "method:4000"}) return 1;
  std::cout << "EVENT_DESTRUCTION_REFERENCE";
  for (const auto& record : model.records) std::cout << " " << record;
  std::cout << "\n";
  return 0;
}

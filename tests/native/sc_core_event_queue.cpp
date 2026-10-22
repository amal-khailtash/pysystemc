/** @file sc_core_event_queue.cpp
 * @brief Event queue delivery/cancellation and virtual interface reference.
 * Module owns observer; queue/default event outlive every callback.
 */
#include <iostream>
#include <systemc>
#include <type_traits>
#include <utility>
#include <vector>
/** @brief Observe queued notifications and retain exact time/delta records. */
struct Model : sc_core::sc_module
{
  SC_HAS_PROCESS(Model);
  sc_core::sc_event_queue queue;
  std::vector<std::pair<sc_dt::uint64, sc_dt::uint64>> records;
  /** @brief Register sensitivity to borrowed queue default event. */
  explicit Model(sc_core::sc_module_name name) : sc_module(name), queue("queue")
  {
    SC_METHOD(observe);
    sensitive << queue.default_event();
    dont_initialize();
  }
  /** @brief Capture every queued delivery. */
  void observe()
  {
    records.emplace_back(sc_core::sc_time_stamp().value(), sc_core::sc_delta_count());
  }
};
/** @brief Assert equal-time delta separation, cancellation and recovery. */
int sc_main(int, char**)
{
  using namespace sc_core;
  Model model("queues");
  sc_event_queue_if& interface = model.queue;
  static_assert(std::is_same<sc_event_queue::SC_CURRENT_USER_MODULE, sc_event_queue>::value);
  if (std::string(model.queue.kind()) != "sc_event_queue") return 1;
  interface.notify(sc_time(1, SC_NS));
  interface.notify(1, SC_NS);
  model.queue.notify(2, SC_NS);
  sc_start(3, SC_NS);
  if (model.records.size() != 3 || model.records[0].first != 1000 || model.records[1].first != 1000 ||
      model.records[2].first != 2000 || model.records[0].second >= model.records[1].second)
    return 2;
  interface.notify(1, SC_NS);
  interface.cancel_all();
  model.queue.notify(sc_time(2, SC_NS));
  model.queue.cancel_all();
  model.queue.cancel_all();
  sc_start(3, SC_NS);
  if (model.records.size() != 3) return 3;
  model.queue.notify(sc_time(1, SC_NS));
  sc_start(2, SC_NS);
  sc_stop();
  if (model.records.size() != 4 || model.records.back().first != 7000) return 4;
  for (auto record : model.records)
    std::cout << "EVENT_QUEUE_REFERENCE " << record.first << " " << record.second << "\n";
  return 0;
}

/** @file sc_core_notify_control.cpp
 * @brief Native free notify, initialization and pause/resume reference.
 * @details Events/module survive callbacks; no Python ownership or GIL requirements.
 */
#include <systemc>
#include <vector>
std::vector<std::pair<sc_dt::uint64, sc_dt::uint64>> records;
std::vector<sc_dt::uint64> thread_records;
/** @brief Observe notification times and pause at 2ns. */
struct model : sc_core::sc_module
{
  sc_core::sc_event event{"trigger"};
  SC_HAS_PROCESS(model);
  model(sc_core::sc_module_name name) : sc_module(name)
  {
    SC_METHOD(observer);
    sensitive << event;
    dont_initialize();
    SC_THREAD(initializer);
  }
  void observer()
  {
    records.emplace_back(sc_core::sc_time_stamp().value(), sc_core::sc_delta_count());
    if (sc_core::sc_time_stamp() == sc_core::sc_time(2, sc_core::SC_NS)) sc_core::sc_pause();
  }
  /** @brief Execute during initialize and resume at 1ns on the native pthread. */
  void initializer()
  {
    thread_records.push_back(sc_core::sc_time_stamp().value());
    sc_core::wait(1, sc_core::SC_NS);
    thread_records.push_back(sc_core::sc_time_stamp().value());
  }
};
/** @brief Drive immediate, delta, time and scalar-time notification overloads. */
int sc_main(int, char**)
{
  using namespace sc_core;
  model top("control");
  sc_initialize();
  if (thread_records != std::vector<sc_dt::uint64>{0}) return 4;
  sc_start(SC_ZERO_TIME);
  notify(top.event);
  sc_start(SC_ZERO_TIME);
  notify(SC_ZERO_TIME, top.event);
  sc_start(SC_ZERO_TIME);
  sc_start(SC_ZERO_TIME);
  notify(2, SC_NS, top.event);
  sc_start(10, SC_NS);
  if (sc_time_stamp() != sc_time(2, SC_NS)) return 2;
  notify(sc_time(1, SC_NS), top.event);
  sc_start(2, SC_NS, SC_RUN_TO_TIME);
  sc_stop();
  if (records.size() != 4) return 3;
  if (thread_records != std::vector<sc_dt::uint64>{0, 1000}) return 5;
  for (size_t i = 0; i < records.size(); ++i)
    std::cout << "NOTIFY_CONTROL_REFERENCE " << i << " " << records[i].first << " " << records[i].second << "\n";
  std::cout << "NOTIFY_FINAL_REFERENCE " << sc_time_stamp().value() << " " << int(sc_get_status()) << "\n";
  return 0;
}

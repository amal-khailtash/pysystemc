/** @file reset_teardown.cpp
 * @brief Independent native reference for blocking unwind then channel deletion.
 * @details Channel and port are heap-owned by sc_main. Destroy the port first,
 * after all process handles terminate. No Python callbacks or GIL operations
 * occur. Cleanup guards record native stack unwinding; no kernel reset occurs.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

std::vector<std::string> records;
/** @brief Record exact native cleanup times without retaining kernel pointers. */
void record(const std::string& owner, const std::string& action)
{
  records.push_back(owner + " " + action + " " + sc_core::sc_time_stamp().to_string());
}
/** @brief Record mandatory scope exit during reset and kill unwind. */
struct cleanup
{
  std::string owner;
  std::string action;
  ~cleanup()
  {
    record(owner, action);
  }
};
/** @brief Native blocking FIFO adapter with destructor evidence. */
struct channel : sc_core::sc_module, sc_core::sc_fifo_blocking_in_if<int>
{
  explicit channel(sc_core::sc_module_name name) : sc_module(name)
  {
  }
  /** @brief Suspend before assigning caller output; native scheduler unwind propagates. */
  int read() override
  {
    record("fifo", "callback");
    cleanup guard{"fifo", "callback-cleanup"};
    sc_core::wait(100, sc_core::SC_NS);
    throw std::runtime_error("interrupted callback returned");
  }
  /** @brief Assign only after a successful blocking return. */
  void read(int& value) override
  {
    value = read();
  }
  /** @brief Record native destruction after borrowers and processes finish. */
  ~channel() override
  {
    record("all", "deleted");
  }
};
/** @brief Own one native borrower until after process termination. */
struct model : sc_core::sc_module
{
  sc_core::sc_port<sc_core::sc_fifo_blocking_in_if<int>> port;
  explicit model(sc_core::sc_module_name name) : sc_module(name), port("port")
  {
  }
};
/** @brief Compare native callback/worker cleanup, termination and registry removal. */
int sc_main(int, char**)
{
  auto* adapter = new channel("adapter");
  auto* borrower = new model("borrower");
  borrower->port.bind(*adapter);
  auto worker = sc_core::sc_spawn(
    [&]
    {
      record("fifo", "worker");
      cleanup guard{"fifo", "worker-cleanup"};
      borrower->port->read();
      throw std::runtime_error("interrupted worker returned");
    });
  auto control = sc_core::sc_spawn(
    [&]
    {
      sc_core::wait(2, sc_core::SC_NS);
      worker.reset();
      sc_core::wait(1, sc_core::SC_NS);
      worker.kill();
    });
  sc_core::sc_start(4, sc_core::SC_NS);
  if (!worker.terminated() || !control.terminated()) throw std::runtime_error("termination");
  sc_core::sc_stop();
  delete borrower;
  delete adapter;
  if (sc_core::sc_find_object("adapter")) throw std::runtime_error("registry");
  const std::vector<std::string> expected = {
    "fifo worker 0 s",  "fifo callback 0 s",  "fifo callback-cleanup 2 ns", "fifo worker-cleanup 2 ns",
    "fifo worker 2 ns", "fifo callback 2 ns", "fifo callback-cleanup 3 ns", "fifo worker-cleanup 3 ns",
    "all deleted 4 ns"};
  if (records != expected) throw std::runtime_error("ordering");
  for (const auto& entry : records) std::cout << "TEARDOWN_REFERENCE " << entry << '\n';
  return 0;
}

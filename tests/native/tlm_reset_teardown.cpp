/** @file tlm_reset_teardown.cpp
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
#include <tlm>
#include <vector>

std::vector<std::string> records;
std::string path;
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
/** @brief Native blocking adapter with borrower-first destruction evidence. */
template <class Interface>
struct channel : sc_core::sc_module, Interface
{
  explicit channel(sc_core::sc_module_name name) : sc_module(name)
  {
  }
  /** @brief Suspend a native TLM value callback until scheduler unwind. */
  int sample() const
  {
    record(path, "callback");
    cleanup guard{path, "callback-cleanup"};
    sc_core::wait(100, sc_core::SC_NS);
    throw std::runtime_error("interrupted callback returned");
  }
  /** @brief Return only after successful get; scheduler unwind propagates. */
  int get(tlm::tlm_tag<int>* = nullptr) override
  {
    return sample();
  }
  /** @brief Preserve caller output until a successful get. */
  void get(int& value) override
  {
    value = sample();
  }
  /** @brief Return only after successful peek; scheduler unwind propagates. */
  int peek(tlm::tlm_tag<int>* = nullptr) const override
  {
    return sample();
  }
  /** @brief Preserve caller output until a successful peek. */
  void peek(int& value) const override
  {
    value = sample();
  }
  /** @brief Complete the compound put contract; this probe blocks on get/peek. */
  void put(const int&)
  {
    throw std::runtime_error("unexpected compound put");
  }
  /** @brief Record native destruction after borrowers and processes finish. */
  ~channel() override
  {
    record("all", "deleted");
  }
};
/** @brief Own one native borrower until after process termination. */
template <class Interface>
struct model : sc_core::sc_module
{
  sc_core::sc_port<Interface> port;
  explicit model(sc_core::sc_module_name name) : sc_module(name), port("port")
  {
  }
};
/** @brief Compare native callback/worker cleanup, termination and registry removal. */
template <class Interface>
int run_reference()
{
  auto* adapter = new channel<Interface>("adapter");
  auto* borrower = new model<Interface>("borrower");
  borrower->port.bind(*adapter);
  auto worker = sc_core::sc_spawn(
    [&]
    {
      record(path, "worker");
      cleanup guard{path, "worker-cleanup"};
      if (path == "get" || path == "master")
        borrower->port->get();
      else
        borrower->port->peek();
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
  std::vector<std::string> expected = {
    "fifo worker 0 s",  "fifo callback 0 s",  "fifo callback-cleanup 2 ns", "fifo worker-cleanup 2 ns",
    "fifo worker 2 ns", "fifo callback 2 ns", "fifo callback-cleanup 3 ns", "fifo worker-cleanup 3 ns",
    "all deleted 4 ns"};
  for (auto& entry : expected)
    if (entry.rfind("fifo ", 0) == 0) entry.replace(0, 4, path);
  if (records != expected) throw std::runtime_error("ordering");
  for (const auto& entry : records) std::cout << "TEARDOWN_REFERENCE " << entry << '\n';
  return 0;
}

/** @brief Select base or native compound declarations without Python adapters. */
int sc_main(int argc, char** argv)
{
  if (argc != 2) return 2;
  path = argv[1];
  if (path == "get") return run_reference<tlm::tlm_blocking_get_peek_if<int>>();
  if (path == "master") return run_reference<tlm::tlm_blocking_master_if<int, int>>();
  if (path == "slave") return run_reference<tlm::tlm_blocking_slave_if<int, int>>();
  return 2;
}

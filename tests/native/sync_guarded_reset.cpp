/** @file sync_guarded_reset.cpp
 * @brief Independent native synchronization reset/kill and resource reference.
 * @details Module owns channels until all processes terminate. No Python/GIL
 * operations occur; scheduler unwind is rethrown after recording cleanup.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

/** @brief Own native mutex/semaphore and their borrowed ports. */
struct model : sc_core::sc_module
{
  sc_core::sc_mutex mutex{"mutex"};
  sc_core::sc_semaphore semaphore{"semaphore", 0};
  sc_core::sc_port<sc_core::sc_mutex_if> mutex_port{"mutex_port"};
  sc_core::sc_port<sc_core::sc_semaphore_if> semaphore_port{"semaphore_port"};
  std::vector<std::string> records;
  sc_core::sc_process_handle worker, controller, owner;
  bool use_mutex, use_port;

  /** @brief Bind interfaces and start resource owner before contender. */
  model(sc_core::sc_module_name name, bool kind, bool access) : sc_module(name), use_mutex(kind), use_port(access)
  {
    mutex_port.bind(mutex);
    semaphore_port.bind(semaphore);
    if (use_mutex)
      owner = sc_core::sc_spawn(
        [this]
        {
          if (mutex.lock() != 0) throw std::runtime_error("holder lock");
          sc_core::wait(4, sc_core::SC_NS);
          if (mutex.unlock() != 0) throw std::runtime_error("holder unlock");
        });
    worker = sc_core::sc_spawn([this] { consume(); });
    controller = sc_core::sc_spawn(
      [this]
      {
        sc_core::wait(2, sc_core::SC_NS);
        worker.reset();
        sc_core::wait(1, sc_core::SC_NS);
        worker.kill();
      });
  }
  /** @brief Record cleanup in the native unwind frame, then rethrow. */
  void consume()
  {
    records.push_back("start " + sc_core::sc_time_stamp().to_string());
    try
    {
      if (use_mutex)
        (use_port ? *mutex_port.operator->() : static_cast<sc_core::sc_mutex_if&>(mutex)).lock();
      else
        (use_port ? *semaphore_port.operator->() : static_cast<sc_core::sc_semaphore_if&>(semaphore)).wait();
    }
    catch (const sc_core::sc_unwind_exception& unwind)
    {
      if (unwind.is_reset() != (records.size() == 1)) throw std::runtime_error("unwind kind");
      records.push_back("cleanup " + sc_core::sc_time_stamp().to_string());
      throw;
    }
    throw std::runtime_error("blocked operation returned");
  }
  /** @brief Verify resource state remains intact after both interruptions. */
  void verify()
  {
    if (!worker.terminated() || !controller.terminated()) throw std::runtime_error("termination");
    if (use_mutex)
    {
      if (!owner.terminated() || mutex.trylock() != 0 || mutex.unlock() != 0) throw std::runtime_error("mutex state");
    }
    else if (semaphore.get_value() != 0 || semaphore.post() != 0 || semaphore.trywait() != 0)
      throw std::runtime_error("semaphore state");
    if (records != std::vector<std::string>{"start 0 s", "cleanup 2 ns", "start 2 ns", "cleanup 3 ns"})
      throw std::runtime_error("cleanup order");
    for (const auto& record : records) std::cout << "SYNC_RESET_REFERENCE " << record << '\n';
  }
};
/** @brief Execute one independent native case with the Python test's arguments. */
int sc_main(int argc, char** argv)
{
  if (argc != 3) throw std::runtime_error("expected kind and access");
  model top("top", std::string(argv[1]) == "mutex", std::string(argv[2]) == "port");
  sc_core::sc_start(5, sc_core::SC_NS);
  top.verify();
  sc_core::sc_stop();
  return 0;
}

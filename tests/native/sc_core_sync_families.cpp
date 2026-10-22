/** @file sc_core_sync_families.cpp
 * @brief Native mutex/semaphore public ownership and timing reference.
 * Channels and borrowed interfaces remain alive for all waiting processes.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <vector>
/** @brief Own synchronization channels and native process callbacks. */
struct Model : sc_core::sc_module
{
  SC_HAS_PROCESS(Model);
  sc_core::sc_mutex mutex;
  sc_core::sc_semaphore semaphore;
  bool valid = true;
  std::vector<std::string> records;
  /** @brief Register holder and contender in deterministic creation order. */
  explicit Model(sc_core::sc_module_name name) : sc_module(name), mutex("mutex"), semaphore("semaphore", 0)
  {
    SC_THREAD(holder);
    SC_THREAD(contender);
  }
  /** @brief Verify owner re-entry, release and semaphore posting. */
  void holder()
  {
    using namespace sc_core;
    if (mutex.lock() != 0 || mutex.trylock() != 0 || mutex.lock() != 0) valid = false;
    wait(2, SC_NS);
    if (mutex.unlock() != 0) valid = false;
    wait(1, SC_NS);
    if (semaphore.post() != 0 || semaphore.get_value() != 1) valid = false;
  }
  /** @brief Verify foreign-owner failure and blocking acquisition via interfaces. */
  void contender()
  {
    using namespace sc_core;
    sc_mutex_if& mi = mutex;
    sc_semaphore_if& si = semaphore;
    wait(1, SC_NS);
    if (mi.trylock() != -1 || mi.unlock() != -1 || si.trywait() != -1 || si.get_value() != 0) valid = false;
    if (mi.lock() != 0) valid = false;
    records.push_back("mutex " + std::to_string(sc_time_stamp().value()));
    if (mi.unlock() != 0 || si.wait() != 0 || si.get_value() != 0) valid = false;
    records.push_back("semaphore " + std::to_string(sc_time_stamp().value()));
    if (si.post() != 0 || si.trywait() != 0 || si.get_value() != 0) valid = false;
    sc_stop();
  }
};
/** @brief Check exact native return values and resume timestamps. */
int sc_main(int, char**)
{
  sc_core::sc_mutex generated_mutex;
  sc_core::sc_semaphore generated_semaphore(2);
  if (std::string(generated_mutex.basename()).find("mutex_") != 0 ||
      std::string(generated_semaphore.basename()).find("semaphore_") != 0 || generated_semaphore.get_value() != 2)
    return 3;
  Model model("sync");
  if (std::string(model.mutex.kind()) != "sc_mutex" || std::string(model.semaphore.kind()) != "sc_semaphore") return 1;
  sc_core::sc_start();
  if (!model.valid || model.records != std::vector<std::string>{"mutex 2000", "semaphore 3000"}) return 2;
  for (const auto& record : model.records) std::cout << "SYNC_FAMILY_REFERENCE " << record << "\n";
  return 0;
}

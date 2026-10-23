/** @file sc_core_sync_public.cpp
 * @brief Native reference for remaining mutex and semaphore family contracts.
 * Probe objects only expose protected members for verification. All objects
 * and report strings have local native ownership; no Python callbacks or GIL
 * operations occur in this reference.
 */
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>

namespace sync_public_probe
{
int mutex_destroyed = 0;
int semaphore_destroyed = 0;

/** @brief Expose the protected mutex state query for contract testing. */
struct Mutex : sc_core::sc_mutex
{
  Mutex() = default;
  explicit Mutex(const char* name) : sc_core::sc_mutex(name)
  {
  }
  ~Mutex() override
  {
    ++mutex_destroyed;
  }
  bool protected_in_use() const
  {
    return in_use();
  }
};

/** @brief Expose protected semaphore support and report members. */
struct Semaphore : sc_core::sc_semaphore
{
  explicit Semaphore(int value) : sc_core::sc_semaphore(value)
  {
  }
  Semaphore(const char* name, int value) : sc_core::sc_semaphore(name, value)
  {
  }
  ~Semaphore() override
  {
    ++semaphore_destroyed;
  }
  bool protected_in_use() const
  {
    return in_use();
  }
  std::string protected_report_error() const
  {
    try
    {
      report_error("sync_public_probe", "details");
    }
    catch (const sc_core::sc_report& report)
    {
      std::ostringstream result;
      result << report.get_id() << '\t' << static_cast<int>(report.get_severity()) << '\t' << report.get_msg_type()
             << '\t' << report.get_msg();
      return result.str();
    }
    return "no-report";
  }
};

/** @brief Constructible implementation that exposes the protected interface constructor. */
struct MutexInterface : sc_core::sc_mutex_if
{
  int lock() override
  {
    return 10;
  }
  int trylock() override
  {
    return 11;
  }
  int unlock() override
  {
    return 12;
  }
};

/** @brief Constructible implementation that exposes the protected interface constructor. */
struct SemaphoreInterface : sc_core::sc_semaphore_if
{
  int wait() override
  {
    return 20;
  }
  int trywait() override
  {
    return 21;
  }
  int post() override
  {
    return 22;
  }
  int get_value() const override
  {
    return 23;
  }
};
}  // namespace sync_public_probe

/** @brief Own concrete probes during process-context protected-state checks. */
struct Model : sc_core::sc_module
{
  sync_public_probe::Mutex default_mutex;
  sync_public_probe::Mutex mutex;
  sync_public_probe::Semaphore default_semaphore;
  sync_public_probe::Semaphore semaphore;
  sync_public_probe::Semaphore report_semaphore;
  bool valid = true;
  SC_HAS_PROCESS(Model);
  explicit Model(sc_core::sc_module_name name)
    : sc_module(name)
    , default_mutex()
    , mutex("named_mutex")
    , default_semaphore(1)
    , semaphore("named_semaphore", 1)
    , report_semaphore("report_semaphore", 1)
  {
    SC_THREAD(exercise);
  }
  /** @brief Exercise the protected in-use queries around real native calls. */
  void exercise()
  {
    using namespace sc_core;
    if (mutex.protected_in_use() || mutex.lock() != 0 || !mutex.protected_in_use() || mutex.unlock() != 0 ||
        mutex.protected_in_use())
      valid = false;
    if (semaphore.protected_in_use() || semaphore.get_value() != 1 || semaphore.wait() != 0 ||
        !semaphore.protected_in_use() || semaphore.get_value() != 0 || semaphore.post() != 0 ||
        semaphore.protected_in_use() || semaphore.get_value() != 1)
      valid = false;
    sc_stop();
  }
};

/** @brief Compare construction, interface callbacks, report fields and destruction. */
int sc_main(int, char**)
{
  sync_public_probe::MutexInterface mutex_if;
  sync_public_probe::SemaphoreInterface semaphore_if;
  if (mutex_if.lock() != 10 || mutex_if.trylock() != 11 || mutex_if.unlock() != 12 || semaphore_if.wait() != 20 ||
      semaphore_if.trywait() != 21 || semaphore_if.post() != 22 || semaphore_if.get_value() != 23)
    return 1;
  std::cout << "SYNC_PUBLIC_REFERENCE\tmutex_interface\t10\t11\t12\n";
  std::cout << "SYNC_PUBLIC_REFERENCE\tsemaphore_interface\t20\t21\t22\t23\n";

  {
    Model model("sync_public");
    if (std::string(model.default_mutex.basename()).find("mutex_") != 0 ||
        std::string(model.default_semaphore.basename()).find("semaphore_") != 0 ||
        std::string(model.mutex.basename()) != "named_mutex" ||
        std::string(model.semaphore.basename()) != "named_semaphore" || std::string(model.mutex.kind()) != "sc_mutex" ||
        std::string(model.semaphore.kind()) != "sc_semaphore")
      return 2;
    std::cout << "SYNC_PUBLIC_REFERENCE\treport_error\t" << model.report_semaphore.protected_report_error() << '\n';
    sc_core::sc_start();
    if (!model.valid) return 3;
  }
  std::cout << "SYNC_PUBLIC_REFERENCE\tin_use\t0\t1\t0\t0\t1\t0\t1\n";
  if (sync_public_probe::mutex_destroyed != 2 || sync_public_probe::semaphore_destroyed != 3) return 4;
  std::cout << "SYNC_PUBLIC_REFERENCE\tdestruction\t" << sync_public_probe::mutex_destroyed << '\t'
            << sync_public_probe::semaphore_destroyed << '\n';
  return 0;
}

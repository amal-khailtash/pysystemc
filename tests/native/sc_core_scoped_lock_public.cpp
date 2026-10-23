/** @file Native scoped lock contention, release and reset/kill reference. */
#include <iostream>
#include <systemc>
/** Expose protected state for a native destruction assertion. */
struct ScopedMutexProbe : sc_core::sc_mutex
{
  explicit ScopedMutexProbe(const char* name) : sc_mutex(name)
  {
  }
  bool active() const
  {
    return in_use();
  }
};
int sc_main(int, char**)
{
  ScopedMutexProbe mutex("mutex");
  auto owner = sc_core::sc_spawn(
    [&]
    {
      sc_core::sc_scoped_lock lock(mutex);
      sc_core::wait(2, sc_core::SC_NS);
      bool first = lock.release(), second = lock.release();
      std::cout << "SCOPED_LOCK\trelease\t" << sc_core::sc_time_stamp() << "\t" << (first ? "True" : "False") << "\t"
                << (second ? "True" : "False") << '\n';
    },
    "owner");
  auto waiter = sc_core::sc_spawn(
    [&]
    {
      sc_core::wait(1, sc_core::SC_NS);
      {
        sc_core::sc_scoped_lock lock(mutex);
        std::cout << "SCOPED_LOCK\tacquired\t" << sc_core::sc_time_stamp() << "\t"
                  << (mutex.trylock() == 0 ? "True" : "False") << '\n';
      }
      std::cout << "SCOPED_LOCK\tdestroyed\t" << sc_core::sc_time_stamp() << "\t"
                << (!mutex.active() ? "True" : "False") << '\n';
      mutex.unlock();
    },
    "waiter");
  sc_core::sc_start(3, sc_core::SC_NS);
  std::string cleanup;
  auto control = sc_core::sc_spawn(
    [&]
    {
      sc_core::sc_scoped_lock lock(mutex);
      auto child = sc_core::sc_spawn(
        [&]
        {
          try
          {
            sc_core::sc_scoped_lock blocked(mutex);
          }
          catch (const sc_core::sc_unwind_exception&)
          {
            cleanup += "\t" + sc_core::sc_time_stamp().to_string();
            throw;
          }
        },
        "blocked");
      sc_core::wait(1, sc_core::SC_NS);
      child.reset();
      sc_core::wait(1, sc_core::SC_NS);
      child.kill();
      lock.release();
    },
    "control");
  sc_core::sc_start(3, sc_core::SC_NS);
  std::cout << "SCOPED_UNWIND" << cleanup << '\n';
  sc_core::sc_stop();
  return 0;
}

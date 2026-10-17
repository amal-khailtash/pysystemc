#include <stdexcept>
#include <systemc>

/** @brief Native reference owns synchronization objects; no Python or GIL use. */
struct top : sc_core::sc_module
{
  sc_core::sc_mutex mutex;
  sc_core::sc_semaphore sem;
  sc_core::sc_event_queue queue;
  sc_core::sc_signal<sc_dt::sc_biguint<137>, sc_core::SC_MANY_WRITERS> signal;
  int calls = 0;
  SC_HAS_PROCESS(top);
  explicit top(sc_core::sc_module_name name) : sc_module(name), sem("sem", 0)
  {
    SC_THREAD(controller);
    SC_THREAD(worker);
    SC_METHOD(observe);
    sensitive << queue.default_event();
    dont_initialize();
  }
  /** @brief Assert contention and exact native wakeup times. */
  void worker()
  {
    sc_core::wait(sc_core::SC_ZERO_TIME);
    if (mutex.trylock() != -1 || mutex.lock() || sc_core::sc_time_stamp() != sc_core::sc_time(2, sc_core::SC_NS))
      throw std::runtime_error("mutex");
    mutex.unlock();
    if (sem.wait() || sc_core::sc_time_stamp() != sc_core::sc_time(3, sc_core::SC_NS))
      throw std::runtime_error("semaphore");
  }
  /** @brief Release synchronization objects at distinct times. */
  void controller()
  {
    mutex.trylock();
    sc_core::wait(2, sc_core::SC_NS);
    mutex.unlock();
    sc_core::wait(1, sc_core::SC_NS);
    sem.post();
  }
  /** @brief Verify both surviving queue notifications. */
  void observe()
  {
    ++calls;
    if (sc_core::sc_time_stamp() != sc_core::sc_time(calls == 1 ? 1 : 4, sc_core::SC_NS))
      throw std::runtime_error("queue");
  }
};

/** @brief Check queue overloads, cancellation, copied writes and policy. */
int sc_main(int, char**)
{
  top model("top");
  sc_dt::sc_biguint<137> value = 17;
  value[136] = true;
  model.signal.write(value);
  value = 0;
  model.queue.notify(9, sc_core::SC_NS);
  model.queue.cancel_all();
  model.queue.notify(sc_core::sc_time(1, sc_core::SC_NS));
  model.queue.notify(4, sc_core::SC_NS);
  sc_core::sc_start(5, sc_core::SC_NS);
  if (model.calls != 2 || !model.signal.read()[136] || model.signal.read().to_uint() != 17 || model.sem.get_value())
    throw std::runtime_error("final state");
  std::cout << "SYNC_RESULT queue 1 mutex 2 semaphore 3 queue 4 value 17 high 1\n";
  return 0;
}

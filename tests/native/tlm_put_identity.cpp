/** @file tlm_put_identity.cpp
 * @brief Independent native put virtual identity and readiness reference.
 * @details Native modules own samples/events. All access runs on the kernel
 * thread without Python callbacks, retained external pointers or GIL operations.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <iostream>
#include <stdexcept>
#include <systemc>
#include <tlm>
#include <vector>

/** @brief Combined native put owner with copied samples and readiness state. */
template <class T>
struct owner : sc_core::sc_module, tlm::tlm_put_if<T>
{
  std::vector<T> values;
  bool ready = true;
  sc_core::sc_event changed;
  explicit owner(sc_core::sc_module_name name) : sc_module(name)
  {
  }
  /** @brief Copy input storage owned by the caller. */
  void put(const T& value) override
  {
    values.push_back(value);
  }
  /** @brief Copy only successful nonblocking inputs. */
  bool nb_put(const T& value) override
  {
    if (ready) put(value);
    return ready;
  }
  /** @brief Query readiness without changing stored samples. */
  bool nb_can_put(tlm::tlm_tag<T>* = nullptr) const override
  {
    return ready;
  }
  /** @brief Borrow the stable owner event. */
  const sc_core::sc_event& ok_to_put(tlm::tlm_tag<T>* = nullptr) const override
  {
    return changed;
  }
};

/** @brief Verify native interface bases and copied values for one specialization. */
template <class T>
void check(owner<T>& channel, T value, int& observations)
{
  tlm::tlm_put_if<T>& full = channel;
  tlm::tlm_blocking_put_if<T>& blocking = full;
  tlm::tlm_nonblocking_put_if<T>& nb = full;
  if (static_cast<sc_core::sc_interface*>(&blocking) != static_cast<sc_core::sc_interface*>(&nb) ||
      &full.ok_to_put() != &nb.ok_to_put())
    throw std::runtime_error("identity");
  blocking.put(value);
  if (!nb.nb_put(value)) throw std::runtime_error("put");
  channel.ready = false;
  if (nb.nb_can_put() || nb.nb_put(value) || channel.values.size() != 2 || channel.values[0] != value ||
      channel.values[1] != value)
    throw std::runtime_error("copies");
  value = 0;
  sc_core::sc_spawn_options options;
  options.spawn_method();
  options.dont_initialize();
  options.set_sensitivity(&nb.ok_to_put());
  sc_core::sc_spawn(
    [&observations]
    {
      if (sc_core::sc_time_stamp() != sc_core::sc_time(1, sc_core::SC_NS)) throw std::runtime_error("time");
      ++observations;
    },
    sc_core::sc_gen_unique_name("ready"), &options);
  channel.changed.notify(1, sc_core::SC_NS);
}

/** @brief Compare three native specializations and exactly timed readiness callbacks. */
int sc_main(int, char**)
{
  owner<int> a("a");
  owner<long long> b("b");
  owner<sc_dt::sc_biguint<137>> c("c");
  int observations = 0;
  sc_dt::sc_biguint<137> wide = 17;
  wide[136] = true;
  check(a, 17, observations);
  check(b, 17LL, observations);
  check(c, wide, observations);
  sc_core::sc_start(2, sc_core::SC_NS);
  if (observations != 3) throw std::runtime_error("callbacks");
  std::cout << "TLM_PUT_NATIVE 3 2 1 ns\n";
  return 0;
}

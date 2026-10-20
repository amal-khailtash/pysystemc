/** @file fifo_output_reset.cpp
 * @brief Independent native FIFO/TLM caller-output scheduler-unwind reference.
 * @details Channels and events belong to the module until all processes terminate.
 * No Python callbacks or GIL operations occur. Scheduler unwind is always rethrown.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <array>
#include <stdexcept>
#include <systemc>
#include <tlm>

/** @brief Own three empty channels and reset/kill their blocked consumers. */
template <class T>
struct reset_model : sc_core::sc_module
{
  sc_core::sc_fifo<T> fifo;
  tlm::tlm_fifo<T> get_fifo;
  tlm::tlm_fifo<T> peek_fifo;
  std::array<sc_core::sc_process_handle, 5> handles;
  std::array<int, 5> starts{};
  std::array<int, 5> unwinds{};
  SC_HAS_PROCESS(reset_model);

  /** @brief Register consumers; native storage outlives every borrowed interface. */
  explicit reset_model(sc_core::sc_module_name name)
    : sc_module(name), fifo("fifo"), get_fifo("get_fifo"), peek_fifo("peek_fifo")
  {
    for (int path = 0; path < 5; ++path) handles[path] = sc_core::sc_spawn([this, path] { consume(path); });
    SC_THREAD(control);
  }

  /** @brief Check output in the native unwind frame before mandatory rethrow. */
  void consume(int path)
  {
    const auto expected_start = sc_core::sc_time(starts[path] == 0 ? 0 : 2, sc_core::SC_NS);
    if (sc_core::sc_time_stamp() != expected_start) throw std::runtime_error("start time");
    ++starts[path];
    T output = 71;
    try
    {
      if (path == 0)
        fifo.read(output);
      else if (path == 1)
        static_cast<tlm::tlm_get_if<T>&>(get_fifo).get(output);
      else if (path == 2)
        static_cast<tlm::tlm_peek_if<T>&>(peek_fifo).peek(output);
      else if (path == 3)
        static_cast<tlm::tlm_fifo_get_if<T>&>(get_fifo).get(output);
      else
        static_cast<tlm::tlm_fifo_get_if<T>&>(peek_fifo).peek(output);
    }
    catch (const sc_core::sc_unwind_exception& unwind)
    {
      const bool resetting = unwinds[path] == 0;
      if (output != T(71) || unwind.is_reset() != resetting ||
          sc_core::sc_time_stamp() != sc_core::sc_time(resetting ? 2 : 3, sc_core::SC_NS))
        throw std::runtime_error("unwind output/time/kind mismatch");
      ++unwinds[path];
      throw;
    }
    throw std::runtime_error("empty blocking read returned");
  }

  /** @brief Interrupt blocked consumers twice without changing channel state. */
  void control()
  {
    sc_core::wait(2, sc_core::SC_NS);
    for (auto& handle : handles) handle.reset();
    sc_core::wait(1, sc_core::SC_NS);
    for (auto& handle : handles) handle.kill();
  }

  /** @brief Verify exact restart/unwind counts and final channel state. */
  void verify() const
  {
    for (int path = 0; path < 5; ++path)
      if (starts[path] != 2 || unwinds[path] != 2 || !handles[path].terminated())
        throw std::runtime_error("process lifecycle mismatch");
    if (fifo.num_available() || get_fifo.used() || peek_fifo.used()) throw std::runtime_error("empty channel changed");
  }
};

/** @brief Compare all fourteen structural-matrix value types in one fresh kernel. */
int sc_main(int, char**)
{
  reset_model<int> a("int");
  reset_model<short> b("short");
  reset_model<unsigned short> c("ushort");
  reset_model<unsigned int> d("uint");
  reset_model<long> e("long");
  reset_model<unsigned long> f("ulong");
  reset_model<long long> g("longlong");
  reset_model<unsigned long long> h("ulonglong");
  reset_model<float> i("float");
  reset_model<double> j("double");
  reset_model<long double> k("longdouble");
  reset_model<sc_dt::sc_bigint<128>> l("bigint128");
  reset_model<sc_dt::sc_biguint<128>> m("biguint128");
  reset_model<sc_dt::sc_biguint<137>> n("biguint137");
  sc_core::sc_start(4, sc_core::SC_NS);
  a.verify();
  b.verify();
  c.verify();
  d.verify();
  e.verify();
  f.verify();
  g.verify();
  h.verify();
  i.verify();
  j.verify();
  k.verify();
  l.verify();
  m.verify();
  n.verify();
  if (sc_core::sc_time_stamp() != sc_core::sc_time(4, sc_core::SC_NS)) throw std::runtime_error("final time mismatch");
  std::cout << "FIFO_OUTPUT_RESET_REFERENCE 14 5 2 3 4 ns\n";
  return 0;
}

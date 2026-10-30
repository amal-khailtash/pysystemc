/** @file tlm_adapter_error_edges.cpp
 *  @brief Native reference for FIFO state after rejected Python argument lists.
 *  @details The reference performs the valid peek/get sequence at the same
 *  simulated time and checks native queue values and occupancy.
 */
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <tlm>

namespace
{
/** @brief Throw when a native reference observation differs from expectation.
 *  @param condition Native FIFO or scheduler invariant to check.
 *  @param message Short explanation reported by the standalone reference.
 */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Compare blocking FIFO values and simulation times with Python.
 *  @details The FIFO is owned by this module and lives until sc_main exits.
 *  The SystemC thread has no Python callback or GIL interaction.
 */
struct Reader : sc_core::sc_module
{
  tlm::tlm_fifo<int> fifo;
  tlm::tlm_tag<int> tag;

  SC_HAS_PROCESS(Reader);

  /** @brief Create a one-element queue and begin the native reference thread.
   *  @param name Stable SystemC module name.
   */
  explicit Reader(sc_core::sc_module_name name) : sc_module(name), fifo("edge_fifo", 1)
  {
    SC_THREAD(run);
    verify(fifo.nb_put(314), "native FIFO seed insertion failed");
  }

  /** @brief Peek and consume the seeded value after a one-nanosecond wait. */
  void run()
  {
    const sc_core::sc_time start_time = sc_core::sc_time_stamp();
    wait(1, sc_core::SC_NS);
    const int peeked = fifo.peek(&tag);
    const int used_after_peek = fifo.used();
    const int received = fifo.get(&tag);
    const int used_after_get = fifo.used();
    verify(start_time == sc_core::SC_ZERO_TIME, "native reader started at the wrong time");
    verify(peeked == 314 && received == 314, "native blocking FIFO values differ");
    verify(used_after_peek == 1 && used_after_get == 0, "native peek/get occupancy differs");
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(1, sc_core::SC_NS), "native read time differs");
    std::cout << "TLM_ADAPTER_FIFO " << start_time << ' ' << start_time << ' ' << sc_core::sc_time_stamp() << ' '
              << peeked << ' ' << received << ' ' << used_after_peek << ' ' << used_after_get << '\n';
  }
};
}  // namespace

/** @brief Run an isolated native SystemC FIFO reference simulation. */
int sc_main(int, char**)
{
  Reader reader("top");
  sc_core::sc_start(2, sc_core::SC_NS);
  sc_core::sc_stop();
  return 0;
}

/** @file core_small_adapter_edges.cpp
 *  @brief Native reference for small public adapter edge contracts.
 *  @details The module owns its signal, exports and FIFO for the full kernel
 *  lifetime. The pointer vector owns its slots and borrows static pointees.
 *  No Python callbacks, borrowed Python references, or GIL operations occur.
 */
#include <iostream>
#include <stdexcept>
#include <systemc>

/** @brief Exercise native export, FIFO, pointer-vector, signal, and scheduler behavior. */
struct core_small_adapter_edges_model : sc_core::sc_module
{
  sc_core::sc_signal<int> signal{"edge_signal"};
  sc_core::sc_export<sc_core::sc_signal_in_if<int>> first_export{"call_export"};
  sc_core::sc_export<sc_core::sc_signal_in_if<int>> second_export{"index_export"};
  sc_core::sc_fifo<int> fifo{"edge_fifo", 2};

  SC_HAS_PROCESS(core_small_adapter_edges_model);

  /** @brief Bind two exports and queue deterministic nonblocking FIFO values. */
  explicit core_small_adapter_edges_model(sc_core::sc_module_name name) : sc_module(name)
  {
    first_export.bind(signal);
    second_export.bind(signal);
    signal.write(23);
    if (!fifo.nb_write(31) || !fifo.nb_write(47)) throw std::runtime_error("native FIFO setup failed");
  }
};

/** @brief Run independent native operations and print comparable semantic records. */
int sc_main(int, char**)
{
  using namespace sc_core;
  core_small_adapter_edges_model top("core_small_adapter_edges");
  if (top.first_export.get_interface() != &top.signal || top.second_export.get_interface() != &top.signal)
    throw std::runtime_error("native export interface identity failed");

  sc_time first_time(11, SC_NS);
  sc_time second_time(22, SC_NS);
  sc_pvector<sc_time*> vector;
  vector[0] = &first_time;
  vector[3] = &second_time;
  if (vector.size() != 4 || vector.fetch(0) != &first_time || vector.fetch(1) != nullptr ||
      vector.fetch(2) != nullptr || vector.fetch(3) != &second_time)
    throw std::runtime_error("native pointer-vector slots differ");

  sc_start(SC_ZERO_TIME);
  int first = -1;
  int second = -1;
  if (top.fifo.num_available() != 2 || !top.fifo.nb_read(first) || !top.fifo.nb_read(second) || first != 31 ||
      second != 47)
    throw std::runtime_error("native FIFO values differ");
  sc_start(1, SC_NS);
  if (sc_time_stamp() != sc_time(1, SC_NS) || top.signal.read() != 23)
    throw std::runtime_error("native scheduler or signal value differs");

  std::cout << "CORE_EXPORT_EDGE value=" << top.signal.read() << " bindings=2\n";
  std::cout << "CORE_FIFO_EDGE values=" << first << ',' << second << " capacity=2\n";
  std::cout << "CORE_PVECTOR_EDGE slots=11,null,null,22 size=" << vector.size() << '\n';
  std::cout << "CORE_SIGNAL_VALUE_EDGE value=" << top.signal.read() << '\n';
  std::cout << "CORE_START_EDGE time_ns=" << sc_time_stamp().to_seconds() * 1e9 << '\n';
  return 0;
}

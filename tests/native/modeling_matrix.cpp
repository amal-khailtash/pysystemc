/**
 * @file modeling_matrix.cpp
 * @brief Native reference for update scheduling, ports and vector/integer VCD.
 * @details All channels and trace storage outlive simulation. No Python callbacks or GIL operations are used.
 */
#include <iostream>
#include <string>
#include <systemc>
using namespace sc_core;
using namespace sc_dt;

/** @brief Observe signal changes and repeated buffer updates. */
struct Observer : sc_module
{
  sc_in<bool> input{"input"};
  sc_inout<bool> io{"io"};
  int changes = 0, updates = 0;
  SC_HAS_PROCESS(Observer);
  /** @brief Bind ports and register noninitializing native methods. */
  Observer(sc_module_name name, sc_signal<bool>& signal, sc_buffer<bool>& buffer) : sc_module(name)
  {
    input(signal);
    io(buffer);
    SC_METHOD(signal_tick);
    sensitive << input;
    dont_initialize();
    SC_METHOD(buffer_tick);
    sensitive << io;
    dont_initialize();
  }
  /** @brief Count signal changes. */
  void signal_tick() { ++changes; }
  /** @brief Count buffer updates. */
  void buffer_tick() { ++updates; }
};

/** @brief Run identical native stimuli and emit final records. */
int sc_main(int, char**)
{
  sc_signal<bool> signal("signal");
  sc_buffer<bool> buffer("buffer");
  Observer model("observer", signal, buffer);
  sc_signal<sc_uint<17>> u("u");
  sc_signal<sc_int<17>> i("i");
  sc_signal<sc_biguint<137>> bu("bu");
  sc_signal<sc_bigint<137>> bi("bi");
  sc_signal<sc_bv<137>> bits("bits");
  sc_signal<sc_lv<17>> logic("logic");
  auto trace = sc_create_vcd_trace_file("native_matrix");
  trace->set_time_unit(1, SC_NS);
  sc_trace(trace, signal, "boolean");
  sc_trace(trace, u, "integer_0");
  sc_trace(trace, i, "integer_1");
  sc_trace(trace, bu, "integer_2");
  sc_trace(trace, bi, "integer_3");
  sc_trace(trace, bits, "bits137");
  sc_trace(trace, logic, "logic17");
  sc_start(1, SC_NS);
  u.write(9);
  i.write(9);
  bu.write(9);
  bi.write(9);
  bits.write((std::string(133, '0') + "1010").c_str());
  logic.write("01XZ");
  signal.write(true);
  buffer.write(true);
  if (u.read() != 0) return 1;
  sc_start(1, SC_NS);
  signal.write(true);
  buffer.write(true);
  sc_start(1, SC_NS);
  signal.write(false);
  sc_start(1, SC_NS);
  sc_close_vcd_trace_file(trace);
  std::cout << "MATRIX_RECORD " << model.changes << "|" << model.updates << "|" << sc_time_stamp() << '\n';
  std::cout << "MATRIX_VALUES " << u.read() << "|" << i.read() << "|" << bu.read().to_uint64() << "|"
            << bi.read().to_int64() << '\n';
  sc_stop();
  return 0;
}

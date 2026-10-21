/** @file structural_float_trace.cpp
 * @brief Independent floating signal VCD declarations and transitions.
 * @details Native signals own storage until trace closure. No Python/GIL calls.
 */
#include <systemc>
/** @brief Trace the native-supported float and double specializations. */
int sc_main(int, char**)
{
  sc_core::sc_signal<float> single("single");
  sc_core::sc_signal<double> real("real");
  single.write(1.25f);
  real.write(1.25);
  auto* trace = sc_core::sc_create_vcd_trace_file("native_float");
  trace->set_time_unit(1, sc_core::SC_NS);
  sc_core::sc_trace(trace, single, "float");
  sc_core::sc_trace(trace, real, "double");
  sc_core::sc_start(1, sc_core::SC_NS);
  single.write(-2.5f);
  real.write(-2.5);
  sc_core::sc_start(1, sc_core::SC_NS);
  sc_core::sc_stop();
  sc_core::sc_close_vcd_trace_file(trace);
  return 0;
}

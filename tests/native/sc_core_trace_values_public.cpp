/** @file sc_core_trace_values_public.cpp
 * @brief Native references for stable primitive, enum and wide-value traces.
 */
#include <iostream>
#include <systemc>

/** @brief Keep native value and enum-label storage alive through trace closure. */
int sc_main(int, char**)
{
  using namespace sc_core;
  float single = 1.5f;
  double real = 2.25;
  unsigned short small = 3;
  sc_dt::uint64 wide = (sc_dt::uint64(1) << 63) + 5;
  bool boolean = false;
  unsigned int state = 0;
  const char* labels[] = {"idle", "running", "done", nullptr};
  sc_dt::sc_biguint<137> vector = 17;
  auto* trace = sc_create_vcd_trace_file("native_trace_values");
  trace->set_time_unit(1, SC_PS);
  trace->trace(single, "float_value");
  trace->trace(real, "double_value");
  trace->trace(small, "short_value", 9);
  sc_trace(trace, wide, "wide_integer", 64);
  trace->trace(boolean, "boolean");
  trace->trace(state, "state", labels);
  sc_trace(trace, vector, "wide_vector");
  sc_start(1, SC_NS);
  single = 2.75f;
  real = -3.5;
  small = 319;
  wide = (sc_dt::uint64(1) << 63) + 17;
  boolean = true;
  state = 1;
  vector = (sc_dt::sc_biguint<137>(1) << 136) + 257;
  sc_start(1, SC_NS);
  sc_stop();
  sc_close_vcd_trace_file(trace);
  std::cout << "TRACE_VALUES_TIME " << sc_time_stamp().value() << '\n';
  return 0;
}

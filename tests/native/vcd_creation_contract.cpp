/** @file vcd_creation_contract.cpp
 *  @brief Native reference for VCD creation failures and recovery.
 *  @details The empty-name failure is caught as a native report, after which
 *  a valid trace is created, used, and explicitly closed. The signal remains
 *  alive through closure. This fixture uses no Python callbacks, borrowed
 *  Python references, or GIL operations.
 */
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>

/** @brief Verify the native VCD creation error and subsequent trace lifecycle. */
int sc_main(int, char**)
{
  using namespace sc_core;

  bool caught_expected_report = false;
  try
  {
    sc_create_vcd_trace_file("");
  }
  catch (const sc_report& report)
  {
    if (std::string(report.get_msg_type()) != std::string(SC_ID_TRACING_FOPEN_FAILED_) || report.get_id() <= 0)
      throw std::runtime_error("empty VCD filename raised an unexpected SystemC report");
    std::cout << "VCD_CREATION_ERROR " << report.get_id() << '\n';
    caught_expected_report = true;
  }
  if (!caught_expected_report) throw std::runtime_error("empty VCD filename unexpectedly returned a trace file");

  sc_signal<int> signal("creation_signal");
  sc_trace_file* trace = sc_create_vcd_trace_file("creation_recovery");
  if (trace == nullptr) throw std::runtime_error("valid VCD creation returned null");
  sc_trace(trace, signal, "signal");
  trace->set_time_unit(1, SC_NS);
  sc_start(1, SC_NS);
  if (signal.read() != 0) throw std::runtime_error("initial traced signal value differs");
  signal.write(7);
  sc_start(1, SC_NS);
  if (signal.read() != 7 || sc_time_stamp() != sc_time(2, SC_NS))
    throw std::runtime_error("recovered trace simulation did not reach the expected value and time");
  sc_close_vcd_trace_file(trace);
  std::cout << "VCD_CREATION_RECOVERY 2000\n";
  return 0;
}

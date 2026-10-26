/** @file sc_core_trace_format_matrix.cpp
 * @brief Native VCD and WIF references for the public typed trace matrix.
 */
#include <cstdlib>
#include <iostream>
#ifndef SC_INCLUDE_FX
#define SC_INCLUDE_FX
#endif
#include <sysc/tracing/sc_tracing_ids.h>

#include <systemc>

/** @brief Own every waveform sample, event and time storage for trace closure.
 * @details Native trace files borrow field storage from this object. The model
 * keeps it alive until each file closes. No callbacks or GIL operations occur;
 * SystemC reports and exceptions propagate to the native runner.
 */
struct TraceFormatSamples
{
  bool boolean = false;
  sc_dt::sc_bit bit{false};
  sc_dt::sc_logic logic = sc_dt::SC_LOGIC_0;
  char schar = -3;
  unsigned char uchar = 3;
  short sshort = -5;
  unsigned short ushort = 5;
  int sint = -7;
  unsigned int uint = 7;
  long slong = -9;
  unsigned long ulong = 9;
  sc_dt::int64 int64_value = -11;
  sc_dt::uint64 uint64_value = (sc_dt::uint64(1) << 48) + 13;
  float single = 1.25f;
  double real = 2.25;
  sc_dt::sc_int<19> signed_fixed_width = -17;
  sc_dt::sc_uint<21> unsigned_fixed_width = 19;
  sc_dt::sc_signed signed_arbitrary{73};
  sc_dt::sc_unsigned unsigned_arbitrary{73};
  sc_dt::sc_bv<73> bits{"0000000000000000000000000000000000000000000000000000000000000000000000001"};
  sc_dt::sc_lv<73> logic_vector{"00000000000000000000000000000000000000000000000000000000000000000000000X1"};
  sc_dt::sc_fxval fx_value{1.25};
  sc_dt::sc_fxval_fast fx_fast_value{2.5};
  sc_dt::sc_fixed<16, 8> fixed_value{1.25};
  sc_dt::sc_fixed_fast<16, 8> fixed_fast_value{2.5};
  unsigned int enumeration = 0;
  const char* enum_literals[4] = {"IDLE", "BUSY", "DONE", nullptr};
  sc_core::sc_time time_value{3, sc_core::SC_NS};
  sc_core::sc_event event_value{"matrix_event"};

  /** @brief Initialize arbitrary-precision native values after construction. */
  TraceFormatSamples()
  {
    signed_arbitrary = -17;
    unsigned_arbitrary = 17;
  }
};

/** @brief Apply one deterministic phase to every mutable waveform value.
 * @param samples Stable native references already registered with the file.
 * @param phase Either 1 or 2, providing distinct values at increasing times.
 * @details The operation is synchronous and retains no callbacks or borrowed
 * arguments. No GIL is acquired in the native model; SystemC reports propagate.
 */
void assign_phase(TraceFormatSamples& samples, int phase)
{
  const int signed_byte = phase == 1 ? -13 : -21;
  const int unsigned_byte = phase == 1 ? 33 : 65;
  const int signed_word = phase == 1 ? -29 : 31;
  const int unsigned_word = phase == 1 ? 129 : 257;
  const double floating = phase == 1 ? 2.5 : -3.75;
  const double real = phase == 1 ? -1.5 : 4.5;
  samples.boolean = phase == 2;
  samples.bit = sc_dt::sc_bit(phase == 1);
  samples.logic = phase == 1 ? sc_dt::SC_LOGIC_1 : sc_dt::SC_LOGIC_Z;
  samples.schar = static_cast<char>(signed_byte);
  samples.uchar = static_cast<unsigned char>(unsigned_byte);
  samples.sshort = static_cast<short>(signed_word);
  samples.ushort = static_cast<unsigned short>(unsigned_word);
  samples.sint = signed_byte * 100;
  samples.uint = static_cast<unsigned int>(unsigned_byte * 100);
  samples.slong = signed_word * 100;
  samples.ulong = static_cast<unsigned long>(unsigned_word * 100);
  samples.int64_value = signed_byte * 1000;
  samples.uint64_value = (sc_dt::uint64(1) << 48) + unsigned_word * 1000;
  samples.single = static_cast<float>(floating);
  samples.real = real;
  samples.signed_fixed_width = signed_byte;
  samples.unsigned_fixed_width = unsigned_word;
  samples.signed_arbitrary = signed_word;
  samples.unsigned_arbitrary = unsigned_word;
  samples.bits = phase == 1 ? "1000000000000000000000000000000000000000000000000000000000000000000000001"
                            : "1000000000000000000000000000000000000000000000000000000000000000000000011";
  samples.logic_vector = phase == 1 ? "1000000000000000000000000000000000000000000000000000000000000000000000X1"
                                    : "1000000000000000000000000000000000000000000000000000000000000000000000Z0";
  samples.fx_value = floating;
  samples.fx_fast_value = real;
  samples.fixed_value = floating;
  samples.fixed_fast_value = real;
  samples.enumeration = static_cast<unsigned int>(phase);
  samples.time_value = sc_core::sc_time(phase == 1 ? 5 : 8, sc_core::SC_NS);
  samples.event_value.notify(sc_core::SC_ZERO_TIME);
}

/** @brief Register every typed native overload accepted by each waveform file.
 * @param file Live native trace file borrowing fields from ``samples``.
 * @param samples Owner of all references through file closure.
 * @param vcd True for VCD, which supports time and event; otherwise WIF.
 * @details WIF reports time and event as ignored objects. The caller suppresses
 * only the expected report action while checking their report counts.
 * The trace file borrows samples until closure. This native call retains no
 * Python references or callbacks and does not interact with the GIL.
 */
void register_all(sc_core::sc_trace_file* file, TraceFormatSamples& samples, bool vcd)
{
  using namespace sc_core;
  sc_trace(file, samples.boolean, "boolean");
  sc_trace(file, samples.bit, "bit");
  sc_trace(file, samples.logic, "logic");
  sc_trace(file, samples.schar, "schar", 8);
  sc_trace(file, samples.uchar, "uchar", 8);
  sc_trace(file, samples.sshort, "sshort", 13);
  sc_trace(file, samples.ushort, "ushort", 13);
  sc_trace(file, samples.sint, "sint", 21);
  sc_trace(file, samples.uint, "uint", 21);
  sc_trace(file, samples.slong, "slong", 33);
  sc_trace(file, samples.ulong, "ulong", 33);
  sc_trace(file, samples.int64_value, "int64", 47);
  sc_trace(file, samples.uint64_value, "uint64", 57);
  sc_trace(file, samples.single, "float");
  sc_trace(file, samples.real, "double");
  sc_trace(file, samples.signed_fixed_width, "sc_int19");
  sc_trace(file, samples.unsigned_fixed_width, "sc_uint21");
  sc_trace(file, samples.signed_arbitrary, "sc_signed73");
  sc_trace(file, samples.unsigned_arbitrary, "sc_unsigned73");
  sc_trace(file, samples.bits, "sc_bv73");
  sc_trace(file, samples.logic_vector, "sc_lv73");
  sc_trace(file, samples.fx_value, "fxval");
  sc_trace(file, samples.fx_fast_value, "fxval_fast");
  sc_trace(file, samples.fixed_value, "fxnum");
  sc_trace(file, samples.fixed_fast_value, "fxnum_fast");
  sc_trace(file, samples.enumeration, "enumeration", samples.enum_literals);
  if (vcd)
  {
    sc_trace(file, samples.time_value, "time");
    sc_trace(file, samples.event_value, "event");
  }
  else
  {
    const int errors = sc_report_handler::get_count(SC_ERROR);
    const auto actions = sc_report_handler::set_actions(SC_ID_TRACING_OBJECT_IGNORED_, SC_DO_NOTHING);
    sc_trace(file, samples.time_value, "time");
    sc_trace(file, samples.event_value, "event");
    sc_report_handler::set_actions(SC_ID_TRACING_OBJECT_IGNORED_, actions);
    if (sc_report_handler::get_count(SC_ERROR) != errors + 2) std::abort();
  }
  const int warnings = sc_report_handler::get_count(SC_WARNING);
  const auto warning_actions = sc_report_handler::set_actions(SC_WARNING, SC_DO_NOTHING);
  sc_trace(file, static_cast<const void*>(nullptr), "ignored_pointer");
  sc_report_handler::set_actions(SC_WARNING, warning_actions);
  if (sc_report_handler::get_count(SC_WARNING) != warnings + 1) std::abort();
}

/** @brief Run one fresh VCD or WIF trace simulation selected by argv.
 * @param argc Argument count; argv[1] is ``vcd`` or ``wif``.
 * @param argv Format selector used to create matching native output.
 * @return Zero on complete tracing and transition checks.
 */
int sc_main(int argc, char** argv)
{
  using namespace sc_core;
  if (argc != 2) return 2;
  const std::string format(argv[1]);
  const bool vcd = format == "vcd";
  if (!vcd && format != "wif") return 2;
  TraceFormatSamples samples;
  sc_trace_file* trace =
    vcd ? sc_create_vcd_trace_file("native_trace_matrix_vcd") : sc_create_wif_trace_file("native_trace_matrix_wif");
  if (!trace) return 2;
  trace->set_time_unit(1, vcd ? SC_PS : SC_NS);
  register_all(trace, samples, vcd);
  sc_start(1, SC_NS);
  assign_phase(samples, 1);
  sc_start(1, SC_NS);
  assign_phase(samples, 2);
  sc_start(1, SC_NS);
  sc_stop();
  if (sc_time_stamp() != sc_time(3, SC_NS)) return 3;
  if (vcd)
    sc_close_vcd_trace_file(trace);
  else
    sc_close_wif_trace_file(trace);
  std::cout << "TRACE_FORMAT_MATRIX_OK " << format << ' ' << sc_time_stamp() << '\n';
  return 0;
}

/** @file sc_core_trace_file_public.cpp
 * @brief Native reference for all sc_trace_file declarations and dispatch.
 */
#include <iostream>

#include "sc_core_trace_file_probe.h"

using sc_core::TraceFileProbe;

/** @brief Exercise each abstract trace overload and helper on a native recorder. */
int sc_main(int, char**)
{
  {
    TraceFileProbe trace;
    trace.trace(trace.event_sample(), "event");
    trace.trace(trace.time_sample(), "time");
    trace.trace(trace.bool_sample(), "bool");
    trace.trace(trace.bit_sample(), "bit");
    trace.trace(trace.logic_sample(), "logic");
    trace.trace(trace.uchar_sample(), "uchar", 5);
    trace.trace(trace.ushort_sample(), "ushort", 9);
    trace.trace(trace.uint_sample(), "uint", 11);
    trace.trace(trace.ulong_sample(), "ulong", 13);
    trace.trace(trace.char_sample(), "char", 6);
    trace.trace(trace.short_sample(), "short", 10);
    trace.trace(trace.int_sample(), "int", 12);
    trace.trace(trace.long_sample(), "long", 14);
    trace.trace(trace.int64_sample(), "int64", 17);
    trace.trace(trace.uint64_sample(), "uint64", 19);
    trace.trace(trace.float_sample(), "float");
    trace.trace(trace.double_sample(), "double");
    trace.trace(trace.sc_int_sample(), "sc_int");
    trace.trace(trace.sc_uint_sample(), "sc_uint");
    trace.trace(trace.signed_wide_sample(), "signed");
    trace.trace(trace.unsigned_wide_sample(), "unsigned");
    trace.trace(trace.fxval_sample(), "fxval");
    trace.trace(trace.fxval_fast_sample(), "fxval_fast");
    trace.trace(trace.fixed_sample(), "fxnum");
    trace.trace(trace.fixed_fast_sample(), "fxnum_fast");
    trace.trace(trace.bits_sample(), "bv");
    trace.trace(trace.logic_vector_sample(), "lv");
    trace.trace(trace.uint_sample(), "enum", trace.enum_literals());

    trace.write_comment("native comment");
    trace.space(2);
    trace.delta_cycles(true);
    trace.delta_cycles(false);
    trace.set_time_unit(0.5, sc_core::SC_PS);
    trace.invoke_cycle(true);
    trace.invoke_cycle(false);

    for (const auto& record : trace.records())
    {
      std::cout << "TRACE_FILE|" << record << '\n';
    }
  }
  std::cout << "TRACE_FILE_DESTRUCTIONS|" << TraceFileProbe::destruction_count() << '\n';
  return 0;
}

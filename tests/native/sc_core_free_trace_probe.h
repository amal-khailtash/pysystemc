/** @file sc_core_free_trace_probe.h
 * @brief Stable native references and typed pointers for free tracing overloads.
 * @details The recorder owns its samples; native files borrow pointers until
 * closure. Helpers hold the GIL, retain no callbacks and propagate native errors.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_FREE_TRACE_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_FREE_TRACE_PROBE_H
#include "sc_core_trace_file_probe.h"
namespace free_trace_probe
{
/** @brief Borrow the recorder's live sc_core::sc_event sample address. */
inline const sc_core::sc_event* event_pointer(sc_core::TraceFileProbe& file)
{
  return &file.event_sample();
}
/** @brief Borrow the recorder's live sc_core::sc_time sample address. */
inline const sc_core::sc_time* time_pointer(sc_core::TraceFileProbe& file)
{
  return &file.time_sample();
}
/** @brief Borrow the recorder's live bool sample address. */
inline const bool* bool_pointer(sc_core::TraceFileProbe& file)
{
  return &file.bool_sample();
}
/** @brief Borrow the recorder's live float sample address. */
inline const float* float_pointer(sc_core::TraceFileProbe& file)
{
  return &file.float_sample();
}
/** @brief Borrow the recorder's live double sample address. */
inline const double* double_pointer(sc_core::TraceFileProbe& file)
{
  return &file.double_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_bit sample address. */
inline const sc_dt::sc_bit* bit_pointer(sc_core::TraceFileProbe& file)
{
  return &file.bit_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_logic sample address. */
inline const sc_dt::sc_logic* logic_pointer(sc_core::TraceFileProbe& file)
{
  return &file.logic_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_int_base sample address. */
inline const sc_dt::sc_int_base* sc_int_pointer(sc_core::TraceFileProbe& file)
{
  return &file.sc_int_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_uint_base sample address. */
inline const sc_dt::sc_uint_base* sc_uint_pointer(sc_core::TraceFileProbe& file)
{
  return &file.sc_uint_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_signed sample address. */
inline const sc_dt::sc_signed* signed_wide_pointer(sc_core::TraceFileProbe& file)
{
  return &file.signed_wide_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_unsigned sample address. */
inline const sc_dt::sc_unsigned* unsigned_wide_pointer(sc_core::TraceFileProbe& file)
{
  return &file.unsigned_wide_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_bv_base sample address. */
inline const sc_dt::sc_bv_base* bits_pointer(sc_core::TraceFileProbe& file)
{
  return &file.bits_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_lv_base sample address. */
inline const sc_dt::sc_lv_base* logic_vector_pointer(sc_core::TraceFileProbe& file)
{
  return &file.logic_vector_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_fxval sample address. */
inline const sc_dt::sc_fxval* fxval_pointer(sc_core::TraceFileProbe& file)
{
  return &file.fxval_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_fxval_fast sample address. */
inline const sc_dt::sc_fxval_fast* fxval_fast_pointer(sc_core::TraceFileProbe& file)
{
  return &file.fxval_fast_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_fxnum sample address. */
inline const sc_dt::sc_fxnum* fixed_pointer(sc_core::TraceFileProbe& file)
{
  return &file.fixed_sample();
}
/** @brief Borrow the recorder's live sc_dt::sc_fxnum_fast sample address. */
inline const sc_dt::sc_fxnum_fast* fixed_fast_pointer(sc_core::TraceFileProbe& file)
{
  return &file.fixed_fast_sample();
}
/** @brief Borrow the recorder's live unsigned char sample address. */
inline const unsigned char* uchar_pointer(sc_core::TraceFileProbe& file)
{
  return &file.uchar_sample();
}
/** @brief Borrow the recorder's live unsigned short sample address. */
inline const unsigned short* ushort_pointer(sc_core::TraceFileProbe& file)
{
  return &file.ushort_sample();
}
/** @brief Borrow the recorder's live unsigned int sample address. */
inline const unsigned int* uint_pointer(sc_core::TraceFileProbe& file)
{
  return &file.uint_sample();
}
/** @brief Borrow the recorder's live unsigned long sample address. */
inline const unsigned long* ulong_pointer(sc_core::TraceFileProbe& file)
{
  return &file.ulong_sample();
}
/** @brief Borrow the recorder's live char sample address. */
inline const char* char_pointer(sc_core::TraceFileProbe& file)
{
  return &file.char_sample();
}
/** @brief Borrow the recorder's live short sample address. */
inline const short* short_pointer(sc_core::TraceFileProbe& file)
{
  return &file.short_sample();
}
/** @brief Borrow the recorder's live int sample address. */
inline const int* int_pointer(sc_core::TraceFileProbe& file)
{
  return &file.int_sample();
}
/** @brief Borrow the recorder's live long sample address. */
inline const long* long_pointer(sc_core::TraceFileProbe& file)
{
  return &file.long_sample();
}
/** @brief Borrow the recorder's live sc_dt::int64 sample address. */
inline const sc_dt::int64* int64_pointer(sc_core::TraceFileProbe& file)
{
  return &file.int64_sample();
}
/** @brief Borrow the recorder's live sc_dt::uint64 sample address. */
inline const sc_dt::uint64* uint64_pointer(sc_core::TraceFileProbe& file)
{
  return &file.uint64_sample();
}
/** @brief Native module owning unbound ports for delayed trace registration. */
struct ports : sc_core::sc_module
{
  sc_core::sc_in<int> input{"input"};
  sc_core::sc_inout<int> inout{"inout"};
  sc_core::sc_in<sc_dt::sc_biguint<137>> wide_input{"wide_input"};
  sc_core::sc_inout<sc_dt::sc_biguint<137>> wide_inout{"wide_inout"};
  /** @brief Register hierarchy while leaving ports unbound for the caller. */
  explicit ports(sc_core::sc_module_name name) : sc_module(name)
  {
  }
};
}  // namespace free_trace_probe
#endif

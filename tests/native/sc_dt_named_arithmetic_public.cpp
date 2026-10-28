/** @file sc_dt_named_arithmetic_public.cpp
 * @brief Independent exact-function-pointer references for named arithmetic.
 * @details Every output and operand signature is compiled against a native
 * function pointer selected from the public declaration matrix. Sources are
 * borrowed only during each call, wide output objects own their state, and
 * observer objects outlive every number that borrows them. No Python callbacks,
 * waits, or kernel singleton resets occur.
 */
#include <systemc>

#include "sc_dt_named_arithmetic_probe.h"

#if !defined(NAMED_ARITH_OUTPUT) || !defined(NAMED_ARITH_MODE)
#error "the test selector must choose an output family and deterministic matrix half"
#endif

#define RUN_CASE(OUTPUT, LEFT, RIGHT) \
  sc_dt_named_arithmetic_probe::run_case<OUTPUT, LEFT, RIGHT>(#OUTPUT, #LEFT, #RIGHT)

/** @brief Run one compile-time selected half of the independent exact signature matrix. */
int sc_main(int, char **)
{
#if NAMED_ARITH_OUTPUT == 0
#if NAMED_ARITH_MODE == 0
  RUN_CASE(sc_dt::sc_fxval, const char *, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, const char *, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, const char *);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, double);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, float);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, int);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, long);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, sc_dt::int64);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, sc_dt::uint64);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, unsigned int);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum &, unsigned long);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, const char *);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, const sc_dt::sc_int_base &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, const sc_dt::sc_signed &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, double);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, float);
#else
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, int);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, long);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, sc_dt::int64);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, sc_dt::uint64);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, unsigned int);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval &, unsigned long);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_int_base &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_signed &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, double, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, double, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, float, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, float, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, int, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, int, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, long, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, long, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, sc_dt::int64, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, sc_dt::int64, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, sc_dt::uint64, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, sc_dt::uint64, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, unsigned int, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, unsigned int, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval, unsigned long, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval, unsigned long, const sc_dt::sc_fxval &);
#endif
#elif NAMED_ARITH_OUTPUT == 1
#if NAMED_ARITH_MODE == 0
  RUN_CASE(sc_dt::sc_fxval_fast, const char *, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const char *, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, const char *);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, double);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, float);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, int);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, long);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, sc_dt::int64);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, unsigned int);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxnum_fast &, unsigned long);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, const char *);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &);
#else
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, double);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, float);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, int);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, long);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, sc_dt::int64);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, sc_dt::uint64);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, unsigned int);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_fxval_fast &, unsigned long);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, double, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, double, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, float, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, float, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, int, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, int, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, long, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, long, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, sc_dt::int64, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, sc_dt::int64, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, sc_dt::uint64, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, unsigned int, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, unsigned int, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, unsigned long, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxval_fast, unsigned long, const sc_dt::sc_fxval_fast &);
#endif
#elif NAMED_ARITH_OUTPUT == 2
#if NAMED_ARITH_MODE == 0
  RUN_CASE(sc_dt::sc_fxnum, const char *, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, const char *);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, double);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, float);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, int);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, long);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, sc_dt::int64);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, sc_dt::uint64);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, unsigned int);
#else
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum &, unsigned long);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, double, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, float, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, int, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, long, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, sc_dt::int64, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, sc_dt::uint64, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, unsigned int, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum, unsigned long, const sc_dt::sc_fxnum &);
#endif
#elif NAMED_ARITH_OUTPUT == 3
#if NAMED_ARITH_MODE == 0
  RUN_CASE(sc_dt::sc_fxnum_fast, const char *, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, const char *);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, double);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, float);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, int);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, long);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, sc_dt::int64);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
#else
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, unsigned int);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxnum_fast &, unsigned long);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, double, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, float, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, int, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, long, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, sc_dt::int64, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, unsigned int, const sc_dt::sc_fxnum_fast &);
  RUN_CASE(sc_dt::sc_fxnum_fast, unsigned long, const sc_dt::sc_fxnum_fast &);
#endif
#else
#error "unknown native output family selector"
#endif
#if NAMED_ARITH_MODE == 0
#if NAMED_ARITH_OUTPUT == 0
  // DEFN_BIN_FNC deletes sc_fxval output representation before reading inputs.
  std::cout << "NAR_ALIAS_UNSUPPORTED\tsc_dt::sc_fxval\tSC_FXVAL_OUTPUT_MUST_NOT_ALIAS_INPUT\n";
#elif NAMED_ARITH_OUTPUT == 1
  sc_dt_named_arithmetic_probe::run_alias_cases<sc_dt::sc_fxval_fast>("sc_dt::sc_fxval_fast");
#elif NAMED_ARITH_OUTPUT == 2
  // DEFN_BIN_FNC deletes sc_fxnum output representation before reading inputs.
  std::cout << "NAR_ALIAS_UNSUPPORTED\tsc_dt::sc_fxnum\tDEFN_BIN_FNC_OUTPUT_MUST_NOT_ALIAS_INPUT\n";
#else
  sc_dt_named_arithmetic_probe::run_alias_cases<sc_dt::sc_fxnum_fast>("sc_dt::sc_fxnum_fast");
#endif
#endif
  return 0;
}

#undef RUN_CASE

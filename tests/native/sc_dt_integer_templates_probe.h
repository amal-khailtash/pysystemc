/** @file sc_dt_integer_templates_probe.h
 * @brief Native scalar and compile-time probes for SystemC integer templates.
 * @details Scalar factories preserve the native C++ type selected by each
 *          public overload. Returned fixed-point and numeric sources are
 *          ordinary values; callers retain their own source objects. Helpers
 *          are synchronous, retain no Python state, and do not enter the
 *          SystemC scheduler.
 */
#ifndef PYSYSTEMC_TEST_SC_DT_INTEGER_TEMPLATES_PROBE_H
#define PYSYSTEMC_TEST_SC_DT_INTEGER_TEMPLATES_PROBE_H

#include <cstdint>
#include <systemc>
#include <type_traits>

namespace sc_dt_integer_templates_probe
{

using Int64 = sc_dt::int64;
using UInt64 = sc_dt::uint64;

/** @brief Return a typed signed 64-bit source for exact overload selection. */
inline Int64 int64_value()
{
  return static_cast<Int64>(-0x123456789abLL);
}

/** @brief Return a typed unsigned 64-bit source with high bits set. */
inline UInt64 uint64_value()
{
  return static_cast<UInt64>(0xfedcba9876543210ULL);
}

/** @brief Return a typed native long source. */
inline long long_value()
{
  return -123456789L;
}

/** @brief Return a typed native unsigned long source. */
inline unsigned long ulong_value()
{
  return 345678901UL;
}

/** @brief Return a typed native int source. */
inline int int_value()
{
  return -1234567;
}

/** @brief Return a typed native unsigned int source. */
inline unsigned int uint_value()
{
  return 2345678U;
}

/** @brief Return a native double source retaining a fractional component. */
inline double double_value()
{
  return -9876.75;
}

/** @brief Return whether the native headers enabled fixed-point overloads. */
inline constexpr bool fixed_point_enabled()
{
#ifdef SC_INCLUDE_FX
  return true;
#else
  return false;
#endif
}

/** @brief Expose a const bounded signed range while its source remains alive.
 * @param source Live fixed-width owner borrowed by the returned range shell.
 * @param high Inclusive high endpoint.
 * @param low Inclusive low endpoint.
 * @return Borrowed native const range reference.
 */
inline const sc_dt::sc_int_subref_r& const_int_range(const sc_dt::sc_int_base& source, int high, int low)
{
  return source.range(high, low);
}

/** @brief Expose a const bounded unsigned range while its source remains alive.
 * @param source Live fixed-width owner borrowed by the returned range shell.
 * @param high Inclusive high endpoint.
 * @param low Inclusive low endpoint.
 * @return Borrowed native const range reference.
 */
inline const sc_dt::sc_uint_subref_r& const_uint_range(const sc_dt::sc_uint_base& source, int high, int low)
{
  return source.range(high, low);
}

/** @brief Make native SystemC errors catchable during invalid-input probes.
 * @return Prior process-wide SystemC error actions; restore them afterward.
 * @note This changes process-global state and is intended only for the fresh
 *       one-shot test subprocess.
 */
inline sc_core::sc_actions throw_errors()
{
  return sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
}

/** @brief Restore the process-wide report action saved by throw_errors().
 * @param actions Previous process-wide SC_ERROR action.
 */
inline void restore_errors(sc_core::sc_actions actions)
{
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, actions);
}

static_assert(std::is_base_of<sc_dt::sc_signed, sc_dt::sc_bigint<48>>::value,
              "sc_bigint publicly derives from sc_signed");
static_assert(std::is_base_of<sc_dt::sc_unsigned, sc_dt::sc_biguint<48>>::value,
              "sc_biguint publicly derives from sc_unsigned");
static_assert(std::is_base_of<sc_dt::sc_int_base, sc_dt::sc_int<59>>::value,
              "sc_int publicly derives from sc_int_base");
static_assert(std::is_base_of<sc_dt::sc_uint_base, sc_dt::sc_uint<59>>::value,
              "sc_uint publicly derives from sc_uint_base");
static_assert(std::is_copy_constructible<sc_dt::sc_bigint<48>>::value &&
                std::is_copy_assignable<sc_dt::sc_bigint<48>>::value,
              "sc_bigint exposes value copy construction and assignment");
static_assert(std::is_copy_constructible<sc_dt::sc_biguint<48>>::value &&
                std::is_copy_assignable<sc_dt::sc_biguint<48>>::value,
              "sc_biguint exposes value copy construction and assignment");
static_assert(std::is_copy_constructible<sc_dt::sc_int<59>>::value && std::is_copy_assignable<sc_dt::sc_int<59>>::value,
              "sc_int exposes value copy construction and assignment");
static_assert(std::is_copy_constructible<sc_dt::sc_uint<59>>::value &&
                std::is_copy_assignable<sc_dt::sc_uint<59>>::value,
              "sc_uint exposes value copy construction and assignment");
static_assert(std::is_default_constructible<sc_dt::sc_bigint<48>>::value &&
                std::is_default_constructible<sc_dt::sc_biguint<48>>::value &&
                std::is_default_constructible<sc_dt::sc_int<59>>::value &&
                std::is_default_constructible<sc_dt::sc_uint<59>>::value,
              "all four public integer templates expose default construction");
static_assert(std::is_same<sc_dt::int_type, sc_dt::int64>::value, "sc_int uses the public SystemC int_type alias");
static_assert(std::is_same<sc_dt::uint_type, sc_dt::uint64>::value, "sc_uint uses the public SystemC uint_type alias");

#ifdef SC_INCLUDE_FX
static_assert(std::is_constructible<sc_dt::sc_bigint<48>, const sc_dt::sc_fxval&>::value,
              "sc_bigint fixed-point value constructor is enabled");
static_assert(std::is_constructible<sc_dt::sc_biguint<48>, const sc_dt::sc_fxnum_fast&>::value,
              "sc_biguint fixed-point number constructor is enabled");
static_assert(std::is_constructible<sc_dt::sc_int<59>, const sc_dt::sc_fxval_fast&>::value,
              "sc_int fixed-point value constructor is enabled");
static_assert(std::is_constructible<sc_dt::sc_uint<59>, const sc_dt::sc_fxnum&>::value,
              "sc_uint fixed-point number constructor is enabled");
#endif

}  // namespace sc_dt_integer_templates_probe

#endif  // PYSYSTEMC_TEST_SC_DT_INTEGER_TEMPLATES_PROBE_H

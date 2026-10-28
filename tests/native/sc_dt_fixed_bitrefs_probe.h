/** @file sc_dt_fixed_bitrefs_probe.h
 * @brief Safe helpers for inspecting fixed-point bit proxy families.
 * @details Proxy objects borrow their parent fixed-point number. Helpers take
 *          that owner by reference for the duration of each call and return
 *          scalar or copied string results. They never delete a borrowed
 *          proxy, call Python, or retain callbacks, so there is no GIL or
 *          Python-reference lifetime requirement. SystemC reports can escape
 *          as native exceptions when the caller selects SC_THROW.
 */
#ifndef TESTS_NATIVE_SC_DT_FIXED_BITREFS_PROBE_H
#define TESTS_NATIVE_SC_DT_FIXED_BITREFS_PROBE_H

#include <sysc/datatypes/bit/sc_bv_base.h>
#include <sysc/datatypes/fx/sc_fixed.h>

#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace sc_dt_fixed_bitrefs_probe
{

static_assert(!std::is_constructible<sc_dt::sc_fxnum_bitref, sc_dt::sc_fxnum&, int>::value,
              "mutable fixed bit proxies are created by their owner");
static_assert(!std::is_constructible<sc_dt::sc_fxnum_bitref_r, sc_dt::sc_fxnum&, int>::value,
              "read-only fixed bit proxies are created by a const owner");
static_assert(!std::is_constructible<sc_dt::sc_fxnum_fast_bitref, sc_dt::sc_fxnum_fast&, int>::value,
              "mutable fast fixed bit proxies are created by their owner");
static_assert(!std::is_constructible<sc_dt::sc_fxnum_fast_bitref_r, sc_dt::sc_fxnum_fast&, int>::value,
              "read-only fast fixed bit proxies are created by a const owner");
static_assert(std::is_destructible<sc_dt::sc_fxnum_bitref>::value, "an owned proxy value is destructible");
static_assert(std::is_destructible<sc_dt::sc_fxnum_bitref_r>::value, "an owned proxy value is destructible");
static_assert(std::is_destructible<sc_dt::sc_fxnum_fast_bitref>::value, "an owned proxy value is destructible");
static_assert(std::is_destructible<sc_dt::sc_fxnum_fast_bitref_r>::value, "an owned proxy value is destructible");
static_assert(std::is_constructible<sc_dt::sc_fxnum_bitref, const sc_dt::sc_fxnum_bitref&>::value,
              "mutable fixed proxy copies are public");
static_assert(std::is_constructible<sc_dt::sc_fxnum_bitref_r, const sc_dt::sc_fxnum_bitref_r&>::value,
              "read-only fixed proxy copies are public");
static_assert(std::is_constructible<sc_dt::sc_fxnum_fast_bitref, const sc_dt::sc_fxnum_fast_bitref&>::value,
              "mutable fast proxy copies are public");
static_assert(std::is_constructible<sc_dt::sc_fxnum_fast_bitref_r, const sc_dt::sc_fxnum_fast_bitref_r&>::value,
              "read-only fast proxy copies are public");
static_assert(std::is_same<decltype(std::declval<sc_dt::sc_fxnum&>().bit(0)), sc_dt::sc_fxnum_bitref>::value,
              "a mutable owner returns a mutable proxy from bit()");
static_assert(std::is_same<decltype(std::declval<const sc_dt::sc_fxnum&>().bit(0)), sc_dt::sc_fxnum_bitref_r>::value,
              "a const owner returns a read-only proxy from bit()");
static_assert(std::is_same<decltype(std::declval<sc_dt::sc_fxnum_fast&>().bit(0)), sc_dt::sc_fxnum_fast_bitref>::value,
              "a mutable fast owner returns a mutable proxy from bit()");
static_assert(
  std::is_same<decltype(std::declval<const sc_dt::sc_fxnum_fast&>().bit(0)), sc_dt::sc_fxnum_fast_bitref_r>::value,
  "a const fast owner returns a read-only proxy from bit()");

/** @brief Check the const overload's concrete proxy type at runtime.
 * @param number Borrowed const owner; it must outlive this call.
 * @return True when SystemC selected its read-only proxy type.
 */
inline bool fixed_const_view_is_readonly(const sc_dt::sc_fxnum& number)
{
  using Proxy = decltype(number.bit(0));
  return std::is_same<Proxy, sc_dt::sc_fxnum_bitref_r>::value;
}

/** @brief Check the fast const overload's concrete proxy type at runtime.
 * @param number Borrowed const owner; it must outlive this call.
 * @return True when SystemC selected its read-only proxy type.
 */
inline bool fast_const_view_is_readonly(const sc_dt::sc_fxnum_fast& number)
{
  using Proxy = decltype(number.bit(0));
  return std::is_same<Proxy, sc_dt::sc_fxnum_fast_bitref_r>::value;
}

/** @brief Read a bit through the const fixed-point overload.
 * @param number Borrowed const number; it must outlive this call.
 * @param index Public fixed-point bit index, including fractional positions.
 * @return The native bit value.
 * @throws sc_core::sc_report If index is outside [0, number.wl()).
 */
inline bool read_fixed(const sc_dt::sc_fxnum& number, int index)
{
  const auto proxy = number.bit(index);
  return static_cast<bool>(proxy);
}

/** @brief Read a bit through the const fast fixed-point overload.
 * @param number Borrowed const number; it must outlive this call.
 * @param index Public fixed-point bit index, including fractional positions.
 * @return The native bit value.
 * @throws sc_core::sc_report If index is outside [0, number.wl()).
 */
inline bool read_fast(const sc_dt::sc_fxnum_fast& number, int index)
{
  const auto proxy = number.bit(index);
  return static_cast<bool>(proxy);
}

/** @brief Print a bit selected from a const arbitrary-precision number.
 * @param number Borrowed const owner; it must outlive this call.
 * @param index Public bit index.
 * @return Copied native print text.
 * @throws sc_core::sc_report If index is out of range.
 */
inline std::string print_fixed(const sc_dt::sc_fxnum& number, int index)
{
  std::ostringstream output;
  number.bit(index).print(output);
  return output.str();
}

/** @brief Print a bit selected from a const fast number.
 * @param number Borrowed const owner; it must outlive this call.
 * @param index Public bit index.
 * @return Copied native print text.
 * @throws sc_core::sc_report If index is out of range.
 */
inline std::string print_fast(const sc_dt::sc_fxnum_fast& number, int index)
{
  std::ostringstream output;
  number.bit(index).print(output);
  return output.str();
}

/** @brief Dump a bit selected from a const arbitrary-precision number.
 * @param number Borrowed const owner; it must outlive this call.
 * @param index Public bit index.
 * @return Copied native dump text.
 * @throws sc_core::sc_report If index is out of range.
 */
inline std::string dump_fixed(const sc_dt::sc_fxnum& number, int index)
{
  std::ostringstream output;
  number.bit(index).dump(output);
  return output.str();
}

/** @brief Dump a bit selected from a const fast number.
 * @param number Borrowed const owner; it must outlive this call.
 * @param index Public bit index.
 * @return Copied native dump text.
 * @throws sc_core::sc_report If index is out of range.
 */
inline std::string dump_fast(const sc_dt::sc_fxnum_fast& number, int index)
{
  std::ostringstream output;
  number.bit(index).dump(output);
  return output.str();
}

/** @brief Exercise bool and negated-bool conversions on a bit-vector proxy.
 * @param bits Borrowed const bit vector; it must outlive this call.
 * @param index Public bit index.
 * @return Pair containing the explicit bool conversion and operator! result.
 * @throws sc_core::sc_report If index is out of range.
 */
inline std::pair<bool, bool> bitref_conversions(const sc_dt::sc_bv_base& bits, int index)
{
  const auto proxy = bits[index];
  return {static_cast<bool>(proxy), !proxy};
}

}  // namespace sc_dt_fixed_bitrefs_probe

#endif  // TESTS_NATIVE_SC_DT_FIXED_BITREFS_PROBE_H

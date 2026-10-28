/** @file sc_dt_fixed_subrefs_probe.h
 * @brief Const-view and conversion probes for fixed-point part references.
 * @details A returned subreference owns its internal bit-vector cache and
 *          borrows its fixed-point parent. Keep the parent alive through every
 *          proxy use and destroy proxy values normally; callers must not delete
 *          borrowed parents or caches. Helpers retain no Python references and
 *          call no callbacks. Invoke with the GIL held; no waits are performed.
 *          Native SystemC reports propagate to the caller.
 */
#ifndef TESTS_NATIVE_SC_DT_FIXED_SUBREFS_PROBE_H
#define TESTS_NATIVE_SC_DT_FIXED_SUBREFS_PROBE_H

#include <sysc/datatypes/fx/sc_fixed.h>

#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace sc_dt_fixed_subrefs_probe
{

/** @brief Report whether native compatibility conversion declarations are enabled.
 * @return True when SystemC was compiled with SC_DT_DEPRECATED.
 */
inline bool deprecated_conversions_enabled()
{
#ifdef SC_DT_DEPRECATED
  return true;
#else
  return false;
#endif
}

static_assert(!std::is_constructible<sc_dt::sc_fxnum_subref_r, sc_dt::sc_fxnum&, int, int>::value,
              "read-only arbitrary fixed ranges come from their owner");
static_assert(!std::is_constructible<sc_dt::sc_fxnum_fast_subref_r, sc_dt::sc_fxnum_fast&, int, int>::value,
              "read-only fast ranges come from their owner");
static_assert(!std::is_constructible<sc_dt::sc_fxnum_subref, sc_dt::sc_fxnum&, int, int>::value,
              "mutable arbitrary fixed ranges come from their owner");
static_assert(!std::is_constructible<sc_dt::sc_fxnum_fast_subref, sc_dt::sc_fxnum_fast&, int, int>::value,
              "mutable fast ranges come from their owner");
static_assert(std::is_constructible<sc_dt::sc_fxnum_subref_r, const sc_dt::sc_fxnum_subref_r&>::value,
              "read-only fixed range copies are public");
static_assert(std::is_constructible<sc_dt::sc_fxnum_fast_subref_r, const sc_dt::sc_fxnum_fast_subref_r&>::value,
              "read-only fast range copies are public");
static_assert(std::is_constructible<sc_dt::sc_fxnum_subref, const sc_dt::sc_fxnum_subref&>::value,
              "mutable fixed range copies are public");
static_assert(std::is_constructible<sc_dt::sc_fxnum_fast_subref, const sc_dt::sc_fxnum_fast_subref&>::value,
              "mutable fast range copies are public");
static_assert(std::is_destructible<sc_dt::sc_fxnum_subref_r>::value, "read-only fixed proxies own their cache");
static_assert(std::is_destructible<sc_dt::sc_fxnum_fast_subref_r>::value, "read-only fast proxies own their cache");
static_assert(std::is_destructible<sc_dt::sc_fxnum_subref>::value, "mutable fixed proxies own their cache");
static_assert(std::is_destructible<sc_dt::sc_fxnum_fast_subref>::value, "mutable fast proxies own their cache");
static_assert(std::is_same<decltype(std::declval<sc_dt::sc_fxnum&>().range(1, 0)), sc_dt::sc_fxnum_subref>::value,
              "non-const fixed range() returns a mutable proxy");
static_assert(
  std::is_same<decltype(std::declval<const sc_dt::sc_fxnum&>().range(1, 0)), sc_dt::sc_fxnum_subref_r>::value,
  "const fixed range() returns a read-only proxy");
static_assert(
  std::is_same<decltype(std::declval<sc_dt::sc_fxnum_fast&>().range(1, 0)), sc_dt::sc_fxnum_fast_subref>::value,
  "non-const fast range() returns a mutable proxy");
static_assert(
  std::is_same<decltype(std::declval<const sc_dt::sc_fxnum_fast&>().range(1, 0)), sc_dt::sc_fxnum_fast_subref_r>::value,
  "const fast range() returns a read-only proxy");

/** @brief Select a const arbitrary-precision range through native overload resolution.
 * @param number Borrowed const owner; it must outlive the returned proxy.
 * @param high Inclusive public high index.
 * @param low Inclusive public low index.
 * @return An owned read-only proxy value borrowing number.
 * @throws sc_core::sc_report If either index is outside the configured word.
 */
inline sc_dt::sc_fxnum_subref_r fixed_const_range(const sc_dt::sc_fxnum& number, int high, int low)
{
  return number.range(high, low);
}

/** @brief Select a const arbitrary-precision part through operator().
 * @param number Borrowed const owner; it must outlive the returned proxy.
 * @param high Inclusive public high index.
 * @param low Inclusive public low index.
 * @return An owned read-only proxy value borrowing number.
 * @throws sc_core::sc_report If either index is outside the configured word.
 */
inline sc_dt::sc_fxnum_subref_r fixed_const_call(const sc_dt::sc_fxnum& number, int high, int low)
{
  return number(high, low);
}

/** @brief Select a const arbitrary-precision whole-word part through operator().
 * @param number Borrowed const owner; it must outlive the returned proxy.
 * @return An owned read-only proxy value borrowing number.
 * @throws sc_core::sc_report If native range validation fails.
 */
inline sc_dt::sc_fxnum_subref_r fixed_const_call_whole(const sc_dt::sc_fxnum& number)
{
  return number();
}

/** @brief Select a const whole-word range through native overload resolution.
 * @param number Borrowed const owner; it must outlive the returned proxy.
 * @return An owned read-only proxy value borrowing number.
 * @throws sc_core::sc_report If native range validation fails.
 */
inline sc_dt::sc_fxnum_subref_r fixed_const_whole(const sc_dt::sc_fxnum& number)
{
  return number.range();
}

/** @brief Select a const fast-number range through native overload resolution.
 * @param number Borrowed const owner; it must outlive the returned proxy.
 * @param high Inclusive public high index.
 * @param low Inclusive public low index.
 * @return An owned read-only proxy value borrowing number.
 * @throws sc_core::sc_report If either index is outside the configured word.
 */
inline sc_dt::sc_fxnum_fast_subref_r fast_const_range(const sc_dt::sc_fxnum_fast& number, int high, int low)
{
  return number.range(high, low);
}

/** @brief Select a const fast part through operator().
 * @param number Borrowed const owner; it must outlive the returned proxy.
 * @param high Inclusive public high index.
 * @param low Inclusive public low index.
 * @return An owned read-only proxy value borrowing number.
 * @throws sc_core::sc_report If either index is outside the configured word.
 */
inline sc_dt::sc_fxnum_fast_subref_r fast_const_call(const sc_dt::sc_fxnum_fast& number, int high, int low)
{
  return number(high, low);
}

/** @brief Select a const fast whole-word part through operator().
 * @param number Borrowed const owner; it must outlive the returned proxy.
 * @return An owned read-only proxy value borrowing number.
 * @throws sc_core::sc_report If native range validation fails.
 */
inline sc_dt::sc_fxnum_fast_subref_r fast_const_call_whole(const sc_dt::sc_fxnum_fast& number)
{
  return number();
}

/** @brief Select a const fast whole-word range through native overload resolution.
 * @param number Borrowed const owner; it must outlive the returned proxy.
 * @return An owned read-only proxy value borrowing number.
 * @throws sc_core::sc_report If native range validation fails.
 */
inline sc_dt::sc_fxnum_fast_subref_r fast_const_whole(const sc_dt::sc_fxnum_fast& number)
{
  return number.range();
}

/** @brief Convert a const fixed subreference to independent bit-vector storage.
 * @param view Borrowed view whose parent must remain alive for this call.
 * @return Owned copy of the selected bits.
 */
inline sc_dt::sc_bv_base to_bit_vector(const sc_dt::sc_fxnum_subref_r& view)
{
  return static_cast<sc_dt::sc_bv_base>(view);
}

/** @brief Convert a const fast subreference to independent bit-vector storage.
 * @param view Borrowed view whose parent must remain alive for this call.
 * @return Owned copy of the selected bits.
 */
inline sc_dt::sc_bv_base to_fast_bit_vector(const sc_dt::sc_fxnum_fast_subref_r& view)
{
  return static_cast<sc_dt::sc_bv_base>(view);
}

/** @brief Concatenate a copied fixed range and one bool through the native helper.
 * @param view Borrowed range; its parent must remain alive for this call.
 * @param suffix Bit appended after the selected range.
 * @return Binary text of independent concatenated bits.
 */
inline std::string concat_fixed(const sc_dt::sc_fxnum_subref_r& view, bool suffix)
{
  const auto bits = static_cast<sc_dt::sc_bv_base>(view);
  const auto joined = sc_dt::concat(bits, suffix);
  return joined.to_string(sc_dt::SC_BIN, true);
}

/** @brief Concatenate a copied fast range and one bool through the native helper.
 * @param view Borrowed range; its parent must remain alive for this call.
 * @param suffix Bit appended after the selected range.
 * @return Binary text of independent concatenated bits.
 */
inline std::string concat_fast(const sc_dt::sc_fxnum_fast_subref_r& view, bool suffix)
{
  const auto bits = static_cast<sc_dt::sc_bv_base>(view);
  const auto joined = sc_dt::concat(bits, suffix);
  return joined.to_string(sc_dt::SC_BIN, true);
}

/** @brief Return a pointer to a stable bool array for the native bool-pointer assignment overload.
 * @return Borrowed static storage valid until process exit; do not delete or mutate it.
 */
inline const bool* bool_assignment_pattern()
{
  static const bool pattern[8]{true, false, true, true, false, false, true, false};
  return pattern;
}

}  // namespace sc_dt_fixed_subrefs_probe

#endif  // TESTS_NATIVE_SC_DT_FIXED_SUBREFS_PROBE_H

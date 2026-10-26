/** @file Exact sc_dt left-shift and stream-overload probes.
 * @details Helpers create ordinary native values and live proxy owners for
 *          independent Python and C++ checks. Returned concat and range views
 *          borrow their parents; callers must keep those parents alive. Calls
 *          hold the GIL in Python, retain no callbacks or Python references,
 *          and never enter the SystemC scheduler. Native reports and
 *          allocation exceptions propagate to the caller.
 */
#pragma once

#include <sysc/utils/sc_string.h>

#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>

namespace sc_dt_left_shift_probe
{

using ulong = unsigned long;
using uint = unsigned int;

/** @brief Build a fixed-point owner with native configured word lengths.
 * @param value Initial numeric value copied into the new owner.
 * @return An independently owned fixed-point value.
 */
template <class T>
T fixed_value(double value)
{
  constexpr bool fast = std::is_base_of_v<sc_dt::sc_fxnum_fast, T>;
  return T(value, fast ? 19 : 137, fast ? 9 : 65, sc_dt::SC_TRN, sc_dt::SC_WRAP, 0);
}

template <class T, class = void>
struct has_length : std::false_type
{
};

template <class T>
struct has_length<T, std::void_t<decltype(std::declval<const T&>().length())>> : std::true_type
{
};

/** @brief Allocate a live concatenation shell borrowing its two operands.
 * @param left Borrowed first leaf; it must outlive the returned shell.
 * @param right Borrowed second leaf; it must outlive the returned shell.
 * @return A borrowed shell from SystemC's process-global concat pool.
 */
inline const sc_dt::sc_concatref& concat(const sc_dt::sc_value_base& left, const sc_dt::sc_value_base& right)
{
  auto* node = sc_dt::sc_concatref::m_pool.allocate();
  node->initialize(left, right);
  return *node;
}

/** @brief Return a read-only arbitrary fixed-point bit proxy.
 * @param value Borrowed fixed-point parent; it must outlive the returned proxy.
 * @param index Native zero-based storage bit index.
 * @return An owned proxy value borrowing the supplied parent.
 */
inline sc_dt::sc_fxnum_bitref_r fixed_bit(const sc_dt::sc_fxnum& value, int index)
{
  return value.bit(index);
}

/** @brief Return a read-only fast fixed-point bit proxy.
 * @param value Borrowed fast fixed-point parent; it must outlive the proxy.
 * @param index Native zero-based storage bit index.
 * @return An owned proxy value borrowing the supplied parent.
 */
inline sc_dt::sc_fxnum_fast_bitref_r fixed_fast_bit(const sc_dt::sc_fxnum_fast& value, int index)
{
  return value.bit(index);
}

/** @brief Return the public mutable arbitrary fixed-point range proxy.
 * @param value Borrowed fixed-point parent; it must outlive the returned proxy.
 * @param high Inclusive high storage bit index.
 * @param low Inclusive low storage bit index.
 * @return An owned proxy value borrowing the supplied parent.
 */
inline sc_dt::sc_fxnum_subref fixed_range(sc_dt::sc_fxnum& value, int high, int low)
{
  return value.range(high, low);
}

/** @brief Return a read-only fast fixed-point range proxy.
 * @param value Borrowed fast fixed-point parent; it must outlive the proxy.
 * @param high Inclusive high storage bit index.
 * @param low Inclusive low storage bit index.
 * @return An owned proxy value borrowing the supplied parent.
 */
inline sc_dt::sc_fxnum_fast_subref_r fixed_fast_range(const sc_dt::sc_fxnum_fast& value, int high, int low)
{
  return value.range(high, low);
}

/** @brief Print an exact numeric shift result and its native width when present.
 * @param row Inventory row label for the operator overload.
 * @param value Independent result returned by the native shift expression.
 */
template <class T>
void emit_shift(int row, const T& value)
{
  int width = -1;
  if constexpr (has_length<T>::value) width = value.length();
  std::cout << "LSM\t" << row << "\tS\t" << width << "\t" << value.to_string(sc_dt::SC_DEC) << '\n';
}

/** @brief Print native stream text and confirm the inserted stream was returned.
 * @param row Inventory row label for the insertion overload.
 * @param value Borrowed value inserted through its public native overload.
 */
template <class T>
void emit_stream(int row, const T& value)
{
  std::ostringstream output;
  auto& returned = output << value;
  std::cout << "LSM\t" << row << "\tO\t" << output.str() << "\t" << (&returned == &output) << '\n';
}

/** @brief Select the public generic proxy insertion overload explicitly.
 * @param row Inventory row label for the sc_proxy function template.
 * @param value Borrowed proxy base whose live owner remains elsewhere.
 */
template <class X>
void emit_proxy_stream(int row, const sc_dt::sc_proxy<X>& value)
{
  std::ostringstream output;
  auto& returned = sc_dt::operator<< <X>(output, value);
  std::cout << "LSM\t" << row << "\tO\t" << output.str() << "\t" << (&returned == &output) << '\n';
}

// Function-pointer checks bind every distinct native signature in the family.
// Two inventory rows repeat signed/unsigned signatures as friend declarations.
#define SC_DT_SHIFT_SIGNATURE(RETURN, LEFT, RIGHT)                                                        \
  static_assert(std::is_same_v<decltype(static_cast<RETURN (*)(const LEFT&, RIGHT)>(&sc_dt::operator<<)), \
                               RETURN (*)(const LEFT&, RIGHT)>)

SC_DT_SHIFT_SIGNATURE(sc_dt::sc_unsigned, sc_dt::sc_unsigned, const sc_dt::sc_int_base&);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_unsigned, sc_dt::sc_unsigned, const sc_dt::sc_unsigned&);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_unsigned, sc_dt::sc_unsigned, sc_dt::int64);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_unsigned, sc_dt::sc_unsigned, unsigned long);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_unsigned, sc_dt::sc_unsigned, const sc_dt::sc_signed&);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_unsigned, sc_dt::sc_unsigned, long);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_unsigned, sc_dt::sc_unsigned, sc_dt::uint64);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_unsigned, sc_dt::sc_unsigned, unsigned int);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_unsigned, sc_dt::sc_unsigned, const sc_dt::sc_uint_base&);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_unsigned, sc_dt::sc_unsigned, int);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_signed, sc_dt::sc_signed, unsigned long);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_signed, sc_dt::sc_signed, unsigned int);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_signed, sc_dt::sc_signed, sc_dt::int64);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_signed, sc_dt::sc_signed, const sc_dt::sc_signed&);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_signed, sc_dt::sc_signed, const sc_dt::sc_unsigned&);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_signed, sc_dt::sc_signed, long);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_signed, sc_dt::sc_signed, sc_dt::uint64);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_signed, sc_dt::sc_signed, const sc_dt::sc_int_base&);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_signed, sc_dt::sc_signed, int);
SC_DT_SHIFT_SIGNATURE(sc_dt::sc_signed, sc_dt::sc_signed, const sc_dt::sc_uint_base&);
static_assert(
  std::is_same_v<decltype(static_cast<const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval&, int)>(&sc_dt::operator<<)),
                 const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval&, int)>);
static_assert(
  std::is_same_v<decltype(static_cast<const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum&, int)>(&sc_dt::operator<<)),
                 const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum&, int)>);
static_assert(std::is_same_v<decltype(static_cast<const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast&, int)>(
                               &sc_dt::operator<<)),
                             const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast&, int)>);
static_assert(std::is_same_v<decltype(static_cast<const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast&, int)>(
                               &sc_dt::operator<<)),
                             const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast&, int)>);
static_assert(std::is_same_v<
              decltype(static_cast<const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, long)>(&sc_dt::operator<<)),
              const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, long)>);
static_assert(std::is_same_v<
              decltype(static_cast<const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, int)>(&sc_dt::operator<<)),
              const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, int)>);
static_assert(
  std::is_same_v<
    decltype(static_cast<const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, sc_dt::int64)>(&sc_dt::operator<<)),
    const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, sc_dt::int64)>);
static_assert(
  std::is_same_v<
    decltype(static_cast<const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, sc_dt::uint64)>(&sc_dt::operator<<)),
    const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, sc_dt::uint64)>);
static_assert(
  std::is_same_v<
    decltype(static_cast<const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, unsigned long)>(&sc_dt::operator<<)),
    const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, unsigned long)>);
static_assert(
  std::is_same_v<
    decltype(static_cast<const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, unsigned int)>(&sc_dt::operator<<)),
    const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref&, unsigned int)>);

#undef SC_DT_SHIFT_SIGNATURE

#define SC_DT_STREAM_SIGNATURE(TYPE)                                                                         \
  static_assert(                                                                                             \
    std::is_same_v<decltype(static_cast<std::ostream& (*)(std::ostream&, const TYPE&)>(&sc_dt::operator<<)), \
                   std::ostream& (*)(std::ostream&, const TYPE&)>)

SC_DT_STREAM_SIGNATURE(sc_dt::sc_uint_bitref_r);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_uint_base);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_fxnum_fast);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_int_subref_r);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_signed_bitref_r);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_fxnum_fast_bitref_r);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_int_bitref_r);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_uint_subref_r);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_fxnum_bitref_r);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_length_param);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_unsigned_bitref_r);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_fxval);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_bit);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_fxnum_fast_subref_r);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_fxnum);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_fxval_fast);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_fxcast_switch);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_signed_subref_r);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_int_base);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_unsigned);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_concatref);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_logic);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_unsigned_subref_r);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_fxtype_params);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_string_old);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_signed);
SC_DT_STREAM_SIGNATURE(sc_dt::sc_fxnum_subref);

#undef SC_DT_STREAM_SIGNATURE

static_assert(
  std::is_same_v<decltype(static_cast<std::ostream& (*)(std::ostream&, sc_dt::sc_o_mode)>(&sc_dt::operator<<)),
                 std::ostream& (*)(std::ostream&, sc_dt::sc_o_mode)>);
static_assert(
  std::is_same_v<decltype(static_cast<std::ostream& (*)(std::ostream&, sc_dt::sc_numrep)>(&sc_dt::operator<<)),
                 std::ostream& (*)(std::ostream&, sc_dt::sc_numrep)>);
static_assert(
  std::is_same_v<decltype(static_cast<std::ostream& (*)(std::ostream&, sc_dt::sc_q_mode)>(&sc_dt::operator<<)),
                 std::ostream& (*)(std::ostream&, sc_dt::sc_q_mode)>);
static_assert(std::is_same_v<decltype(static_cast<std::ostream& (*)(std::ostream&, sc_dt::sc_fmt)>(&sc_dt::operator<<)),
                             std::ostream& (*)(std::ostream&, sc_dt::sc_fmt)>);
static_assert(std::is_same_v<decltype(static_cast<std::ostream& (*)(std::ostream&, sc_dt::sc_enc)>(&sc_dt::operator<<)),
                             std::ostream& (*)(std::ostream&, sc_dt::sc_enc)>);
static_assert(
  std::is_same_v<decltype(static_cast<std::ostream& (*)(std::ostream&, sc_dt::sc_switch)>(&sc_dt::operator<<)),
                 std::ostream& (*)(std::ostream&, sc_dt::sc_switch)>);
static_assert(
  std::is_same_v<decltype(static_cast<std::ostream& (*)(std::ostream&, const sc_dt::sc_proxy<sc_dt::sc_bv_base>&)>(
                   &sc_dt::operator<< <sc_dt::sc_bv_base>)),
                 std::ostream& (*)(std::ostream&, const sc_dt::sc_proxy<sc_dt::sc_bv_base>&)>);

}  // namespace sc_dt_left_shift_probe

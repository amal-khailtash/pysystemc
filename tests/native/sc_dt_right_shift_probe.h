/** @file Shared formatting and type traits for the right-shift audit. */
#pragma once

#include <sysc/utils/sc_string.h>

#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>

namespace sc_dt_right_shift_probe
{
using IntCount = int;
using UnsignedIntCount = unsigned int;
using LongCount = long;
using UnsignedLongCount = unsigned long;
using Int64Count = sc_dt::int64;
using UInt64Count = sc_dt::uint64;

/** @brief Format a native result through its public stream insertion overload.
 * @param value Borrowed native result or target.
 * @return Independent text copied from a temporary output stream.
 * @details The call holds the GIL when entered from Python and retains no
 *          references or callbacks. Native stream and report behavior applies.
 */
template <class T>
struct Text
{
  /** @brief Format a borrowed native datatype through stream insertion.
   * @param value Borrowed value or proxy whose owner remains live.
   * @return Independent standard string.
   * @details Hold the GIL when called from Python. No callback or reference is
   *          retained and native stream/report behavior applies.
   */
  static std::string get(const T& value)
  {
    std::ostringstream stream;
    stream << value;
    return stream.str();
  }
};

template <class T>
std::string text(const T& value)
{
  return Text<T>::get(value);
}

template <class T, class = void>
struct has_length : std::false_type
{
};

template <class T>
struct has_length<T, std::void_t<decltype(std::declval<const T&>().length())>> : std::true_type
{
};

/** @brief Read a native bit width where the shifted result exposes one.
 * @param value Borrowed native shift result.
 * @return Public native length or -1 for fixed-point values without a length.
 */
template <class T>
int length(const T& value)
{
  if constexpr (has_length<T>::value)
    return value.length();
  else
    return -1;
}

using Bv = sc_dt::sc_bv<8>;
using Lv = sc_dt::sc_lv<8>;
using BvBit = decltype(std::declval<Bv&>()[0]);
using LvBit = decltype(std::declval<Lv&>()[0]);
using BvRange = decltype(std::declval<Bv&>()(3, 0));
using LvRange = decltype(std::declval<Lv&>()(3, 0));
using Concrete = sc_dt::sc_concref<sc_dt::sc_bv_base, sc_dt::sc_bv_base>;
using Fixed = sc_dt::sc_fixed<16, 8>;
using FixedFast = sc_dt::sc_fixed_fast<16, 8>;
using FixedBit = decltype(std::declval<Fixed&>().bit(0));
using FixedFastBit = decltype(std::declval<FixedFast&>().bit(0));
using FixedRange = decltype(std::declval<Fixed&>()(3, 0));
using FixedFastRange = decltype(std::declval<FixedFast&>()(3, 0));

#define ASSERT_SHIFT_OBJECT_COUNT(Result, Value, Count)                                                         \
  static_assert(std::is_same<decltype(static_cast<Result (*)(const Value&, const Count&)>(&sc_dt::operator>>)), \
                             Result (*)(const Value&, const Count&)>::value,                                    \
                "public object-count right shift signature changed")
#define ASSERT_SHIFT_SCALAR_COUNT(Result, Value, Count)                                                  \
  static_assert(std::is_same<decltype(static_cast<Result (*)(const Value&, Count)>(&sc_dt::operator>>)), \
                             Result (*)(const Value&, Count)>::value,                                    \
                "public scalar-count right shift signature changed")
#define ASSERT_STREAM_REF(Value)                                                                                  \
  static_assert(std::is_same<decltype(static_cast<std::istream& (*)(std::istream&, Value&)>(&sc_dt::operator>>)), \
                             std::istream& (*)(std::istream&, Value&)>::value,                                    \
                "public stream reference signature changed")
#define ASSERT_STREAM_VALUE(Value)                                                                               \
  static_assert(std::is_same<decltype(static_cast<std::istream& (*)(std::istream&, Value)>(&sc_dt::operator>>)), \
                             std::istream& (*)(std::istream&, Value)>::value,                                    \
                "public stream proxy signature changed")

ASSERT_SHIFT_OBJECT_COUNT(sc_dt::sc_signed, sc_dt::sc_signed, sc_dt::sc_unsigned);
ASSERT_SHIFT_OBJECT_COUNT(sc_dt::sc_signed, sc_dt::sc_signed, sc_dt::sc_signed);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_signed, sc_dt::sc_signed, sc_dt::int64);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_signed, sc_dt::sc_signed, sc_dt::uint64);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_signed, sc_dt::sc_signed, long);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_signed, sc_dt::sc_signed, unsigned long);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_signed, sc_dt::sc_signed, int);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_signed, sc_dt::sc_signed, unsigned int);
ASSERT_SHIFT_OBJECT_COUNT(sc_dt::sc_signed, sc_dt::sc_signed, sc_dt::sc_int_base);
ASSERT_SHIFT_OBJECT_COUNT(sc_dt::sc_signed, sc_dt::sc_signed, sc_dt::sc_uint_base);
ASSERT_SHIFT_OBJECT_COUNT(sc_dt::sc_unsigned, sc_dt::sc_unsigned, sc_dt::sc_signed);
ASSERT_SHIFT_OBJECT_COUNT(sc_dt::sc_unsigned, sc_dt::sc_unsigned, sc_dt::sc_unsigned);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_unsigned, sc_dt::sc_unsigned, sc_dt::int64);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_unsigned, sc_dt::sc_unsigned, sc_dt::uint64);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_unsigned, sc_dt::sc_unsigned, long);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_unsigned, sc_dt::sc_unsigned, unsigned long);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_unsigned, sc_dt::sc_unsigned, int);
ASSERT_SHIFT_SCALAR_COUNT(sc_dt::sc_unsigned, sc_dt::sc_unsigned, unsigned int);
ASSERT_SHIFT_OBJECT_COUNT(sc_dt::sc_unsigned, sc_dt::sc_unsigned, sc_dt::sc_int_base);
ASSERT_SHIFT_OBJECT_COUNT(sc_dt::sc_unsigned, sc_dt::sc_unsigned, sc_dt::sc_uint_base);
ASSERT_SHIFT_SCALAR_COUNT(const sc_dt::sc_unsigned, sc_dt::sc_concatref, sc_dt::int64);
ASSERT_SHIFT_SCALAR_COUNT(const sc_dt::sc_unsigned, sc_dt::sc_concatref, sc_dt::uint64);
ASSERT_SHIFT_SCALAR_COUNT(const sc_dt::sc_unsigned, sc_dt::sc_concatref, long);
ASSERT_SHIFT_SCALAR_COUNT(const sc_dt::sc_unsigned, sc_dt::sc_concatref, unsigned long);
ASSERT_SHIFT_SCALAR_COUNT(const sc_dt::sc_unsigned, sc_dt::sc_concatref, int);
ASSERT_SHIFT_SCALAR_COUNT(const sc_dt::sc_unsigned, sc_dt::sc_concatref, unsigned int);
ASSERT_SHIFT_SCALAR_COUNT(const sc_dt::sc_fxval, sc_dt::sc_fxval, int);
ASSERT_SHIFT_SCALAR_COUNT(const sc_dt::sc_fxval, sc_dt::sc_fxnum, int);
ASSERT_SHIFT_SCALAR_COUNT(const sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast, int);
ASSERT_SHIFT_SCALAR_COUNT(const sc_dt::sc_fxval_fast, sc_dt::sc_fxnum_fast, int);

ASSERT_STREAM_VALUE(BvRange);
ASSERT_STREAM_REF(sc_dt::sc_int_base);
ASSERT_STREAM_VALUE(BvBit);
ASSERT_STREAM_REF(sc_dt::sc_uint_subref);
ASSERT_STREAM_REF(sc_dt::sc_uint_bitref);
ASSERT_STREAM_REF(sc_dt::sc_fxval);
ASSERT_STREAM_VALUE(Concrete);
ASSERT_STREAM_REF(sc_dt::sc_fxval_fast);
ASSERT_STREAM_REF(sc_dt::sc_fxnum_subref);
static_assert(
  std::is_same<decltype(static_cast<std::istream& (*)(std::istream&, sc_dt::sc_proxy<Bv>&)>(&sc_dt::operator>>)),
               std::istream& (*)(std::istream&, sc_dt::sc_proxy<Bv>&)>::value,
  "public vector base stream signature changed");
ASSERT_STREAM_REF(sc_dt::sc_uint_base);
ASSERT_STREAM_REF(sc_dt::sc_unsigned_bitref);
ASSERT_STREAM_VALUE(LvRange);
ASSERT_STREAM_REF(sc_dt::sc_fxnum_fast_bitref);
ASSERT_STREAM_REF(sc_dt::sc_int_subref);
ASSERT_STREAM_REF(sc_dt::sc_fxnum_fast);
ASSERT_STREAM_REF(sc_dt::sc_unsigned);
ASSERT_STREAM_REF(sc_dt::sc_unsigned_subref);
static_assert(
  std::is_same<decltype(std::declval<std::istream&>() >> std::declval<sc_dt::sc_string_old&>()), std::istream&>::value,
  "legacy string extraction must remain reachable by ADL");
ASSERT_STREAM_REF(sc_dt::sc_signed_subref);
ASSERT_STREAM_REF(sc_dt::sc_bit);
ASSERT_STREAM_REF(sc_dt::sc_fxnum_bitref);
ASSERT_STREAM_REF(sc_dt::sc_logic);
ASSERT_STREAM_REF(sc_dt::sc_fxnum_fast_subref);
ASSERT_STREAM_REF(sc_dt::sc_int_bitref);
ASSERT_STREAM_REF(sc_dt::sc_fxnum);
ASSERT_STREAM_REF(sc_dt::sc_concatref);
ASSERT_STREAM_VALUE(BvBit);
ASSERT_STREAM_REF(sc_dt::sc_signed);
ASSERT_STREAM_VALUE(Concrete);
ASSERT_STREAM_REF(sc_dt::sc_signed_bitref);

#undef ASSERT_SHIFT_OBJECT_COUNT
#undef ASSERT_SHIFT_SCALAR_COUNT
#undef ASSERT_STREAM_REF
#undef ASSERT_STREAM_VALUE
}  // namespace sc_dt_right_shift_probe

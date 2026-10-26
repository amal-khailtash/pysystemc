/** @file sc_dt_comparisons_probe.h
 * @brief Exact native probes and typed source values for SystemC comparisons.
 * @details Every invocation borrows operands from one live Sources instance;
 *          pointer arrays are null terminated where needed, bit/fixed-point
 *          proxies retain their native parents, and comparison results are
 *          copied to bool before the fixture is destroyed. No Python callback
 *          or reference is retained by the native probe.
 */
#pragma once
#ifndef SC_INCLUDE_FX
#define SC_INCLUDE_FX
#endif
#include <array>
#include <stdexcept>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>
namespace sc_dt_comparisons_probe
{
using Bool = bool;
using Char = char;
using Float = float;
using SignedInt = int;
using UnsignedInt = unsigned int;
using SignedLong = long;
using UnsignedLong = unsigned long;
using SignedScalar = sc_dt::int64;
using UnsignedScalar = sc_dt::uint64;

/** @brief Own the exact class operands and borrowed reference sources. */
struct Sources
{
  bool bool_value;
  char char_value;
  int int_value;
  unsigned int uint_value;
  long long_value;
  unsigned long ulong_value;
  float float_value;
  double double_value;
  sc_dt::int64 signed_scalar;
  sc_dt::uint64 unsigned_scalar;
  sc_dt::sc_signed signed_value;
  sc_dt::sc_unsigned unsigned_value;
  sc_dt::sc_int_base signed_fixed;
  sc_dt::sc_uint_base unsigned_fixed;
  sc_dt::sc_fixed<8, 4> fx_number;
  sc_dt::sc_fixed_fast<8, 4> fx_fast_number;
  sc_dt::sc_fxval fx_value;
  sc_dt::sc_fxval_fast fx_fast_value;
  sc_dt::sc_fxnum_subref_r fx_subref;
  sc_dt::sc_fxnum_fast_subref_r fx_fast_subref;
  sc_dt::sc_bit bit_value;
  sc_dt::sc_logic logic_value;
  sc_dt::sc_logic_value_t logic_state;
  sc_dt::sc_bv<8> bv;
  sc_dt::sc_lv<8> lv;
  sc_dt::sc_bitref_r<sc_dt::sc_bv_base> bv_ref;
  sc_dt::sc_bitref_r<sc_dt::sc_lv_base> lv_ref;
  std::array<bool, 8> bool_bits;
  std::array<char, 9> char_bits;
  std::array<sc_dt::sc_logic, 8> logic_bits;
  sc_dt::sc_signed concat_high;
  sc_dt::sc_unsigned concat_low;
  const sc_dt::sc_concatref &concat_value;
  sc_dt::sc_fxcast_switch cast_switch;
  sc_dt::sc_fxtype_params type_params;
  sc_dt::sc_length_param length_param;

  /** @brief Initialize representative signed, unsigned, vector and fixed values. */
  Sources()
    : bool_value(true)
    , char_value('1')
    , int_value(1)
    , uint_value(0x12345U)
    , long_value(-1234567L)
    , ulong_value(0xabcdefUL)
    , float_value(2.75F)
    , double_value(-3.5)
    , signed_scalar(-123456789)
    , unsigned_scalar(0xfedcba9876543210ULL)
    , signed_value(19)
    , unsigned_value(21)
    , signed_fixed(12)
    , unsigned_fixed(15)
    , fx_number(5.25)
    , fx_fast_number(6.5)
    , fx_value(3.25)
    , fx_fast_value(-2.75)
    , fx_subref(fx_number.range(7, 0))
    , fx_fast_subref(fx_fast_number.range(7, 0))
    , bit_value(true)
    , logic_value('Z')
    , logic_state(sc_dt::Log_Z)
    , bv("10110010")
    , lv("10ZX0101")
    , bv_ref(static_cast<const sc_dt::sc_bv_base &>(bv), 0)
    , lv_ref(static_cast<const sc_dt::sc_lv_base &>(lv), 1)
    , concat_high(9)
    , concat_low(7)
    , concat_value((concat_high, concat_low))
    , cast_switch()
    , type_params()
    , length_param()
  {
    signed_value = -12345;
    unsigned_value = 0x1a55a;
    signed_fixed = -311;
    unsigned_fixed = 0x4a31;
    concat_high = 0x12f;
    concat_low = 0x53;
    const bool bools[8] = {true, false, true, true, false, false, true, false};
    const char chars[9] = {'1', '0', '1', '1', '0', '0', '1', '0', '\0'};
    const char logics[8] = {'1', '0', 'Z', 'X', '0', '1', '0', '1'};
    for (int i = 0; i < 8; ++i)
    {
      bool_bits[i] = bools[i];
      char_bits[i] = chars[i];
      logic_bits[i] = sc_dt::sc_logic(logics[i]);
    }
    char_bits[8] = '\0';
  }
};

/** @brief Compare fixed-point values that differ below binary64 precision.
 * @return True when the exactly represented high value compares greater.
 * @details Each fixed-point source owns 128 bits with 96 integer bits; no
 *          double conversion occurs before the native comparison.
 */
inline bool precision_greater()
{
  const sc_dt::sc_fixed<128, 96> low("9007199254740992");
  const sc_dt::sc_fixed<128, 96> high("9007199254740993");
  using Function = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
  const auto compare = static_cast<Function>(&sc_dt::operator>);
  return compare(high, low);
}

/** @brief Compare fixed-point values around the binary64 integer limit.
 * @return True if the two distinct native values compare equal.
 */
inline bool precision_equal()
{
  const sc_dt::sc_fixed<128, 96> low("9007199254740992");
  const sc_dt::sc_fixed<128, 96> high("9007199254740993");
  using Function = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
  const auto compare = static_cast<Function>(&sc_dt::operator==);
  return compare(high, low);
}

/** @brief Verify unequal fixed-point values remain unequal above 2^53.
 * @return True when the distinct native values compare unequal.
 */
inline bool precision_not_equal()
{
  const sc_dt::sc_fixed<128, 96> low("9007199254740992");
  const sc_dt::sc_fixed<128, 96> high("9007199254740993");
  using Function = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
  const auto compare = static_cast<Function>(&sc_dt::operator!=);
  return compare(high, low);
}

/** @brief Compare four-state vectors while preserving their X and Z symbols.
 * @return True only when equal length and all four-state values match.
 */
inline bool xz_equal()
{
  const sc_dt::sc_lv<4> left("10XZ");
  const sc_dt::sc_lv<4> same("10XZ");
  using Function = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_lv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &);
  const auto compare = static_cast<Function>(&sc_dt::operator==);
  return compare(left, same);
}

/** @brief Compare four-state vectors whose final logic values differ.
 * @return True when the native vector comparison sees equal X/Z content.
 */
inline bool xz_different()
{
  const sc_dt::sc_lv<4> left("10XZ");
  const sc_dt::sc_lv<4> changed("10X0");
  using Function = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_lv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &);
  const auto compare = static_cast<Function>(&sc_dt::operator==);
  return compare(left, changed);
}

/** @brief Compare equal low bits at two different vector widths.
 * @return The exact base-proxy equality result, including native width rules.
 */
inline bool width_equal()
{
  const sc_dt::sc_bv<4> short_value("1010");
  const sc_dt::sc_bv<8> long_value("00001010");
  using Function = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
  const auto compare = static_cast<Function>(&sc_dt::operator==);
  return compare(short_value, long_value);
}

/** @brief Check a copied bool after its concatenation owners are destroyed.
 * @return The comparison result computed while the borrowed concat view is live.
 */
inline bool concat_lifetime_result()
{
  bool result = false;
  {
    Sources sources;
    using Function = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
    const auto compare = static_cast<Function>(&sc_dt::operator==);
    result = compare(sources.concat_value, sources.signed_value);
  }
  return result;
}

/** @brief Set invalid-bit report handling to throw so callers can catch it.
 * @return Previous SystemC action flags for SC_ID_VALUE_NOT_VALID_.
 */
inline int begin_invalid_value_throw()
{
  return static_cast<int>(sc_core::sc_report_handler::set_actions(sc_core::SC_ID_VALUE_NOT_VALID_, sc_core::SC_THROW));
}

/** @brief Restore invalid-bit report handling after a deliberately bad conversion.
 * @param previous The action flags returned by begin_invalid_value_throw().
 */
inline void restore_invalid_value_action(int previous)
{
  sc_core::sc_report_handler::set_actions(sc_core::SC_ID_VALUE_NOT_VALID_, static_cast<sc_core::sc_actions>(previous));
}

/** @brief Invoke the invalid char-to-bit overload under SC_THROW.
 * @return Report ID, report type, and source-bit preservation as delimited text.
 */
inline std::string native_invalid_bit_error()
{
  const int previous = begin_invalid_value_throw();
  const sc_dt::sc_bit bit(true);
  std::string result("NO_REPORT");
  try
  {
    using Function = bool (*)(char, const sc_dt::sc_bit &);
    const auto compare = static_cast<Function>(&sc_dt::operator==);
    (void)compare('?', bit);
  }
  catch (const sc_core::sc_report &report)
  {
    result = std::to_string(report.get_id()) + "|" + report.get_msg_type() + "|" + report.get_msg() + "|" +
             std::to_string(bit.to_bool());
  }
  catch (...)
  {
    restore_invalid_value_action(previous);
    throw;
  }
  restore_invalid_value_action(previous);
  return result;
}

template <class Argument>
decltype(auto) operand(Sources &s)
{
  using Value = std::remove_cv_t<std::remove_reference_t<Argument>>;
#define COMPARISON_ARG(Type, expression)          \
  else if constexpr (std::is_same_v<Value, Type>) \
  {                                               \
    return static_cast<Argument>(expression);     \
  }
  if constexpr (std::is_same_v<Value, bool>) return static_cast<Argument>(s.bool_value);
  COMPARISON_ARG(char, s.char_value)
  COMPARISON_ARG(int, s.int_value)
  COMPARISON_ARG(unsigned int, s.uint_value)
  COMPARISON_ARG(long, s.long_value)
  COMPARISON_ARG(unsigned long, s.ulong_value)
  COMPARISON_ARG(float, s.float_value)
  COMPARISON_ARG(double, s.double_value)
  COMPARISON_ARG(sc_dt::int64, s.signed_scalar)
  COMPARISON_ARG(sc_dt::uint64, s.unsigned_scalar)
  COMPARISON_ARG(const bool *, s.bool_bits.data())
  COMPARISON_ARG(const char *, s.char_bits.data())
  COMPARISON_ARG(const sc_dt::sc_logic *, s.logic_bits.data())
  COMPARISON_ARG(sc_dt::sc_logic_value_t, s.logic_state)
  COMPARISON_ARG(sc_dt::sc_signed, s.signed_value)
  COMPARISON_ARG(sc_dt::sc_unsigned, s.unsigned_value)
  COMPARISON_ARG(sc_dt::sc_int_base, s.signed_fixed)
  COMPARISON_ARG(sc_dt::sc_uint_base, s.unsigned_fixed)
  COMPARISON_ARG(sc_dt::sc_fxnum, s.fx_number)
  COMPARISON_ARG(sc_dt::sc_fxnum_fast, s.fx_fast_number)
  COMPARISON_ARG(sc_dt::sc_fxval, s.fx_value)
  COMPARISON_ARG(sc_dt::sc_fxval_fast, s.fx_fast_value)
  COMPARISON_ARG(sc_dt::sc_fxnum_subref_r, s.fx_subref)
  COMPARISON_ARG(sc_dt::sc_fxnum_fast_subref_r, s.fx_fast_subref)
  COMPARISON_ARG(sc_dt::sc_bit, s.bit_value)
  COMPARISON_ARG(sc_dt::sc_logic, s.logic_value)
  COMPARISON_ARG(sc_dt::sc_concatref, s.concat_value)
  COMPARISON_ARG(sc_dt::sc_fxcast_switch, s.cast_switch)
  COMPARISON_ARG(sc_dt::sc_fxtype_params, s.type_params)
  COMPARISON_ARG(sc_dt::sc_length_param, s.length_param)
  COMPARISON_ARG(sc_dt::sc_proxy<sc_dt::sc_bv_base>, s.bv)
  COMPARISON_ARG(sc_dt::sc_proxy<sc_dt::sc_lv_base>, s.lv)
  COMPARISON_ARG(sc_dt::sc_bitref_r<sc_dt::sc_bv_base>, s.bv_ref)
  COMPARISON_ARG(sc_dt::sc_bitref_r<sc_dt::sc_lv_base>, s.lv_ref)
  COMPARISON_ARG(sc_dt::sc_bv_base, s.bv)
  COMPARISON_ARG(sc_dt::sc_lv_base, s.lv)
  else static_assert(!sizeof(Argument), "no comparison operand factory for exact signature type");
#undef COMPARISON_ARG
}

template <class Function, class Left, class Right>
bool invoke(Function function, Sources &s)
{
  return function(operand<Left>(s), operand<Right>(s));
}
using Fn_lt_000 = bool (*)(bool, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_000>(&sc_dt::operator<)), Fn_lt_000>,
              "exact overload sc_dt::operator< bool (bool, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_000(Sources &s)
{
  return invoke<Fn_lt_000, bool, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_001 = bool (*)(const char *, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_001>(&sc_dt::operator<)), Fn_lt_001>,
              "exact overload sc_dt::operator< bool (const char *, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_001(Sources &s)
{
  return invoke<Fn_lt_001, const char *, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_002 = bool (*)(const char *, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_002>(&sc_dt::operator<)), Fn_lt_002>,
              "exact overload sc_dt::operator< bool (const char *, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_002(Sources &s)
{
  return invoke<Fn_lt_002, const char *, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_003 = bool (*)(const char *, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_003>(&sc_dt::operator<)), Fn_lt_003>,
              "exact overload sc_dt::operator< bool (const char *, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_003(Sources &s)
{
  return invoke<Fn_lt_003, const char *, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_004 = bool (*)(const char *, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_004>(&sc_dt::operator<)), Fn_lt_004>,
              "exact overload sc_dt::operator< bool (const char *, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_004(Sources &s)
{
  return invoke<Fn_lt_004, const char *, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_005 = bool (*)(const sc_dt::sc_concatref &, bool);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_005>(&sc_dt::operator<)), Fn_lt_005>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, bool)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_005(Sources &s)
{
  return invoke<Fn_lt_005, const sc_dt::sc_concatref &, bool>(&sc_dt::operator<, s);
}
using Fn_lt_006 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_006>(&sc_dt::operator<)), Fn_lt_006>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_006(Sources &s)
{
  return invoke<Fn_lt_006, const sc_dt::sc_concatref &, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_007 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_007>(&sc_dt::operator<)), Fn_lt_007>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_007(Sources &s)
{
  return invoke<Fn_lt_007, const sc_dt::sc_concatref &, const sc_dt::sc_int_base &>(&sc_dt::operator<, s);
}
using Fn_lt_008 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_008>(&sc_dt::operator<)), Fn_lt_008>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_008(Sources &s)
{
  return invoke<Fn_lt_008, const sc_dt::sc_concatref &, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_009 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_009>(&sc_dt::operator<)), Fn_lt_009>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_009(Sources &s)
{
  return invoke<Fn_lt_009, const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &>(&sc_dt::operator<, s);
}
using Fn_lt_010 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_010>(&sc_dt::operator<)), Fn_lt_010>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_010(Sources &s)
{
  return invoke<Fn_lt_010, const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_011 = bool (*)(const sc_dt::sc_concatref &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_011>(&sc_dt::operator<)), Fn_lt_011>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_011(Sources &s)
{
  return invoke<Fn_lt_011, const sc_dt::sc_concatref &, int>(&sc_dt::operator<, s);
}
using Fn_lt_012 = bool (*)(const sc_dt::sc_concatref &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_012>(&sc_dt::operator<)), Fn_lt_012>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_012(Sources &s)
{
  return invoke<Fn_lt_012, const sc_dt::sc_concatref &, long>(&sc_dt::operator<, s);
}
using Fn_lt_013 = bool (*)(const sc_dt::sc_concatref &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_013>(&sc_dt::operator<)), Fn_lt_013>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_013(Sources &s)
{
  return invoke<Fn_lt_013, const sc_dt::sc_concatref &, sc_dt::int64>(&sc_dt::operator<, s);
}
using Fn_lt_014 = bool (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_014>(&sc_dt::operator<)), Fn_lt_014>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_014(Sources &s)
{
  return invoke<Fn_lt_014, const sc_dt::sc_concatref &, sc_dt::uint64>(&sc_dt::operator<, s);
}
using Fn_lt_015 = bool (*)(const sc_dt::sc_concatref &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_015>(&sc_dt::operator<)), Fn_lt_015>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_015(Sources &s)
{
  return invoke<Fn_lt_015, const sc_dt::sc_concatref &, unsigned int>(&sc_dt::operator<, s);
}
using Fn_lt_016 = bool (*)(const sc_dt::sc_concatref &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_016>(&sc_dt::operator<)), Fn_lt_016>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_concatref &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_016(Sources &s)
{
  return invoke<Fn_lt_016, const sc_dt::sc_concatref &, unsigned long>(&sc_dt::operator<, s);
}
using Fn_lt_017 = bool (*)(const sc_dt::sc_fxnum &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_017>(&sc_dt::operator<)), Fn_lt_017>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_017(Sources &s)
{
  return invoke<Fn_lt_017, const sc_dt::sc_fxnum &, const char *>(&sc_dt::operator<, s);
}
using Fn_lt_018 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_018>(&sc_dt::operator<)), Fn_lt_018>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_018(Sources &s)
{
  return invoke<Fn_lt_018, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_019 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_019>(&sc_dt::operator<)), Fn_lt_019>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_019(Sources &s)
{
  return invoke<Fn_lt_019, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_020 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_020>(&sc_dt::operator<)), Fn_lt_020>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_020(Sources &s)
{
  return invoke<Fn_lt_020, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_021 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_021>(&sc_dt::operator<)), Fn_lt_021>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_021(Sources &s)
{
  return invoke<Fn_lt_021, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_022 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_022>(&sc_dt::operator<)), Fn_lt_022>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_022(Sources &s)
{
  return invoke<Fn_lt_022, const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &>(&sc_dt::operator<, s);
}
using Fn_lt_023 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_023>(&sc_dt::operator<)), Fn_lt_023>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_023(Sources &s)
{
  return invoke<Fn_lt_023, const sc_dt::sc_fxnum &, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_024 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_024>(&sc_dt::operator<)), Fn_lt_024>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_024(Sources &s)
{
  return invoke<Fn_lt_024, const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &>(&sc_dt::operator<, s);
}
using Fn_lt_025 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_025>(&sc_dt::operator<)), Fn_lt_025>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_025(Sources &s)
{
  return invoke<Fn_lt_025, const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_026 = bool (*)(const sc_dt::sc_fxnum &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_026>(&sc_dt::operator<)), Fn_lt_026>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_026(Sources &s)
{
  return invoke<Fn_lt_026, const sc_dt::sc_fxnum &, double>(&sc_dt::operator<, s);
}
using Fn_lt_027 = bool (*)(const sc_dt::sc_fxnum &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_027>(&sc_dt::operator<)), Fn_lt_027>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_027(Sources &s)
{
  return invoke<Fn_lt_027, const sc_dt::sc_fxnum &, float>(&sc_dt::operator<, s);
}
using Fn_lt_028 = bool (*)(const sc_dt::sc_fxnum &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_028>(&sc_dt::operator<)), Fn_lt_028>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_028(Sources &s)
{
  return invoke<Fn_lt_028, const sc_dt::sc_fxnum &, int>(&sc_dt::operator<, s);
}
using Fn_lt_029 = bool (*)(const sc_dt::sc_fxnum &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_029>(&sc_dt::operator<)), Fn_lt_029>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_029(Sources &s)
{
  return invoke<Fn_lt_029, const sc_dt::sc_fxnum &, long>(&sc_dt::operator<, s);
}
using Fn_lt_030 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_030>(&sc_dt::operator<)), Fn_lt_030>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_030(Sources &s)
{
  return invoke<Fn_lt_030, const sc_dt::sc_fxnum &, sc_dt::int64>(&sc_dt::operator<, s);
}
using Fn_lt_031 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_031>(&sc_dt::operator<)), Fn_lt_031>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_031(Sources &s)
{
  return invoke<Fn_lt_031, const sc_dt::sc_fxnum &, sc_dt::uint64>(&sc_dt::operator<, s);
}
using Fn_lt_032 = bool (*)(const sc_dt::sc_fxnum &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_032>(&sc_dt::operator<)), Fn_lt_032>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_032(Sources &s)
{
  return invoke<Fn_lt_032, const sc_dt::sc_fxnum &, unsigned int>(&sc_dt::operator<, s);
}
using Fn_lt_033 = bool (*)(const sc_dt::sc_fxnum &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_033>(&sc_dt::operator<)), Fn_lt_033>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_033(Sources &s)
{
  return invoke<Fn_lt_033, const sc_dt::sc_fxnum &, unsigned long>(&sc_dt::operator<, s);
}
using Fn_lt_034 = bool (*)(const sc_dt::sc_fxnum_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_034>(&sc_dt::operator<)), Fn_lt_034>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_034(Sources &s)
{
  return invoke<Fn_lt_034, const sc_dt::sc_fxnum_fast &, const char *>(&sc_dt::operator<, s);
}
using Fn_lt_035 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_035>(&sc_dt::operator<)), Fn_lt_035>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_035(Sources &s)
{
  return invoke<Fn_lt_035, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_036 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_036>(&sc_dt::operator<)), Fn_lt_036>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_036(Sources &s)
{
  return invoke<Fn_lt_036, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_037 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_037>(&sc_dt::operator<)), Fn_lt_037>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_037(Sources &s)
{
  return invoke<Fn_lt_037, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_038 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_038>(&sc_dt::operator<)), Fn_lt_038>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_038(Sources &s)
{
  return invoke<Fn_lt_038, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_039 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_039>(&sc_dt::operator<)), Fn_lt_039>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_039(Sources &s)
{
  return invoke<Fn_lt_039, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator<, s);
}
using Fn_lt_040 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_040>(&sc_dt::operator<)), Fn_lt_040>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_040(Sources &s)
{
  return invoke<Fn_lt_040, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_041 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_041>(&sc_dt::operator<)), Fn_lt_041>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_041(Sources &s)
{
  return invoke<Fn_lt_041, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator<, s);
}
using Fn_lt_042 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_042>(&sc_dt::operator<)), Fn_lt_042>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_042(Sources &s)
{
  return invoke<Fn_lt_042, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_043 = bool (*)(const sc_dt::sc_fxnum_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_043>(&sc_dt::operator<)), Fn_lt_043>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_043(Sources &s)
{
  return invoke<Fn_lt_043, const sc_dt::sc_fxnum_fast &, double>(&sc_dt::operator<, s);
}
using Fn_lt_044 = bool (*)(const sc_dt::sc_fxnum_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_044>(&sc_dt::operator<)), Fn_lt_044>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_044(Sources &s)
{
  return invoke<Fn_lt_044, const sc_dt::sc_fxnum_fast &, float>(&sc_dt::operator<, s);
}
using Fn_lt_045 = bool (*)(const sc_dt::sc_fxnum_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_045>(&sc_dt::operator<)), Fn_lt_045>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_045(Sources &s)
{
  return invoke<Fn_lt_045, const sc_dt::sc_fxnum_fast &, int>(&sc_dt::operator<, s);
}
using Fn_lt_046 = bool (*)(const sc_dt::sc_fxnum_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_046>(&sc_dt::operator<)), Fn_lt_046>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_046(Sources &s)
{
  return invoke<Fn_lt_046, const sc_dt::sc_fxnum_fast &, long>(&sc_dt::operator<, s);
}
using Fn_lt_047 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_047>(&sc_dt::operator<)), Fn_lt_047>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_047(Sources &s)
{
  return invoke<Fn_lt_047, const sc_dt::sc_fxnum_fast &, sc_dt::int64>(&sc_dt::operator<, s);
}
using Fn_lt_048 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_048>(&sc_dt::operator<)), Fn_lt_048>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_048(Sources &s)
{
  return invoke<Fn_lt_048, const sc_dt::sc_fxnum_fast &, sc_dt::uint64>(&sc_dt::operator<, s);
}
using Fn_lt_049 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_049>(&sc_dt::operator<)), Fn_lt_049>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_049(Sources &s)
{
  return invoke<Fn_lt_049, const sc_dt::sc_fxnum_fast &, unsigned int>(&sc_dt::operator<, s);
}
using Fn_lt_050 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_050>(&sc_dt::operator<)), Fn_lt_050>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxnum_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_050(Sources &s)
{
  return invoke<Fn_lt_050, const sc_dt::sc_fxnum_fast &, unsigned long>(&sc_dt::operator<, s);
}
using Fn_lt_051 = bool (*)(const sc_dt::sc_fxval &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_051>(&sc_dt::operator<)), Fn_lt_051>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_051(Sources &s)
{
  return invoke<Fn_lt_051, const sc_dt::sc_fxval &, const char *>(&sc_dt::operator<, s);
}
using Fn_lt_052 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_052>(&sc_dt::operator<)), Fn_lt_052>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_052(Sources &s)
{
  return invoke<Fn_lt_052, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_053 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_053>(&sc_dt::operator<)), Fn_lt_053>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_053(Sources &s)
{
  return invoke<Fn_lt_053, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_054 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_054>(&sc_dt::operator<)), Fn_lt_054>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_054(Sources &s)
{
  return invoke<Fn_lt_054, const sc_dt::sc_fxval &, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_055 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_055>(&sc_dt::operator<)), Fn_lt_055>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_055(Sources &s)
{
  return invoke<Fn_lt_055, const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_056 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_056>(&sc_dt::operator<)), Fn_lt_056>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_056(Sources &s)
{
  return invoke<Fn_lt_056, const sc_dt::sc_fxval &, const sc_dt::sc_int_base &>(&sc_dt::operator<, s);
}
using Fn_lt_057 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_057>(&sc_dt::operator<)), Fn_lt_057>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_057(Sources &s)
{
  return invoke<Fn_lt_057, const sc_dt::sc_fxval &, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_058 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_058>(&sc_dt::operator<)), Fn_lt_058>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_058(Sources &s)
{
  return invoke<Fn_lt_058, const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &>(&sc_dt::operator<, s);
}
using Fn_lt_059 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_059>(&sc_dt::operator<)), Fn_lt_059>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_059(Sources &s)
{
  return invoke<Fn_lt_059, const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_060 = bool (*)(const sc_dt::sc_fxval &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_060>(&sc_dt::operator<)), Fn_lt_060>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_060(Sources &s)
{
  return invoke<Fn_lt_060, const sc_dt::sc_fxval &, double>(&sc_dt::operator<, s);
}
using Fn_lt_061 = bool (*)(const sc_dt::sc_fxval &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_061>(&sc_dt::operator<)), Fn_lt_061>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_061(Sources &s)
{
  return invoke<Fn_lt_061, const sc_dt::sc_fxval &, float>(&sc_dt::operator<, s);
}
using Fn_lt_062 = bool (*)(const sc_dt::sc_fxval &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_062>(&sc_dt::operator<)), Fn_lt_062>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_062(Sources &s)
{
  return invoke<Fn_lt_062, const sc_dt::sc_fxval &, int>(&sc_dt::operator<, s);
}
using Fn_lt_063 = bool (*)(const sc_dt::sc_fxval &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_063>(&sc_dt::operator<)), Fn_lt_063>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_063(Sources &s)
{
  return invoke<Fn_lt_063, const sc_dt::sc_fxval &, long>(&sc_dt::operator<, s);
}
using Fn_lt_064 = bool (*)(const sc_dt::sc_fxval &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_064>(&sc_dt::operator<)), Fn_lt_064>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_064(Sources &s)
{
  return invoke<Fn_lt_064, const sc_dt::sc_fxval &, sc_dt::int64>(&sc_dt::operator<, s);
}
using Fn_lt_065 = bool (*)(const sc_dt::sc_fxval &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_065>(&sc_dt::operator<)), Fn_lt_065>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_065(Sources &s)
{
  return invoke<Fn_lt_065, const sc_dt::sc_fxval &, sc_dt::uint64>(&sc_dt::operator<, s);
}
using Fn_lt_066 = bool (*)(const sc_dt::sc_fxval &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_066>(&sc_dt::operator<)), Fn_lt_066>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_066(Sources &s)
{
  return invoke<Fn_lt_066, const sc_dt::sc_fxval &, unsigned int>(&sc_dt::operator<, s);
}
using Fn_lt_067 = bool (*)(const sc_dt::sc_fxval &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_067>(&sc_dt::operator<)), Fn_lt_067>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_067(Sources &s)
{
  return invoke<Fn_lt_067, const sc_dt::sc_fxval &, unsigned long>(&sc_dt::operator<, s);
}
using Fn_lt_068 = bool (*)(const sc_dt::sc_fxval_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_068>(&sc_dt::operator<)), Fn_lt_068>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_068(Sources &s)
{
  return invoke<Fn_lt_068, const sc_dt::sc_fxval_fast &, const char *>(&sc_dt::operator<, s);
}
using Fn_lt_069 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_069>(&sc_dt::operator<)), Fn_lt_069>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_069(Sources &s)
{
  return invoke<Fn_lt_069, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_070 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_070>(&sc_dt::operator<)), Fn_lt_070>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_070(Sources &s)
{
  return invoke<Fn_lt_070, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_071 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_071>(&sc_dt::operator<)), Fn_lt_071>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_071(Sources &s)
{
  return invoke<Fn_lt_071, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_072 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_072>(&sc_dt::operator<)), Fn_lt_072>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_072(Sources &s)
{
  return invoke<Fn_lt_072, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_073 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_073>(&sc_dt::operator<)), Fn_lt_073>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_073(Sources &s)
{
  return invoke<Fn_lt_073, const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator<, s);
}
using Fn_lt_074 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_074>(&sc_dt::operator<)), Fn_lt_074>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_074(Sources &s)
{
  return invoke<Fn_lt_074, const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_075 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_075>(&sc_dt::operator<)), Fn_lt_075>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_075(Sources &s)
{
  return invoke<Fn_lt_075, const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator<, s);
}
using Fn_lt_076 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_076>(&sc_dt::operator<)), Fn_lt_076>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_076(Sources &s)
{
  return invoke<Fn_lt_076, const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_077 = bool (*)(const sc_dt::sc_fxval_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_077>(&sc_dt::operator<)), Fn_lt_077>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_077(Sources &s)
{
  return invoke<Fn_lt_077, const sc_dt::sc_fxval_fast &, double>(&sc_dt::operator<, s);
}
using Fn_lt_078 = bool (*)(const sc_dt::sc_fxval_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_078>(&sc_dt::operator<)), Fn_lt_078>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_078(Sources &s)
{
  return invoke<Fn_lt_078, const sc_dt::sc_fxval_fast &, float>(&sc_dt::operator<, s);
}
using Fn_lt_079 = bool (*)(const sc_dt::sc_fxval_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_079>(&sc_dt::operator<)), Fn_lt_079>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_079(Sources &s)
{
  return invoke<Fn_lt_079, const sc_dt::sc_fxval_fast &, int>(&sc_dt::operator<, s);
}
using Fn_lt_080 = bool (*)(const sc_dt::sc_fxval_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_080>(&sc_dt::operator<)), Fn_lt_080>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_080(Sources &s)
{
  return invoke<Fn_lt_080, const sc_dt::sc_fxval_fast &, long>(&sc_dt::operator<, s);
}
using Fn_lt_081 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_081>(&sc_dt::operator<)), Fn_lt_081>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_081(Sources &s)
{
  return invoke<Fn_lt_081, const sc_dt::sc_fxval_fast &, sc_dt::int64>(&sc_dt::operator<, s);
}
using Fn_lt_082 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_082>(&sc_dt::operator<)), Fn_lt_082>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_082(Sources &s)
{
  return invoke<Fn_lt_082, const sc_dt::sc_fxval_fast &, sc_dt::uint64>(&sc_dt::operator<, s);
}
using Fn_lt_083 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_083>(&sc_dt::operator<)), Fn_lt_083>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_083(Sources &s)
{
  return invoke<Fn_lt_083, const sc_dt::sc_fxval_fast &, unsigned int>(&sc_dt::operator<, s);
}
using Fn_lt_084 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_084>(&sc_dt::operator<)), Fn_lt_084>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_fxval_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_084(Sources &s)
{
  return invoke<Fn_lt_084, const sc_dt::sc_fxval_fast &, unsigned long>(&sc_dt::operator<, s);
}
using Fn_lt_085 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_085>(&sc_dt::operator<)), Fn_lt_085>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_085(Sources &s)
{
  return invoke<Fn_lt_085, const sc_dt::sc_int_base &, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_086 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_086>(&sc_dt::operator<)), Fn_lt_086>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_086(Sources &s)
{
  return invoke<Fn_lt_086, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_087 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_087>(&sc_dt::operator<)), Fn_lt_087>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_087(Sources &s)
{
  return invoke<Fn_lt_087, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_088 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_088>(&sc_dt::operator<)), Fn_lt_088>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_088(Sources &s)
{
  return invoke<Fn_lt_088, const sc_dt::sc_int_base &, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_089 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_089>(&sc_dt::operator<)), Fn_lt_089>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_089(Sources &s)
{
  return invoke<Fn_lt_089, const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_090 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_090>(&sc_dt::operator<)), Fn_lt_090>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_int_base &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_090(Sources &s)
{
  return invoke<Fn_lt_090, const sc_dt::sc_int_base &, const sc_dt::sc_int_base &>(&sc_dt::operator<, s);
}
using Fn_lt_091 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_091>(&sc_dt::operator<)), Fn_lt_091>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_091(Sources &s)
{
  return invoke<Fn_lt_091, const sc_dt::sc_int_base &, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_092 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_092>(&sc_dt::operator<)), Fn_lt_092>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_092(Sources &s)
{
  return invoke<Fn_lt_092, const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_093 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_093>(&sc_dt::operator<)), Fn_lt_093>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_093(Sources &s)
{
  return invoke<Fn_lt_093, const sc_dt::sc_signed &, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_094 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_094>(&sc_dt::operator<)), Fn_lt_094>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_094(Sources &s)
{
  return invoke<Fn_lt_094, const sc_dt::sc_signed &, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_095 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_095>(&sc_dt::operator<)), Fn_lt_095>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_095(Sources &s)
{
  return invoke<Fn_lt_095, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_096 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_096>(&sc_dt::operator<)), Fn_lt_096>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_096(Sources &s)
{
  return invoke<Fn_lt_096, const sc_dt::sc_signed &, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_097 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_097>(&sc_dt::operator<)), Fn_lt_097>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_097(Sources &s)
{
  return invoke<Fn_lt_097, const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_098 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_098>(&sc_dt::operator<)), Fn_lt_098>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_098(Sources &s)
{
  return invoke<Fn_lt_098, const sc_dt::sc_signed &, const sc_dt::sc_int_base &>(&sc_dt::operator<, s);
}
using Fn_lt_099 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_099>(&sc_dt::operator<)), Fn_lt_099>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_099(Sources &s)
{
  return invoke<Fn_lt_099, const sc_dt::sc_signed &, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_100 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_100>(&sc_dt::operator<)), Fn_lt_100>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_100(Sources &s)
{
  return invoke<Fn_lt_100, const sc_dt::sc_signed &, const sc_dt::sc_uint_base &>(&sc_dt::operator<, s);
}
using Fn_lt_101 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_101>(&sc_dt::operator<)), Fn_lt_101>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_101(Sources &s)
{
  return invoke<Fn_lt_101, const sc_dt::sc_signed &, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_102 = bool (*)(const sc_dt::sc_signed &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_102>(&sc_dt::operator<)), Fn_lt_102>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_102(Sources &s)
{
  return invoke<Fn_lt_102, const sc_dt::sc_signed &, int>(&sc_dt::operator<, s);
}
using Fn_lt_103 = bool (*)(const sc_dt::sc_signed &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_103>(&sc_dt::operator<)), Fn_lt_103>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_103(Sources &s)
{
  return invoke<Fn_lt_103, const sc_dt::sc_signed &, long>(&sc_dt::operator<, s);
}
using Fn_lt_104 = bool (*)(const sc_dt::sc_signed &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_104>(&sc_dt::operator<)), Fn_lt_104>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_104(Sources &s)
{
  return invoke<Fn_lt_104, const sc_dt::sc_signed &, sc_dt::int64>(&sc_dt::operator<, s);
}
using Fn_lt_105 = bool (*)(const sc_dt::sc_signed &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_105>(&sc_dt::operator<)), Fn_lt_105>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_105(Sources &s)
{
  return invoke<Fn_lt_105, const sc_dt::sc_signed &, sc_dt::uint64>(&sc_dt::operator<, s);
}
using Fn_lt_106 = bool (*)(const sc_dt::sc_signed &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_106>(&sc_dt::operator<)), Fn_lt_106>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_106(Sources &s)
{
  return invoke<Fn_lt_106, const sc_dt::sc_signed &, unsigned int>(&sc_dt::operator<, s);
}
using Fn_lt_107 = bool (*)(const sc_dt::sc_signed &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_107>(&sc_dt::operator<)), Fn_lt_107>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_signed &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_107(Sources &s)
{
  return invoke<Fn_lt_107, const sc_dt::sc_signed &, unsigned long>(&sc_dt::operator<, s);
}
using Fn_lt_108 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_108>(&sc_dt::operator<)), Fn_lt_108>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_108(Sources &s)
{
  return invoke<Fn_lt_108, const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_109 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_109>(&sc_dt::operator<)), Fn_lt_109>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_109(Sources &s)
{
  return invoke<Fn_lt_109, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_110 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_110>(&sc_dt::operator<)), Fn_lt_110>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_110(Sources &s)
{
  return invoke<Fn_lt_110, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_111 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_111>(&sc_dt::operator<)), Fn_lt_111>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_111(Sources &s)
{
  return invoke<Fn_lt_111, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_112 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_112>(&sc_dt::operator<)), Fn_lt_112>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_112(Sources &s)
{
  return invoke<Fn_lt_112, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_113 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_113>(&sc_dt::operator<)), Fn_lt_113>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_113(Sources &s)
{
  return invoke<Fn_lt_113, const sc_dt::sc_uint_base &, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_114 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_114>(&sc_dt::operator<)), Fn_lt_114>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_114(Sources &s)
{
  return invoke<Fn_lt_114, const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &>(&sc_dt::operator<, s);
}
using Fn_lt_115 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_115>(&sc_dt::operator<)), Fn_lt_115>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_115(Sources &s)
{
  return invoke<Fn_lt_115, const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_116 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_116>(&sc_dt::operator<)), Fn_lt_116>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_116(Sources &s)
{
  return invoke<Fn_lt_116, const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_117 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_117>(&sc_dt::operator<)), Fn_lt_117>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_117(Sources &s)
{
  return invoke<Fn_lt_117, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_118 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_118>(&sc_dt::operator<)), Fn_lt_118>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_118(Sources &s)
{
  return invoke<Fn_lt_118, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_119 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_119>(&sc_dt::operator<)), Fn_lt_119>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_119(Sources &s)
{
  return invoke<Fn_lt_119, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_120 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_120>(&sc_dt::operator<)), Fn_lt_120>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_120(Sources &s)
{
  return invoke<Fn_lt_120, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_121 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_121>(&sc_dt::operator<)), Fn_lt_121>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_121(Sources &s)
{
  return invoke<Fn_lt_121, const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &>(&sc_dt::operator<, s);
}
using Fn_lt_122 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_122>(&sc_dt::operator<)), Fn_lt_122>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_122(Sources &s)
{
  return invoke<Fn_lt_122, const sc_dt::sc_unsigned &, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_123 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_123>(&sc_dt::operator<)), Fn_lt_123>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_123(Sources &s)
{
  return invoke<Fn_lt_123, const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &>(&sc_dt::operator<, s);
}
using Fn_lt_124 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_124>(&sc_dt::operator<)), Fn_lt_124>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_124(Sources &s)
{
  return invoke<Fn_lt_124, const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_125 = bool (*)(const sc_dt::sc_unsigned &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_125>(&sc_dt::operator<)), Fn_lt_125>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_125(Sources &s)
{
  return invoke<Fn_lt_125, const sc_dt::sc_unsigned &, int>(&sc_dt::operator<, s);
}
using Fn_lt_126 = bool (*)(const sc_dt::sc_unsigned &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_126>(&sc_dt::operator<)), Fn_lt_126>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_126(Sources &s)
{
  return invoke<Fn_lt_126, const sc_dt::sc_unsigned &, long>(&sc_dt::operator<, s);
}
using Fn_lt_127 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_127>(&sc_dt::operator<)), Fn_lt_127>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_127(Sources &s)
{
  return invoke<Fn_lt_127, const sc_dt::sc_unsigned &, sc_dt::int64>(&sc_dt::operator<, s);
}
using Fn_lt_128 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_128>(&sc_dt::operator<)), Fn_lt_128>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_128(Sources &s)
{
  return invoke<Fn_lt_128, const sc_dt::sc_unsigned &, sc_dt::uint64>(&sc_dt::operator<, s);
}
using Fn_lt_129 = bool (*)(const sc_dt::sc_unsigned &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_129>(&sc_dt::operator<)), Fn_lt_129>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_129(Sources &s)
{
  return invoke<Fn_lt_129, const sc_dt::sc_unsigned &, unsigned int>(&sc_dt::operator<, s);
}
using Fn_lt_130 = bool (*)(const sc_dt::sc_unsigned &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_130>(&sc_dt::operator<)), Fn_lt_130>,
              "exact overload sc_dt::operator< bool (const sc_dt::sc_unsigned &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_130(Sources &s)
{
  return invoke<Fn_lt_130, const sc_dt::sc_unsigned &, unsigned long>(&sc_dt::operator<, s);
}
using Fn_lt_131 = bool (*)(double, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_131>(&sc_dt::operator<)), Fn_lt_131>,
              "exact overload sc_dt::operator< bool (double, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_131(Sources &s)
{
  return invoke<Fn_lt_131, double, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_132 = bool (*)(double, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_132>(&sc_dt::operator<)), Fn_lt_132>,
              "exact overload sc_dt::operator< bool (double, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_132(Sources &s)
{
  return invoke<Fn_lt_132, double, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_133 = bool (*)(double, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_133>(&sc_dt::operator<)), Fn_lt_133>,
              "exact overload sc_dt::operator< bool (double, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_133(Sources &s)
{
  return invoke<Fn_lt_133, double, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_134 = bool (*)(double, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_134>(&sc_dt::operator<)), Fn_lt_134>,
              "exact overload sc_dt::operator< bool (double, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_134(Sources &s)
{
  return invoke<Fn_lt_134, double, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_135 = bool (*)(float, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_135>(&sc_dt::operator<)), Fn_lt_135>,
              "exact overload sc_dt::operator< bool (float, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_135(Sources &s)
{
  return invoke<Fn_lt_135, float, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_136 = bool (*)(float, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_136>(&sc_dt::operator<)), Fn_lt_136>,
              "exact overload sc_dt::operator< bool (float, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_136(Sources &s)
{
  return invoke<Fn_lt_136, float, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_137 = bool (*)(float, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_137>(&sc_dt::operator<)), Fn_lt_137>,
              "exact overload sc_dt::operator< bool (float, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_137(Sources &s)
{
  return invoke<Fn_lt_137, float, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_138 = bool (*)(float, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_138>(&sc_dt::operator<)), Fn_lt_138>,
              "exact overload sc_dt::operator< bool (float, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_138(Sources &s)
{
  return invoke<Fn_lt_138, float, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_139 = bool (*)(int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_139>(&sc_dt::operator<)), Fn_lt_139>,
              "exact overload sc_dt::operator< bool (int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_139(Sources &s)
{
  return invoke<Fn_lt_139, int, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_140 = bool (*)(int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_140>(&sc_dt::operator<)), Fn_lt_140>,
              "exact overload sc_dt::operator< bool (int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_140(Sources &s)
{
  return invoke<Fn_lt_140, int, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_141 = bool (*)(int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_141>(&sc_dt::operator<)), Fn_lt_141>,
              "exact overload sc_dt::operator< bool (int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_141(Sources &s)
{
  return invoke<Fn_lt_141, int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_142 = bool (*)(int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_142>(&sc_dt::operator<)), Fn_lt_142>,
              "exact overload sc_dt::operator< bool (int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_142(Sources &s)
{
  return invoke<Fn_lt_142, int, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_143 = bool (*)(int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_143>(&sc_dt::operator<)), Fn_lt_143>,
              "exact overload sc_dt::operator< bool (int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_143(Sources &s)
{
  return invoke<Fn_lt_143, int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_144 = bool (*)(int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_144>(&sc_dt::operator<)), Fn_lt_144>,
              "exact overload sc_dt::operator< bool (int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_144(Sources &s)
{
  return invoke<Fn_lt_144, int, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_145 = bool (*)(int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_145>(&sc_dt::operator<)), Fn_lt_145>,
              "exact overload sc_dt::operator< bool (int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_145(Sources &s)
{
  return invoke<Fn_lt_145, int, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_146 = bool (*)(long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_146>(&sc_dt::operator<)), Fn_lt_146>,
              "exact overload sc_dt::operator< bool (long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_146(Sources &s)
{
  return invoke<Fn_lt_146, long, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_147 = bool (*)(long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_147>(&sc_dt::operator<)), Fn_lt_147>,
              "exact overload sc_dt::operator< bool (long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_147(Sources &s)
{
  return invoke<Fn_lt_147, long, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_148 = bool (*)(long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_148>(&sc_dt::operator<)), Fn_lt_148>,
              "exact overload sc_dt::operator< bool (long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_148(Sources &s)
{
  return invoke<Fn_lt_148, long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_149 = bool (*)(long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_149>(&sc_dt::operator<)), Fn_lt_149>,
              "exact overload sc_dt::operator< bool (long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_149(Sources &s)
{
  return invoke<Fn_lt_149, long, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_150 = bool (*)(long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_150>(&sc_dt::operator<)), Fn_lt_150>,
              "exact overload sc_dt::operator< bool (long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_150(Sources &s)
{
  return invoke<Fn_lt_150, long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_151 = bool (*)(long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_151>(&sc_dt::operator<)), Fn_lt_151>,
              "exact overload sc_dt::operator< bool (long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_151(Sources &s)
{
  return invoke<Fn_lt_151, long, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_152 = bool (*)(long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_152>(&sc_dt::operator<)), Fn_lt_152>,
              "exact overload sc_dt::operator< bool (long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_152(Sources &s)
{
  return invoke<Fn_lt_152, long, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_153 = bool (*)(sc_dt::int64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_153>(&sc_dt::operator<)), Fn_lt_153>,
              "exact overload sc_dt::operator< bool (sc_dt::int64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_153(Sources &s)
{
  return invoke<Fn_lt_153, sc_dt::int64, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_154 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_154>(&sc_dt::operator<)), Fn_lt_154>,
              "exact overload sc_dt::operator< bool (sc_dt::int64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_154(Sources &s)
{
  return invoke<Fn_lt_154, sc_dt::int64, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_155 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_155>(&sc_dt::operator<)), Fn_lt_155>,
              "exact overload sc_dt::operator< bool (sc_dt::int64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_155(Sources &s)
{
  return invoke<Fn_lt_155, sc_dt::int64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_156 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_156>(&sc_dt::operator<)), Fn_lt_156>,
              "exact overload sc_dt::operator< bool (sc_dt::int64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_156(Sources &s)
{
  return invoke<Fn_lt_156, sc_dt::int64, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_157 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_157>(&sc_dt::operator<)), Fn_lt_157>,
              "exact overload sc_dt::operator< bool (sc_dt::int64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_157(Sources &s)
{
  return invoke<Fn_lt_157, sc_dt::int64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_158 = bool (*)(sc_dt::int64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_158>(&sc_dt::operator<)), Fn_lt_158>,
              "exact overload sc_dt::operator< bool (sc_dt::int64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_158(Sources &s)
{
  return invoke<Fn_lt_158, sc_dt::int64, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_159 = bool (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_159>(&sc_dt::operator<)), Fn_lt_159>,
              "exact overload sc_dt::operator< bool (sc_dt::int64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_159(Sources &s)
{
  return invoke<Fn_lt_159, sc_dt::int64, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_160 = bool (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_160>(&sc_dt::operator<)), Fn_lt_160>,
              "exact overload sc_dt::operator< bool (sc_dt::uint64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_160(Sources &s)
{
  return invoke<Fn_lt_160, sc_dt::uint64, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_161 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_161>(&sc_dt::operator<)), Fn_lt_161>,
              "exact overload sc_dt::operator< bool (sc_dt::uint64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_161(Sources &s)
{
  return invoke<Fn_lt_161, sc_dt::uint64, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_162 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_162>(&sc_dt::operator<)), Fn_lt_162>,
              "exact overload sc_dt::operator< bool (sc_dt::uint64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_162(Sources &s)
{
  return invoke<Fn_lt_162, sc_dt::uint64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_163 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_163>(&sc_dt::operator<)), Fn_lt_163>,
              "exact overload sc_dt::operator< bool (sc_dt::uint64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_163(Sources &s)
{
  return invoke<Fn_lt_163, sc_dt::uint64, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_164 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_164>(&sc_dt::operator<)), Fn_lt_164>,
              "exact overload sc_dt::operator< bool (sc_dt::uint64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_164(Sources &s)
{
  return invoke<Fn_lt_164, sc_dt::uint64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_165 = bool (*)(sc_dt::uint64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_165>(&sc_dt::operator<)), Fn_lt_165>,
              "exact overload sc_dt::operator< bool (sc_dt::uint64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_165(Sources &s)
{
  return invoke<Fn_lt_165, sc_dt::uint64, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_166 = bool (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_166>(&sc_dt::operator<)), Fn_lt_166>,
              "exact overload sc_dt::operator< bool (sc_dt::uint64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_166(Sources &s)
{
  return invoke<Fn_lt_166, sc_dt::uint64, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_167 = bool (*)(unsigned int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_167>(&sc_dt::operator<)), Fn_lt_167>,
              "exact overload sc_dt::operator< bool (unsigned int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_167(Sources &s)
{
  return invoke<Fn_lt_167, unsigned int, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_168 = bool (*)(unsigned int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_168>(&sc_dt::operator<)), Fn_lt_168>,
              "exact overload sc_dt::operator< bool (unsigned int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_168(Sources &s)
{
  return invoke<Fn_lt_168, unsigned int, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_169 = bool (*)(unsigned int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_169>(&sc_dt::operator<)), Fn_lt_169>,
              "exact overload sc_dt::operator< bool (unsigned int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_169(Sources &s)
{
  return invoke<Fn_lt_169, unsigned int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_170 = bool (*)(unsigned int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_170>(&sc_dt::operator<)), Fn_lt_170>,
              "exact overload sc_dt::operator< bool (unsigned int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_170(Sources &s)
{
  return invoke<Fn_lt_170, unsigned int, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_171 = bool (*)(unsigned int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_171>(&sc_dt::operator<)), Fn_lt_171>,
              "exact overload sc_dt::operator< bool (unsigned int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_171(Sources &s)
{
  return invoke<Fn_lt_171, unsigned int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_172 = bool (*)(unsigned int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_172>(&sc_dt::operator<)), Fn_lt_172>,
              "exact overload sc_dt::operator< bool (unsigned int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_172(Sources &s)
{
  return invoke<Fn_lt_172, unsigned int, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_173 = bool (*)(unsigned int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_173>(&sc_dt::operator<)), Fn_lt_173>,
              "exact overload sc_dt::operator< bool (unsigned int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_173(Sources &s)
{
  return invoke<Fn_lt_173, unsigned int, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
using Fn_lt_174 = bool (*)(unsigned long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_174>(&sc_dt::operator<)), Fn_lt_174>,
              "exact overload sc_dt::operator< bool (unsigned long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_174(Sources &s)
{
  return invoke<Fn_lt_174, unsigned long, const sc_dt::sc_concatref &>(&sc_dt::operator<, s);
}
using Fn_lt_175 = bool (*)(unsigned long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_175>(&sc_dt::operator<)), Fn_lt_175>,
              "exact overload sc_dt::operator< bool (unsigned long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_175(Sources &s)
{
  return invoke<Fn_lt_175, unsigned long, const sc_dt::sc_fxnum &>(&sc_dt::operator<, s);
}
using Fn_lt_176 = bool (*)(unsigned long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_176>(&sc_dt::operator<)), Fn_lt_176>,
              "exact overload sc_dt::operator< bool (unsigned long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_176(Sources &s)
{
  return invoke<Fn_lt_176, unsigned long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_177 = bool (*)(unsigned long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_177>(&sc_dt::operator<)), Fn_lt_177>,
              "exact overload sc_dt::operator< bool (unsigned long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_177(Sources &s)
{
  return invoke<Fn_lt_177, unsigned long, const sc_dt::sc_fxval &>(&sc_dt::operator<, s);
}
using Fn_lt_178 = bool (*)(unsigned long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_178>(&sc_dt::operator<)), Fn_lt_178>,
              "exact overload sc_dt::operator< bool (unsigned long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_178(Sources &s)
{
  return invoke<Fn_lt_178, unsigned long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<, s);
}
using Fn_lt_179 = bool (*)(unsigned long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_179>(&sc_dt::operator<)), Fn_lt_179>,
              "exact overload sc_dt::operator< bool (unsigned long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_179(Sources &s)
{
  return invoke<Fn_lt_179, unsigned long, const sc_dt::sc_signed &>(&sc_dt::operator<, s);
}
using Fn_lt_180 = bool (*)(unsigned long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_lt_180>(&sc_dt::operator<)), Fn_lt_180>,
              "exact overload sc_dt::operator< bool (unsigned long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt_180(Sources &s)
{
  return invoke<Fn_lt_180, unsigned long, const sc_dt::sc_unsigned &>(&sc_dt::operator<, s);
}
inline unsigned count_lt = 181;
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_lt(unsigned index, Sources &s)
{
  switch (index)
  {
  case 0:
    return case_lt_000(s);
  case 1:
    return case_lt_001(s);
  case 2:
    return case_lt_002(s);
  case 3:
    return case_lt_003(s);
  case 4:
    return case_lt_004(s);
  case 5:
    return case_lt_005(s);
  case 6:
    return case_lt_006(s);
  case 7:
    return case_lt_007(s);
  case 8:
    return case_lt_008(s);
  case 9:
    return case_lt_009(s);
  case 10:
    return case_lt_010(s);
  case 11:
    return case_lt_011(s);
  case 12:
    return case_lt_012(s);
  case 13:
    return case_lt_013(s);
  case 14:
    return case_lt_014(s);
  case 15:
    return case_lt_015(s);
  case 16:
    return case_lt_016(s);
  case 17:
    return case_lt_017(s);
  case 18:
    return case_lt_018(s);
  case 19:
    return case_lt_019(s);
  case 20:
    return case_lt_020(s);
  case 21:
    return case_lt_021(s);
  case 22:
    return case_lt_022(s);
  case 23:
    return case_lt_023(s);
  case 24:
    return case_lt_024(s);
  case 25:
    return case_lt_025(s);
  case 26:
    return case_lt_026(s);
  case 27:
    return case_lt_027(s);
  case 28:
    return case_lt_028(s);
  case 29:
    return case_lt_029(s);
  case 30:
    return case_lt_030(s);
  case 31:
    return case_lt_031(s);
  case 32:
    return case_lt_032(s);
  case 33:
    return case_lt_033(s);
  case 34:
    return case_lt_034(s);
  case 35:
    return case_lt_035(s);
  case 36:
    return case_lt_036(s);
  case 37:
    return case_lt_037(s);
  case 38:
    return case_lt_038(s);
  case 39:
    return case_lt_039(s);
  case 40:
    return case_lt_040(s);
  case 41:
    return case_lt_041(s);
  case 42:
    return case_lt_042(s);
  case 43:
    return case_lt_043(s);
  case 44:
    return case_lt_044(s);
  case 45:
    return case_lt_045(s);
  case 46:
    return case_lt_046(s);
  case 47:
    return case_lt_047(s);
  case 48:
    return case_lt_048(s);
  case 49:
    return case_lt_049(s);
  case 50:
    return case_lt_050(s);
  case 51:
    return case_lt_051(s);
  case 52:
    return case_lt_052(s);
  case 53:
    return case_lt_053(s);
  case 54:
    return case_lt_054(s);
  case 55:
    return case_lt_055(s);
  case 56:
    return case_lt_056(s);
  case 57:
    return case_lt_057(s);
  case 58:
    return case_lt_058(s);
  case 59:
    return case_lt_059(s);
  case 60:
    return case_lt_060(s);
  case 61:
    return case_lt_061(s);
  case 62:
    return case_lt_062(s);
  case 63:
    return case_lt_063(s);
  case 64:
    return case_lt_064(s);
  case 65:
    return case_lt_065(s);
  case 66:
    return case_lt_066(s);
  case 67:
    return case_lt_067(s);
  case 68:
    return case_lt_068(s);
  case 69:
    return case_lt_069(s);
  case 70:
    return case_lt_070(s);
  case 71:
    return case_lt_071(s);
  case 72:
    return case_lt_072(s);
  case 73:
    return case_lt_073(s);
  case 74:
    return case_lt_074(s);
  case 75:
    return case_lt_075(s);
  case 76:
    return case_lt_076(s);
  case 77:
    return case_lt_077(s);
  case 78:
    return case_lt_078(s);
  case 79:
    return case_lt_079(s);
  case 80:
    return case_lt_080(s);
  case 81:
    return case_lt_081(s);
  case 82:
    return case_lt_082(s);
  case 83:
    return case_lt_083(s);
  case 84:
    return case_lt_084(s);
  case 85:
    return case_lt_085(s);
  case 86:
    return case_lt_086(s);
  case 87:
    return case_lt_087(s);
  case 88:
    return case_lt_088(s);
  case 89:
    return case_lt_089(s);
  case 90:
    return case_lt_090(s);
  case 91:
    return case_lt_091(s);
  case 92:
    return case_lt_092(s);
  case 93:
    return case_lt_093(s);
  case 94:
    return case_lt_094(s);
  case 95:
    return case_lt_095(s);
  case 96:
    return case_lt_096(s);
  case 97:
    return case_lt_097(s);
  case 98:
    return case_lt_098(s);
  case 99:
    return case_lt_099(s);
  case 100:
    return case_lt_100(s);
  case 101:
    return case_lt_101(s);
  case 102:
    return case_lt_102(s);
  case 103:
    return case_lt_103(s);
  case 104:
    return case_lt_104(s);
  case 105:
    return case_lt_105(s);
  case 106:
    return case_lt_106(s);
  case 107:
    return case_lt_107(s);
  case 108:
    return case_lt_108(s);
  case 109:
    return case_lt_109(s);
  case 110:
    return case_lt_110(s);
  case 111:
    return case_lt_111(s);
  case 112:
    return case_lt_112(s);
  case 113:
    return case_lt_113(s);
  case 114:
    return case_lt_114(s);
  case 115:
    return case_lt_115(s);
  case 116:
    return case_lt_116(s);
  case 117:
    return case_lt_117(s);
  case 118:
    return case_lt_118(s);
  case 119:
    return case_lt_119(s);
  case 120:
    return case_lt_120(s);
  case 121:
    return case_lt_121(s);
  case 122:
    return case_lt_122(s);
  case 123:
    return case_lt_123(s);
  case 124:
    return case_lt_124(s);
  case 125:
    return case_lt_125(s);
  case 126:
    return case_lt_126(s);
  case 127:
    return case_lt_127(s);
  case 128:
    return case_lt_128(s);
  case 129:
    return case_lt_129(s);
  case 130:
    return case_lt_130(s);
  case 131:
    return case_lt_131(s);
  case 132:
    return case_lt_132(s);
  case 133:
    return case_lt_133(s);
  case 134:
    return case_lt_134(s);
  case 135:
    return case_lt_135(s);
  case 136:
    return case_lt_136(s);
  case 137:
    return case_lt_137(s);
  case 138:
    return case_lt_138(s);
  case 139:
    return case_lt_139(s);
  case 140:
    return case_lt_140(s);
  case 141:
    return case_lt_141(s);
  case 142:
    return case_lt_142(s);
  case 143:
    return case_lt_143(s);
  case 144:
    return case_lt_144(s);
  case 145:
    return case_lt_145(s);
  case 146:
    return case_lt_146(s);
  case 147:
    return case_lt_147(s);
  case 148:
    return case_lt_148(s);
  case 149:
    return case_lt_149(s);
  case 150:
    return case_lt_150(s);
  case 151:
    return case_lt_151(s);
  case 152:
    return case_lt_152(s);
  case 153:
    return case_lt_153(s);
  case 154:
    return case_lt_154(s);
  case 155:
    return case_lt_155(s);
  case 156:
    return case_lt_156(s);
  case 157:
    return case_lt_157(s);
  case 158:
    return case_lt_158(s);
  case 159:
    return case_lt_159(s);
  case 160:
    return case_lt_160(s);
  case 161:
    return case_lt_161(s);
  case 162:
    return case_lt_162(s);
  case 163:
    return case_lt_163(s);
  case 164:
    return case_lt_164(s);
  case 165:
    return case_lt_165(s);
  case 166:
    return case_lt_166(s);
  case 167:
    return case_lt_167(s);
  case 168:
    return case_lt_168(s);
  case 169:
    return case_lt_169(s);
  case 170:
    return case_lt_170(s);
  case 171:
    return case_lt_171(s);
  case 172:
    return case_lt_172(s);
  case 173:
    return case_lt_173(s);
  case 174:
    return case_lt_174(s);
  case 175:
    return case_lt_175(s);
  case 176:
    return case_lt_176(s);
  case 177:
    return case_lt_177(s);
  case 178:
    return case_lt_178(s);
  case 179:
    return case_lt_179(s);
  case 180:
    return case_lt_180(s);
  default:
    throw std::out_of_range("unknown comparison case index");
  }
}
using Fn_le_000 = bool (*)(bool, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_000>(&sc_dt::operator<=)), Fn_le_000>,
              "exact overload sc_dt::operator<= bool (bool, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_000(Sources &s)
{
  return invoke<Fn_le_000, bool, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_001 = bool (*)(const char *, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_001>(&sc_dt::operator<=)), Fn_le_001>,
              "exact overload sc_dt::operator<= bool (const char *, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_001(Sources &s)
{
  return invoke<Fn_le_001, const char *, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_002 = bool (*)(const char *, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_002>(&sc_dt::operator<=)), Fn_le_002>,
              "exact overload sc_dt::operator<= bool (const char *, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_002(Sources &s)
{
  return invoke<Fn_le_002, const char *, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_003 = bool (*)(const char *, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_003>(&sc_dt::operator<=)), Fn_le_003>,
              "exact overload sc_dt::operator<= bool (const char *, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_003(Sources &s)
{
  return invoke<Fn_le_003, const char *, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_004 = bool (*)(const char *, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_004>(&sc_dt::operator<=)), Fn_le_004>,
              "exact overload sc_dt::operator<= bool (const char *, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_004(Sources &s)
{
  return invoke<Fn_le_004, const char *, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_005 = bool (*)(const sc_dt::sc_concatref &, bool);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_005>(&sc_dt::operator<=)), Fn_le_005>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, bool)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_005(Sources &s)
{
  return invoke<Fn_le_005, const sc_dt::sc_concatref &, bool>(&sc_dt::operator<=, s);
}
using Fn_le_006 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_006>(&sc_dt::operator<=)), Fn_le_006>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_006(Sources &s)
{
  return invoke<Fn_le_006, const sc_dt::sc_concatref &, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_007 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_007>(&sc_dt::operator<=)), Fn_le_007>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_007(Sources &s)
{
  return invoke<Fn_le_007, const sc_dt::sc_concatref &, const sc_dt::sc_int_base &>(&sc_dt::operator<=, s);
}
using Fn_le_008 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_008>(&sc_dt::operator<=)), Fn_le_008>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_008(Sources &s)
{
  return invoke<Fn_le_008, const sc_dt::sc_concatref &, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_009 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_009>(&sc_dt::operator<=)), Fn_le_009>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_009(Sources &s)
{
  return invoke<Fn_le_009, const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &>(&sc_dt::operator<=, s);
}
using Fn_le_010 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_010>(&sc_dt::operator<=)), Fn_le_010>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_010(Sources &s)
{
  return invoke<Fn_le_010, const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_011 = bool (*)(const sc_dt::sc_concatref &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_011>(&sc_dt::operator<=)), Fn_le_011>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_011(Sources &s)
{
  return invoke<Fn_le_011, const sc_dt::sc_concatref &, int>(&sc_dt::operator<=, s);
}
using Fn_le_012 = bool (*)(const sc_dt::sc_concatref &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_012>(&sc_dt::operator<=)), Fn_le_012>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_012(Sources &s)
{
  return invoke<Fn_le_012, const sc_dt::sc_concatref &, long>(&sc_dt::operator<=, s);
}
using Fn_le_013 = bool (*)(const sc_dt::sc_concatref &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_013>(&sc_dt::operator<=)), Fn_le_013>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_013(Sources &s)
{
  return invoke<Fn_le_013, const sc_dt::sc_concatref &, sc_dt::int64>(&sc_dt::operator<=, s);
}
using Fn_le_014 = bool (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_014>(&sc_dt::operator<=)), Fn_le_014>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_014(Sources &s)
{
  return invoke<Fn_le_014, const sc_dt::sc_concatref &, sc_dt::uint64>(&sc_dt::operator<=, s);
}
using Fn_le_015 = bool (*)(const sc_dt::sc_concatref &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_015>(&sc_dt::operator<=)), Fn_le_015>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_015(Sources &s)
{
  return invoke<Fn_le_015, const sc_dt::sc_concatref &, unsigned int>(&sc_dt::operator<=, s);
}
using Fn_le_016 = bool (*)(const sc_dt::sc_concatref &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_016>(&sc_dt::operator<=)), Fn_le_016>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_concatref &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_016(Sources &s)
{
  return invoke<Fn_le_016, const sc_dt::sc_concatref &, unsigned long>(&sc_dt::operator<=, s);
}
using Fn_le_017 = bool (*)(const sc_dt::sc_fxnum &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_017>(&sc_dt::operator<=)), Fn_le_017>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_017(Sources &s)
{
  return invoke<Fn_le_017, const sc_dt::sc_fxnum &, const char *>(&sc_dt::operator<=, s);
}
using Fn_le_018 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_018>(&sc_dt::operator<=)), Fn_le_018>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_018(Sources &s)
{
  return invoke<Fn_le_018, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_019 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_019>(&sc_dt::operator<=)), Fn_le_019>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_019(Sources &s)
{
  return invoke<Fn_le_019, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_020 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_020>(&sc_dt::operator<=)), Fn_le_020>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_020(Sources &s)
{
  return invoke<Fn_le_020, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_021 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_021>(&sc_dt::operator<=)), Fn_le_021>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_021(Sources &s)
{
  return invoke<Fn_le_021, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_022 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_022>(&sc_dt::operator<=)), Fn_le_022>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_022(Sources &s)
{
  return invoke<Fn_le_022, const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &>(&sc_dt::operator<=, s);
}
using Fn_le_023 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_023>(&sc_dt::operator<=)), Fn_le_023>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_023(Sources &s)
{
  return invoke<Fn_le_023, const sc_dt::sc_fxnum &, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_024 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_024>(&sc_dt::operator<=)), Fn_le_024>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_024(Sources &s)
{
  return invoke<Fn_le_024, const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &>(&sc_dt::operator<=, s);
}
using Fn_le_025 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_025>(&sc_dt::operator<=)), Fn_le_025>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_025(Sources &s)
{
  return invoke<Fn_le_025, const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_026 = bool (*)(const sc_dt::sc_fxnum &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_026>(&sc_dt::operator<=)), Fn_le_026>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_026(Sources &s)
{
  return invoke<Fn_le_026, const sc_dt::sc_fxnum &, double>(&sc_dt::operator<=, s);
}
using Fn_le_027 = bool (*)(const sc_dt::sc_fxnum &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_027>(&sc_dt::operator<=)), Fn_le_027>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_027(Sources &s)
{
  return invoke<Fn_le_027, const sc_dt::sc_fxnum &, float>(&sc_dt::operator<=, s);
}
using Fn_le_028 = bool (*)(const sc_dt::sc_fxnum &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_028>(&sc_dt::operator<=)), Fn_le_028>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_028(Sources &s)
{
  return invoke<Fn_le_028, const sc_dt::sc_fxnum &, int>(&sc_dt::operator<=, s);
}
using Fn_le_029 = bool (*)(const sc_dt::sc_fxnum &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_029>(&sc_dt::operator<=)), Fn_le_029>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_029(Sources &s)
{
  return invoke<Fn_le_029, const sc_dt::sc_fxnum &, long>(&sc_dt::operator<=, s);
}
using Fn_le_030 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_030>(&sc_dt::operator<=)), Fn_le_030>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_030(Sources &s)
{
  return invoke<Fn_le_030, const sc_dt::sc_fxnum &, sc_dt::int64>(&sc_dt::operator<=, s);
}
using Fn_le_031 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_031>(&sc_dt::operator<=)), Fn_le_031>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_031(Sources &s)
{
  return invoke<Fn_le_031, const sc_dt::sc_fxnum &, sc_dt::uint64>(&sc_dt::operator<=, s);
}
using Fn_le_032 = bool (*)(const sc_dt::sc_fxnum &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_032>(&sc_dt::operator<=)), Fn_le_032>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_032(Sources &s)
{
  return invoke<Fn_le_032, const sc_dt::sc_fxnum &, unsigned int>(&sc_dt::operator<=, s);
}
using Fn_le_033 = bool (*)(const sc_dt::sc_fxnum &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_033>(&sc_dt::operator<=)), Fn_le_033>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_033(Sources &s)
{
  return invoke<Fn_le_033, const sc_dt::sc_fxnum &, unsigned long>(&sc_dt::operator<=, s);
}
using Fn_le_034 = bool (*)(const sc_dt::sc_fxnum_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_034>(&sc_dt::operator<=)), Fn_le_034>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_034(Sources &s)
{
  return invoke<Fn_le_034, const sc_dt::sc_fxnum_fast &, const char *>(&sc_dt::operator<=, s);
}
using Fn_le_035 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_035>(&sc_dt::operator<=)), Fn_le_035>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_035(Sources &s)
{
  return invoke<Fn_le_035, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_036 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_036>(&sc_dt::operator<=)), Fn_le_036>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_036(Sources &s)
{
  return invoke<Fn_le_036, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_037 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_037>(&sc_dt::operator<=)), Fn_le_037>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_037(Sources &s)
{
  return invoke<Fn_le_037, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_038 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_038>(&sc_dt::operator<=)), Fn_le_038>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_038(Sources &s)
{
  return invoke<Fn_le_038, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_039 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_039>(&sc_dt::operator<=)), Fn_le_039>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_039(Sources &s)
{
  return invoke<Fn_le_039, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator<=, s);
}
using Fn_le_040 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_040>(&sc_dt::operator<=)), Fn_le_040>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_040(Sources &s)
{
  return invoke<Fn_le_040, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_041 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_041>(&sc_dt::operator<=)), Fn_le_041>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_041(Sources &s)
{
  return invoke<Fn_le_041, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator<=, s);
}
using Fn_le_042 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_042>(&sc_dt::operator<=)), Fn_le_042>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_042(Sources &s)
{
  return invoke<Fn_le_042, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_043 = bool (*)(const sc_dt::sc_fxnum_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_043>(&sc_dt::operator<=)), Fn_le_043>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_043(Sources &s)
{
  return invoke<Fn_le_043, const sc_dt::sc_fxnum_fast &, double>(&sc_dt::operator<=, s);
}
using Fn_le_044 = bool (*)(const sc_dt::sc_fxnum_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_044>(&sc_dt::operator<=)), Fn_le_044>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_044(Sources &s)
{
  return invoke<Fn_le_044, const sc_dt::sc_fxnum_fast &, float>(&sc_dt::operator<=, s);
}
using Fn_le_045 = bool (*)(const sc_dt::sc_fxnum_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_045>(&sc_dt::operator<=)), Fn_le_045>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_045(Sources &s)
{
  return invoke<Fn_le_045, const sc_dt::sc_fxnum_fast &, int>(&sc_dt::operator<=, s);
}
using Fn_le_046 = bool (*)(const sc_dt::sc_fxnum_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_046>(&sc_dt::operator<=)), Fn_le_046>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_046(Sources &s)
{
  return invoke<Fn_le_046, const sc_dt::sc_fxnum_fast &, long>(&sc_dt::operator<=, s);
}
using Fn_le_047 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_047>(&sc_dt::operator<=)), Fn_le_047>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_047(Sources &s)
{
  return invoke<Fn_le_047, const sc_dt::sc_fxnum_fast &, sc_dt::int64>(&sc_dt::operator<=, s);
}
using Fn_le_048 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_048>(&sc_dt::operator<=)), Fn_le_048>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_048(Sources &s)
{
  return invoke<Fn_le_048, const sc_dt::sc_fxnum_fast &, sc_dt::uint64>(&sc_dt::operator<=, s);
}
using Fn_le_049 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_049>(&sc_dt::operator<=)), Fn_le_049>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_049(Sources &s)
{
  return invoke<Fn_le_049, const sc_dt::sc_fxnum_fast &, unsigned int>(&sc_dt::operator<=, s);
}
using Fn_le_050 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_050>(&sc_dt::operator<=)), Fn_le_050>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxnum_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_050(Sources &s)
{
  return invoke<Fn_le_050, const sc_dt::sc_fxnum_fast &, unsigned long>(&sc_dt::operator<=, s);
}
using Fn_le_051 = bool (*)(const sc_dt::sc_fxval &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_051>(&sc_dt::operator<=)), Fn_le_051>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_051(Sources &s)
{
  return invoke<Fn_le_051, const sc_dt::sc_fxval &, const char *>(&sc_dt::operator<=, s);
}
using Fn_le_052 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_052>(&sc_dt::operator<=)), Fn_le_052>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_052(Sources &s)
{
  return invoke<Fn_le_052, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_053 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_053>(&sc_dt::operator<=)), Fn_le_053>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_053(Sources &s)
{
  return invoke<Fn_le_053, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_054 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_054>(&sc_dt::operator<=)), Fn_le_054>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_054(Sources &s)
{
  return invoke<Fn_le_054, const sc_dt::sc_fxval &, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_055 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_055>(&sc_dt::operator<=)), Fn_le_055>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_055(Sources &s)
{
  return invoke<Fn_le_055, const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_056 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_056>(&sc_dt::operator<=)), Fn_le_056>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_056(Sources &s)
{
  return invoke<Fn_le_056, const sc_dt::sc_fxval &, const sc_dt::sc_int_base &>(&sc_dt::operator<=, s);
}
using Fn_le_057 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_057>(&sc_dt::operator<=)), Fn_le_057>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_057(Sources &s)
{
  return invoke<Fn_le_057, const sc_dt::sc_fxval &, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_058 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_058>(&sc_dt::operator<=)), Fn_le_058>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_058(Sources &s)
{
  return invoke<Fn_le_058, const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &>(&sc_dt::operator<=, s);
}
using Fn_le_059 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_059>(&sc_dt::operator<=)), Fn_le_059>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_059(Sources &s)
{
  return invoke<Fn_le_059, const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_060 = bool (*)(const sc_dt::sc_fxval &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_060>(&sc_dt::operator<=)), Fn_le_060>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_060(Sources &s)
{
  return invoke<Fn_le_060, const sc_dt::sc_fxval &, double>(&sc_dt::operator<=, s);
}
using Fn_le_061 = bool (*)(const sc_dt::sc_fxval &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_061>(&sc_dt::operator<=)), Fn_le_061>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_061(Sources &s)
{
  return invoke<Fn_le_061, const sc_dt::sc_fxval &, float>(&sc_dt::operator<=, s);
}
using Fn_le_062 = bool (*)(const sc_dt::sc_fxval &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_062>(&sc_dt::operator<=)), Fn_le_062>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_062(Sources &s)
{
  return invoke<Fn_le_062, const sc_dt::sc_fxval &, int>(&sc_dt::operator<=, s);
}
using Fn_le_063 = bool (*)(const sc_dt::sc_fxval &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_063>(&sc_dt::operator<=)), Fn_le_063>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_063(Sources &s)
{
  return invoke<Fn_le_063, const sc_dt::sc_fxval &, long>(&sc_dt::operator<=, s);
}
using Fn_le_064 = bool (*)(const sc_dt::sc_fxval &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_064>(&sc_dt::operator<=)), Fn_le_064>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_064(Sources &s)
{
  return invoke<Fn_le_064, const sc_dt::sc_fxval &, sc_dt::int64>(&sc_dt::operator<=, s);
}
using Fn_le_065 = bool (*)(const sc_dt::sc_fxval &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_065>(&sc_dt::operator<=)), Fn_le_065>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_065(Sources &s)
{
  return invoke<Fn_le_065, const sc_dt::sc_fxval &, sc_dt::uint64>(&sc_dt::operator<=, s);
}
using Fn_le_066 = bool (*)(const sc_dt::sc_fxval &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_066>(&sc_dt::operator<=)), Fn_le_066>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_066(Sources &s)
{
  return invoke<Fn_le_066, const sc_dt::sc_fxval &, unsigned int>(&sc_dt::operator<=, s);
}
using Fn_le_067 = bool (*)(const sc_dt::sc_fxval &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_067>(&sc_dt::operator<=)), Fn_le_067>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_067(Sources &s)
{
  return invoke<Fn_le_067, const sc_dt::sc_fxval &, unsigned long>(&sc_dt::operator<=, s);
}
using Fn_le_068 = bool (*)(const sc_dt::sc_fxval_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_068>(&sc_dt::operator<=)), Fn_le_068>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_068(Sources &s)
{
  return invoke<Fn_le_068, const sc_dt::sc_fxval_fast &, const char *>(&sc_dt::operator<=, s);
}
using Fn_le_069 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_069>(&sc_dt::operator<=)), Fn_le_069>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_069(Sources &s)
{
  return invoke<Fn_le_069, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_070 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_070>(&sc_dt::operator<=)), Fn_le_070>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_070(Sources &s)
{
  return invoke<Fn_le_070, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_071 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_071>(&sc_dt::operator<=)), Fn_le_071>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_071(Sources &s)
{
  return invoke<Fn_le_071, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_072 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_072>(&sc_dt::operator<=)), Fn_le_072>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_072(Sources &s)
{
  return invoke<Fn_le_072, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_073 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_073>(&sc_dt::operator<=)), Fn_le_073>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_073(Sources &s)
{
  return invoke<Fn_le_073, const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator<=, s);
}
using Fn_le_074 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_074>(&sc_dt::operator<=)), Fn_le_074>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_074(Sources &s)
{
  return invoke<Fn_le_074, const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_075 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_075>(&sc_dt::operator<=)), Fn_le_075>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_075(Sources &s)
{
  return invoke<Fn_le_075, const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator<=, s);
}
using Fn_le_076 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_076>(&sc_dt::operator<=)), Fn_le_076>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_076(Sources &s)
{
  return invoke<Fn_le_076, const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_077 = bool (*)(const sc_dt::sc_fxval_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_077>(&sc_dt::operator<=)), Fn_le_077>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_077(Sources &s)
{
  return invoke<Fn_le_077, const sc_dt::sc_fxval_fast &, double>(&sc_dt::operator<=, s);
}
using Fn_le_078 = bool (*)(const sc_dt::sc_fxval_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_078>(&sc_dt::operator<=)), Fn_le_078>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_078(Sources &s)
{
  return invoke<Fn_le_078, const sc_dt::sc_fxval_fast &, float>(&sc_dt::operator<=, s);
}
using Fn_le_079 = bool (*)(const sc_dt::sc_fxval_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_079>(&sc_dt::operator<=)), Fn_le_079>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_079(Sources &s)
{
  return invoke<Fn_le_079, const sc_dt::sc_fxval_fast &, int>(&sc_dt::operator<=, s);
}
using Fn_le_080 = bool (*)(const sc_dt::sc_fxval_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_080>(&sc_dt::operator<=)), Fn_le_080>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_080(Sources &s)
{
  return invoke<Fn_le_080, const sc_dt::sc_fxval_fast &, long>(&sc_dt::operator<=, s);
}
using Fn_le_081 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_081>(&sc_dt::operator<=)), Fn_le_081>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_081(Sources &s)
{
  return invoke<Fn_le_081, const sc_dt::sc_fxval_fast &, sc_dt::int64>(&sc_dt::operator<=, s);
}
using Fn_le_082 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_082>(&sc_dt::operator<=)), Fn_le_082>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_082(Sources &s)
{
  return invoke<Fn_le_082, const sc_dt::sc_fxval_fast &, sc_dt::uint64>(&sc_dt::operator<=, s);
}
using Fn_le_083 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_083>(&sc_dt::operator<=)), Fn_le_083>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_083(Sources &s)
{
  return invoke<Fn_le_083, const sc_dt::sc_fxval_fast &, unsigned int>(&sc_dt::operator<=, s);
}
using Fn_le_084 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_084>(&sc_dt::operator<=)), Fn_le_084>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_fxval_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_084(Sources &s)
{
  return invoke<Fn_le_084, const sc_dt::sc_fxval_fast &, unsigned long>(&sc_dt::operator<=, s);
}
using Fn_le_085 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_085>(&sc_dt::operator<=)), Fn_le_085>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_085(Sources &s)
{
  return invoke<Fn_le_085, const sc_dt::sc_int_base &, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_086 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_086>(&sc_dt::operator<=)), Fn_le_086>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_086(Sources &s)
{
  return invoke<Fn_le_086, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_087 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_087>(&sc_dt::operator<=)), Fn_le_087>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_087(Sources &s)
{
  return invoke<Fn_le_087, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_088 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_088>(&sc_dt::operator<=)), Fn_le_088>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_088(Sources &s)
{
  return invoke<Fn_le_088, const sc_dt::sc_int_base &, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_089 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_089>(&sc_dt::operator<=)), Fn_le_089>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_089(Sources &s)
{
  return invoke<Fn_le_089, const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_090 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_090>(&sc_dt::operator<=)), Fn_le_090>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_int_base &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_090(Sources &s)
{
  return invoke<Fn_le_090, const sc_dt::sc_int_base &, const sc_dt::sc_int_base &>(&sc_dt::operator<=, s);
}
using Fn_le_091 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_091>(&sc_dt::operator<=)), Fn_le_091>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_091(Sources &s)
{
  return invoke<Fn_le_091, const sc_dt::sc_int_base &, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_092 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_092>(&sc_dt::operator<=)), Fn_le_092>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_092(Sources &s)
{
  return invoke<Fn_le_092, const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_093 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_093>(&sc_dt::operator<=)), Fn_le_093>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_093(Sources &s)
{
  return invoke<Fn_le_093, const sc_dt::sc_signed &, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_094 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_094>(&sc_dt::operator<=)), Fn_le_094>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_094(Sources &s)
{
  return invoke<Fn_le_094, const sc_dt::sc_signed &, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_095 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_095>(&sc_dt::operator<=)), Fn_le_095>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_095(Sources &s)
{
  return invoke<Fn_le_095, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_096 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_096>(&sc_dt::operator<=)), Fn_le_096>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_096(Sources &s)
{
  return invoke<Fn_le_096, const sc_dt::sc_signed &, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_097 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_097>(&sc_dt::operator<=)), Fn_le_097>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_097(Sources &s)
{
  return invoke<Fn_le_097, const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_098 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_098>(&sc_dt::operator<=)), Fn_le_098>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_098(Sources &s)
{
  return invoke<Fn_le_098, const sc_dt::sc_signed &, const sc_dt::sc_int_base &>(&sc_dt::operator<=, s);
}
using Fn_le_099 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_099>(&sc_dt::operator<=)), Fn_le_099>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_099(Sources &s)
{
  return invoke<Fn_le_099, const sc_dt::sc_signed &, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_100 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_100>(&sc_dt::operator<=)), Fn_le_100>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_100(Sources &s)
{
  return invoke<Fn_le_100, const sc_dt::sc_signed &, const sc_dt::sc_uint_base &>(&sc_dt::operator<=, s);
}
using Fn_le_101 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_101>(&sc_dt::operator<=)), Fn_le_101>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_101(Sources &s)
{
  return invoke<Fn_le_101, const sc_dt::sc_signed &, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_102 = bool (*)(const sc_dt::sc_signed &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_102>(&sc_dt::operator<=)), Fn_le_102>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_102(Sources &s)
{
  return invoke<Fn_le_102, const sc_dt::sc_signed &, int>(&sc_dt::operator<=, s);
}
using Fn_le_103 = bool (*)(const sc_dt::sc_signed &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_103>(&sc_dt::operator<=)), Fn_le_103>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_103(Sources &s)
{
  return invoke<Fn_le_103, const sc_dt::sc_signed &, long>(&sc_dt::operator<=, s);
}
using Fn_le_104 = bool (*)(const sc_dt::sc_signed &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_104>(&sc_dt::operator<=)), Fn_le_104>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_104(Sources &s)
{
  return invoke<Fn_le_104, const sc_dt::sc_signed &, sc_dt::int64>(&sc_dt::operator<=, s);
}
using Fn_le_105 = bool (*)(const sc_dt::sc_signed &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_105>(&sc_dt::operator<=)), Fn_le_105>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_105(Sources &s)
{
  return invoke<Fn_le_105, const sc_dt::sc_signed &, sc_dt::uint64>(&sc_dt::operator<=, s);
}
using Fn_le_106 = bool (*)(const sc_dt::sc_signed &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_106>(&sc_dt::operator<=)), Fn_le_106>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_106(Sources &s)
{
  return invoke<Fn_le_106, const sc_dt::sc_signed &, unsigned int>(&sc_dt::operator<=, s);
}
using Fn_le_107 = bool (*)(const sc_dt::sc_signed &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_107>(&sc_dt::operator<=)), Fn_le_107>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_signed &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_107(Sources &s)
{
  return invoke<Fn_le_107, const sc_dt::sc_signed &, unsigned long>(&sc_dt::operator<=, s);
}
using Fn_le_108 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_108>(&sc_dt::operator<=)), Fn_le_108>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_108(Sources &s)
{
  return invoke<Fn_le_108, const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_109 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_109>(&sc_dt::operator<=)), Fn_le_109>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_109(Sources &s)
{
  return invoke<Fn_le_109, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_110 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_110>(&sc_dt::operator<=)), Fn_le_110>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_110(Sources &s)
{
  return invoke<Fn_le_110, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_111 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_111>(&sc_dt::operator<=)), Fn_le_111>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_111(Sources &s)
{
  return invoke<Fn_le_111, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_112 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_112>(&sc_dt::operator<=)), Fn_le_112>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_112(Sources &s)
{
  return invoke<Fn_le_112, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_113 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_113>(&sc_dt::operator<=)), Fn_le_113>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_113(Sources &s)
{
  return invoke<Fn_le_113, const sc_dt::sc_uint_base &, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_114 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_114>(&sc_dt::operator<=)), Fn_le_114>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_114(Sources &s)
{
  return invoke<Fn_le_114, const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &>(&sc_dt::operator<=, s);
}
using Fn_le_115 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_115>(&sc_dt::operator<=)), Fn_le_115>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_115(Sources &s)
{
  return invoke<Fn_le_115, const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_116 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_116>(&sc_dt::operator<=)), Fn_le_116>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_116(Sources &s)
{
  return invoke<Fn_le_116, const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_117 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_117>(&sc_dt::operator<=)), Fn_le_117>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_117(Sources &s)
{
  return invoke<Fn_le_117, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_118 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_118>(&sc_dt::operator<=)), Fn_le_118>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_118(Sources &s)
{
  return invoke<Fn_le_118, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_119 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_119>(&sc_dt::operator<=)), Fn_le_119>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_119(Sources &s)
{
  return invoke<Fn_le_119, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_120 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_120>(&sc_dt::operator<=)), Fn_le_120>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_120(Sources &s)
{
  return invoke<Fn_le_120, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_121 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_121>(&sc_dt::operator<=)), Fn_le_121>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_121(Sources &s)
{
  return invoke<Fn_le_121, const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &>(&sc_dt::operator<=, s);
}
using Fn_le_122 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_122>(&sc_dt::operator<=)), Fn_le_122>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_122(Sources &s)
{
  return invoke<Fn_le_122, const sc_dt::sc_unsigned &, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_123 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_123>(&sc_dt::operator<=)), Fn_le_123>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_123(Sources &s)
{
  return invoke<Fn_le_123, const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &>(&sc_dt::operator<=, s);
}
using Fn_le_124 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_124>(&sc_dt::operator<=)), Fn_le_124>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_124(Sources &s)
{
  return invoke<Fn_le_124, const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_125 = bool (*)(const sc_dt::sc_unsigned &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_125>(&sc_dt::operator<=)), Fn_le_125>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_125(Sources &s)
{
  return invoke<Fn_le_125, const sc_dt::sc_unsigned &, int>(&sc_dt::operator<=, s);
}
using Fn_le_126 = bool (*)(const sc_dt::sc_unsigned &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_126>(&sc_dt::operator<=)), Fn_le_126>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_126(Sources &s)
{
  return invoke<Fn_le_126, const sc_dt::sc_unsigned &, long>(&sc_dt::operator<=, s);
}
using Fn_le_127 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_127>(&sc_dt::operator<=)), Fn_le_127>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_127(Sources &s)
{
  return invoke<Fn_le_127, const sc_dt::sc_unsigned &, sc_dt::int64>(&sc_dt::operator<=, s);
}
using Fn_le_128 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_128>(&sc_dt::operator<=)), Fn_le_128>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_128(Sources &s)
{
  return invoke<Fn_le_128, const sc_dt::sc_unsigned &, sc_dt::uint64>(&sc_dt::operator<=, s);
}
using Fn_le_129 = bool (*)(const sc_dt::sc_unsigned &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_129>(&sc_dt::operator<=)), Fn_le_129>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_129(Sources &s)
{
  return invoke<Fn_le_129, const sc_dt::sc_unsigned &, unsigned int>(&sc_dt::operator<=, s);
}
using Fn_le_130 = bool (*)(const sc_dt::sc_unsigned &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_130>(&sc_dt::operator<=)), Fn_le_130>,
              "exact overload sc_dt::operator<= bool (const sc_dt::sc_unsigned &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_130(Sources &s)
{
  return invoke<Fn_le_130, const sc_dt::sc_unsigned &, unsigned long>(&sc_dt::operator<=, s);
}
using Fn_le_131 = bool (*)(double, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_131>(&sc_dt::operator<=)), Fn_le_131>,
              "exact overload sc_dt::operator<= bool (double, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_131(Sources &s)
{
  return invoke<Fn_le_131, double, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_132 = bool (*)(double, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_132>(&sc_dt::operator<=)), Fn_le_132>,
              "exact overload sc_dt::operator<= bool (double, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_132(Sources &s)
{
  return invoke<Fn_le_132, double, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_133 = bool (*)(double, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_133>(&sc_dt::operator<=)), Fn_le_133>,
              "exact overload sc_dt::operator<= bool (double, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_133(Sources &s)
{
  return invoke<Fn_le_133, double, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_134 = bool (*)(double, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_134>(&sc_dt::operator<=)), Fn_le_134>,
              "exact overload sc_dt::operator<= bool (double, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_134(Sources &s)
{
  return invoke<Fn_le_134, double, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_135 = bool (*)(float, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_135>(&sc_dt::operator<=)), Fn_le_135>,
              "exact overload sc_dt::operator<= bool (float, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_135(Sources &s)
{
  return invoke<Fn_le_135, float, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_136 = bool (*)(float, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_136>(&sc_dt::operator<=)), Fn_le_136>,
              "exact overload sc_dt::operator<= bool (float, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_136(Sources &s)
{
  return invoke<Fn_le_136, float, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_137 = bool (*)(float, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_137>(&sc_dt::operator<=)), Fn_le_137>,
              "exact overload sc_dt::operator<= bool (float, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_137(Sources &s)
{
  return invoke<Fn_le_137, float, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_138 = bool (*)(float, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_138>(&sc_dt::operator<=)), Fn_le_138>,
              "exact overload sc_dt::operator<= bool (float, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_138(Sources &s)
{
  return invoke<Fn_le_138, float, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_139 = bool (*)(int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_139>(&sc_dt::operator<=)), Fn_le_139>,
              "exact overload sc_dt::operator<= bool (int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_139(Sources &s)
{
  return invoke<Fn_le_139, int, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_140 = bool (*)(int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_140>(&sc_dt::operator<=)), Fn_le_140>,
              "exact overload sc_dt::operator<= bool (int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_140(Sources &s)
{
  return invoke<Fn_le_140, int, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_141 = bool (*)(int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_141>(&sc_dt::operator<=)), Fn_le_141>,
              "exact overload sc_dt::operator<= bool (int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_141(Sources &s)
{
  return invoke<Fn_le_141, int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_142 = bool (*)(int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_142>(&sc_dt::operator<=)), Fn_le_142>,
              "exact overload sc_dt::operator<= bool (int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_142(Sources &s)
{
  return invoke<Fn_le_142, int, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_143 = bool (*)(int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_143>(&sc_dt::operator<=)), Fn_le_143>,
              "exact overload sc_dt::operator<= bool (int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_143(Sources &s)
{
  return invoke<Fn_le_143, int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_144 = bool (*)(int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_144>(&sc_dt::operator<=)), Fn_le_144>,
              "exact overload sc_dt::operator<= bool (int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_144(Sources &s)
{
  return invoke<Fn_le_144, int, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_145 = bool (*)(int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_145>(&sc_dt::operator<=)), Fn_le_145>,
              "exact overload sc_dt::operator<= bool (int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_145(Sources &s)
{
  return invoke<Fn_le_145, int, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_146 = bool (*)(long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_146>(&sc_dt::operator<=)), Fn_le_146>,
              "exact overload sc_dt::operator<= bool (long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_146(Sources &s)
{
  return invoke<Fn_le_146, long, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_147 = bool (*)(long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_147>(&sc_dt::operator<=)), Fn_le_147>,
              "exact overload sc_dt::operator<= bool (long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_147(Sources &s)
{
  return invoke<Fn_le_147, long, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_148 = bool (*)(long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_148>(&sc_dt::operator<=)), Fn_le_148>,
              "exact overload sc_dt::operator<= bool (long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_148(Sources &s)
{
  return invoke<Fn_le_148, long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_149 = bool (*)(long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_149>(&sc_dt::operator<=)), Fn_le_149>,
              "exact overload sc_dt::operator<= bool (long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_149(Sources &s)
{
  return invoke<Fn_le_149, long, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_150 = bool (*)(long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_150>(&sc_dt::operator<=)), Fn_le_150>,
              "exact overload sc_dt::operator<= bool (long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_150(Sources &s)
{
  return invoke<Fn_le_150, long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_151 = bool (*)(long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_151>(&sc_dt::operator<=)), Fn_le_151>,
              "exact overload sc_dt::operator<= bool (long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_151(Sources &s)
{
  return invoke<Fn_le_151, long, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_152 = bool (*)(long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_152>(&sc_dt::operator<=)), Fn_le_152>,
              "exact overload sc_dt::operator<= bool (long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_152(Sources &s)
{
  return invoke<Fn_le_152, long, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_153 = bool (*)(sc_dt::int64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_153>(&sc_dt::operator<=)), Fn_le_153>,
              "exact overload sc_dt::operator<= bool (sc_dt::int64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_153(Sources &s)
{
  return invoke<Fn_le_153, sc_dt::int64, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_154 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_154>(&sc_dt::operator<=)), Fn_le_154>,
              "exact overload sc_dt::operator<= bool (sc_dt::int64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_154(Sources &s)
{
  return invoke<Fn_le_154, sc_dt::int64, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_155 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_155>(&sc_dt::operator<=)), Fn_le_155>,
              "exact overload sc_dt::operator<= bool (sc_dt::int64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_155(Sources &s)
{
  return invoke<Fn_le_155, sc_dt::int64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_156 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_156>(&sc_dt::operator<=)), Fn_le_156>,
              "exact overload sc_dt::operator<= bool (sc_dt::int64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_156(Sources &s)
{
  return invoke<Fn_le_156, sc_dt::int64, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_157 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_157>(&sc_dt::operator<=)), Fn_le_157>,
              "exact overload sc_dt::operator<= bool (sc_dt::int64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_157(Sources &s)
{
  return invoke<Fn_le_157, sc_dt::int64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_158 = bool (*)(sc_dt::int64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_158>(&sc_dt::operator<=)), Fn_le_158>,
              "exact overload sc_dt::operator<= bool (sc_dt::int64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_158(Sources &s)
{
  return invoke<Fn_le_158, sc_dt::int64, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_159 = bool (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_159>(&sc_dt::operator<=)), Fn_le_159>,
              "exact overload sc_dt::operator<= bool (sc_dt::int64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_159(Sources &s)
{
  return invoke<Fn_le_159, sc_dt::int64, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_160 = bool (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_160>(&sc_dt::operator<=)), Fn_le_160>,
              "exact overload sc_dt::operator<= bool (sc_dt::uint64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_160(Sources &s)
{
  return invoke<Fn_le_160, sc_dt::uint64, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_161 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_161>(&sc_dt::operator<=)), Fn_le_161>,
              "exact overload sc_dt::operator<= bool (sc_dt::uint64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_161(Sources &s)
{
  return invoke<Fn_le_161, sc_dt::uint64, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_162 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_162>(&sc_dt::operator<=)), Fn_le_162>,
              "exact overload sc_dt::operator<= bool (sc_dt::uint64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_162(Sources &s)
{
  return invoke<Fn_le_162, sc_dt::uint64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_163 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_163>(&sc_dt::operator<=)), Fn_le_163>,
              "exact overload sc_dt::operator<= bool (sc_dt::uint64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_163(Sources &s)
{
  return invoke<Fn_le_163, sc_dt::uint64, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_164 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_164>(&sc_dt::operator<=)), Fn_le_164>,
              "exact overload sc_dt::operator<= bool (sc_dt::uint64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_164(Sources &s)
{
  return invoke<Fn_le_164, sc_dt::uint64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_165 = bool (*)(sc_dt::uint64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_165>(&sc_dt::operator<=)), Fn_le_165>,
              "exact overload sc_dt::operator<= bool (sc_dt::uint64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_165(Sources &s)
{
  return invoke<Fn_le_165, sc_dt::uint64, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_166 = bool (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_166>(&sc_dt::operator<=)), Fn_le_166>,
              "exact overload sc_dt::operator<= bool (sc_dt::uint64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_166(Sources &s)
{
  return invoke<Fn_le_166, sc_dt::uint64, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_167 = bool (*)(unsigned int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_167>(&sc_dt::operator<=)), Fn_le_167>,
              "exact overload sc_dt::operator<= bool (unsigned int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_167(Sources &s)
{
  return invoke<Fn_le_167, unsigned int, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_168 = bool (*)(unsigned int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_168>(&sc_dt::operator<=)), Fn_le_168>,
              "exact overload sc_dt::operator<= bool (unsigned int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_168(Sources &s)
{
  return invoke<Fn_le_168, unsigned int, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_169 = bool (*)(unsigned int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_169>(&sc_dt::operator<=)), Fn_le_169>,
              "exact overload sc_dt::operator<= bool (unsigned int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_169(Sources &s)
{
  return invoke<Fn_le_169, unsigned int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_170 = bool (*)(unsigned int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_170>(&sc_dt::operator<=)), Fn_le_170>,
              "exact overload sc_dt::operator<= bool (unsigned int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_170(Sources &s)
{
  return invoke<Fn_le_170, unsigned int, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_171 = bool (*)(unsigned int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_171>(&sc_dt::operator<=)), Fn_le_171>,
              "exact overload sc_dt::operator<= bool (unsigned int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_171(Sources &s)
{
  return invoke<Fn_le_171, unsigned int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_172 = bool (*)(unsigned int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_172>(&sc_dt::operator<=)), Fn_le_172>,
              "exact overload sc_dt::operator<= bool (unsigned int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_172(Sources &s)
{
  return invoke<Fn_le_172, unsigned int, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_173 = bool (*)(unsigned int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_173>(&sc_dt::operator<=)), Fn_le_173>,
              "exact overload sc_dt::operator<= bool (unsigned int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_173(Sources &s)
{
  return invoke<Fn_le_173, unsigned int, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
using Fn_le_174 = bool (*)(unsigned long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_174>(&sc_dt::operator<=)), Fn_le_174>,
              "exact overload sc_dt::operator<= bool (unsigned long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_174(Sources &s)
{
  return invoke<Fn_le_174, unsigned long, const sc_dt::sc_concatref &>(&sc_dt::operator<=, s);
}
using Fn_le_175 = bool (*)(unsigned long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_175>(&sc_dt::operator<=)), Fn_le_175>,
              "exact overload sc_dt::operator<= bool (unsigned long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_175(Sources &s)
{
  return invoke<Fn_le_175, unsigned long, const sc_dt::sc_fxnum &>(&sc_dt::operator<=, s);
}
using Fn_le_176 = bool (*)(unsigned long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_176>(&sc_dt::operator<=)), Fn_le_176>,
              "exact overload sc_dt::operator<= bool (unsigned long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_176(Sources &s)
{
  return invoke<Fn_le_176, unsigned long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_177 = bool (*)(unsigned long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_177>(&sc_dt::operator<=)), Fn_le_177>,
              "exact overload sc_dt::operator<= bool (unsigned long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_177(Sources &s)
{
  return invoke<Fn_le_177, unsigned long, const sc_dt::sc_fxval &>(&sc_dt::operator<=, s);
}
using Fn_le_178 = bool (*)(unsigned long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_178>(&sc_dt::operator<=)), Fn_le_178>,
              "exact overload sc_dt::operator<= bool (unsigned long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_178(Sources &s)
{
  return invoke<Fn_le_178, unsigned long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator<=, s);
}
using Fn_le_179 = bool (*)(unsigned long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_179>(&sc_dt::operator<=)), Fn_le_179>,
              "exact overload sc_dt::operator<= bool (unsigned long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_179(Sources &s)
{
  return invoke<Fn_le_179, unsigned long, const sc_dt::sc_signed &>(&sc_dt::operator<=, s);
}
using Fn_le_180 = bool (*)(unsigned long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_le_180>(&sc_dt::operator<=)), Fn_le_180>,
              "exact overload sc_dt::operator<= bool (unsigned long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le_180(Sources &s)
{
  return invoke<Fn_le_180, unsigned long, const sc_dt::sc_unsigned &>(&sc_dt::operator<=, s);
}
inline unsigned count_le = 181;
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_le(unsigned index, Sources &s)
{
  switch (index)
  {
  case 0:
    return case_le_000(s);
  case 1:
    return case_le_001(s);
  case 2:
    return case_le_002(s);
  case 3:
    return case_le_003(s);
  case 4:
    return case_le_004(s);
  case 5:
    return case_le_005(s);
  case 6:
    return case_le_006(s);
  case 7:
    return case_le_007(s);
  case 8:
    return case_le_008(s);
  case 9:
    return case_le_009(s);
  case 10:
    return case_le_010(s);
  case 11:
    return case_le_011(s);
  case 12:
    return case_le_012(s);
  case 13:
    return case_le_013(s);
  case 14:
    return case_le_014(s);
  case 15:
    return case_le_015(s);
  case 16:
    return case_le_016(s);
  case 17:
    return case_le_017(s);
  case 18:
    return case_le_018(s);
  case 19:
    return case_le_019(s);
  case 20:
    return case_le_020(s);
  case 21:
    return case_le_021(s);
  case 22:
    return case_le_022(s);
  case 23:
    return case_le_023(s);
  case 24:
    return case_le_024(s);
  case 25:
    return case_le_025(s);
  case 26:
    return case_le_026(s);
  case 27:
    return case_le_027(s);
  case 28:
    return case_le_028(s);
  case 29:
    return case_le_029(s);
  case 30:
    return case_le_030(s);
  case 31:
    return case_le_031(s);
  case 32:
    return case_le_032(s);
  case 33:
    return case_le_033(s);
  case 34:
    return case_le_034(s);
  case 35:
    return case_le_035(s);
  case 36:
    return case_le_036(s);
  case 37:
    return case_le_037(s);
  case 38:
    return case_le_038(s);
  case 39:
    return case_le_039(s);
  case 40:
    return case_le_040(s);
  case 41:
    return case_le_041(s);
  case 42:
    return case_le_042(s);
  case 43:
    return case_le_043(s);
  case 44:
    return case_le_044(s);
  case 45:
    return case_le_045(s);
  case 46:
    return case_le_046(s);
  case 47:
    return case_le_047(s);
  case 48:
    return case_le_048(s);
  case 49:
    return case_le_049(s);
  case 50:
    return case_le_050(s);
  case 51:
    return case_le_051(s);
  case 52:
    return case_le_052(s);
  case 53:
    return case_le_053(s);
  case 54:
    return case_le_054(s);
  case 55:
    return case_le_055(s);
  case 56:
    return case_le_056(s);
  case 57:
    return case_le_057(s);
  case 58:
    return case_le_058(s);
  case 59:
    return case_le_059(s);
  case 60:
    return case_le_060(s);
  case 61:
    return case_le_061(s);
  case 62:
    return case_le_062(s);
  case 63:
    return case_le_063(s);
  case 64:
    return case_le_064(s);
  case 65:
    return case_le_065(s);
  case 66:
    return case_le_066(s);
  case 67:
    return case_le_067(s);
  case 68:
    return case_le_068(s);
  case 69:
    return case_le_069(s);
  case 70:
    return case_le_070(s);
  case 71:
    return case_le_071(s);
  case 72:
    return case_le_072(s);
  case 73:
    return case_le_073(s);
  case 74:
    return case_le_074(s);
  case 75:
    return case_le_075(s);
  case 76:
    return case_le_076(s);
  case 77:
    return case_le_077(s);
  case 78:
    return case_le_078(s);
  case 79:
    return case_le_079(s);
  case 80:
    return case_le_080(s);
  case 81:
    return case_le_081(s);
  case 82:
    return case_le_082(s);
  case 83:
    return case_le_083(s);
  case 84:
    return case_le_084(s);
  case 85:
    return case_le_085(s);
  case 86:
    return case_le_086(s);
  case 87:
    return case_le_087(s);
  case 88:
    return case_le_088(s);
  case 89:
    return case_le_089(s);
  case 90:
    return case_le_090(s);
  case 91:
    return case_le_091(s);
  case 92:
    return case_le_092(s);
  case 93:
    return case_le_093(s);
  case 94:
    return case_le_094(s);
  case 95:
    return case_le_095(s);
  case 96:
    return case_le_096(s);
  case 97:
    return case_le_097(s);
  case 98:
    return case_le_098(s);
  case 99:
    return case_le_099(s);
  case 100:
    return case_le_100(s);
  case 101:
    return case_le_101(s);
  case 102:
    return case_le_102(s);
  case 103:
    return case_le_103(s);
  case 104:
    return case_le_104(s);
  case 105:
    return case_le_105(s);
  case 106:
    return case_le_106(s);
  case 107:
    return case_le_107(s);
  case 108:
    return case_le_108(s);
  case 109:
    return case_le_109(s);
  case 110:
    return case_le_110(s);
  case 111:
    return case_le_111(s);
  case 112:
    return case_le_112(s);
  case 113:
    return case_le_113(s);
  case 114:
    return case_le_114(s);
  case 115:
    return case_le_115(s);
  case 116:
    return case_le_116(s);
  case 117:
    return case_le_117(s);
  case 118:
    return case_le_118(s);
  case 119:
    return case_le_119(s);
  case 120:
    return case_le_120(s);
  case 121:
    return case_le_121(s);
  case 122:
    return case_le_122(s);
  case 123:
    return case_le_123(s);
  case 124:
    return case_le_124(s);
  case 125:
    return case_le_125(s);
  case 126:
    return case_le_126(s);
  case 127:
    return case_le_127(s);
  case 128:
    return case_le_128(s);
  case 129:
    return case_le_129(s);
  case 130:
    return case_le_130(s);
  case 131:
    return case_le_131(s);
  case 132:
    return case_le_132(s);
  case 133:
    return case_le_133(s);
  case 134:
    return case_le_134(s);
  case 135:
    return case_le_135(s);
  case 136:
    return case_le_136(s);
  case 137:
    return case_le_137(s);
  case 138:
    return case_le_138(s);
  case 139:
    return case_le_139(s);
  case 140:
    return case_le_140(s);
  case 141:
    return case_le_141(s);
  case 142:
    return case_le_142(s);
  case 143:
    return case_le_143(s);
  case 144:
    return case_le_144(s);
  case 145:
    return case_le_145(s);
  case 146:
    return case_le_146(s);
  case 147:
    return case_le_147(s);
  case 148:
    return case_le_148(s);
  case 149:
    return case_le_149(s);
  case 150:
    return case_le_150(s);
  case 151:
    return case_le_151(s);
  case 152:
    return case_le_152(s);
  case 153:
    return case_le_153(s);
  case 154:
    return case_le_154(s);
  case 155:
    return case_le_155(s);
  case 156:
    return case_le_156(s);
  case 157:
    return case_le_157(s);
  case 158:
    return case_le_158(s);
  case 159:
    return case_le_159(s);
  case 160:
    return case_le_160(s);
  case 161:
    return case_le_161(s);
  case 162:
    return case_le_162(s);
  case 163:
    return case_le_163(s);
  case 164:
    return case_le_164(s);
  case 165:
    return case_le_165(s);
  case 166:
    return case_le_166(s);
  case 167:
    return case_le_167(s);
  case 168:
    return case_le_168(s);
  case 169:
    return case_le_169(s);
  case 170:
    return case_le_170(s);
  case 171:
    return case_le_171(s);
  case 172:
    return case_le_172(s);
  case 173:
    return case_le_173(s);
  case 174:
    return case_le_174(s);
  case 175:
    return case_le_175(s);
  case 176:
    return case_le_176(s);
  case 177:
    return case_le_177(s);
  case 178:
    return case_le_178(s);
  case 179:
    return case_le_179(s);
  case 180:
    return case_le_180(s);
  default:
    throw std::out_of_range("unknown comparison case index");
  }
}
using Fn_gt_000 = bool (*)(bool, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_000>(&sc_dt::operator>)), Fn_gt_000>,
              "exact overload sc_dt::operator> bool (bool, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_000(Sources &s)
{
  return invoke<Fn_gt_000, bool, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_001 = bool (*)(const char *, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_001>(&sc_dt::operator>)), Fn_gt_001>,
              "exact overload sc_dt::operator> bool (const char *, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_001(Sources &s)
{
  return invoke<Fn_gt_001, const char *, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_002 = bool (*)(const char *, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_002>(&sc_dt::operator>)), Fn_gt_002>,
              "exact overload sc_dt::operator> bool (const char *, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_002(Sources &s)
{
  return invoke<Fn_gt_002, const char *, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_003 = bool (*)(const char *, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_003>(&sc_dt::operator>)), Fn_gt_003>,
              "exact overload sc_dt::operator> bool (const char *, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_003(Sources &s)
{
  return invoke<Fn_gt_003, const char *, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_004 = bool (*)(const char *, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_004>(&sc_dt::operator>)), Fn_gt_004>,
              "exact overload sc_dt::operator> bool (const char *, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_004(Sources &s)
{
  return invoke<Fn_gt_004, const char *, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_005 = bool (*)(const sc_dt::sc_concatref &, bool);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_005>(&sc_dt::operator>)), Fn_gt_005>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, bool)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_005(Sources &s)
{
  return invoke<Fn_gt_005, const sc_dt::sc_concatref &, bool>(&sc_dt::operator>, s);
}
using Fn_gt_006 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_006>(&sc_dt::operator>)), Fn_gt_006>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_006(Sources &s)
{
  return invoke<Fn_gt_006, const sc_dt::sc_concatref &, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_007 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_007>(&sc_dt::operator>)), Fn_gt_007>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_007(Sources &s)
{
  return invoke<Fn_gt_007, const sc_dt::sc_concatref &, const sc_dt::sc_int_base &>(&sc_dt::operator>, s);
}
using Fn_gt_008 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_008>(&sc_dt::operator>)), Fn_gt_008>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_008(Sources &s)
{
  return invoke<Fn_gt_008, const sc_dt::sc_concatref &, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_009 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_009>(&sc_dt::operator>)), Fn_gt_009>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_009(Sources &s)
{
  return invoke<Fn_gt_009, const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &>(&sc_dt::operator>, s);
}
using Fn_gt_010 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_010>(&sc_dt::operator>)), Fn_gt_010>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_010(Sources &s)
{
  return invoke<Fn_gt_010, const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_011 = bool (*)(const sc_dt::sc_concatref &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_011>(&sc_dt::operator>)), Fn_gt_011>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_011(Sources &s)
{
  return invoke<Fn_gt_011, const sc_dt::sc_concatref &, int>(&sc_dt::operator>, s);
}
using Fn_gt_012 = bool (*)(const sc_dt::sc_concatref &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_012>(&sc_dt::operator>)), Fn_gt_012>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_012(Sources &s)
{
  return invoke<Fn_gt_012, const sc_dt::sc_concatref &, long>(&sc_dt::operator>, s);
}
using Fn_gt_013 = bool (*)(const sc_dt::sc_concatref &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_013>(&sc_dt::operator>)), Fn_gt_013>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_013(Sources &s)
{
  return invoke<Fn_gt_013, const sc_dt::sc_concatref &, sc_dt::int64>(&sc_dt::operator>, s);
}
using Fn_gt_014 = bool (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_014>(&sc_dt::operator>)), Fn_gt_014>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_014(Sources &s)
{
  return invoke<Fn_gt_014, const sc_dt::sc_concatref &, sc_dt::uint64>(&sc_dt::operator>, s);
}
using Fn_gt_015 = bool (*)(const sc_dt::sc_concatref &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_015>(&sc_dt::operator>)), Fn_gt_015>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_015(Sources &s)
{
  return invoke<Fn_gt_015, const sc_dt::sc_concatref &, unsigned int>(&sc_dt::operator>, s);
}
using Fn_gt_016 = bool (*)(const sc_dt::sc_concatref &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_016>(&sc_dt::operator>)), Fn_gt_016>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_concatref &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_016(Sources &s)
{
  return invoke<Fn_gt_016, const sc_dt::sc_concatref &, unsigned long>(&sc_dt::operator>, s);
}
using Fn_gt_017 = bool (*)(const sc_dt::sc_fxnum &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_017>(&sc_dt::operator>)), Fn_gt_017>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_017(Sources &s)
{
  return invoke<Fn_gt_017, const sc_dt::sc_fxnum &, const char *>(&sc_dt::operator>, s);
}
using Fn_gt_018 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_018>(&sc_dt::operator>)), Fn_gt_018>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_018(Sources &s)
{
  return invoke<Fn_gt_018, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_019 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_019>(&sc_dt::operator>)), Fn_gt_019>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_019(Sources &s)
{
  return invoke<Fn_gt_019, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_020 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_020>(&sc_dt::operator>)), Fn_gt_020>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_020(Sources &s)
{
  return invoke<Fn_gt_020, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_021 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_021>(&sc_dt::operator>)), Fn_gt_021>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_021(Sources &s)
{
  return invoke<Fn_gt_021, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_022 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_022>(&sc_dt::operator>)), Fn_gt_022>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_022(Sources &s)
{
  return invoke<Fn_gt_022, const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &>(&sc_dt::operator>, s);
}
using Fn_gt_023 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_023>(&sc_dt::operator>)), Fn_gt_023>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_023(Sources &s)
{
  return invoke<Fn_gt_023, const sc_dt::sc_fxnum &, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_024 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_024>(&sc_dt::operator>)), Fn_gt_024>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_024(Sources &s)
{
  return invoke<Fn_gt_024, const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &>(&sc_dt::operator>, s);
}
using Fn_gt_025 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_025>(&sc_dt::operator>)), Fn_gt_025>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_025(Sources &s)
{
  return invoke<Fn_gt_025, const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_026 = bool (*)(const sc_dt::sc_fxnum &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_026>(&sc_dt::operator>)), Fn_gt_026>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_026(Sources &s)
{
  return invoke<Fn_gt_026, const sc_dt::sc_fxnum &, double>(&sc_dt::operator>, s);
}
using Fn_gt_027 = bool (*)(const sc_dt::sc_fxnum &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_027>(&sc_dt::operator>)), Fn_gt_027>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_027(Sources &s)
{
  return invoke<Fn_gt_027, const sc_dt::sc_fxnum &, float>(&sc_dt::operator>, s);
}
using Fn_gt_028 = bool (*)(const sc_dt::sc_fxnum &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_028>(&sc_dt::operator>)), Fn_gt_028>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_028(Sources &s)
{
  return invoke<Fn_gt_028, const sc_dt::sc_fxnum &, int>(&sc_dt::operator>, s);
}
using Fn_gt_029 = bool (*)(const sc_dt::sc_fxnum &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_029>(&sc_dt::operator>)), Fn_gt_029>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_029(Sources &s)
{
  return invoke<Fn_gt_029, const sc_dt::sc_fxnum &, long>(&sc_dt::operator>, s);
}
using Fn_gt_030 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_030>(&sc_dt::operator>)), Fn_gt_030>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_030(Sources &s)
{
  return invoke<Fn_gt_030, const sc_dt::sc_fxnum &, sc_dt::int64>(&sc_dt::operator>, s);
}
using Fn_gt_031 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_031>(&sc_dt::operator>)), Fn_gt_031>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_031(Sources &s)
{
  return invoke<Fn_gt_031, const sc_dt::sc_fxnum &, sc_dt::uint64>(&sc_dt::operator>, s);
}
using Fn_gt_032 = bool (*)(const sc_dt::sc_fxnum &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_032>(&sc_dt::operator>)), Fn_gt_032>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_032(Sources &s)
{
  return invoke<Fn_gt_032, const sc_dt::sc_fxnum &, unsigned int>(&sc_dt::operator>, s);
}
using Fn_gt_033 = bool (*)(const sc_dt::sc_fxnum &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_033>(&sc_dt::operator>)), Fn_gt_033>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_033(Sources &s)
{
  return invoke<Fn_gt_033, const sc_dt::sc_fxnum &, unsigned long>(&sc_dt::operator>, s);
}
using Fn_gt_034 = bool (*)(const sc_dt::sc_fxnum_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_034>(&sc_dt::operator>)), Fn_gt_034>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_034(Sources &s)
{
  return invoke<Fn_gt_034, const sc_dt::sc_fxnum_fast &, const char *>(&sc_dt::operator>, s);
}
using Fn_gt_035 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_035>(&sc_dt::operator>)), Fn_gt_035>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_035(Sources &s)
{
  return invoke<Fn_gt_035, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_036 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_036>(&sc_dt::operator>)), Fn_gt_036>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_036(Sources &s)
{
  return invoke<Fn_gt_036, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_037 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_037>(&sc_dt::operator>)), Fn_gt_037>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_037(Sources &s)
{
  return invoke<Fn_gt_037, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_038 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_038>(&sc_dt::operator>)), Fn_gt_038>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_038(Sources &s)
{
  return invoke<Fn_gt_038, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_039 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_039>(&sc_dt::operator>)), Fn_gt_039>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_039(Sources &s)
{
  return invoke<Fn_gt_039, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator>, s);
}
using Fn_gt_040 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_040>(&sc_dt::operator>)), Fn_gt_040>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_040(Sources &s)
{
  return invoke<Fn_gt_040, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_041 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_041>(&sc_dt::operator>)), Fn_gt_041>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_041(Sources &s)
{
  return invoke<Fn_gt_041, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator>, s);
}
using Fn_gt_042 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_042>(&sc_dt::operator>)), Fn_gt_042>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_042(Sources &s)
{
  return invoke<Fn_gt_042, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_043 = bool (*)(const sc_dt::sc_fxnum_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_043>(&sc_dt::operator>)), Fn_gt_043>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_043(Sources &s)
{
  return invoke<Fn_gt_043, const sc_dt::sc_fxnum_fast &, double>(&sc_dt::operator>, s);
}
using Fn_gt_044 = bool (*)(const sc_dt::sc_fxnum_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_044>(&sc_dt::operator>)), Fn_gt_044>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_044(Sources &s)
{
  return invoke<Fn_gt_044, const sc_dt::sc_fxnum_fast &, float>(&sc_dt::operator>, s);
}
using Fn_gt_045 = bool (*)(const sc_dt::sc_fxnum_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_045>(&sc_dt::operator>)), Fn_gt_045>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_045(Sources &s)
{
  return invoke<Fn_gt_045, const sc_dt::sc_fxnum_fast &, int>(&sc_dt::operator>, s);
}
using Fn_gt_046 = bool (*)(const sc_dt::sc_fxnum_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_046>(&sc_dt::operator>)), Fn_gt_046>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_046(Sources &s)
{
  return invoke<Fn_gt_046, const sc_dt::sc_fxnum_fast &, long>(&sc_dt::operator>, s);
}
using Fn_gt_047 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_047>(&sc_dt::operator>)), Fn_gt_047>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_047(Sources &s)
{
  return invoke<Fn_gt_047, const sc_dt::sc_fxnum_fast &, sc_dt::int64>(&sc_dt::operator>, s);
}
using Fn_gt_048 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_048>(&sc_dt::operator>)), Fn_gt_048>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_048(Sources &s)
{
  return invoke<Fn_gt_048, const sc_dt::sc_fxnum_fast &, sc_dt::uint64>(&sc_dt::operator>, s);
}
using Fn_gt_049 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_049>(&sc_dt::operator>)), Fn_gt_049>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_049(Sources &s)
{
  return invoke<Fn_gt_049, const sc_dt::sc_fxnum_fast &, unsigned int>(&sc_dt::operator>, s);
}
using Fn_gt_050 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_050>(&sc_dt::operator>)), Fn_gt_050>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxnum_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_050(Sources &s)
{
  return invoke<Fn_gt_050, const sc_dt::sc_fxnum_fast &, unsigned long>(&sc_dt::operator>, s);
}
using Fn_gt_051 = bool (*)(const sc_dt::sc_fxval &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_051>(&sc_dt::operator>)), Fn_gt_051>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_051(Sources &s)
{
  return invoke<Fn_gt_051, const sc_dt::sc_fxval &, const char *>(&sc_dt::operator>, s);
}
using Fn_gt_052 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_052>(&sc_dt::operator>)), Fn_gt_052>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_052(Sources &s)
{
  return invoke<Fn_gt_052, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_053 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_053>(&sc_dt::operator>)), Fn_gt_053>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_053(Sources &s)
{
  return invoke<Fn_gt_053, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_054 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_054>(&sc_dt::operator>)), Fn_gt_054>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_054(Sources &s)
{
  return invoke<Fn_gt_054, const sc_dt::sc_fxval &, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_055 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_055>(&sc_dt::operator>)), Fn_gt_055>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_055(Sources &s)
{
  return invoke<Fn_gt_055, const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_056 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_056>(&sc_dt::operator>)), Fn_gt_056>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_056(Sources &s)
{
  return invoke<Fn_gt_056, const sc_dt::sc_fxval &, const sc_dt::sc_int_base &>(&sc_dt::operator>, s);
}
using Fn_gt_057 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_057>(&sc_dt::operator>)), Fn_gt_057>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_057(Sources &s)
{
  return invoke<Fn_gt_057, const sc_dt::sc_fxval &, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_058 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_058>(&sc_dt::operator>)), Fn_gt_058>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_058(Sources &s)
{
  return invoke<Fn_gt_058, const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &>(&sc_dt::operator>, s);
}
using Fn_gt_059 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_059>(&sc_dt::operator>)), Fn_gt_059>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_059(Sources &s)
{
  return invoke<Fn_gt_059, const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_060 = bool (*)(const sc_dt::sc_fxval &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_060>(&sc_dt::operator>)), Fn_gt_060>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_060(Sources &s)
{
  return invoke<Fn_gt_060, const sc_dt::sc_fxval &, double>(&sc_dt::operator>, s);
}
using Fn_gt_061 = bool (*)(const sc_dt::sc_fxval &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_061>(&sc_dt::operator>)), Fn_gt_061>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_061(Sources &s)
{
  return invoke<Fn_gt_061, const sc_dt::sc_fxval &, float>(&sc_dt::operator>, s);
}
using Fn_gt_062 = bool (*)(const sc_dt::sc_fxval &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_062>(&sc_dt::operator>)), Fn_gt_062>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_062(Sources &s)
{
  return invoke<Fn_gt_062, const sc_dt::sc_fxval &, int>(&sc_dt::operator>, s);
}
using Fn_gt_063 = bool (*)(const sc_dt::sc_fxval &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_063>(&sc_dt::operator>)), Fn_gt_063>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_063(Sources &s)
{
  return invoke<Fn_gt_063, const sc_dt::sc_fxval &, long>(&sc_dt::operator>, s);
}
using Fn_gt_064 = bool (*)(const sc_dt::sc_fxval &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_064>(&sc_dt::operator>)), Fn_gt_064>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_064(Sources &s)
{
  return invoke<Fn_gt_064, const sc_dt::sc_fxval &, sc_dt::int64>(&sc_dt::operator>, s);
}
using Fn_gt_065 = bool (*)(const sc_dt::sc_fxval &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_065>(&sc_dt::operator>)), Fn_gt_065>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_065(Sources &s)
{
  return invoke<Fn_gt_065, const sc_dt::sc_fxval &, sc_dt::uint64>(&sc_dt::operator>, s);
}
using Fn_gt_066 = bool (*)(const sc_dt::sc_fxval &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_066>(&sc_dt::operator>)), Fn_gt_066>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_066(Sources &s)
{
  return invoke<Fn_gt_066, const sc_dt::sc_fxval &, unsigned int>(&sc_dt::operator>, s);
}
using Fn_gt_067 = bool (*)(const sc_dt::sc_fxval &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_067>(&sc_dt::operator>)), Fn_gt_067>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_067(Sources &s)
{
  return invoke<Fn_gt_067, const sc_dt::sc_fxval &, unsigned long>(&sc_dt::operator>, s);
}
using Fn_gt_068 = bool (*)(const sc_dt::sc_fxval_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_068>(&sc_dt::operator>)), Fn_gt_068>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_068(Sources &s)
{
  return invoke<Fn_gt_068, const sc_dt::sc_fxval_fast &, const char *>(&sc_dt::operator>, s);
}
using Fn_gt_069 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_069>(&sc_dt::operator>)), Fn_gt_069>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_069(Sources &s)
{
  return invoke<Fn_gt_069, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_070 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_070>(&sc_dt::operator>)), Fn_gt_070>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_070(Sources &s)
{
  return invoke<Fn_gt_070, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_071 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_071>(&sc_dt::operator>)), Fn_gt_071>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_071(Sources &s)
{
  return invoke<Fn_gt_071, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_072 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_072>(&sc_dt::operator>)), Fn_gt_072>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_072(Sources &s)
{
  return invoke<Fn_gt_072, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_073 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_073>(&sc_dt::operator>)), Fn_gt_073>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_073(Sources &s)
{
  return invoke<Fn_gt_073, const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator>, s);
}
using Fn_gt_074 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_074>(&sc_dt::operator>)), Fn_gt_074>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_074(Sources &s)
{
  return invoke<Fn_gt_074, const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_075 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_075>(&sc_dt::operator>)), Fn_gt_075>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_075(Sources &s)
{
  return invoke<Fn_gt_075, const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator>, s);
}
using Fn_gt_076 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_076>(&sc_dt::operator>)), Fn_gt_076>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_076(Sources &s)
{
  return invoke<Fn_gt_076, const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_077 = bool (*)(const sc_dt::sc_fxval_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_077>(&sc_dt::operator>)), Fn_gt_077>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_077(Sources &s)
{
  return invoke<Fn_gt_077, const sc_dt::sc_fxval_fast &, double>(&sc_dt::operator>, s);
}
using Fn_gt_078 = bool (*)(const sc_dt::sc_fxval_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_078>(&sc_dt::operator>)), Fn_gt_078>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_078(Sources &s)
{
  return invoke<Fn_gt_078, const sc_dt::sc_fxval_fast &, float>(&sc_dt::operator>, s);
}
using Fn_gt_079 = bool (*)(const sc_dt::sc_fxval_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_079>(&sc_dt::operator>)), Fn_gt_079>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_079(Sources &s)
{
  return invoke<Fn_gt_079, const sc_dt::sc_fxval_fast &, int>(&sc_dt::operator>, s);
}
using Fn_gt_080 = bool (*)(const sc_dt::sc_fxval_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_080>(&sc_dt::operator>)), Fn_gt_080>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_080(Sources &s)
{
  return invoke<Fn_gt_080, const sc_dt::sc_fxval_fast &, long>(&sc_dt::operator>, s);
}
using Fn_gt_081 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_081>(&sc_dt::operator>)), Fn_gt_081>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_081(Sources &s)
{
  return invoke<Fn_gt_081, const sc_dt::sc_fxval_fast &, sc_dt::int64>(&sc_dt::operator>, s);
}
using Fn_gt_082 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_082>(&sc_dt::operator>)), Fn_gt_082>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_082(Sources &s)
{
  return invoke<Fn_gt_082, const sc_dt::sc_fxval_fast &, sc_dt::uint64>(&sc_dt::operator>, s);
}
using Fn_gt_083 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_083>(&sc_dt::operator>)), Fn_gt_083>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_083(Sources &s)
{
  return invoke<Fn_gt_083, const sc_dt::sc_fxval_fast &, unsigned int>(&sc_dt::operator>, s);
}
using Fn_gt_084 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_084>(&sc_dt::operator>)), Fn_gt_084>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_fxval_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_084(Sources &s)
{
  return invoke<Fn_gt_084, const sc_dt::sc_fxval_fast &, unsigned long>(&sc_dt::operator>, s);
}
using Fn_gt_085 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_085>(&sc_dt::operator>)), Fn_gt_085>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_085(Sources &s)
{
  return invoke<Fn_gt_085, const sc_dt::sc_int_base &, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_086 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_086>(&sc_dt::operator>)), Fn_gt_086>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_086(Sources &s)
{
  return invoke<Fn_gt_086, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_087 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_087>(&sc_dt::operator>)), Fn_gt_087>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_087(Sources &s)
{
  return invoke<Fn_gt_087, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_088 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_088>(&sc_dt::operator>)), Fn_gt_088>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_088(Sources &s)
{
  return invoke<Fn_gt_088, const sc_dt::sc_int_base &, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_089 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_089>(&sc_dt::operator>)), Fn_gt_089>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_089(Sources &s)
{
  return invoke<Fn_gt_089, const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_090 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_090>(&sc_dt::operator>)), Fn_gt_090>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_int_base &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_090(Sources &s)
{
  return invoke<Fn_gt_090, const sc_dt::sc_int_base &, const sc_dt::sc_int_base &>(&sc_dt::operator>, s);
}
using Fn_gt_091 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_091>(&sc_dt::operator>)), Fn_gt_091>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_091(Sources &s)
{
  return invoke<Fn_gt_091, const sc_dt::sc_int_base &, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_092 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_092>(&sc_dt::operator>)), Fn_gt_092>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_092(Sources &s)
{
  return invoke<Fn_gt_092, const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_093 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_093>(&sc_dt::operator>)), Fn_gt_093>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_093(Sources &s)
{
  return invoke<Fn_gt_093, const sc_dt::sc_signed &, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_094 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_094>(&sc_dt::operator>)), Fn_gt_094>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_094(Sources &s)
{
  return invoke<Fn_gt_094, const sc_dt::sc_signed &, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_095 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_095>(&sc_dt::operator>)), Fn_gt_095>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_095(Sources &s)
{
  return invoke<Fn_gt_095, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_096 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_096>(&sc_dt::operator>)), Fn_gt_096>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_096(Sources &s)
{
  return invoke<Fn_gt_096, const sc_dt::sc_signed &, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_097 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_097>(&sc_dt::operator>)), Fn_gt_097>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_097(Sources &s)
{
  return invoke<Fn_gt_097, const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_098 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_098>(&sc_dt::operator>)), Fn_gt_098>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_098(Sources &s)
{
  return invoke<Fn_gt_098, const sc_dt::sc_signed &, const sc_dt::sc_int_base &>(&sc_dt::operator>, s);
}
using Fn_gt_099 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_099>(&sc_dt::operator>)), Fn_gt_099>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_099(Sources &s)
{
  return invoke<Fn_gt_099, const sc_dt::sc_signed &, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_100 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_100>(&sc_dt::operator>)), Fn_gt_100>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_100(Sources &s)
{
  return invoke<Fn_gt_100, const sc_dt::sc_signed &, const sc_dt::sc_uint_base &>(&sc_dt::operator>, s);
}
using Fn_gt_101 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_101>(&sc_dt::operator>)), Fn_gt_101>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_101(Sources &s)
{
  return invoke<Fn_gt_101, const sc_dt::sc_signed &, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_102 = bool (*)(const sc_dt::sc_signed &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_102>(&sc_dt::operator>)), Fn_gt_102>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_102(Sources &s)
{
  return invoke<Fn_gt_102, const sc_dt::sc_signed &, int>(&sc_dt::operator>, s);
}
using Fn_gt_103 = bool (*)(const sc_dt::sc_signed &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_103>(&sc_dt::operator>)), Fn_gt_103>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_103(Sources &s)
{
  return invoke<Fn_gt_103, const sc_dt::sc_signed &, long>(&sc_dt::operator>, s);
}
using Fn_gt_104 = bool (*)(const sc_dt::sc_signed &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_104>(&sc_dt::operator>)), Fn_gt_104>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_104(Sources &s)
{
  return invoke<Fn_gt_104, const sc_dt::sc_signed &, sc_dt::int64>(&sc_dt::operator>, s);
}
using Fn_gt_105 = bool (*)(const sc_dt::sc_signed &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_105>(&sc_dt::operator>)), Fn_gt_105>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_105(Sources &s)
{
  return invoke<Fn_gt_105, const sc_dt::sc_signed &, sc_dt::uint64>(&sc_dt::operator>, s);
}
using Fn_gt_106 = bool (*)(const sc_dt::sc_signed &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_106>(&sc_dt::operator>)), Fn_gt_106>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_106(Sources &s)
{
  return invoke<Fn_gt_106, const sc_dt::sc_signed &, unsigned int>(&sc_dt::operator>, s);
}
using Fn_gt_107 = bool (*)(const sc_dt::sc_signed &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_107>(&sc_dt::operator>)), Fn_gt_107>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_signed &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_107(Sources &s)
{
  return invoke<Fn_gt_107, const sc_dt::sc_signed &, unsigned long>(&sc_dt::operator>, s);
}
using Fn_gt_108 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_108>(&sc_dt::operator>)), Fn_gt_108>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_108(Sources &s)
{
  return invoke<Fn_gt_108, const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_109 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_109>(&sc_dt::operator>)), Fn_gt_109>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_109(Sources &s)
{
  return invoke<Fn_gt_109, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_110 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_110>(&sc_dt::operator>)), Fn_gt_110>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_110(Sources &s)
{
  return invoke<Fn_gt_110, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_111 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_111>(&sc_dt::operator>)), Fn_gt_111>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_111(Sources &s)
{
  return invoke<Fn_gt_111, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_112 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_112>(&sc_dt::operator>)), Fn_gt_112>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_112(Sources &s)
{
  return invoke<Fn_gt_112, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_113 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_113>(&sc_dt::operator>)), Fn_gt_113>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_113(Sources &s)
{
  return invoke<Fn_gt_113, const sc_dt::sc_uint_base &, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_114 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_114>(&sc_dt::operator>)), Fn_gt_114>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_114(Sources &s)
{
  return invoke<Fn_gt_114, const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &>(&sc_dt::operator>, s);
}
using Fn_gt_115 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_115>(&sc_dt::operator>)), Fn_gt_115>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_115(Sources &s)
{
  return invoke<Fn_gt_115, const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_116 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_116>(&sc_dt::operator>)), Fn_gt_116>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_116(Sources &s)
{
  return invoke<Fn_gt_116, const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_117 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_117>(&sc_dt::operator>)), Fn_gt_117>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_117(Sources &s)
{
  return invoke<Fn_gt_117, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_118 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_118>(&sc_dt::operator>)), Fn_gt_118>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_118(Sources &s)
{
  return invoke<Fn_gt_118, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_119 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_119>(&sc_dt::operator>)), Fn_gt_119>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_119(Sources &s)
{
  return invoke<Fn_gt_119, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_120 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_120>(&sc_dt::operator>)), Fn_gt_120>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_120(Sources &s)
{
  return invoke<Fn_gt_120, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_121 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_121>(&sc_dt::operator>)), Fn_gt_121>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_121(Sources &s)
{
  return invoke<Fn_gt_121, const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &>(&sc_dt::operator>, s);
}
using Fn_gt_122 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_122>(&sc_dt::operator>)), Fn_gt_122>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_122(Sources &s)
{
  return invoke<Fn_gt_122, const sc_dt::sc_unsigned &, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_123 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_123>(&sc_dt::operator>)), Fn_gt_123>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_123(Sources &s)
{
  return invoke<Fn_gt_123, const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &>(&sc_dt::operator>, s);
}
using Fn_gt_124 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_124>(&sc_dt::operator>)), Fn_gt_124>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_124(Sources &s)
{
  return invoke<Fn_gt_124, const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_125 = bool (*)(const sc_dt::sc_unsigned &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_125>(&sc_dt::operator>)), Fn_gt_125>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_125(Sources &s)
{
  return invoke<Fn_gt_125, const sc_dt::sc_unsigned &, int>(&sc_dt::operator>, s);
}
using Fn_gt_126 = bool (*)(const sc_dt::sc_unsigned &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_126>(&sc_dt::operator>)), Fn_gt_126>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_126(Sources &s)
{
  return invoke<Fn_gt_126, const sc_dt::sc_unsigned &, long>(&sc_dt::operator>, s);
}
using Fn_gt_127 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_127>(&sc_dt::operator>)), Fn_gt_127>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_127(Sources &s)
{
  return invoke<Fn_gt_127, const sc_dt::sc_unsigned &, sc_dt::int64>(&sc_dt::operator>, s);
}
using Fn_gt_128 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_128>(&sc_dt::operator>)), Fn_gt_128>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_128(Sources &s)
{
  return invoke<Fn_gt_128, const sc_dt::sc_unsigned &, sc_dt::uint64>(&sc_dt::operator>, s);
}
using Fn_gt_129 = bool (*)(const sc_dt::sc_unsigned &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_129>(&sc_dt::operator>)), Fn_gt_129>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_129(Sources &s)
{
  return invoke<Fn_gt_129, const sc_dt::sc_unsigned &, unsigned int>(&sc_dt::operator>, s);
}
using Fn_gt_130 = bool (*)(const sc_dt::sc_unsigned &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_130>(&sc_dt::operator>)), Fn_gt_130>,
              "exact overload sc_dt::operator> bool (const sc_dt::sc_unsigned &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_130(Sources &s)
{
  return invoke<Fn_gt_130, const sc_dt::sc_unsigned &, unsigned long>(&sc_dt::operator>, s);
}
using Fn_gt_131 = bool (*)(double, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_131>(&sc_dt::operator>)), Fn_gt_131>,
              "exact overload sc_dt::operator> bool (double, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_131(Sources &s)
{
  return invoke<Fn_gt_131, double, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_132 = bool (*)(double, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_132>(&sc_dt::operator>)), Fn_gt_132>,
              "exact overload sc_dt::operator> bool (double, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_132(Sources &s)
{
  return invoke<Fn_gt_132, double, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_133 = bool (*)(double, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_133>(&sc_dt::operator>)), Fn_gt_133>,
              "exact overload sc_dt::operator> bool (double, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_133(Sources &s)
{
  return invoke<Fn_gt_133, double, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_134 = bool (*)(double, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_134>(&sc_dt::operator>)), Fn_gt_134>,
              "exact overload sc_dt::operator> bool (double, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_134(Sources &s)
{
  return invoke<Fn_gt_134, double, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_135 = bool (*)(float, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_135>(&sc_dt::operator>)), Fn_gt_135>,
              "exact overload sc_dt::operator> bool (float, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_135(Sources &s)
{
  return invoke<Fn_gt_135, float, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_136 = bool (*)(float, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_136>(&sc_dt::operator>)), Fn_gt_136>,
              "exact overload sc_dt::operator> bool (float, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_136(Sources &s)
{
  return invoke<Fn_gt_136, float, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_137 = bool (*)(float, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_137>(&sc_dt::operator>)), Fn_gt_137>,
              "exact overload sc_dt::operator> bool (float, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_137(Sources &s)
{
  return invoke<Fn_gt_137, float, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_138 = bool (*)(float, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_138>(&sc_dt::operator>)), Fn_gt_138>,
              "exact overload sc_dt::operator> bool (float, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_138(Sources &s)
{
  return invoke<Fn_gt_138, float, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_139 = bool (*)(int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_139>(&sc_dt::operator>)), Fn_gt_139>,
              "exact overload sc_dt::operator> bool (int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_139(Sources &s)
{
  return invoke<Fn_gt_139, int, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_140 = bool (*)(int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_140>(&sc_dt::operator>)), Fn_gt_140>,
              "exact overload sc_dt::operator> bool (int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_140(Sources &s)
{
  return invoke<Fn_gt_140, int, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_141 = bool (*)(int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_141>(&sc_dt::operator>)), Fn_gt_141>,
              "exact overload sc_dt::operator> bool (int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_141(Sources &s)
{
  return invoke<Fn_gt_141, int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_142 = bool (*)(int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_142>(&sc_dt::operator>)), Fn_gt_142>,
              "exact overload sc_dt::operator> bool (int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_142(Sources &s)
{
  return invoke<Fn_gt_142, int, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_143 = bool (*)(int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_143>(&sc_dt::operator>)), Fn_gt_143>,
              "exact overload sc_dt::operator> bool (int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_143(Sources &s)
{
  return invoke<Fn_gt_143, int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_144 = bool (*)(int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_144>(&sc_dt::operator>)), Fn_gt_144>,
              "exact overload sc_dt::operator> bool (int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_144(Sources &s)
{
  return invoke<Fn_gt_144, int, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_145 = bool (*)(int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_145>(&sc_dt::operator>)), Fn_gt_145>,
              "exact overload sc_dt::operator> bool (int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_145(Sources &s)
{
  return invoke<Fn_gt_145, int, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_146 = bool (*)(long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_146>(&sc_dt::operator>)), Fn_gt_146>,
              "exact overload sc_dt::operator> bool (long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_146(Sources &s)
{
  return invoke<Fn_gt_146, long, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_147 = bool (*)(long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_147>(&sc_dt::operator>)), Fn_gt_147>,
              "exact overload sc_dt::operator> bool (long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_147(Sources &s)
{
  return invoke<Fn_gt_147, long, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_148 = bool (*)(long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_148>(&sc_dt::operator>)), Fn_gt_148>,
              "exact overload sc_dt::operator> bool (long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_148(Sources &s)
{
  return invoke<Fn_gt_148, long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_149 = bool (*)(long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_149>(&sc_dt::operator>)), Fn_gt_149>,
              "exact overload sc_dt::operator> bool (long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_149(Sources &s)
{
  return invoke<Fn_gt_149, long, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_150 = bool (*)(long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_150>(&sc_dt::operator>)), Fn_gt_150>,
              "exact overload sc_dt::operator> bool (long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_150(Sources &s)
{
  return invoke<Fn_gt_150, long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_151 = bool (*)(long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_151>(&sc_dt::operator>)), Fn_gt_151>,
              "exact overload sc_dt::operator> bool (long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_151(Sources &s)
{
  return invoke<Fn_gt_151, long, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_152 = bool (*)(long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_152>(&sc_dt::operator>)), Fn_gt_152>,
              "exact overload sc_dt::operator> bool (long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_152(Sources &s)
{
  return invoke<Fn_gt_152, long, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_153 = bool (*)(sc_dt::int64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_153>(&sc_dt::operator>)), Fn_gt_153>,
              "exact overload sc_dt::operator> bool (sc_dt::int64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_153(Sources &s)
{
  return invoke<Fn_gt_153, sc_dt::int64, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_154 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_154>(&sc_dt::operator>)), Fn_gt_154>,
              "exact overload sc_dt::operator> bool (sc_dt::int64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_154(Sources &s)
{
  return invoke<Fn_gt_154, sc_dt::int64, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_155 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_155>(&sc_dt::operator>)), Fn_gt_155>,
              "exact overload sc_dt::operator> bool (sc_dt::int64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_155(Sources &s)
{
  return invoke<Fn_gt_155, sc_dt::int64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_156 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_156>(&sc_dt::operator>)), Fn_gt_156>,
              "exact overload sc_dt::operator> bool (sc_dt::int64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_156(Sources &s)
{
  return invoke<Fn_gt_156, sc_dt::int64, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_157 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_157>(&sc_dt::operator>)), Fn_gt_157>,
              "exact overload sc_dt::operator> bool (sc_dt::int64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_157(Sources &s)
{
  return invoke<Fn_gt_157, sc_dt::int64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_158 = bool (*)(sc_dt::int64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_158>(&sc_dt::operator>)), Fn_gt_158>,
              "exact overload sc_dt::operator> bool (sc_dt::int64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_158(Sources &s)
{
  return invoke<Fn_gt_158, sc_dt::int64, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_159 = bool (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_159>(&sc_dt::operator>)), Fn_gt_159>,
              "exact overload sc_dt::operator> bool (sc_dt::int64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_159(Sources &s)
{
  return invoke<Fn_gt_159, sc_dt::int64, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_160 = bool (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_160>(&sc_dt::operator>)), Fn_gt_160>,
              "exact overload sc_dt::operator> bool (sc_dt::uint64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_160(Sources &s)
{
  return invoke<Fn_gt_160, sc_dt::uint64, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_161 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_161>(&sc_dt::operator>)), Fn_gt_161>,
              "exact overload sc_dt::operator> bool (sc_dt::uint64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_161(Sources &s)
{
  return invoke<Fn_gt_161, sc_dt::uint64, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_162 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_162>(&sc_dt::operator>)), Fn_gt_162>,
              "exact overload sc_dt::operator> bool (sc_dt::uint64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_162(Sources &s)
{
  return invoke<Fn_gt_162, sc_dt::uint64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_163 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_163>(&sc_dt::operator>)), Fn_gt_163>,
              "exact overload sc_dt::operator> bool (sc_dt::uint64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_163(Sources &s)
{
  return invoke<Fn_gt_163, sc_dt::uint64, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_164 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_164>(&sc_dt::operator>)), Fn_gt_164>,
              "exact overload sc_dt::operator> bool (sc_dt::uint64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_164(Sources &s)
{
  return invoke<Fn_gt_164, sc_dt::uint64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_165 = bool (*)(sc_dt::uint64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_165>(&sc_dt::operator>)), Fn_gt_165>,
              "exact overload sc_dt::operator> bool (sc_dt::uint64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_165(Sources &s)
{
  return invoke<Fn_gt_165, sc_dt::uint64, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_166 = bool (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_166>(&sc_dt::operator>)), Fn_gt_166>,
              "exact overload sc_dt::operator> bool (sc_dt::uint64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_166(Sources &s)
{
  return invoke<Fn_gt_166, sc_dt::uint64, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_167 = bool (*)(unsigned int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_167>(&sc_dt::operator>)), Fn_gt_167>,
              "exact overload sc_dt::operator> bool (unsigned int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_167(Sources &s)
{
  return invoke<Fn_gt_167, unsigned int, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_168 = bool (*)(unsigned int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_168>(&sc_dt::operator>)), Fn_gt_168>,
              "exact overload sc_dt::operator> bool (unsigned int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_168(Sources &s)
{
  return invoke<Fn_gt_168, unsigned int, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_169 = bool (*)(unsigned int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_169>(&sc_dt::operator>)), Fn_gt_169>,
              "exact overload sc_dt::operator> bool (unsigned int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_169(Sources &s)
{
  return invoke<Fn_gt_169, unsigned int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_170 = bool (*)(unsigned int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_170>(&sc_dt::operator>)), Fn_gt_170>,
              "exact overload sc_dt::operator> bool (unsigned int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_170(Sources &s)
{
  return invoke<Fn_gt_170, unsigned int, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_171 = bool (*)(unsigned int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_171>(&sc_dt::operator>)), Fn_gt_171>,
              "exact overload sc_dt::operator> bool (unsigned int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_171(Sources &s)
{
  return invoke<Fn_gt_171, unsigned int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_172 = bool (*)(unsigned int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_172>(&sc_dt::operator>)), Fn_gt_172>,
              "exact overload sc_dt::operator> bool (unsigned int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_172(Sources &s)
{
  return invoke<Fn_gt_172, unsigned int, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_173 = bool (*)(unsigned int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_173>(&sc_dt::operator>)), Fn_gt_173>,
              "exact overload sc_dt::operator> bool (unsigned int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_173(Sources &s)
{
  return invoke<Fn_gt_173, unsigned int, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
using Fn_gt_174 = bool (*)(unsigned long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_174>(&sc_dt::operator>)), Fn_gt_174>,
              "exact overload sc_dt::operator> bool (unsigned long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_174(Sources &s)
{
  return invoke<Fn_gt_174, unsigned long, const sc_dt::sc_concatref &>(&sc_dt::operator>, s);
}
using Fn_gt_175 = bool (*)(unsigned long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_175>(&sc_dt::operator>)), Fn_gt_175>,
              "exact overload sc_dt::operator> bool (unsigned long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_175(Sources &s)
{
  return invoke<Fn_gt_175, unsigned long, const sc_dt::sc_fxnum &>(&sc_dt::operator>, s);
}
using Fn_gt_176 = bool (*)(unsigned long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_176>(&sc_dt::operator>)), Fn_gt_176>,
              "exact overload sc_dt::operator> bool (unsigned long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_176(Sources &s)
{
  return invoke<Fn_gt_176, unsigned long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_177 = bool (*)(unsigned long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_177>(&sc_dt::operator>)), Fn_gt_177>,
              "exact overload sc_dt::operator> bool (unsigned long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_177(Sources &s)
{
  return invoke<Fn_gt_177, unsigned long, const sc_dt::sc_fxval &>(&sc_dt::operator>, s);
}
using Fn_gt_178 = bool (*)(unsigned long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_178>(&sc_dt::operator>)), Fn_gt_178>,
              "exact overload sc_dt::operator> bool (unsigned long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_178(Sources &s)
{
  return invoke<Fn_gt_178, unsigned long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>, s);
}
using Fn_gt_179 = bool (*)(unsigned long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_179>(&sc_dt::operator>)), Fn_gt_179>,
              "exact overload sc_dt::operator> bool (unsigned long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_179(Sources &s)
{
  return invoke<Fn_gt_179, unsigned long, const sc_dt::sc_signed &>(&sc_dt::operator>, s);
}
using Fn_gt_180 = bool (*)(unsigned long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_gt_180>(&sc_dt::operator>)), Fn_gt_180>,
              "exact overload sc_dt::operator> bool (unsigned long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt_180(Sources &s)
{
  return invoke<Fn_gt_180, unsigned long, const sc_dt::sc_unsigned &>(&sc_dt::operator>, s);
}
inline unsigned count_gt = 181;
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_gt(unsigned index, Sources &s)
{
  switch (index)
  {
  case 0:
    return case_gt_000(s);
  case 1:
    return case_gt_001(s);
  case 2:
    return case_gt_002(s);
  case 3:
    return case_gt_003(s);
  case 4:
    return case_gt_004(s);
  case 5:
    return case_gt_005(s);
  case 6:
    return case_gt_006(s);
  case 7:
    return case_gt_007(s);
  case 8:
    return case_gt_008(s);
  case 9:
    return case_gt_009(s);
  case 10:
    return case_gt_010(s);
  case 11:
    return case_gt_011(s);
  case 12:
    return case_gt_012(s);
  case 13:
    return case_gt_013(s);
  case 14:
    return case_gt_014(s);
  case 15:
    return case_gt_015(s);
  case 16:
    return case_gt_016(s);
  case 17:
    return case_gt_017(s);
  case 18:
    return case_gt_018(s);
  case 19:
    return case_gt_019(s);
  case 20:
    return case_gt_020(s);
  case 21:
    return case_gt_021(s);
  case 22:
    return case_gt_022(s);
  case 23:
    return case_gt_023(s);
  case 24:
    return case_gt_024(s);
  case 25:
    return case_gt_025(s);
  case 26:
    return case_gt_026(s);
  case 27:
    return case_gt_027(s);
  case 28:
    return case_gt_028(s);
  case 29:
    return case_gt_029(s);
  case 30:
    return case_gt_030(s);
  case 31:
    return case_gt_031(s);
  case 32:
    return case_gt_032(s);
  case 33:
    return case_gt_033(s);
  case 34:
    return case_gt_034(s);
  case 35:
    return case_gt_035(s);
  case 36:
    return case_gt_036(s);
  case 37:
    return case_gt_037(s);
  case 38:
    return case_gt_038(s);
  case 39:
    return case_gt_039(s);
  case 40:
    return case_gt_040(s);
  case 41:
    return case_gt_041(s);
  case 42:
    return case_gt_042(s);
  case 43:
    return case_gt_043(s);
  case 44:
    return case_gt_044(s);
  case 45:
    return case_gt_045(s);
  case 46:
    return case_gt_046(s);
  case 47:
    return case_gt_047(s);
  case 48:
    return case_gt_048(s);
  case 49:
    return case_gt_049(s);
  case 50:
    return case_gt_050(s);
  case 51:
    return case_gt_051(s);
  case 52:
    return case_gt_052(s);
  case 53:
    return case_gt_053(s);
  case 54:
    return case_gt_054(s);
  case 55:
    return case_gt_055(s);
  case 56:
    return case_gt_056(s);
  case 57:
    return case_gt_057(s);
  case 58:
    return case_gt_058(s);
  case 59:
    return case_gt_059(s);
  case 60:
    return case_gt_060(s);
  case 61:
    return case_gt_061(s);
  case 62:
    return case_gt_062(s);
  case 63:
    return case_gt_063(s);
  case 64:
    return case_gt_064(s);
  case 65:
    return case_gt_065(s);
  case 66:
    return case_gt_066(s);
  case 67:
    return case_gt_067(s);
  case 68:
    return case_gt_068(s);
  case 69:
    return case_gt_069(s);
  case 70:
    return case_gt_070(s);
  case 71:
    return case_gt_071(s);
  case 72:
    return case_gt_072(s);
  case 73:
    return case_gt_073(s);
  case 74:
    return case_gt_074(s);
  case 75:
    return case_gt_075(s);
  case 76:
    return case_gt_076(s);
  case 77:
    return case_gt_077(s);
  case 78:
    return case_gt_078(s);
  case 79:
    return case_gt_079(s);
  case 80:
    return case_gt_080(s);
  case 81:
    return case_gt_081(s);
  case 82:
    return case_gt_082(s);
  case 83:
    return case_gt_083(s);
  case 84:
    return case_gt_084(s);
  case 85:
    return case_gt_085(s);
  case 86:
    return case_gt_086(s);
  case 87:
    return case_gt_087(s);
  case 88:
    return case_gt_088(s);
  case 89:
    return case_gt_089(s);
  case 90:
    return case_gt_090(s);
  case 91:
    return case_gt_091(s);
  case 92:
    return case_gt_092(s);
  case 93:
    return case_gt_093(s);
  case 94:
    return case_gt_094(s);
  case 95:
    return case_gt_095(s);
  case 96:
    return case_gt_096(s);
  case 97:
    return case_gt_097(s);
  case 98:
    return case_gt_098(s);
  case 99:
    return case_gt_099(s);
  case 100:
    return case_gt_100(s);
  case 101:
    return case_gt_101(s);
  case 102:
    return case_gt_102(s);
  case 103:
    return case_gt_103(s);
  case 104:
    return case_gt_104(s);
  case 105:
    return case_gt_105(s);
  case 106:
    return case_gt_106(s);
  case 107:
    return case_gt_107(s);
  case 108:
    return case_gt_108(s);
  case 109:
    return case_gt_109(s);
  case 110:
    return case_gt_110(s);
  case 111:
    return case_gt_111(s);
  case 112:
    return case_gt_112(s);
  case 113:
    return case_gt_113(s);
  case 114:
    return case_gt_114(s);
  case 115:
    return case_gt_115(s);
  case 116:
    return case_gt_116(s);
  case 117:
    return case_gt_117(s);
  case 118:
    return case_gt_118(s);
  case 119:
    return case_gt_119(s);
  case 120:
    return case_gt_120(s);
  case 121:
    return case_gt_121(s);
  case 122:
    return case_gt_122(s);
  case 123:
    return case_gt_123(s);
  case 124:
    return case_gt_124(s);
  case 125:
    return case_gt_125(s);
  case 126:
    return case_gt_126(s);
  case 127:
    return case_gt_127(s);
  case 128:
    return case_gt_128(s);
  case 129:
    return case_gt_129(s);
  case 130:
    return case_gt_130(s);
  case 131:
    return case_gt_131(s);
  case 132:
    return case_gt_132(s);
  case 133:
    return case_gt_133(s);
  case 134:
    return case_gt_134(s);
  case 135:
    return case_gt_135(s);
  case 136:
    return case_gt_136(s);
  case 137:
    return case_gt_137(s);
  case 138:
    return case_gt_138(s);
  case 139:
    return case_gt_139(s);
  case 140:
    return case_gt_140(s);
  case 141:
    return case_gt_141(s);
  case 142:
    return case_gt_142(s);
  case 143:
    return case_gt_143(s);
  case 144:
    return case_gt_144(s);
  case 145:
    return case_gt_145(s);
  case 146:
    return case_gt_146(s);
  case 147:
    return case_gt_147(s);
  case 148:
    return case_gt_148(s);
  case 149:
    return case_gt_149(s);
  case 150:
    return case_gt_150(s);
  case 151:
    return case_gt_151(s);
  case 152:
    return case_gt_152(s);
  case 153:
    return case_gt_153(s);
  case 154:
    return case_gt_154(s);
  case 155:
    return case_gt_155(s);
  case 156:
    return case_gt_156(s);
  case 157:
    return case_gt_157(s);
  case 158:
    return case_gt_158(s);
  case 159:
    return case_gt_159(s);
  case 160:
    return case_gt_160(s);
  case 161:
    return case_gt_161(s);
  case 162:
    return case_gt_162(s);
  case 163:
    return case_gt_163(s);
  case 164:
    return case_gt_164(s);
  case 165:
    return case_gt_165(s);
  case 166:
    return case_gt_166(s);
  case 167:
    return case_gt_167(s);
  case 168:
    return case_gt_168(s);
  case 169:
    return case_gt_169(s);
  case 170:
    return case_gt_170(s);
  case 171:
    return case_gt_171(s);
  case 172:
    return case_gt_172(s);
  case 173:
    return case_gt_173(s);
  case 174:
    return case_gt_174(s);
  case 175:
    return case_gt_175(s);
  case 176:
    return case_gt_176(s);
  case 177:
    return case_gt_177(s);
  case 178:
    return case_gt_178(s);
  case 179:
    return case_gt_179(s);
  case 180:
    return case_gt_180(s);
  default:
    throw std::out_of_range("unknown comparison case index");
  }
}
using Fn_ge_000 = bool (*)(bool, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_000>(&sc_dt::operator>=)), Fn_ge_000>,
              "exact overload sc_dt::operator>= bool (bool, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_000(Sources &s)
{
  return invoke<Fn_ge_000, bool, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_001 = bool (*)(const char *, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_001>(&sc_dt::operator>=)), Fn_ge_001>,
              "exact overload sc_dt::operator>= bool (const char *, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_001(Sources &s)
{
  return invoke<Fn_ge_001, const char *, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_002 = bool (*)(const char *, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_002>(&sc_dt::operator>=)), Fn_ge_002>,
              "exact overload sc_dt::operator>= bool (const char *, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_002(Sources &s)
{
  return invoke<Fn_ge_002, const char *, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_003 = bool (*)(const char *, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_003>(&sc_dt::operator>=)), Fn_ge_003>,
              "exact overload sc_dt::operator>= bool (const char *, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_003(Sources &s)
{
  return invoke<Fn_ge_003, const char *, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_004 = bool (*)(const char *, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_004>(&sc_dt::operator>=)), Fn_ge_004>,
              "exact overload sc_dt::operator>= bool (const char *, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_004(Sources &s)
{
  return invoke<Fn_ge_004, const char *, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_005 = bool (*)(const sc_dt::sc_concatref &, bool);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_005>(&sc_dt::operator>=)), Fn_ge_005>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, bool)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_005(Sources &s)
{
  return invoke<Fn_ge_005, const sc_dt::sc_concatref &, bool>(&sc_dt::operator>=, s);
}
using Fn_ge_006 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_006>(&sc_dt::operator>=)), Fn_ge_006>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_006(Sources &s)
{
  return invoke<Fn_ge_006, const sc_dt::sc_concatref &, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_007 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_007>(&sc_dt::operator>=)), Fn_ge_007>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_007(Sources &s)
{
  return invoke<Fn_ge_007, const sc_dt::sc_concatref &, const sc_dt::sc_int_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_008 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_008>(&sc_dt::operator>=)), Fn_ge_008>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_008(Sources &s)
{
  return invoke<Fn_ge_008, const sc_dt::sc_concatref &, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_009 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_009>(&sc_dt::operator>=)), Fn_ge_009>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_009(Sources &s)
{
  return invoke<Fn_ge_009, const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_010 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_010>(&sc_dt::operator>=)), Fn_ge_010>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_010(Sources &s)
{
  return invoke<Fn_ge_010, const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_011 = bool (*)(const sc_dt::sc_concatref &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_011>(&sc_dt::operator>=)), Fn_ge_011>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_011(Sources &s)
{
  return invoke<Fn_ge_011, const sc_dt::sc_concatref &, int>(&sc_dt::operator>=, s);
}
using Fn_ge_012 = bool (*)(const sc_dt::sc_concatref &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_012>(&sc_dt::operator>=)), Fn_ge_012>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_012(Sources &s)
{
  return invoke<Fn_ge_012, const sc_dt::sc_concatref &, long>(&sc_dt::operator>=, s);
}
using Fn_ge_013 = bool (*)(const sc_dt::sc_concatref &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_013>(&sc_dt::operator>=)), Fn_ge_013>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_013(Sources &s)
{
  return invoke<Fn_ge_013, const sc_dt::sc_concatref &, sc_dt::int64>(&sc_dt::operator>=, s);
}
using Fn_ge_014 = bool (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_014>(&sc_dt::operator>=)), Fn_ge_014>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_014(Sources &s)
{
  return invoke<Fn_ge_014, const sc_dt::sc_concatref &, sc_dt::uint64>(&sc_dt::operator>=, s);
}
using Fn_ge_015 = bool (*)(const sc_dt::sc_concatref &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_015>(&sc_dt::operator>=)), Fn_ge_015>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_015(Sources &s)
{
  return invoke<Fn_ge_015, const sc_dt::sc_concatref &, unsigned int>(&sc_dt::operator>=, s);
}
using Fn_ge_016 = bool (*)(const sc_dt::sc_concatref &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_016>(&sc_dt::operator>=)), Fn_ge_016>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_concatref &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_016(Sources &s)
{
  return invoke<Fn_ge_016, const sc_dt::sc_concatref &, unsigned long>(&sc_dt::operator>=, s);
}
using Fn_ge_017 = bool (*)(const sc_dt::sc_fxnum &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_017>(&sc_dt::operator>=)), Fn_ge_017>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_017(Sources &s)
{
  return invoke<Fn_ge_017, const sc_dt::sc_fxnum &, const char *>(&sc_dt::operator>=, s);
}
using Fn_ge_018 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_018>(&sc_dt::operator>=)), Fn_ge_018>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_018(Sources &s)
{
  return invoke<Fn_ge_018, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_019 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_019>(&sc_dt::operator>=)), Fn_ge_019>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_019(Sources &s)
{
  return invoke<Fn_ge_019, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_020 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_020>(&sc_dt::operator>=)), Fn_ge_020>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_020(Sources &s)
{
  return invoke<Fn_ge_020, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_021 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_021>(&sc_dt::operator>=)), Fn_ge_021>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_021(Sources &s)
{
  return invoke<Fn_ge_021, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_022 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_022>(&sc_dt::operator>=)), Fn_ge_022>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_022(Sources &s)
{
  return invoke<Fn_ge_022, const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_023 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_023>(&sc_dt::operator>=)), Fn_ge_023>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_023(Sources &s)
{
  return invoke<Fn_ge_023, const sc_dt::sc_fxnum &, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_024 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_024>(&sc_dt::operator>=)), Fn_ge_024>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_024(Sources &s)
{
  return invoke<Fn_ge_024, const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_025 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_025>(&sc_dt::operator>=)), Fn_ge_025>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_025(Sources &s)
{
  return invoke<Fn_ge_025, const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_026 = bool (*)(const sc_dt::sc_fxnum &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_026>(&sc_dt::operator>=)), Fn_ge_026>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_026(Sources &s)
{
  return invoke<Fn_ge_026, const sc_dt::sc_fxnum &, double>(&sc_dt::operator>=, s);
}
using Fn_ge_027 = bool (*)(const sc_dt::sc_fxnum &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_027>(&sc_dt::operator>=)), Fn_ge_027>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_027(Sources &s)
{
  return invoke<Fn_ge_027, const sc_dt::sc_fxnum &, float>(&sc_dt::operator>=, s);
}
using Fn_ge_028 = bool (*)(const sc_dt::sc_fxnum &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_028>(&sc_dt::operator>=)), Fn_ge_028>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_028(Sources &s)
{
  return invoke<Fn_ge_028, const sc_dt::sc_fxnum &, int>(&sc_dt::operator>=, s);
}
using Fn_ge_029 = bool (*)(const sc_dt::sc_fxnum &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_029>(&sc_dt::operator>=)), Fn_ge_029>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_029(Sources &s)
{
  return invoke<Fn_ge_029, const sc_dt::sc_fxnum &, long>(&sc_dt::operator>=, s);
}
using Fn_ge_030 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_030>(&sc_dt::operator>=)), Fn_ge_030>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_030(Sources &s)
{
  return invoke<Fn_ge_030, const sc_dt::sc_fxnum &, sc_dt::int64>(&sc_dt::operator>=, s);
}
using Fn_ge_031 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_031>(&sc_dt::operator>=)), Fn_ge_031>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_031(Sources &s)
{
  return invoke<Fn_ge_031, const sc_dt::sc_fxnum &, sc_dt::uint64>(&sc_dt::operator>=, s);
}
using Fn_ge_032 = bool (*)(const sc_dt::sc_fxnum &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_032>(&sc_dt::operator>=)), Fn_ge_032>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_032(Sources &s)
{
  return invoke<Fn_ge_032, const sc_dt::sc_fxnum &, unsigned int>(&sc_dt::operator>=, s);
}
using Fn_ge_033 = bool (*)(const sc_dt::sc_fxnum &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_033>(&sc_dt::operator>=)), Fn_ge_033>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_033(Sources &s)
{
  return invoke<Fn_ge_033, const sc_dt::sc_fxnum &, unsigned long>(&sc_dt::operator>=, s);
}
using Fn_ge_034 = bool (*)(const sc_dt::sc_fxnum_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_034>(&sc_dt::operator>=)), Fn_ge_034>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_034(Sources &s)
{
  return invoke<Fn_ge_034, const sc_dt::sc_fxnum_fast &, const char *>(&sc_dt::operator>=, s);
}
using Fn_ge_035 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_035>(&sc_dt::operator>=)), Fn_ge_035>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_035(Sources &s)
{
  return invoke<Fn_ge_035, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_036 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_036>(&sc_dt::operator>=)), Fn_ge_036>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_036(Sources &s)
{
  return invoke<Fn_ge_036, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_037 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_037>(&sc_dt::operator>=)), Fn_ge_037>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_037(Sources &s)
{
  return invoke<Fn_ge_037, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_038 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_038>(&sc_dt::operator>=)), Fn_ge_038>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_038(Sources &s)
{
  return invoke<Fn_ge_038, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_039 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_039>(&sc_dt::operator>=)), Fn_ge_039>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_039(Sources &s)
{
  return invoke<Fn_ge_039, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_040 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_040>(&sc_dt::operator>=)), Fn_ge_040>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_040(Sources &s)
{
  return invoke<Fn_ge_040, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_041 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_041>(&sc_dt::operator>=)), Fn_ge_041>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_041(Sources &s)
{
  return invoke<Fn_ge_041, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_042 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_042>(&sc_dt::operator>=)), Fn_ge_042>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_042(Sources &s)
{
  return invoke<Fn_ge_042, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_043 = bool (*)(const sc_dt::sc_fxnum_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_043>(&sc_dt::operator>=)), Fn_ge_043>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_043(Sources &s)
{
  return invoke<Fn_ge_043, const sc_dt::sc_fxnum_fast &, double>(&sc_dt::operator>=, s);
}
using Fn_ge_044 = bool (*)(const sc_dt::sc_fxnum_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_044>(&sc_dt::operator>=)), Fn_ge_044>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_044(Sources &s)
{
  return invoke<Fn_ge_044, const sc_dt::sc_fxnum_fast &, float>(&sc_dt::operator>=, s);
}
using Fn_ge_045 = bool (*)(const sc_dt::sc_fxnum_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_045>(&sc_dt::operator>=)), Fn_ge_045>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_045(Sources &s)
{
  return invoke<Fn_ge_045, const sc_dt::sc_fxnum_fast &, int>(&sc_dt::operator>=, s);
}
using Fn_ge_046 = bool (*)(const sc_dt::sc_fxnum_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_046>(&sc_dt::operator>=)), Fn_ge_046>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_046(Sources &s)
{
  return invoke<Fn_ge_046, const sc_dt::sc_fxnum_fast &, long>(&sc_dt::operator>=, s);
}
using Fn_ge_047 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_047>(&sc_dt::operator>=)), Fn_ge_047>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_047(Sources &s)
{
  return invoke<Fn_ge_047, const sc_dt::sc_fxnum_fast &, sc_dt::int64>(&sc_dt::operator>=, s);
}
using Fn_ge_048 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_048>(&sc_dt::operator>=)), Fn_ge_048>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_048(Sources &s)
{
  return invoke<Fn_ge_048, const sc_dt::sc_fxnum_fast &, sc_dt::uint64>(&sc_dt::operator>=, s);
}
using Fn_ge_049 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_049>(&sc_dt::operator>=)), Fn_ge_049>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_049(Sources &s)
{
  return invoke<Fn_ge_049, const sc_dt::sc_fxnum_fast &, unsigned int>(&sc_dt::operator>=, s);
}
using Fn_ge_050 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_050>(&sc_dt::operator>=)), Fn_ge_050>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxnum_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_050(Sources &s)
{
  return invoke<Fn_ge_050, const sc_dt::sc_fxnum_fast &, unsigned long>(&sc_dt::operator>=, s);
}
using Fn_ge_051 = bool (*)(const sc_dt::sc_fxval &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_051>(&sc_dt::operator>=)), Fn_ge_051>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_051(Sources &s)
{
  return invoke<Fn_ge_051, const sc_dt::sc_fxval &, const char *>(&sc_dt::operator>=, s);
}
using Fn_ge_052 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_052>(&sc_dt::operator>=)), Fn_ge_052>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_052(Sources &s)
{
  return invoke<Fn_ge_052, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_053 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_053>(&sc_dt::operator>=)), Fn_ge_053>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_053(Sources &s)
{
  return invoke<Fn_ge_053, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_054 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_054>(&sc_dt::operator>=)), Fn_ge_054>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_054(Sources &s)
{
  return invoke<Fn_ge_054, const sc_dt::sc_fxval &, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_055 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_055>(&sc_dt::operator>=)), Fn_ge_055>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_055(Sources &s)
{
  return invoke<Fn_ge_055, const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_056 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_056>(&sc_dt::operator>=)), Fn_ge_056>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_056(Sources &s)
{
  return invoke<Fn_ge_056, const sc_dt::sc_fxval &, const sc_dt::sc_int_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_057 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_057>(&sc_dt::operator>=)), Fn_ge_057>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_057(Sources &s)
{
  return invoke<Fn_ge_057, const sc_dt::sc_fxval &, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_058 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_058>(&sc_dt::operator>=)), Fn_ge_058>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_058(Sources &s)
{
  return invoke<Fn_ge_058, const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_059 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_059>(&sc_dt::operator>=)), Fn_ge_059>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_059(Sources &s)
{
  return invoke<Fn_ge_059, const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_060 = bool (*)(const sc_dt::sc_fxval &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_060>(&sc_dt::operator>=)), Fn_ge_060>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_060(Sources &s)
{
  return invoke<Fn_ge_060, const sc_dt::sc_fxval &, double>(&sc_dt::operator>=, s);
}
using Fn_ge_061 = bool (*)(const sc_dt::sc_fxval &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_061>(&sc_dt::operator>=)), Fn_ge_061>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_061(Sources &s)
{
  return invoke<Fn_ge_061, const sc_dt::sc_fxval &, float>(&sc_dt::operator>=, s);
}
using Fn_ge_062 = bool (*)(const sc_dt::sc_fxval &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_062>(&sc_dt::operator>=)), Fn_ge_062>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_062(Sources &s)
{
  return invoke<Fn_ge_062, const sc_dt::sc_fxval &, int>(&sc_dt::operator>=, s);
}
using Fn_ge_063 = bool (*)(const sc_dt::sc_fxval &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_063>(&sc_dt::operator>=)), Fn_ge_063>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_063(Sources &s)
{
  return invoke<Fn_ge_063, const sc_dt::sc_fxval &, long>(&sc_dt::operator>=, s);
}
using Fn_ge_064 = bool (*)(const sc_dt::sc_fxval &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_064>(&sc_dt::operator>=)), Fn_ge_064>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_064(Sources &s)
{
  return invoke<Fn_ge_064, const sc_dt::sc_fxval &, sc_dt::int64>(&sc_dt::operator>=, s);
}
using Fn_ge_065 = bool (*)(const sc_dt::sc_fxval &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_065>(&sc_dt::operator>=)), Fn_ge_065>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_065(Sources &s)
{
  return invoke<Fn_ge_065, const sc_dt::sc_fxval &, sc_dt::uint64>(&sc_dt::operator>=, s);
}
using Fn_ge_066 = bool (*)(const sc_dt::sc_fxval &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_066>(&sc_dt::operator>=)), Fn_ge_066>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_066(Sources &s)
{
  return invoke<Fn_ge_066, const sc_dt::sc_fxval &, unsigned int>(&sc_dt::operator>=, s);
}
using Fn_ge_067 = bool (*)(const sc_dt::sc_fxval &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_067>(&sc_dt::operator>=)), Fn_ge_067>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_067(Sources &s)
{
  return invoke<Fn_ge_067, const sc_dt::sc_fxval &, unsigned long>(&sc_dt::operator>=, s);
}
using Fn_ge_068 = bool (*)(const sc_dt::sc_fxval_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_068>(&sc_dt::operator>=)), Fn_ge_068>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_068(Sources &s)
{
  return invoke<Fn_ge_068, const sc_dt::sc_fxval_fast &, const char *>(&sc_dt::operator>=, s);
}
using Fn_ge_069 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_069>(&sc_dt::operator>=)), Fn_ge_069>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_069(Sources &s)
{
  return invoke<Fn_ge_069, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_070 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_070>(&sc_dt::operator>=)), Fn_ge_070>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_070(Sources &s)
{
  return invoke<Fn_ge_070, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_071 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_071>(&sc_dt::operator>=)), Fn_ge_071>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_071(Sources &s)
{
  return invoke<Fn_ge_071, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_072 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_072>(&sc_dt::operator>=)), Fn_ge_072>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_072(Sources &s)
{
  return invoke<Fn_ge_072, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_073 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_073>(&sc_dt::operator>=)), Fn_ge_073>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_073(Sources &s)
{
  return invoke<Fn_ge_073, const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_074 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_074>(&sc_dt::operator>=)), Fn_ge_074>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_074(Sources &s)
{
  return invoke<Fn_ge_074, const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_075 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_075>(&sc_dt::operator>=)), Fn_ge_075>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_075(Sources &s)
{
  return invoke<Fn_ge_075, const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_076 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_076>(&sc_dt::operator>=)), Fn_ge_076>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_076(Sources &s)
{
  return invoke<Fn_ge_076, const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_077 = bool (*)(const sc_dt::sc_fxval_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_077>(&sc_dt::operator>=)), Fn_ge_077>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_077(Sources &s)
{
  return invoke<Fn_ge_077, const sc_dt::sc_fxval_fast &, double>(&sc_dt::operator>=, s);
}
using Fn_ge_078 = bool (*)(const sc_dt::sc_fxval_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_078>(&sc_dt::operator>=)), Fn_ge_078>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_078(Sources &s)
{
  return invoke<Fn_ge_078, const sc_dt::sc_fxval_fast &, float>(&sc_dt::operator>=, s);
}
using Fn_ge_079 = bool (*)(const sc_dt::sc_fxval_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_079>(&sc_dt::operator>=)), Fn_ge_079>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_079(Sources &s)
{
  return invoke<Fn_ge_079, const sc_dt::sc_fxval_fast &, int>(&sc_dt::operator>=, s);
}
using Fn_ge_080 = bool (*)(const sc_dt::sc_fxval_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_080>(&sc_dt::operator>=)), Fn_ge_080>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_080(Sources &s)
{
  return invoke<Fn_ge_080, const sc_dt::sc_fxval_fast &, long>(&sc_dt::operator>=, s);
}
using Fn_ge_081 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_081>(&sc_dt::operator>=)), Fn_ge_081>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_081(Sources &s)
{
  return invoke<Fn_ge_081, const sc_dt::sc_fxval_fast &, sc_dt::int64>(&sc_dt::operator>=, s);
}
using Fn_ge_082 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_082>(&sc_dt::operator>=)), Fn_ge_082>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_082(Sources &s)
{
  return invoke<Fn_ge_082, const sc_dt::sc_fxval_fast &, sc_dt::uint64>(&sc_dt::operator>=, s);
}
using Fn_ge_083 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_083>(&sc_dt::operator>=)), Fn_ge_083>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_083(Sources &s)
{
  return invoke<Fn_ge_083, const sc_dt::sc_fxval_fast &, unsigned int>(&sc_dt::operator>=, s);
}
using Fn_ge_084 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_084>(&sc_dt::operator>=)), Fn_ge_084>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_fxval_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_084(Sources &s)
{
  return invoke<Fn_ge_084, const sc_dt::sc_fxval_fast &, unsigned long>(&sc_dt::operator>=, s);
}
using Fn_ge_085 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_085>(&sc_dt::operator>=)), Fn_ge_085>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_085(Sources &s)
{
  return invoke<Fn_ge_085, const sc_dt::sc_int_base &, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_086 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_086>(&sc_dt::operator>=)), Fn_ge_086>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_086(Sources &s)
{
  return invoke<Fn_ge_086, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_087 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_087>(&sc_dt::operator>=)), Fn_ge_087>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_087(Sources &s)
{
  return invoke<Fn_ge_087, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_088 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_088>(&sc_dt::operator>=)), Fn_ge_088>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_088(Sources &s)
{
  return invoke<Fn_ge_088, const sc_dt::sc_int_base &, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_089 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_089>(&sc_dt::operator>=)), Fn_ge_089>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_089(Sources &s)
{
  return invoke<Fn_ge_089, const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_090 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_090>(&sc_dt::operator>=)), Fn_ge_090>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_int_base &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_090(Sources &s)
{
  return invoke<Fn_ge_090, const sc_dt::sc_int_base &, const sc_dt::sc_int_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_091 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_091>(&sc_dt::operator>=)), Fn_ge_091>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_091(Sources &s)
{
  return invoke<Fn_ge_091, const sc_dt::sc_int_base &, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_092 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_092>(&sc_dt::operator>=)), Fn_ge_092>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_092(Sources &s)
{
  return invoke<Fn_ge_092, const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_093 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_093>(&sc_dt::operator>=)), Fn_ge_093>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_093(Sources &s)
{
  return invoke<Fn_ge_093, const sc_dt::sc_signed &, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_094 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_094>(&sc_dt::operator>=)), Fn_ge_094>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_094(Sources &s)
{
  return invoke<Fn_ge_094, const sc_dt::sc_signed &, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_095 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_095>(&sc_dt::operator>=)), Fn_ge_095>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_095(Sources &s)
{
  return invoke<Fn_ge_095, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_096 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_096>(&sc_dt::operator>=)), Fn_ge_096>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_096(Sources &s)
{
  return invoke<Fn_ge_096, const sc_dt::sc_signed &, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_097 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_097>(&sc_dt::operator>=)), Fn_ge_097>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_097(Sources &s)
{
  return invoke<Fn_ge_097, const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_098 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_098>(&sc_dt::operator>=)), Fn_ge_098>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_098(Sources &s)
{
  return invoke<Fn_ge_098, const sc_dt::sc_signed &, const sc_dt::sc_int_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_099 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_099>(&sc_dt::operator>=)), Fn_ge_099>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_099(Sources &s)
{
  return invoke<Fn_ge_099, const sc_dt::sc_signed &, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_100 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_100>(&sc_dt::operator>=)), Fn_ge_100>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_100(Sources &s)
{
  return invoke<Fn_ge_100, const sc_dt::sc_signed &, const sc_dt::sc_uint_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_101 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_101>(&sc_dt::operator>=)), Fn_ge_101>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_101(Sources &s)
{
  return invoke<Fn_ge_101, const sc_dt::sc_signed &, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_102 = bool (*)(const sc_dt::sc_signed &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_102>(&sc_dt::operator>=)), Fn_ge_102>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_102(Sources &s)
{
  return invoke<Fn_ge_102, const sc_dt::sc_signed &, int>(&sc_dt::operator>=, s);
}
using Fn_ge_103 = bool (*)(const sc_dt::sc_signed &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_103>(&sc_dt::operator>=)), Fn_ge_103>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_103(Sources &s)
{
  return invoke<Fn_ge_103, const sc_dt::sc_signed &, long>(&sc_dt::operator>=, s);
}
using Fn_ge_104 = bool (*)(const sc_dt::sc_signed &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_104>(&sc_dt::operator>=)), Fn_ge_104>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_104(Sources &s)
{
  return invoke<Fn_ge_104, const sc_dt::sc_signed &, sc_dt::int64>(&sc_dt::operator>=, s);
}
using Fn_ge_105 = bool (*)(const sc_dt::sc_signed &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_105>(&sc_dt::operator>=)), Fn_ge_105>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_105(Sources &s)
{
  return invoke<Fn_ge_105, const sc_dt::sc_signed &, sc_dt::uint64>(&sc_dt::operator>=, s);
}
using Fn_ge_106 = bool (*)(const sc_dt::sc_signed &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_106>(&sc_dt::operator>=)), Fn_ge_106>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_106(Sources &s)
{
  return invoke<Fn_ge_106, const sc_dt::sc_signed &, unsigned int>(&sc_dt::operator>=, s);
}
using Fn_ge_107 = bool (*)(const sc_dt::sc_signed &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_107>(&sc_dt::operator>=)), Fn_ge_107>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_signed &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_107(Sources &s)
{
  return invoke<Fn_ge_107, const sc_dt::sc_signed &, unsigned long>(&sc_dt::operator>=, s);
}
using Fn_ge_108 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_108>(&sc_dt::operator>=)), Fn_ge_108>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_108(Sources &s)
{
  return invoke<Fn_ge_108, const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_109 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_109>(&sc_dt::operator>=)), Fn_ge_109>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_109(Sources &s)
{
  return invoke<Fn_ge_109, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_110 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_110>(&sc_dt::operator>=)), Fn_ge_110>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_110(Sources &s)
{
  return invoke<Fn_ge_110, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_111 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_111>(&sc_dt::operator>=)), Fn_ge_111>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_111(Sources &s)
{
  return invoke<Fn_ge_111, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_112 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_112>(&sc_dt::operator>=)), Fn_ge_112>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_112(Sources &s)
{
  return invoke<Fn_ge_112, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_113 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_113>(&sc_dt::operator>=)), Fn_ge_113>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_113(Sources &s)
{
  return invoke<Fn_ge_113, const sc_dt::sc_uint_base &, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_114 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_114>(&sc_dt::operator>=)), Fn_ge_114>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_114(Sources &s)
{
  return invoke<Fn_ge_114, const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_115 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_115>(&sc_dt::operator>=)), Fn_ge_115>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_115(Sources &s)
{
  return invoke<Fn_ge_115, const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_116 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_116>(&sc_dt::operator>=)), Fn_ge_116>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_116(Sources &s)
{
  return invoke<Fn_ge_116, const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_117 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_117>(&sc_dt::operator>=)), Fn_ge_117>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_117(Sources &s)
{
  return invoke<Fn_ge_117, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_118 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_118>(&sc_dt::operator>=)), Fn_ge_118>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_118(Sources &s)
{
  return invoke<Fn_ge_118, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_119 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_119>(&sc_dt::operator>=)), Fn_ge_119>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_119(Sources &s)
{
  return invoke<Fn_ge_119, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_120 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_120>(&sc_dt::operator>=)), Fn_ge_120>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_120(Sources &s)
{
  return invoke<Fn_ge_120, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_121 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_121>(&sc_dt::operator>=)), Fn_ge_121>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_121(Sources &s)
{
  return invoke<Fn_ge_121, const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_122 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_122>(&sc_dt::operator>=)), Fn_ge_122>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_122(Sources &s)
{
  return invoke<Fn_ge_122, const sc_dt::sc_unsigned &, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_123 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_123>(&sc_dt::operator>=)), Fn_ge_123>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_123(Sources &s)
{
  return invoke<Fn_ge_123, const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &>(&sc_dt::operator>=, s);
}
using Fn_ge_124 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_124>(&sc_dt::operator>=)), Fn_ge_124>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_124(Sources &s)
{
  return invoke<Fn_ge_124, const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_125 = bool (*)(const sc_dt::sc_unsigned &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_125>(&sc_dt::operator>=)), Fn_ge_125>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_125(Sources &s)
{
  return invoke<Fn_ge_125, const sc_dt::sc_unsigned &, int>(&sc_dt::operator>=, s);
}
using Fn_ge_126 = bool (*)(const sc_dt::sc_unsigned &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_126>(&sc_dt::operator>=)), Fn_ge_126>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_126(Sources &s)
{
  return invoke<Fn_ge_126, const sc_dt::sc_unsigned &, long>(&sc_dt::operator>=, s);
}
using Fn_ge_127 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_127>(&sc_dt::operator>=)), Fn_ge_127>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_127(Sources &s)
{
  return invoke<Fn_ge_127, const sc_dt::sc_unsigned &, sc_dt::int64>(&sc_dt::operator>=, s);
}
using Fn_ge_128 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_128>(&sc_dt::operator>=)), Fn_ge_128>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_128(Sources &s)
{
  return invoke<Fn_ge_128, const sc_dt::sc_unsigned &, sc_dt::uint64>(&sc_dt::operator>=, s);
}
using Fn_ge_129 = bool (*)(const sc_dt::sc_unsigned &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_129>(&sc_dt::operator>=)), Fn_ge_129>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_129(Sources &s)
{
  return invoke<Fn_ge_129, const sc_dt::sc_unsigned &, unsigned int>(&sc_dt::operator>=, s);
}
using Fn_ge_130 = bool (*)(const sc_dt::sc_unsigned &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_130>(&sc_dt::operator>=)), Fn_ge_130>,
              "exact overload sc_dt::operator>= bool (const sc_dt::sc_unsigned &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_130(Sources &s)
{
  return invoke<Fn_ge_130, const sc_dt::sc_unsigned &, unsigned long>(&sc_dt::operator>=, s);
}
using Fn_ge_131 = bool (*)(double, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_131>(&sc_dt::operator>=)), Fn_ge_131>,
              "exact overload sc_dt::operator>= bool (double, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_131(Sources &s)
{
  return invoke<Fn_ge_131, double, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_132 = bool (*)(double, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_132>(&sc_dt::operator>=)), Fn_ge_132>,
              "exact overload sc_dt::operator>= bool (double, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_132(Sources &s)
{
  return invoke<Fn_ge_132, double, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_133 = bool (*)(double, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_133>(&sc_dt::operator>=)), Fn_ge_133>,
              "exact overload sc_dt::operator>= bool (double, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_133(Sources &s)
{
  return invoke<Fn_ge_133, double, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_134 = bool (*)(double, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_134>(&sc_dt::operator>=)), Fn_ge_134>,
              "exact overload sc_dt::operator>= bool (double, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_134(Sources &s)
{
  return invoke<Fn_ge_134, double, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_135 = bool (*)(float, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_135>(&sc_dt::operator>=)), Fn_ge_135>,
              "exact overload sc_dt::operator>= bool (float, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_135(Sources &s)
{
  return invoke<Fn_ge_135, float, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_136 = bool (*)(float, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_136>(&sc_dt::operator>=)), Fn_ge_136>,
              "exact overload sc_dt::operator>= bool (float, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_136(Sources &s)
{
  return invoke<Fn_ge_136, float, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_137 = bool (*)(float, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_137>(&sc_dt::operator>=)), Fn_ge_137>,
              "exact overload sc_dt::operator>= bool (float, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_137(Sources &s)
{
  return invoke<Fn_ge_137, float, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_138 = bool (*)(float, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_138>(&sc_dt::operator>=)), Fn_ge_138>,
              "exact overload sc_dt::operator>= bool (float, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_138(Sources &s)
{
  return invoke<Fn_ge_138, float, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_139 = bool (*)(int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_139>(&sc_dt::operator>=)), Fn_ge_139>,
              "exact overload sc_dt::operator>= bool (int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_139(Sources &s)
{
  return invoke<Fn_ge_139, int, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_140 = bool (*)(int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_140>(&sc_dt::operator>=)), Fn_ge_140>,
              "exact overload sc_dt::operator>= bool (int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_140(Sources &s)
{
  return invoke<Fn_ge_140, int, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_141 = bool (*)(int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_141>(&sc_dt::operator>=)), Fn_ge_141>,
              "exact overload sc_dt::operator>= bool (int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_141(Sources &s)
{
  return invoke<Fn_ge_141, int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_142 = bool (*)(int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_142>(&sc_dt::operator>=)), Fn_ge_142>,
              "exact overload sc_dt::operator>= bool (int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_142(Sources &s)
{
  return invoke<Fn_ge_142, int, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_143 = bool (*)(int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_143>(&sc_dt::operator>=)), Fn_ge_143>,
              "exact overload sc_dt::operator>= bool (int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_143(Sources &s)
{
  return invoke<Fn_ge_143, int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_144 = bool (*)(int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_144>(&sc_dt::operator>=)), Fn_ge_144>,
              "exact overload sc_dt::operator>= bool (int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_144(Sources &s)
{
  return invoke<Fn_ge_144, int, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_145 = bool (*)(int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_145>(&sc_dt::operator>=)), Fn_ge_145>,
              "exact overload sc_dt::operator>= bool (int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_145(Sources &s)
{
  return invoke<Fn_ge_145, int, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_146 = bool (*)(long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_146>(&sc_dt::operator>=)), Fn_ge_146>,
              "exact overload sc_dt::operator>= bool (long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_146(Sources &s)
{
  return invoke<Fn_ge_146, long, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_147 = bool (*)(long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_147>(&sc_dt::operator>=)), Fn_ge_147>,
              "exact overload sc_dt::operator>= bool (long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_147(Sources &s)
{
  return invoke<Fn_ge_147, long, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_148 = bool (*)(long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_148>(&sc_dt::operator>=)), Fn_ge_148>,
              "exact overload sc_dt::operator>= bool (long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_148(Sources &s)
{
  return invoke<Fn_ge_148, long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_149 = bool (*)(long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_149>(&sc_dt::operator>=)), Fn_ge_149>,
              "exact overload sc_dt::operator>= bool (long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_149(Sources &s)
{
  return invoke<Fn_ge_149, long, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_150 = bool (*)(long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_150>(&sc_dt::operator>=)), Fn_ge_150>,
              "exact overload sc_dt::operator>= bool (long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_150(Sources &s)
{
  return invoke<Fn_ge_150, long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_151 = bool (*)(long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_151>(&sc_dt::operator>=)), Fn_ge_151>,
              "exact overload sc_dt::operator>= bool (long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_151(Sources &s)
{
  return invoke<Fn_ge_151, long, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_152 = bool (*)(long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_152>(&sc_dt::operator>=)), Fn_ge_152>,
              "exact overload sc_dt::operator>= bool (long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_152(Sources &s)
{
  return invoke<Fn_ge_152, long, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_153 = bool (*)(sc_dt::int64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_153>(&sc_dt::operator>=)), Fn_ge_153>,
              "exact overload sc_dt::operator>= bool (sc_dt::int64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_153(Sources &s)
{
  return invoke<Fn_ge_153, sc_dt::int64, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_154 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_154>(&sc_dt::operator>=)), Fn_ge_154>,
              "exact overload sc_dt::operator>= bool (sc_dt::int64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_154(Sources &s)
{
  return invoke<Fn_ge_154, sc_dt::int64, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_155 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_155>(&sc_dt::operator>=)), Fn_ge_155>,
              "exact overload sc_dt::operator>= bool (sc_dt::int64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_155(Sources &s)
{
  return invoke<Fn_ge_155, sc_dt::int64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_156 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_156>(&sc_dt::operator>=)), Fn_ge_156>,
              "exact overload sc_dt::operator>= bool (sc_dt::int64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_156(Sources &s)
{
  return invoke<Fn_ge_156, sc_dt::int64, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_157 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_157>(&sc_dt::operator>=)), Fn_ge_157>,
              "exact overload sc_dt::operator>= bool (sc_dt::int64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_157(Sources &s)
{
  return invoke<Fn_ge_157, sc_dt::int64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_158 = bool (*)(sc_dt::int64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_158>(&sc_dt::operator>=)), Fn_ge_158>,
              "exact overload sc_dt::operator>= bool (sc_dt::int64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_158(Sources &s)
{
  return invoke<Fn_ge_158, sc_dt::int64, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_159 = bool (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_159>(&sc_dt::operator>=)), Fn_ge_159>,
              "exact overload sc_dt::operator>= bool (sc_dt::int64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_159(Sources &s)
{
  return invoke<Fn_ge_159, sc_dt::int64, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_160 = bool (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_160>(&sc_dt::operator>=)), Fn_ge_160>,
              "exact overload sc_dt::operator>= bool (sc_dt::uint64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_160(Sources &s)
{
  return invoke<Fn_ge_160, sc_dt::uint64, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_161 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_161>(&sc_dt::operator>=)), Fn_ge_161>,
              "exact overload sc_dt::operator>= bool (sc_dt::uint64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_161(Sources &s)
{
  return invoke<Fn_ge_161, sc_dt::uint64, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_162 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_162>(&sc_dt::operator>=)), Fn_ge_162>,
              "exact overload sc_dt::operator>= bool (sc_dt::uint64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_162(Sources &s)
{
  return invoke<Fn_ge_162, sc_dt::uint64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_163 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_163>(&sc_dt::operator>=)), Fn_ge_163>,
              "exact overload sc_dt::operator>= bool (sc_dt::uint64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_163(Sources &s)
{
  return invoke<Fn_ge_163, sc_dt::uint64, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_164 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_164>(&sc_dt::operator>=)), Fn_ge_164>,
              "exact overload sc_dt::operator>= bool (sc_dt::uint64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_164(Sources &s)
{
  return invoke<Fn_ge_164, sc_dt::uint64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_165 = bool (*)(sc_dt::uint64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_165>(&sc_dt::operator>=)), Fn_ge_165>,
              "exact overload sc_dt::operator>= bool (sc_dt::uint64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_165(Sources &s)
{
  return invoke<Fn_ge_165, sc_dt::uint64, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_166 = bool (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_166>(&sc_dt::operator>=)), Fn_ge_166>,
              "exact overload sc_dt::operator>= bool (sc_dt::uint64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_166(Sources &s)
{
  return invoke<Fn_ge_166, sc_dt::uint64, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_167 = bool (*)(unsigned int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_167>(&sc_dt::operator>=)), Fn_ge_167>,
              "exact overload sc_dt::operator>= bool (unsigned int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_167(Sources &s)
{
  return invoke<Fn_ge_167, unsigned int, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_168 = bool (*)(unsigned int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_168>(&sc_dt::operator>=)), Fn_ge_168>,
              "exact overload sc_dt::operator>= bool (unsigned int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_168(Sources &s)
{
  return invoke<Fn_ge_168, unsigned int, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_169 = bool (*)(unsigned int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_169>(&sc_dt::operator>=)), Fn_ge_169>,
              "exact overload sc_dt::operator>= bool (unsigned int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_169(Sources &s)
{
  return invoke<Fn_ge_169, unsigned int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_170 = bool (*)(unsigned int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_170>(&sc_dt::operator>=)), Fn_ge_170>,
              "exact overload sc_dt::operator>= bool (unsigned int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_170(Sources &s)
{
  return invoke<Fn_ge_170, unsigned int, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_171 = bool (*)(unsigned int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_171>(&sc_dt::operator>=)), Fn_ge_171>,
              "exact overload sc_dt::operator>= bool (unsigned int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_171(Sources &s)
{
  return invoke<Fn_ge_171, unsigned int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_172 = bool (*)(unsigned int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_172>(&sc_dt::operator>=)), Fn_ge_172>,
              "exact overload sc_dt::operator>= bool (unsigned int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_172(Sources &s)
{
  return invoke<Fn_ge_172, unsigned int, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_173 = bool (*)(unsigned int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_173>(&sc_dt::operator>=)), Fn_ge_173>,
              "exact overload sc_dt::operator>= bool (unsigned int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_173(Sources &s)
{
  return invoke<Fn_ge_173, unsigned int, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
using Fn_ge_174 = bool (*)(unsigned long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_174>(&sc_dt::operator>=)), Fn_ge_174>,
              "exact overload sc_dt::operator>= bool (unsigned long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_174(Sources &s)
{
  return invoke<Fn_ge_174, unsigned long, const sc_dt::sc_concatref &>(&sc_dt::operator>=, s);
}
using Fn_ge_175 = bool (*)(unsigned long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_175>(&sc_dt::operator>=)), Fn_ge_175>,
              "exact overload sc_dt::operator>= bool (unsigned long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_175(Sources &s)
{
  return invoke<Fn_ge_175, unsigned long, const sc_dt::sc_fxnum &>(&sc_dt::operator>=, s);
}
using Fn_ge_176 = bool (*)(unsigned long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_176>(&sc_dt::operator>=)), Fn_ge_176>,
              "exact overload sc_dt::operator>= bool (unsigned long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_176(Sources &s)
{
  return invoke<Fn_ge_176, unsigned long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_177 = bool (*)(unsigned long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_177>(&sc_dt::operator>=)), Fn_ge_177>,
              "exact overload sc_dt::operator>= bool (unsigned long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_177(Sources &s)
{
  return invoke<Fn_ge_177, unsigned long, const sc_dt::sc_fxval &>(&sc_dt::operator>=, s);
}
using Fn_ge_178 = bool (*)(unsigned long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_178>(&sc_dt::operator>=)), Fn_ge_178>,
              "exact overload sc_dt::operator>= bool (unsigned long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_178(Sources &s)
{
  return invoke<Fn_ge_178, unsigned long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator>=, s);
}
using Fn_ge_179 = bool (*)(unsigned long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_179>(&sc_dt::operator>=)), Fn_ge_179>,
              "exact overload sc_dt::operator>= bool (unsigned long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_179(Sources &s)
{
  return invoke<Fn_ge_179, unsigned long, const sc_dt::sc_signed &>(&sc_dt::operator>=, s);
}
using Fn_ge_180 = bool (*)(unsigned long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ge_180>(&sc_dt::operator>=)), Fn_ge_180>,
              "exact overload sc_dt::operator>= bool (unsigned long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge_180(Sources &s)
{
  return invoke<Fn_ge_180, unsigned long, const sc_dt::sc_unsigned &>(&sc_dt::operator>=, s);
}
inline unsigned count_ge = 181;
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ge(unsigned index, Sources &s)
{
  switch (index)
  {
  case 0:
    return case_ge_000(s);
  case 1:
    return case_ge_001(s);
  case 2:
    return case_ge_002(s);
  case 3:
    return case_ge_003(s);
  case 4:
    return case_ge_004(s);
  case 5:
    return case_ge_005(s);
  case 6:
    return case_ge_006(s);
  case 7:
    return case_ge_007(s);
  case 8:
    return case_ge_008(s);
  case 9:
    return case_ge_009(s);
  case 10:
    return case_ge_010(s);
  case 11:
    return case_ge_011(s);
  case 12:
    return case_ge_012(s);
  case 13:
    return case_ge_013(s);
  case 14:
    return case_ge_014(s);
  case 15:
    return case_ge_015(s);
  case 16:
    return case_ge_016(s);
  case 17:
    return case_ge_017(s);
  case 18:
    return case_ge_018(s);
  case 19:
    return case_ge_019(s);
  case 20:
    return case_ge_020(s);
  case 21:
    return case_ge_021(s);
  case 22:
    return case_ge_022(s);
  case 23:
    return case_ge_023(s);
  case 24:
    return case_ge_024(s);
  case 25:
    return case_ge_025(s);
  case 26:
    return case_ge_026(s);
  case 27:
    return case_ge_027(s);
  case 28:
    return case_ge_028(s);
  case 29:
    return case_ge_029(s);
  case 30:
    return case_ge_030(s);
  case 31:
    return case_ge_031(s);
  case 32:
    return case_ge_032(s);
  case 33:
    return case_ge_033(s);
  case 34:
    return case_ge_034(s);
  case 35:
    return case_ge_035(s);
  case 36:
    return case_ge_036(s);
  case 37:
    return case_ge_037(s);
  case 38:
    return case_ge_038(s);
  case 39:
    return case_ge_039(s);
  case 40:
    return case_ge_040(s);
  case 41:
    return case_ge_041(s);
  case 42:
    return case_ge_042(s);
  case 43:
    return case_ge_043(s);
  case 44:
    return case_ge_044(s);
  case 45:
    return case_ge_045(s);
  case 46:
    return case_ge_046(s);
  case 47:
    return case_ge_047(s);
  case 48:
    return case_ge_048(s);
  case 49:
    return case_ge_049(s);
  case 50:
    return case_ge_050(s);
  case 51:
    return case_ge_051(s);
  case 52:
    return case_ge_052(s);
  case 53:
    return case_ge_053(s);
  case 54:
    return case_ge_054(s);
  case 55:
    return case_ge_055(s);
  case 56:
    return case_ge_056(s);
  case 57:
    return case_ge_057(s);
  case 58:
    return case_ge_058(s);
  case 59:
    return case_ge_059(s);
  case 60:
    return case_ge_060(s);
  case 61:
    return case_ge_061(s);
  case 62:
    return case_ge_062(s);
  case 63:
    return case_ge_063(s);
  case 64:
    return case_ge_064(s);
  case 65:
    return case_ge_065(s);
  case 66:
    return case_ge_066(s);
  case 67:
    return case_ge_067(s);
  case 68:
    return case_ge_068(s);
  case 69:
    return case_ge_069(s);
  case 70:
    return case_ge_070(s);
  case 71:
    return case_ge_071(s);
  case 72:
    return case_ge_072(s);
  case 73:
    return case_ge_073(s);
  case 74:
    return case_ge_074(s);
  case 75:
    return case_ge_075(s);
  case 76:
    return case_ge_076(s);
  case 77:
    return case_ge_077(s);
  case 78:
    return case_ge_078(s);
  case 79:
    return case_ge_079(s);
  case 80:
    return case_ge_080(s);
  case 81:
    return case_ge_081(s);
  case 82:
    return case_ge_082(s);
  case 83:
    return case_ge_083(s);
  case 84:
    return case_ge_084(s);
  case 85:
    return case_ge_085(s);
  case 86:
    return case_ge_086(s);
  case 87:
    return case_ge_087(s);
  case 88:
    return case_ge_088(s);
  case 89:
    return case_ge_089(s);
  case 90:
    return case_ge_090(s);
  case 91:
    return case_ge_091(s);
  case 92:
    return case_ge_092(s);
  case 93:
    return case_ge_093(s);
  case 94:
    return case_ge_094(s);
  case 95:
    return case_ge_095(s);
  case 96:
    return case_ge_096(s);
  case 97:
    return case_ge_097(s);
  case 98:
    return case_ge_098(s);
  case 99:
    return case_ge_099(s);
  case 100:
    return case_ge_100(s);
  case 101:
    return case_ge_101(s);
  case 102:
    return case_ge_102(s);
  case 103:
    return case_ge_103(s);
  case 104:
    return case_ge_104(s);
  case 105:
    return case_ge_105(s);
  case 106:
    return case_ge_106(s);
  case 107:
    return case_ge_107(s);
  case 108:
    return case_ge_108(s);
  case 109:
    return case_ge_109(s);
  case 110:
    return case_ge_110(s);
  case 111:
    return case_ge_111(s);
  case 112:
    return case_ge_112(s);
  case 113:
    return case_ge_113(s);
  case 114:
    return case_ge_114(s);
  case 115:
    return case_ge_115(s);
  case 116:
    return case_ge_116(s);
  case 117:
    return case_ge_117(s);
  case 118:
    return case_ge_118(s);
  case 119:
    return case_ge_119(s);
  case 120:
    return case_ge_120(s);
  case 121:
    return case_ge_121(s);
  case 122:
    return case_ge_122(s);
  case 123:
    return case_ge_123(s);
  case 124:
    return case_ge_124(s);
  case 125:
    return case_ge_125(s);
  case 126:
    return case_ge_126(s);
  case 127:
    return case_ge_127(s);
  case 128:
    return case_ge_128(s);
  case 129:
    return case_ge_129(s);
  case 130:
    return case_ge_130(s);
  case 131:
    return case_ge_131(s);
  case 132:
    return case_ge_132(s);
  case 133:
    return case_ge_133(s);
  case 134:
    return case_ge_134(s);
  case 135:
    return case_ge_135(s);
  case 136:
    return case_ge_136(s);
  case 137:
    return case_ge_137(s);
  case 138:
    return case_ge_138(s);
  case 139:
    return case_ge_139(s);
  case 140:
    return case_ge_140(s);
  case 141:
    return case_ge_141(s);
  case 142:
    return case_ge_142(s);
  case 143:
    return case_ge_143(s);
  case 144:
    return case_ge_144(s);
  case 145:
    return case_ge_145(s);
  case 146:
    return case_ge_146(s);
  case 147:
    return case_ge_147(s);
  case 148:
    return case_ge_148(s);
  case 149:
    return case_ge_149(s);
  case 150:
    return case_ge_150(s);
  case 151:
    return case_ge_151(s);
  case 152:
    return case_ge_152(s);
  case 153:
    return case_ge_153(s);
  case 154:
    return case_ge_154(s);
  case 155:
    return case_ge_155(s);
  case 156:
    return case_ge_156(s);
  case 157:
    return case_ge_157(s);
  case 158:
    return case_ge_158(s);
  case 159:
    return case_ge_159(s);
  case 160:
    return case_ge_160(s);
  case 161:
    return case_ge_161(s);
  case 162:
    return case_ge_162(s);
  case 163:
    return case_ge_163(s);
  case 164:
    return case_ge_164(s);
  case 165:
    return case_ge_165(s);
  case 166:
    return case_ge_166(s);
  case 167:
    return case_ge_167(s);
  case 168:
    return case_ge_168(s);
  case 169:
    return case_ge_169(s);
  case 170:
    return case_ge_170(s);
  case 171:
    return case_ge_171(s);
  case 172:
    return case_ge_172(s);
  case 173:
    return case_ge_173(s);
  case 174:
    return case_ge_174(s);
  case 175:
    return case_ge_175(s);
  case 176:
    return case_ge_176(s);
  case 177:
    return case_ge_177(s);
  case 178:
    return case_ge_178(s);
  case 179:
    return case_ge_179(s);
  case 180:
    return case_ge_180(s);
  default:
    throw std::out_of_range("unknown comparison case index");
  }
}
using Fn_eq_000 = bool (*)(bool, const sc_dt::sc_bit &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_000>(&sc_dt::operator==)), Fn_eq_000>,
              "exact overload sc_dt::operator== bool (bool, const sc_dt::sc_bit &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_000(Sources &s)
{
  return invoke<Fn_eq_000, bool, const sc_dt::sc_bit &>(&sc_dt::operator==, s);
}
using Fn_eq_001 = bool (*)(bool, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_001>(&sc_dt::operator==)), Fn_eq_001>,
              "exact overload sc_dt::operator== bool (bool, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_001(Sources &s)
{
  return invoke<Fn_eq_001, bool, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_002 = bool (*)(bool, const sc_dt::sc_logic &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_002>(&sc_dt::operator==)), Fn_eq_002>,
              "exact overload sc_dt::operator== bool (bool, const sc_dt::sc_logic &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_002(Sources &s)
{
  return invoke<Fn_eq_002, bool, const sc_dt::sc_logic &>(&sc_dt::operator==, s);
}
using Fn_eq_003 = bool (*)(char, const sc_dt::sc_bit &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_003>(&sc_dt::operator==)), Fn_eq_003>,
              "exact overload sc_dt::operator== bool (char, const sc_dt::sc_bit &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_003(Sources &s)
{
  return invoke<Fn_eq_003, char, const sc_dt::sc_bit &>(&sc_dt::operator==, s);
}
using Fn_eq_004 = bool (*)(char, const sc_dt::sc_logic &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_004>(&sc_dt::operator==)), Fn_eq_004>,
              "exact overload sc_dt::operator== bool (char, const sc_dt::sc_logic &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_004(Sources &s)
{
  return invoke<Fn_eq_004, char, const sc_dt::sc_logic &>(&sc_dt::operator==, s);
}
using Fn_eq_005 = bool (*)(const bool *, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_005>(&sc_dt::operator==)), Fn_eq_005>,
              "exact overload sc_dt::operator== bool (const bool *, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_005(Sources &s)
{
  return invoke<Fn_eq_005, const bool *, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_006 = bool (*)(const bool *, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_006>(&sc_dt::operator==)), Fn_eq_006>,
              "exact overload sc_dt::operator== bool (const bool *, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_006(Sources &s)
{
  return invoke<Fn_eq_006, const bool *, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_007 = bool (*)(const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_007>(&sc_dt::operator==)), Fn_eq_007>,
              "exact overload sc_dt::operator== bool (const bool *, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_007(Sources &s)
{
  return invoke<Fn_eq_007, const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==, s);
}
using Fn_eq_008 = bool (*)(const char *, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_008>(&sc_dt::operator==)), Fn_eq_008>,
              "exact overload sc_dt::operator== bool (const char *, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_008(Sources &s)
{
  return invoke<Fn_eq_008, const char *, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_009 = bool (*)(const char *, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_009>(&sc_dt::operator==)), Fn_eq_009>,
              "exact overload sc_dt::operator== bool (const char *, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_009(Sources &s)
{
  return invoke<Fn_eq_009, const char *, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_010 = bool (*)(const char *, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_010>(&sc_dt::operator==)), Fn_eq_010>,
              "exact overload sc_dt::operator== bool (const char *, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_010(Sources &s)
{
  return invoke<Fn_eq_010, const char *, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_011 = bool (*)(const char *, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_011>(&sc_dt::operator==)), Fn_eq_011>,
              "exact overload sc_dt::operator== bool (const char *, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_011(Sources &s)
{
  return invoke<Fn_eq_011, const char *, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_012 = bool (*)(const char *, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_012>(&sc_dt::operator==)), Fn_eq_012>,
              "exact overload sc_dt::operator== bool (const char *, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_012(Sources &s)
{
  return invoke<Fn_eq_012, const char *, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_013 = bool (*)(const char *, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_013>(&sc_dt::operator==)), Fn_eq_013>,
              "exact overload sc_dt::operator== bool (const char *, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_013(Sources &s)
{
  return invoke<Fn_eq_013, const char *, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_014 = bool (*)(const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_014>(&sc_dt::operator==)), Fn_eq_014>,
              "exact overload sc_dt::operator== bool (const char *, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_014(Sources &s)
{
  return invoke<Fn_eq_014, const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==, s);
}
using Fn_eq_015 = bool (*)(const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &,
                           const sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_015>(&sc_dt::operator==)), Fn_eq_015>,
              "exact overload sc_dt::operator== bool (const sc_bitref_r<T1> &, const sc_bitref_r<T2> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_015(Sources &s)
{
  return invoke<Fn_eq_015, const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &,
                const sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &>(&sc_dt::operator==, s);
}
using Fn_eq_016 = bool (*)(const sc_dt::sc_bit &, bool);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_016>(&sc_dt::operator==)), Fn_eq_016>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_bit &, bool)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_016(Sources &s)
{
  return invoke<Fn_eq_016, const sc_dt::sc_bit &, bool>(&sc_dt::operator==, s);
}
using Fn_eq_017 = bool (*)(const sc_dt::sc_bit &, char);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_017>(&sc_dt::operator==)), Fn_eq_017>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_bit &, char)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_017(Sources &s)
{
  return invoke<Fn_eq_017, const sc_dt::sc_bit &, char>(&sc_dt::operator==, s);
}
using Fn_eq_018 = bool (*)(const sc_dt::sc_bit &, const sc_dt::sc_bit &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_018>(&sc_dt::operator==)), Fn_eq_018>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_bit &, const sc_dt::sc_bit &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_018(Sources &s)
{
  return invoke<Fn_eq_018, const sc_dt::sc_bit &, const sc_dt::sc_bit &>(&sc_dt::operator==, s);
}
using Fn_eq_019 = bool (*)(const sc_dt::sc_bit &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_019>(&sc_dt::operator==)), Fn_eq_019>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_bit &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_019(Sources &s)
{
  return invoke<Fn_eq_019, const sc_dt::sc_bit &, int>(&sc_dt::operator==, s);
}
using Fn_eq_020 = bool (*)(const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_020>(&sc_dt::operator==)), Fn_eq_020>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_020(Sources &s)
{
  return invoke<Fn_eq_020, const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_021 = bool (*)(const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_021>(&sc_dt::operator==)), Fn_eq_021>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_021(Sources &s)
{
  return invoke<Fn_eq_021, const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_022 = bool (*)(const sc_dt::sc_concatref &, bool);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_022>(&sc_dt::operator==)), Fn_eq_022>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, bool)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_022(Sources &s)
{
  return invoke<Fn_eq_022, const sc_dt::sc_concatref &, bool>(&sc_dt::operator==, s);
}
using Fn_eq_023 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_023>(&sc_dt::operator==)), Fn_eq_023>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_023(Sources &s)
{
  return invoke<Fn_eq_023, const sc_dt::sc_concatref &, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_024 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_024>(&sc_dt::operator==)), Fn_eq_024>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_024(Sources &s)
{
  return invoke<Fn_eq_024, const sc_dt::sc_concatref &, const sc_dt::sc_int_base &>(&sc_dt::operator==, s);
}
using Fn_eq_025 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_025>(&sc_dt::operator==)), Fn_eq_025>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_025(Sources &s)
{
  return invoke<Fn_eq_025, const sc_dt::sc_concatref &, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_026 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_026>(&sc_dt::operator==)), Fn_eq_026>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_026(Sources &s)
{
  return invoke<Fn_eq_026, const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &>(&sc_dt::operator==, s);
}
using Fn_eq_027 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_027>(&sc_dt::operator==)), Fn_eq_027>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_027(Sources &s)
{
  return invoke<Fn_eq_027, const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_028 = bool (*)(const sc_dt::sc_concatref &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_028>(&sc_dt::operator==)), Fn_eq_028>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_028(Sources &s)
{
  return invoke<Fn_eq_028, const sc_dt::sc_concatref &, int>(&sc_dt::operator==, s);
}
using Fn_eq_029 = bool (*)(const sc_dt::sc_concatref &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_029>(&sc_dt::operator==)), Fn_eq_029>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_029(Sources &s)
{
  return invoke<Fn_eq_029, const sc_dt::sc_concatref &, long>(&sc_dt::operator==, s);
}
using Fn_eq_030 = bool (*)(const sc_dt::sc_concatref &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_030>(&sc_dt::operator==)), Fn_eq_030>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_030(Sources &s)
{
  return invoke<Fn_eq_030, const sc_dt::sc_concatref &, sc_dt::int64>(&sc_dt::operator==, s);
}
using Fn_eq_031 = bool (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_031>(&sc_dt::operator==)), Fn_eq_031>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_031(Sources &s)
{
  return invoke<Fn_eq_031, const sc_dt::sc_concatref &, sc_dt::uint64>(&sc_dt::operator==, s);
}
using Fn_eq_032 = bool (*)(const sc_dt::sc_concatref &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_032>(&sc_dt::operator==)), Fn_eq_032>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_032(Sources &s)
{
  return invoke<Fn_eq_032, const sc_dt::sc_concatref &, unsigned int>(&sc_dt::operator==, s);
}
using Fn_eq_033 = bool (*)(const sc_dt::sc_concatref &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_033>(&sc_dt::operator==)), Fn_eq_033>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_concatref &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_033(Sources &s)
{
  return invoke<Fn_eq_033, const sc_dt::sc_concatref &, unsigned long>(&sc_dt::operator==, s);
}
using Fn_eq_034 = bool (*)(const sc_dt::sc_fxcast_switch &, const sc_dt::sc_fxcast_switch &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_034>(&sc_dt::operator==)), Fn_eq_034>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_fxcast_switch &, const sc_dt::sc_fxcast_switch &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_034(Sources &s)
{
  return invoke<Fn_eq_034, const sc_dt::sc_fxcast_switch &, const sc_dt::sc_fxcast_switch &>(&sc_dt::operator==, s);
}
using Fn_eq_035 = bool (*)(const sc_dt::sc_fxnum &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_035>(&sc_dt::operator==)), Fn_eq_035>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_035(Sources &s)
{
  return invoke<Fn_eq_035, const sc_dt::sc_fxnum &, const char *>(&sc_dt::operator==, s);
}
using Fn_eq_036 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_036>(&sc_dt::operator==)), Fn_eq_036>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_036(Sources &s)
{
  return invoke<Fn_eq_036, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_037 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_037>(&sc_dt::operator==)), Fn_eq_037>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_037(Sources &s)
{
  return invoke<Fn_eq_037, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_038 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_038>(&sc_dt::operator==)), Fn_eq_038>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_038(Sources &s)
{
  return invoke<Fn_eq_038, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_039 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_039>(&sc_dt::operator==)), Fn_eq_039>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_039(Sources &s)
{
  return invoke<Fn_eq_039, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_040 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_040>(&sc_dt::operator==)), Fn_eq_040>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_040(Sources &s)
{
  return invoke<Fn_eq_040, const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &>(&sc_dt::operator==, s);
}
using Fn_eq_041 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_041>(&sc_dt::operator==)), Fn_eq_041>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_041(Sources &s)
{
  return invoke<Fn_eq_041, const sc_dt::sc_fxnum &, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_042 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_042>(&sc_dt::operator==)), Fn_eq_042>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_042(Sources &s)
{
  return invoke<Fn_eq_042, const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &>(&sc_dt::operator==, s);
}
using Fn_eq_043 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_043>(&sc_dt::operator==)), Fn_eq_043>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_043(Sources &s)
{
  return invoke<Fn_eq_043, const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_044 = bool (*)(const sc_dt::sc_fxnum &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_044>(&sc_dt::operator==)), Fn_eq_044>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_044(Sources &s)
{
  return invoke<Fn_eq_044, const sc_dt::sc_fxnum &, double>(&sc_dt::operator==, s);
}
using Fn_eq_045 = bool (*)(const sc_dt::sc_fxnum &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_045>(&sc_dt::operator==)), Fn_eq_045>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_045(Sources &s)
{
  return invoke<Fn_eq_045, const sc_dt::sc_fxnum &, float>(&sc_dt::operator==, s);
}
using Fn_eq_046 = bool (*)(const sc_dt::sc_fxnum &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_046>(&sc_dt::operator==)), Fn_eq_046>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_046(Sources &s)
{
  return invoke<Fn_eq_046, const sc_dt::sc_fxnum &, int>(&sc_dt::operator==, s);
}
using Fn_eq_047 = bool (*)(const sc_dt::sc_fxnum &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_047>(&sc_dt::operator==)), Fn_eq_047>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_047(Sources &s)
{
  return invoke<Fn_eq_047, const sc_dt::sc_fxnum &, long>(&sc_dt::operator==, s);
}
using Fn_eq_048 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_048>(&sc_dt::operator==)), Fn_eq_048>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_048(Sources &s)
{
  return invoke<Fn_eq_048, const sc_dt::sc_fxnum &, sc_dt::int64>(&sc_dt::operator==, s);
}
using Fn_eq_049 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_049>(&sc_dt::operator==)), Fn_eq_049>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_049(Sources &s)
{
  return invoke<Fn_eq_049, const sc_dt::sc_fxnum &, sc_dt::uint64>(&sc_dt::operator==, s);
}
using Fn_eq_050 = bool (*)(const sc_dt::sc_fxnum &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_050>(&sc_dt::operator==)), Fn_eq_050>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_050(Sources &s)
{
  return invoke<Fn_eq_050, const sc_dt::sc_fxnum &, unsigned int>(&sc_dt::operator==, s);
}
using Fn_eq_051 = bool (*)(const sc_dt::sc_fxnum &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_051>(&sc_dt::operator==)), Fn_eq_051>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_051(Sources &s)
{
  return invoke<Fn_eq_051, const sc_dt::sc_fxnum &, unsigned long>(&sc_dt::operator==, s);
}
using Fn_eq_052 = bool (*)(const sc_dt::sc_fxnum_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_052>(&sc_dt::operator==)), Fn_eq_052>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_052(Sources &s)
{
  return invoke<Fn_eq_052, const sc_dt::sc_fxnum_fast &, const char *>(&sc_dt::operator==, s);
}
using Fn_eq_053 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_053>(&sc_dt::operator==)), Fn_eq_053>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_053(Sources &s)
{
  return invoke<Fn_eq_053, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_054 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_054>(&sc_dt::operator==)), Fn_eq_054>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_054(Sources &s)
{
  return invoke<Fn_eq_054, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_055 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_055>(&sc_dt::operator==)), Fn_eq_055>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_055(Sources &s)
{
  return invoke<Fn_eq_055, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_056 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_056>(&sc_dt::operator==)), Fn_eq_056>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_056(Sources &s)
{
  return invoke<Fn_eq_056, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_057 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_057>(&sc_dt::operator==)), Fn_eq_057>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_057(Sources &s)
{
  return invoke<Fn_eq_057, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator==, s);
}
using Fn_eq_058 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_058>(&sc_dt::operator==)), Fn_eq_058>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_058(Sources &s)
{
  return invoke<Fn_eq_058, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_059 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_059>(&sc_dt::operator==)), Fn_eq_059>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_059(Sources &s)
{
  return invoke<Fn_eq_059, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator==, s);
}
using Fn_eq_060 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_060>(&sc_dt::operator==)), Fn_eq_060>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_060(Sources &s)
{
  return invoke<Fn_eq_060, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_061 = bool (*)(const sc_dt::sc_fxnum_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_061>(&sc_dt::operator==)), Fn_eq_061>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_061(Sources &s)
{
  return invoke<Fn_eq_061, const sc_dt::sc_fxnum_fast &, double>(&sc_dt::operator==, s);
}
using Fn_eq_062 = bool (*)(const sc_dt::sc_fxnum_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_062>(&sc_dt::operator==)), Fn_eq_062>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_062(Sources &s)
{
  return invoke<Fn_eq_062, const sc_dt::sc_fxnum_fast &, float>(&sc_dt::operator==, s);
}
using Fn_eq_063 = bool (*)(const sc_dt::sc_fxnum_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_063>(&sc_dt::operator==)), Fn_eq_063>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_063(Sources &s)
{
  return invoke<Fn_eq_063, const sc_dt::sc_fxnum_fast &, int>(&sc_dt::operator==, s);
}
using Fn_eq_064 = bool (*)(const sc_dt::sc_fxnum_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_064>(&sc_dt::operator==)), Fn_eq_064>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_064(Sources &s)
{
  return invoke<Fn_eq_064, const sc_dt::sc_fxnum_fast &, long>(&sc_dt::operator==, s);
}
using Fn_eq_065 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_065>(&sc_dt::operator==)), Fn_eq_065>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_065(Sources &s)
{
  return invoke<Fn_eq_065, const sc_dt::sc_fxnum_fast &, sc_dt::int64>(&sc_dt::operator==, s);
}
using Fn_eq_066 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_066>(&sc_dt::operator==)), Fn_eq_066>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_066(Sources &s)
{
  return invoke<Fn_eq_066, const sc_dt::sc_fxnum_fast &, sc_dt::uint64>(&sc_dt::operator==, s);
}
using Fn_eq_067 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_067>(&sc_dt::operator==)), Fn_eq_067>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_067(Sources &s)
{
  return invoke<Fn_eq_067, const sc_dt::sc_fxnum_fast &, unsigned int>(&sc_dt::operator==, s);
}
using Fn_eq_068 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_068>(&sc_dt::operator==)), Fn_eq_068>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_068(Sources &s)
{
  return invoke<Fn_eq_068, const sc_dt::sc_fxnum_fast &, unsigned long>(&sc_dt::operator==, s);
}
using Fn_eq_069 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const bool *);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_069>(&sc_dt::operator==)), Fn_eq_069>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, const bool *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_069(Sources &s)
{
  return invoke<Fn_eq_069, const sc_dt::sc_fxnum_fast_subref_r &, const bool *>(&sc_dt::operator==, s);
}
using Fn_eq_070 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_070>(&sc_dt::operator==)), Fn_eq_070>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_070(Sources &s)
{
  return invoke<Fn_eq_070, const sc_dt::sc_fxnum_fast_subref_r &, const char *>(&sc_dt::operator==, s);
}
using Fn_eq_071 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_bv_base &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_071>(&sc_dt::operator==)), Fn_eq_071>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_bv_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_071(Sources &s)
{
  return invoke<Fn_eq_071, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_bv_base &>(&sc_dt::operator==, s);
}
using Fn_eq_072 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_072>(&sc_dt::operator==)), Fn_eq_072>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, const "
              "sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_072(Sources &s)
{
  return invoke<Fn_eq_072, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_fxnum_fast_subref_r &>(
    &sc_dt::operator==, s);
}
using Fn_eq_073 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_fxnum_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_073>(&sc_dt::operator==)), Fn_eq_073>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_073(Sources &s)
{
  return invoke<Fn_eq_073, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==,
                                                                                                    s);
}
using Fn_eq_074 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_lv_base &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_074>(&sc_dt::operator==)), Fn_eq_074>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_lv_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_074(Sources &s)
{
  return invoke<Fn_eq_074, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_lv_base &>(&sc_dt::operator==, s);
}
using Fn_eq_075 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_signed &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_075>(&sc_dt::operator==)), Fn_eq_075>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_075(Sources &s)
{
  return invoke<Fn_eq_075, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_076 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_076>(&sc_dt::operator==)), Fn_eq_076>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_076(Sources &s)
{
  return invoke<Fn_eq_076, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_077 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_077>(&sc_dt::operator==)), Fn_eq_077>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_077(Sources &s)
{
  return invoke<Fn_eq_077, const sc_dt::sc_fxnum_fast_subref_r &, int>(&sc_dt::operator==, s);
}
using Fn_eq_078 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_078>(&sc_dt::operator==)), Fn_eq_078>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_078(Sources &s)
{
  return invoke<Fn_eq_078, const sc_dt::sc_fxnum_fast_subref_r &, long>(&sc_dt::operator==, s);
}
using Fn_eq_079 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_079>(&sc_dt::operator==)), Fn_eq_079>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_079(Sources &s)
{
  return invoke<Fn_eq_079, const sc_dt::sc_fxnum_fast_subref_r &, unsigned int>(&sc_dt::operator==, s);
}
using Fn_eq_080 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_080>(&sc_dt::operator==)), Fn_eq_080>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_fast_subref_r &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_080(Sources &s)
{
  return invoke<Fn_eq_080, const sc_dt::sc_fxnum_fast_subref_r &, unsigned long>(&sc_dt::operator==, s);
}
using Fn_eq_081 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const bool *);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_081>(&sc_dt::operator==)), Fn_eq_081>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, const bool *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_081(Sources &s)
{
  return invoke<Fn_eq_081, const sc_dt::sc_fxnum_subref_r &, const bool *>(&sc_dt::operator==, s);
}
using Fn_eq_082 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_082>(&sc_dt::operator==)), Fn_eq_082>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_082(Sources &s)
{
  return invoke<Fn_eq_082, const sc_dt::sc_fxnum_subref_r &, const char *>(&sc_dt::operator==, s);
}
using Fn_eq_083 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_bv_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_083>(&sc_dt::operator==)), Fn_eq_083>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_bv_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_083(Sources &s)
{
  return invoke<Fn_eq_083, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_bv_base &>(&sc_dt::operator==, s);
}
using Fn_eq_084 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_084>(&sc_dt::operator==)), Fn_eq_084>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_084(Sources &s)
{
  return invoke<Fn_eq_084, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator==,
                                                                                                    s);
}
using Fn_eq_085 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_085>(&sc_dt::operator==)), Fn_eq_085>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_085(Sources &s)
{
  return invoke<Fn_eq_085, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_086 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_lv_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_086>(&sc_dt::operator==)), Fn_eq_086>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_lv_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_086(Sources &s)
{
  return invoke<Fn_eq_086, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_lv_base &>(&sc_dt::operator==, s);
}
using Fn_eq_087 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_087>(&sc_dt::operator==)), Fn_eq_087>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_087(Sources &s)
{
  return invoke<Fn_eq_087, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_088 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_088>(&sc_dt::operator==)), Fn_eq_088>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_088(Sources &s)
{
  return invoke<Fn_eq_088, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_089 = bool (*)(const sc_dt::sc_fxnum_subref_r &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_089>(&sc_dt::operator==)), Fn_eq_089>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_089(Sources &s)
{
  return invoke<Fn_eq_089, const sc_dt::sc_fxnum_subref_r &, int>(&sc_dt::operator==, s);
}
using Fn_eq_090 = bool (*)(const sc_dt::sc_fxnum_subref_r &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_090>(&sc_dt::operator==)), Fn_eq_090>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_090(Sources &s)
{
  return invoke<Fn_eq_090, const sc_dt::sc_fxnum_subref_r &, long>(&sc_dt::operator==, s);
}
using Fn_eq_091 = bool (*)(const sc_dt::sc_fxnum_subref_r &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_091>(&sc_dt::operator==)), Fn_eq_091>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_091(Sources &s)
{
  return invoke<Fn_eq_091, const sc_dt::sc_fxnum_subref_r &, unsigned int>(&sc_dt::operator==, s);
}
using Fn_eq_092 = bool (*)(const sc_dt::sc_fxnum_subref_r &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_092>(&sc_dt::operator==)), Fn_eq_092>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxnum_subref_r &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_092(Sources &s)
{
  return invoke<Fn_eq_092, const sc_dt::sc_fxnum_subref_r &, unsigned long>(&sc_dt::operator==, s);
}
using Fn_eq_093 = bool (*)(const sc_dt::sc_fxtype_params &, const sc_dt::sc_fxtype_params &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_093>(&sc_dt::operator==)), Fn_eq_093>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_fxtype_params &, const sc_dt::sc_fxtype_params &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_093(Sources &s)
{
  return invoke<Fn_eq_093, const sc_dt::sc_fxtype_params &, const sc_dt::sc_fxtype_params &>(&sc_dt::operator==, s);
}
using Fn_eq_094 = bool (*)(const sc_dt::sc_fxval &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_094>(&sc_dt::operator==)), Fn_eq_094>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_094(Sources &s)
{
  return invoke<Fn_eq_094, const sc_dt::sc_fxval &, const char *>(&sc_dt::operator==, s);
}
using Fn_eq_095 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_095>(&sc_dt::operator==)), Fn_eq_095>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_095(Sources &s)
{
  return invoke<Fn_eq_095, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_096 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_096>(&sc_dt::operator==)), Fn_eq_096>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_096(Sources &s)
{
  return invoke<Fn_eq_096, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_097 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_097>(&sc_dt::operator==)), Fn_eq_097>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_097(Sources &s)
{
  return invoke<Fn_eq_097, const sc_dt::sc_fxval &, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_098 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_098>(&sc_dt::operator==)), Fn_eq_098>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_098(Sources &s)
{
  return invoke<Fn_eq_098, const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_099 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_099>(&sc_dt::operator==)), Fn_eq_099>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_099(Sources &s)
{
  return invoke<Fn_eq_099, const sc_dt::sc_fxval &, const sc_dt::sc_int_base &>(&sc_dt::operator==, s);
}
using Fn_eq_100 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_100>(&sc_dt::operator==)), Fn_eq_100>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_100(Sources &s)
{
  return invoke<Fn_eq_100, const sc_dt::sc_fxval &, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_101 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_101>(&sc_dt::operator==)), Fn_eq_101>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_101(Sources &s)
{
  return invoke<Fn_eq_101, const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &>(&sc_dt::operator==, s);
}
using Fn_eq_102 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_102>(&sc_dt::operator==)), Fn_eq_102>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_102(Sources &s)
{
  return invoke<Fn_eq_102, const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_103 = bool (*)(const sc_dt::sc_fxval &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_103>(&sc_dt::operator==)), Fn_eq_103>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_103(Sources &s)
{
  return invoke<Fn_eq_103, const sc_dt::sc_fxval &, double>(&sc_dt::operator==, s);
}
using Fn_eq_104 = bool (*)(const sc_dt::sc_fxval &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_104>(&sc_dt::operator==)), Fn_eq_104>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_104(Sources &s)
{
  return invoke<Fn_eq_104, const sc_dt::sc_fxval &, float>(&sc_dt::operator==, s);
}
using Fn_eq_105 = bool (*)(const sc_dt::sc_fxval &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_105>(&sc_dt::operator==)), Fn_eq_105>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_105(Sources &s)
{
  return invoke<Fn_eq_105, const sc_dt::sc_fxval &, int>(&sc_dt::operator==, s);
}
using Fn_eq_106 = bool (*)(const sc_dt::sc_fxval &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_106>(&sc_dt::operator==)), Fn_eq_106>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_106(Sources &s)
{
  return invoke<Fn_eq_106, const sc_dt::sc_fxval &, long>(&sc_dt::operator==, s);
}
using Fn_eq_107 = bool (*)(const sc_dt::sc_fxval &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_107>(&sc_dt::operator==)), Fn_eq_107>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_107(Sources &s)
{
  return invoke<Fn_eq_107, const sc_dt::sc_fxval &, sc_dt::int64>(&sc_dt::operator==, s);
}
using Fn_eq_108 = bool (*)(const sc_dt::sc_fxval &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_108>(&sc_dt::operator==)), Fn_eq_108>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_108(Sources &s)
{
  return invoke<Fn_eq_108, const sc_dt::sc_fxval &, sc_dt::uint64>(&sc_dt::operator==, s);
}
using Fn_eq_109 = bool (*)(const sc_dt::sc_fxval &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_109>(&sc_dt::operator==)), Fn_eq_109>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_109(Sources &s)
{
  return invoke<Fn_eq_109, const sc_dt::sc_fxval &, unsigned int>(&sc_dt::operator==, s);
}
using Fn_eq_110 = bool (*)(const sc_dt::sc_fxval &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_110>(&sc_dt::operator==)), Fn_eq_110>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_110(Sources &s)
{
  return invoke<Fn_eq_110, const sc_dt::sc_fxval &, unsigned long>(&sc_dt::operator==, s);
}
using Fn_eq_111 = bool (*)(const sc_dt::sc_fxval_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_111>(&sc_dt::operator==)), Fn_eq_111>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_111(Sources &s)
{
  return invoke<Fn_eq_111, const sc_dt::sc_fxval_fast &, const char *>(&sc_dt::operator==, s);
}
using Fn_eq_112 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_112>(&sc_dt::operator==)), Fn_eq_112>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_112(Sources &s)
{
  return invoke<Fn_eq_112, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_113 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_113>(&sc_dt::operator==)), Fn_eq_113>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_113(Sources &s)
{
  return invoke<Fn_eq_113, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_114 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_114>(&sc_dt::operator==)), Fn_eq_114>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_114(Sources &s)
{
  return invoke<Fn_eq_114, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_115 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_115>(&sc_dt::operator==)), Fn_eq_115>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_115(Sources &s)
{
  return invoke<Fn_eq_115, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_116 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_116>(&sc_dt::operator==)), Fn_eq_116>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_116(Sources &s)
{
  return invoke<Fn_eq_116, const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator==, s);
}
using Fn_eq_117 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_117>(&sc_dt::operator==)), Fn_eq_117>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_117(Sources &s)
{
  return invoke<Fn_eq_117, const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_118 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_118>(&sc_dt::operator==)), Fn_eq_118>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_118(Sources &s)
{
  return invoke<Fn_eq_118, const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator==, s);
}
using Fn_eq_119 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_119>(&sc_dt::operator==)), Fn_eq_119>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_119(Sources &s)
{
  return invoke<Fn_eq_119, const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_120 = bool (*)(const sc_dt::sc_fxval_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_120>(&sc_dt::operator==)), Fn_eq_120>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_120(Sources &s)
{
  return invoke<Fn_eq_120, const sc_dt::sc_fxval_fast &, double>(&sc_dt::operator==, s);
}
using Fn_eq_121 = bool (*)(const sc_dt::sc_fxval_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_121>(&sc_dt::operator==)), Fn_eq_121>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_121(Sources &s)
{
  return invoke<Fn_eq_121, const sc_dt::sc_fxval_fast &, float>(&sc_dt::operator==, s);
}
using Fn_eq_122 = bool (*)(const sc_dt::sc_fxval_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_122>(&sc_dt::operator==)), Fn_eq_122>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_122(Sources &s)
{
  return invoke<Fn_eq_122, const sc_dt::sc_fxval_fast &, int>(&sc_dt::operator==, s);
}
using Fn_eq_123 = bool (*)(const sc_dt::sc_fxval_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_123>(&sc_dt::operator==)), Fn_eq_123>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_123(Sources &s)
{
  return invoke<Fn_eq_123, const sc_dt::sc_fxval_fast &, long>(&sc_dt::operator==, s);
}
using Fn_eq_124 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_124>(&sc_dt::operator==)), Fn_eq_124>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_124(Sources &s)
{
  return invoke<Fn_eq_124, const sc_dt::sc_fxval_fast &, sc_dt::int64>(&sc_dt::operator==, s);
}
using Fn_eq_125 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_125>(&sc_dt::operator==)), Fn_eq_125>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_125(Sources &s)
{
  return invoke<Fn_eq_125, const sc_dt::sc_fxval_fast &, sc_dt::uint64>(&sc_dt::operator==, s);
}
using Fn_eq_126 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_126>(&sc_dt::operator==)), Fn_eq_126>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_126(Sources &s)
{
  return invoke<Fn_eq_126, const sc_dt::sc_fxval_fast &, unsigned int>(&sc_dt::operator==, s);
}
using Fn_eq_127 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_127>(&sc_dt::operator==)), Fn_eq_127>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_fxval_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_127(Sources &s)
{
  return invoke<Fn_eq_127, const sc_dt::sc_fxval_fast &, unsigned long>(&sc_dt::operator==, s);
}
using Fn_eq_128 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_128>(&sc_dt::operator==)), Fn_eq_128>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_128(Sources &s)
{
  return invoke<Fn_eq_128, const sc_dt::sc_int_base &, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_129 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_129>(&sc_dt::operator==)), Fn_eq_129>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_129(Sources &s)
{
  return invoke<Fn_eq_129, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_130 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_130>(&sc_dt::operator==)), Fn_eq_130>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_130(Sources &s)
{
  return invoke<Fn_eq_130, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_131 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_131>(&sc_dt::operator==)), Fn_eq_131>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_131(Sources &s)
{
  return invoke<Fn_eq_131, const sc_dt::sc_int_base &, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_132 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_132>(&sc_dt::operator==)), Fn_eq_132>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_132(Sources &s)
{
  return invoke<Fn_eq_132, const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_133 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_133>(&sc_dt::operator==)), Fn_eq_133>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_int_base &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_133(Sources &s)
{
  return invoke<Fn_eq_133, const sc_dt::sc_int_base &, const sc_dt::sc_int_base &>(&sc_dt::operator==, s);
}
using Fn_eq_134 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_134>(&sc_dt::operator==)), Fn_eq_134>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_134(Sources &s)
{
  return invoke<Fn_eq_134, const sc_dt::sc_int_base &, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_135 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_135>(&sc_dt::operator==)), Fn_eq_135>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_135(Sources &s)
{
  return invoke<Fn_eq_135, const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_136 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_136>(&sc_dt::operator==)), Fn_eq_136>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_int_base &, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_136(Sources &s)
{
  return invoke<Fn_eq_136, const sc_dt::sc_int_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==,
                                                                                                   s);
}
using Fn_eq_137 = bool (*)(const sc_dt::sc_length_param &, const sc_dt::sc_length_param &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_137>(&sc_dt::operator==)), Fn_eq_137>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_length_param &, const sc_dt::sc_length_param &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_137(Sources &s)
{
  return invoke<Fn_eq_137, const sc_dt::sc_length_param &, const sc_dt::sc_length_param &>(&sc_dt::operator==, s);
}
using Fn_eq_138 = bool (*)(const sc_dt::sc_logic &, bool);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_138>(&sc_dt::operator==)), Fn_eq_138>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_logic &, bool)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_138(Sources &s)
{
  return invoke<Fn_eq_138, const sc_dt::sc_logic &, bool>(&sc_dt::operator==, s);
}
using Fn_eq_139 = bool (*)(const sc_dt::sc_logic &, char);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_139>(&sc_dt::operator==)), Fn_eq_139>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_logic &, char)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_139(Sources &s)
{
  return invoke<Fn_eq_139, const sc_dt::sc_logic &, char>(&sc_dt::operator==, s);
}
using Fn_eq_140 = bool (*)(const sc_dt::sc_logic &, const sc_dt::sc_logic &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_140>(&sc_dt::operator==)), Fn_eq_140>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_logic &, const sc_dt::sc_logic &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_140(Sources &s)
{
  return invoke<Fn_eq_140, const sc_dt::sc_logic &, const sc_dt::sc_logic &>(&sc_dt::operator==, s);
}
using Fn_eq_141 = bool (*)(const sc_dt::sc_logic &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_141>(&sc_dt::operator==)), Fn_eq_141>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_logic &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_141(Sources &s)
{
  return invoke<Fn_eq_141, const sc_dt::sc_logic &, int>(&sc_dt::operator==, s);
}
using Fn_eq_142 = bool (*)(const sc_dt::sc_logic &, sc_dt::sc_logic_value_t);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_142>(&sc_dt::operator==)), Fn_eq_142>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_logic &, sc_dt::sc_logic_value_t)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_142(Sources &s)
{
  return invoke<Fn_eq_142, const sc_dt::sc_logic &, sc_dt::sc_logic_value_t>(&sc_dt::operator==, s);
}
using Fn_eq_143 = bool (*)(const sc_dt::sc_logic *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_143>(&sc_dt::operator==)), Fn_eq_143>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_logic *, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_143(Sources &s)
{
  return invoke<Fn_eq_143, const sc_dt::sc_logic *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==, s);
}
using Fn_eq_144 = bool (*)(const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_144>(&sc_dt::operator==)), Fn_eq_144>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_144(Sources &s)
{
  return invoke<Fn_eq_144, const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_145 = bool (*)(const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_145>(&sc_dt::operator==)), Fn_eq_145>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_145(Sources &s)
{
  return invoke<Fn_eq_145, const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_146 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_146>(&sc_dt::operator==)), Fn_eq_146>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_146(Sources &s)
{
  return invoke<Fn_eq_146, const sc_dt::sc_signed &, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_147 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_147>(&sc_dt::operator==)), Fn_eq_147>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_147(Sources &s)
{
  return invoke<Fn_eq_147, const sc_dt::sc_signed &, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_148 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_148>(&sc_dt::operator==)), Fn_eq_148>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_148(Sources &s)
{
  return invoke<Fn_eq_148, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_149 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_149>(&sc_dt::operator==)), Fn_eq_149>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_149(Sources &s)
{
  return invoke<Fn_eq_149, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_150 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_150>(&sc_dt::operator==)), Fn_eq_150>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_150(Sources &s)
{
  return invoke<Fn_eq_150, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_151 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_151>(&sc_dt::operator==)), Fn_eq_151>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_151(Sources &s)
{
  return invoke<Fn_eq_151, const sc_dt::sc_signed &, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_152 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_152>(&sc_dt::operator==)), Fn_eq_152>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_152(Sources &s)
{
  return invoke<Fn_eq_152, const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_153 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_153>(&sc_dt::operator==)), Fn_eq_153>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_153(Sources &s)
{
  return invoke<Fn_eq_153, const sc_dt::sc_signed &, const sc_dt::sc_int_base &>(&sc_dt::operator==, s);
}
using Fn_eq_154 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_154>(&sc_dt::operator==)), Fn_eq_154>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_154(Sources &s)
{
  return invoke<Fn_eq_154, const sc_dt::sc_signed &, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_155 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_155>(&sc_dt::operator==)), Fn_eq_155>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_155(Sources &s)
{
  return invoke<Fn_eq_155, const sc_dt::sc_signed &, const sc_dt::sc_uint_base &>(&sc_dt::operator==, s);
}
using Fn_eq_156 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_156>(&sc_dt::operator==)), Fn_eq_156>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_156(Sources &s)
{
  return invoke<Fn_eq_156, const sc_dt::sc_signed &, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_157 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_157>(&sc_dt::operator==)), Fn_eq_157>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_157(Sources &s)
{
  return invoke<Fn_eq_157, const sc_dt::sc_signed &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==, s);
}
using Fn_eq_158 = bool (*)(const sc_dt::sc_signed &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_158>(&sc_dt::operator==)), Fn_eq_158>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_158(Sources &s)
{
  return invoke<Fn_eq_158, const sc_dt::sc_signed &, int>(&sc_dt::operator==, s);
}
using Fn_eq_159 = bool (*)(const sc_dt::sc_signed &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_159>(&sc_dt::operator==)), Fn_eq_159>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_159(Sources &s)
{
  return invoke<Fn_eq_159, const sc_dt::sc_signed &, long>(&sc_dt::operator==, s);
}
using Fn_eq_160 = bool (*)(const sc_dt::sc_signed &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_160>(&sc_dt::operator==)), Fn_eq_160>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_160(Sources &s)
{
  return invoke<Fn_eq_160, const sc_dt::sc_signed &, sc_dt::int64>(&sc_dt::operator==, s);
}
using Fn_eq_161 = bool (*)(const sc_dt::sc_signed &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_161>(&sc_dt::operator==)), Fn_eq_161>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_161(Sources &s)
{
  return invoke<Fn_eq_161, const sc_dt::sc_signed &, sc_dt::uint64>(&sc_dt::operator==, s);
}
using Fn_eq_162 = bool (*)(const sc_dt::sc_signed &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_162>(&sc_dt::operator==)), Fn_eq_162>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_162(Sources &s)
{
  return invoke<Fn_eq_162, const sc_dt::sc_signed &, unsigned int>(&sc_dt::operator==, s);
}
using Fn_eq_163 = bool (*)(const sc_dt::sc_signed &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_163>(&sc_dt::operator==)), Fn_eq_163>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_signed &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_163(Sources &s)
{
  return invoke<Fn_eq_163, const sc_dt::sc_signed &, unsigned long>(&sc_dt::operator==, s);
}
using Fn_eq_164 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_164>(&sc_dt::operator==)), Fn_eq_164>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_164(Sources &s)
{
  return invoke<Fn_eq_164, const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_165 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_165>(&sc_dt::operator==)), Fn_eq_165>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_165(Sources &s)
{
  return invoke<Fn_eq_165, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_166 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_166>(&sc_dt::operator==)), Fn_eq_166>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_166(Sources &s)
{
  return invoke<Fn_eq_166, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_167 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_167>(&sc_dt::operator==)), Fn_eq_167>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_167(Sources &s)
{
  return invoke<Fn_eq_167, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_168 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_168>(&sc_dt::operator==)), Fn_eq_168>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_168(Sources &s)
{
  return invoke<Fn_eq_168, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_169 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_169>(&sc_dt::operator==)), Fn_eq_169>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_169(Sources &s)
{
  return invoke<Fn_eq_169, const sc_dt::sc_uint_base &, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_170 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_170>(&sc_dt::operator==)), Fn_eq_170>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_170(Sources &s)
{
  return invoke<Fn_eq_170, const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &>(&sc_dt::operator==, s);
}
using Fn_eq_171 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_171>(&sc_dt::operator==)), Fn_eq_171>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_171(Sources &s)
{
  return invoke<Fn_eq_171, const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_172 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_172>(&sc_dt::operator==)), Fn_eq_172>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_uint_base &, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_172(Sources &s)
{
  return invoke<Fn_eq_172, const sc_dt::sc_uint_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==,
                                                                                                    s);
}
using Fn_eq_173 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_173>(&sc_dt::operator==)), Fn_eq_173>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_173(Sources &s)
{
  return invoke<Fn_eq_173, const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_174 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_174>(&sc_dt::operator==)), Fn_eq_174>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_174(Sources &s)
{
  return invoke<Fn_eq_174, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_175 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_175>(&sc_dt::operator==)), Fn_eq_175>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_175(Sources &s)
{
  return invoke<Fn_eq_175, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_176 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_176>(&sc_dt::operator==)), Fn_eq_176>,
  "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_176(Sources &s)
{
  return invoke<Fn_eq_176, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_177 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_177>(&sc_dt::operator==)), Fn_eq_177>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_177(Sources &s)
{
  return invoke<Fn_eq_177, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_178 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_178>(&sc_dt::operator==)), Fn_eq_178>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_178(Sources &s)
{
  return invoke<Fn_eq_178, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_179 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_179>(&sc_dt::operator==)), Fn_eq_179>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_179(Sources &s)
{
  return invoke<Fn_eq_179, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_180 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_180>(&sc_dt::operator==)), Fn_eq_180>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_180(Sources &s)
{
  return invoke<Fn_eq_180, const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &>(&sc_dt::operator==, s);
}
using Fn_eq_181 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_181>(&sc_dt::operator==)), Fn_eq_181>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_181(Sources &s)
{
  return invoke<Fn_eq_181, const sc_dt::sc_unsigned &, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_182 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_182>(&sc_dt::operator==)), Fn_eq_182>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_182(Sources &s)
{
  return invoke<Fn_eq_182, const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &>(&sc_dt::operator==, s);
}
using Fn_eq_183 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_183>(&sc_dt::operator==)), Fn_eq_183>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_183(Sources &s)
{
  return invoke<Fn_eq_183, const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_184 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_184>(&sc_dt::operator==)), Fn_eq_184>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_184(Sources &s)
{
  return invoke<Fn_eq_184, const sc_dt::sc_unsigned &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==,
                                                                                                   s);
}
using Fn_eq_185 = bool (*)(const sc_dt::sc_unsigned &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_185>(&sc_dt::operator==)), Fn_eq_185>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_185(Sources &s)
{
  return invoke<Fn_eq_185, const sc_dt::sc_unsigned &, int>(&sc_dt::operator==, s);
}
using Fn_eq_186 = bool (*)(const sc_dt::sc_unsigned &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_186>(&sc_dt::operator==)), Fn_eq_186>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_186(Sources &s)
{
  return invoke<Fn_eq_186, const sc_dt::sc_unsigned &, long>(&sc_dt::operator==, s);
}
using Fn_eq_187 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_187>(&sc_dt::operator==)), Fn_eq_187>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_187(Sources &s)
{
  return invoke<Fn_eq_187, const sc_dt::sc_unsigned &, sc_dt::int64>(&sc_dt::operator==, s);
}
using Fn_eq_188 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_188>(&sc_dt::operator==)), Fn_eq_188>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_188(Sources &s)
{
  return invoke<Fn_eq_188, const sc_dt::sc_unsigned &, sc_dt::uint64>(&sc_dt::operator==, s);
}
using Fn_eq_189 = bool (*)(const sc_dt::sc_unsigned &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_189>(&sc_dt::operator==)), Fn_eq_189>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_189(Sources &s)
{
  return invoke<Fn_eq_189, const sc_dt::sc_unsigned &, unsigned int>(&sc_dt::operator==, s);
}
using Fn_eq_190 = bool (*)(const sc_dt::sc_unsigned &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_190>(&sc_dt::operator==)), Fn_eq_190>,
              "exact overload sc_dt::operator== bool (const sc_dt::sc_unsigned &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_190(Sources &s)
{
  return invoke<Fn_eq_190, const sc_dt::sc_unsigned &, unsigned long>(&sc_dt::operator==, s);
}
using Fn_eq_191 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_191>(&sc_dt::operator==)), Fn_eq_191>,
              "exact overload sc_dt::operator== bool (const sc_proxy<X> &, const sc_proxy<Y> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_191(Sources &s)
{
  return invoke<Fn_eq_191, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &>(
    &sc_dt::operator==, s);
}
using Fn_eq_192 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_192>(&sc_dt::operator==)), Fn_eq_192>,
  "exact overload sc_dt::operator== bool (const sc_proxy<sc_dt::sc_bv_base> &, const sc_proxy<sc_dt::sc_bv_base> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_192(Sources &s)
{
  return invoke<Fn_eq_192, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    &sc_dt::operator==, s);
}
using Fn_eq_193 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_eq_193>(&sc_dt::operator==)), Fn_eq_193>,
  "exact overload sc_dt::operator== bool (const sc_proxy<sc_dt::sc_bv_base> &, const sc_proxy<sc_dt::sc_lv_base> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_193(Sources &s)
{
  return invoke<Fn_eq_193, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &>(
    &sc_dt::operator==, s);
}
using Fn_eq_194 = bool (*)(double, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_194>(&sc_dt::operator==)), Fn_eq_194>,
              "exact overload sc_dt::operator== bool (double, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_194(Sources &s)
{
  return invoke<Fn_eq_194, double, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_195 = bool (*)(double, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_195>(&sc_dt::operator==)), Fn_eq_195>,
              "exact overload sc_dt::operator== bool (double, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_195(Sources &s)
{
  return invoke<Fn_eq_195, double, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_196 = bool (*)(double, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_196>(&sc_dt::operator==)), Fn_eq_196>,
              "exact overload sc_dt::operator== bool (double, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_196(Sources &s)
{
  return invoke<Fn_eq_196, double, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_197 = bool (*)(double, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_197>(&sc_dt::operator==)), Fn_eq_197>,
              "exact overload sc_dt::operator== bool (double, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_197(Sources &s)
{
  return invoke<Fn_eq_197, double, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_198 = bool (*)(float, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_198>(&sc_dt::operator==)), Fn_eq_198>,
              "exact overload sc_dt::operator== bool (float, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_198(Sources &s)
{
  return invoke<Fn_eq_198, float, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_199 = bool (*)(float, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_199>(&sc_dt::operator==)), Fn_eq_199>,
              "exact overload sc_dt::operator== bool (float, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_199(Sources &s)
{
  return invoke<Fn_eq_199, float, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_200 = bool (*)(float, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_200>(&sc_dt::operator==)), Fn_eq_200>,
              "exact overload sc_dt::operator== bool (float, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_200(Sources &s)
{
  return invoke<Fn_eq_200, float, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_201 = bool (*)(float, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_201>(&sc_dt::operator==)), Fn_eq_201>,
              "exact overload sc_dt::operator== bool (float, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_201(Sources &s)
{
  return invoke<Fn_eq_201, float, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_202 = bool (*)(int, const sc_dt::sc_bit &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_202>(&sc_dt::operator==)), Fn_eq_202>,
              "exact overload sc_dt::operator== bool (int, const sc_dt::sc_bit &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_202(Sources &s)
{
  return invoke<Fn_eq_202, int, const sc_dt::sc_bit &>(&sc_dt::operator==, s);
}
using Fn_eq_203 = bool (*)(int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_203>(&sc_dt::operator==)), Fn_eq_203>,
              "exact overload sc_dt::operator== bool (int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_203(Sources &s)
{
  return invoke<Fn_eq_203, int, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_204 = bool (*)(int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_204>(&sc_dt::operator==)), Fn_eq_204>,
              "exact overload sc_dt::operator== bool (int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_204(Sources &s)
{
  return invoke<Fn_eq_204, int, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_205 = bool (*)(int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_205>(&sc_dt::operator==)), Fn_eq_205>,
              "exact overload sc_dt::operator== bool (int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_205(Sources &s)
{
  return invoke<Fn_eq_205, int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_206 = bool (*)(int, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_206>(&sc_dt::operator==)), Fn_eq_206>,
              "exact overload sc_dt::operator== bool (int, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_206(Sources &s)
{
  return invoke<Fn_eq_206, int, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_207 = bool (*)(int, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_207>(&sc_dt::operator==)), Fn_eq_207>,
              "exact overload sc_dt::operator== bool (int, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_207(Sources &s)
{
  return invoke<Fn_eq_207, int, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_208 = bool (*)(int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_208>(&sc_dt::operator==)), Fn_eq_208>,
              "exact overload sc_dt::operator== bool (int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_208(Sources &s)
{
  return invoke<Fn_eq_208, int, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_209 = bool (*)(int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_209>(&sc_dt::operator==)), Fn_eq_209>,
              "exact overload sc_dt::operator== bool (int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_209(Sources &s)
{
  return invoke<Fn_eq_209, int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_210 = bool (*)(int, const sc_dt::sc_logic &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_210>(&sc_dt::operator==)), Fn_eq_210>,
              "exact overload sc_dt::operator== bool (int, const sc_dt::sc_logic &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_210(Sources &s)
{
  return invoke<Fn_eq_210, int, const sc_dt::sc_logic &>(&sc_dt::operator==, s);
}
using Fn_eq_211 = bool (*)(int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_211>(&sc_dt::operator==)), Fn_eq_211>,
              "exact overload sc_dt::operator== bool (int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_211(Sources &s)
{
  return invoke<Fn_eq_211, int, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_212 = bool (*)(int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_212>(&sc_dt::operator==)), Fn_eq_212>,
              "exact overload sc_dt::operator== bool (int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_212(Sources &s)
{
  return invoke<Fn_eq_212, int, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_213 = bool (*)(int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_213>(&sc_dt::operator==)), Fn_eq_213>,
              "exact overload sc_dt::operator== bool (int, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_213(Sources &s)
{
  return invoke<Fn_eq_213, int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==, s);
}
using Fn_eq_214 = bool (*)(long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_214>(&sc_dt::operator==)), Fn_eq_214>,
              "exact overload sc_dt::operator== bool (long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_214(Sources &s)
{
  return invoke<Fn_eq_214, long, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_215 = bool (*)(long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_215>(&sc_dt::operator==)), Fn_eq_215>,
              "exact overload sc_dt::operator== bool (long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_215(Sources &s)
{
  return invoke<Fn_eq_215, long, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_216 = bool (*)(long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_216>(&sc_dt::operator==)), Fn_eq_216>,
              "exact overload sc_dt::operator== bool (long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_216(Sources &s)
{
  return invoke<Fn_eq_216, long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_217 = bool (*)(long, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_217>(&sc_dt::operator==)), Fn_eq_217>,
              "exact overload sc_dt::operator== bool (long, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_217(Sources &s)
{
  return invoke<Fn_eq_217, long, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_218 = bool (*)(long, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_218>(&sc_dt::operator==)), Fn_eq_218>,
              "exact overload sc_dt::operator== bool (long, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_218(Sources &s)
{
  return invoke<Fn_eq_218, long, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_219 = bool (*)(long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_219>(&sc_dt::operator==)), Fn_eq_219>,
              "exact overload sc_dt::operator== bool (long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_219(Sources &s)
{
  return invoke<Fn_eq_219, long, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_220 = bool (*)(long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_220>(&sc_dt::operator==)), Fn_eq_220>,
              "exact overload sc_dt::operator== bool (long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_220(Sources &s)
{
  return invoke<Fn_eq_220, long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_221 = bool (*)(long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_221>(&sc_dt::operator==)), Fn_eq_221>,
              "exact overload sc_dt::operator== bool (long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_221(Sources &s)
{
  return invoke<Fn_eq_221, long, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_222 = bool (*)(long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_222>(&sc_dt::operator==)), Fn_eq_222>,
              "exact overload sc_dt::operator== bool (long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_222(Sources &s)
{
  return invoke<Fn_eq_222, long, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_223 = bool (*)(long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_223>(&sc_dt::operator==)), Fn_eq_223>,
              "exact overload sc_dt::operator== bool (long, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_223(Sources &s)
{
  return invoke<Fn_eq_223, long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==, s);
}
using Fn_eq_224 = bool (*)(sc_dt::int64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_224>(&sc_dt::operator==)), Fn_eq_224>,
              "exact overload sc_dt::operator== bool (sc_dt::int64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_224(Sources &s)
{
  return invoke<Fn_eq_224, sc_dt::int64, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_225 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_225>(&sc_dt::operator==)), Fn_eq_225>,
              "exact overload sc_dt::operator== bool (sc_dt::int64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_225(Sources &s)
{
  return invoke<Fn_eq_225, sc_dt::int64, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_226 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_226>(&sc_dt::operator==)), Fn_eq_226>,
              "exact overload sc_dt::operator== bool (sc_dt::int64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_226(Sources &s)
{
  return invoke<Fn_eq_226, sc_dt::int64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_227 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_227>(&sc_dt::operator==)), Fn_eq_227>,
              "exact overload sc_dt::operator== bool (sc_dt::int64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_227(Sources &s)
{
  return invoke<Fn_eq_227, sc_dt::int64, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_228 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_228>(&sc_dt::operator==)), Fn_eq_228>,
              "exact overload sc_dt::operator== bool (sc_dt::int64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_228(Sources &s)
{
  return invoke<Fn_eq_228, sc_dt::int64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_229 = bool (*)(sc_dt::int64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_229>(&sc_dt::operator==)), Fn_eq_229>,
              "exact overload sc_dt::operator== bool (sc_dt::int64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_229(Sources &s)
{
  return invoke<Fn_eq_229, sc_dt::int64, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_230 = bool (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_230>(&sc_dt::operator==)), Fn_eq_230>,
              "exact overload sc_dt::operator== bool (sc_dt::int64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_230(Sources &s)
{
  return invoke<Fn_eq_230, sc_dt::int64, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_231 = bool (*)(sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_231>(&sc_dt::operator==)), Fn_eq_231>,
              "exact overload sc_dt::operator== bool (sc_dt::int64, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_231(Sources &s)
{
  return invoke<Fn_eq_231, sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==, s);
}
using Fn_eq_232 = bool (*)(sc_dt::sc_logic_value_t, const sc_dt::sc_logic &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_232>(&sc_dt::operator==)), Fn_eq_232>,
              "exact overload sc_dt::operator== bool (sc_dt::sc_logic_value_t, const sc_dt::sc_logic &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_232(Sources &s)
{
  return invoke<Fn_eq_232, sc_dt::sc_logic_value_t, const sc_dt::sc_logic &>(&sc_dt::operator==, s);
}
using Fn_eq_233 = bool (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_233>(&sc_dt::operator==)), Fn_eq_233>,
              "exact overload sc_dt::operator== bool (sc_dt::uint64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_233(Sources &s)
{
  return invoke<Fn_eq_233, sc_dt::uint64, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_234 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_234>(&sc_dt::operator==)), Fn_eq_234>,
              "exact overload sc_dt::operator== bool (sc_dt::uint64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_234(Sources &s)
{
  return invoke<Fn_eq_234, sc_dt::uint64, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_235 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_235>(&sc_dt::operator==)), Fn_eq_235>,
              "exact overload sc_dt::operator== bool (sc_dt::uint64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_235(Sources &s)
{
  return invoke<Fn_eq_235, sc_dt::uint64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_236 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_236>(&sc_dt::operator==)), Fn_eq_236>,
              "exact overload sc_dt::operator== bool (sc_dt::uint64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_236(Sources &s)
{
  return invoke<Fn_eq_236, sc_dt::uint64, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_237 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_237>(&sc_dt::operator==)), Fn_eq_237>,
              "exact overload sc_dt::operator== bool (sc_dt::uint64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_237(Sources &s)
{
  return invoke<Fn_eq_237, sc_dt::uint64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_238 = bool (*)(sc_dt::uint64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_238>(&sc_dt::operator==)), Fn_eq_238>,
              "exact overload sc_dt::operator== bool (sc_dt::uint64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_238(Sources &s)
{
  return invoke<Fn_eq_238, sc_dt::uint64, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_239 = bool (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_239>(&sc_dt::operator==)), Fn_eq_239>,
              "exact overload sc_dt::operator== bool (sc_dt::uint64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_239(Sources &s)
{
  return invoke<Fn_eq_239, sc_dt::uint64, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_240 = bool (*)(sc_dt::uint64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_240>(&sc_dt::operator==)), Fn_eq_240>,
              "exact overload sc_dt::operator== bool (sc_dt::uint64, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_240(Sources &s)
{
  return invoke<Fn_eq_240, sc_dt::uint64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==, s);
}
using Fn_eq_241 = bool (*)(unsigned int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_241>(&sc_dt::operator==)), Fn_eq_241>,
              "exact overload sc_dt::operator== bool (unsigned int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_241(Sources &s)
{
  return invoke<Fn_eq_241, unsigned int, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_242 = bool (*)(unsigned int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_242>(&sc_dt::operator==)), Fn_eq_242>,
              "exact overload sc_dt::operator== bool (unsigned int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_242(Sources &s)
{
  return invoke<Fn_eq_242, unsigned int, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_243 = bool (*)(unsigned int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_243>(&sc_dt::operator==)), Fn_eq_243>,
              "exact overload sc_dt::operator== bool (unsigned int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_243(Sources &s)
{
  return invoke<Fn_eq_243, unsigned int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_244 = bool (*)(unsigned int, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_244>(&sc_dt::operator==)), Fn_eq_244>,
              "exact overload sc_dt::operator== bool (unsigned int, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_244(Sources &s)
{
  return invoke<Fn_eq_244, unsigned int, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_245 = bool (*)(unsigned int, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_245>(&sc_dt::operator==)), Fn_eq_245>,
              "exact overload sc_dt::operator== bool (unsigned int, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_245(Sources &s)
{
  return invoke<Fn_eq_245, unsigned int, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_246 = bool (*)(unsigned int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_246>(&sc_dt::operator==)), Fn_eq_246>,
              "exact overload sc_dt::operator== bool (unsigned int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_246(Sources &s)
{
  return invoke<Fn_eq_246, unsigned int, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_247 = bool (*)(unsigned int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_247>(&sc_dt::operator==)), Fn_eq_247>,
              "exact overload sc_dt::operator== bool (unsigned int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_247(Sources &s)
{
  return invoke<Fn_eq_247, unsigned int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_248 = bool (*)(unsigned int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_248>(&sc_dt::operator==)), Fn_eq_248>,
              "exact overload sc_dt::operator== bool (unsigned int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_248(Sources &s)
{
  return invoke<Fn_eq_248, unsigned int, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_249 = bool (*)(unsigned int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_249>(&sc_dt::operator==)), Fn_eq_249>,
              "exact overload sc_dt::operator== bool (unsigned int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_249(Sources &s)
{
  return invoke<Fn_eq_249, unsigned int, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_250 = bool (*)(unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_250>(&sc_dt::operator==)), Fn_eq_250>,
              "exact overload sc_dt::operator== bool (unsigned int, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_250(Sources &s)
{
  return invoke<Fn_eq_250, unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==, s);
}
using Fn_eq_251 = bool (*)(unsigned long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_251>(&sc_dt::operator==)), Fn_eq_251>,
              "exact overload sc_dt::operator== bool (unsigned long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_251(Sources &s)
{
  return invoke<Fn_eq_251, unsigned long, const sc_dt::sc_concatref &>(&sc_dt::operator==, s);
}
using Fn_eq_252 = bool (*)(unsigned long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_252>(&sc_dt::operator==)), Fn_eq_252>,
              "exact overload sc_dt::operator== bool (unsigned long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_252(Sources &s)
{
  return invoke<Fn_eq_252, unsigned long, const sc_dt::sc_fxnum &>(&sc_dt::operator==, s);
}
using Fn_eq_253 = bool (*)(unsigned long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_253>(&sc_dt::operator==)), Fn_eq_253>,
              "exact overload sc_dt::operator== bool (unsigned long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_253(Sources &s)
{
  return invoke<Fn_eq_253, unsigned long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_254 = bool (*)(unsigned long, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_254>(&sc_dt::operator==)), Fn_eq_254>,
              "exact overload sc_dt::operator== bool (unsigned long, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_254(Sources &s)
{
  return invoke<Fn_eq_254, unsigned long, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_255 = bool (*)(unsigned long, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_255>(&sc_dt::operator==)), Fn_eq_255>,
              "exact overload sc_dt::operator== bool (unsigned long, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_255(Sources &s)
{
  return invoke<Fn_eq_255, unsigned long, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator==, s);
}
using Fn_eq_256 = bool (*)(unsigned long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_256>(&sc_dt::operator==)), Fn_eq_256>,
              "exact overload sc_dt::operator== bool (unsigned long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_256(Sources &s)
{
  return invoke<Fn_eq_256, unsigned long, const sc_dt::sc_fxval &>(&sc_dt::operator==, s);
}
using Fn_eq_257 = bool (*)(unsigned long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_257>(&sc_dt::operator==)), Fn_eq_257>,
              "exact overload sc_dt::operator== bool (unsigned long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_257(Sources &s)
{
  return invoke<Fn_eq_257, unsigned long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator==, s);
}
using Fn_eq_258 = bool (*)(unsigned long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_258>(&sc_dt::operator==)), Fn_eq_258>,
              "exact overload sc_dt::operator== bool (unsigned long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_258(Sources &s)
{
  return invoke<Fn_eq_258, unsigned long, const sc_dt::sc_signed &>(&sc_dt::operator==, s);
}
using Fn_eq_259 = bool (*)(unsigned long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_259>(&sc_dt::operator==)), Fn_eq_259>,
              "exact overload sc_dt::operator== bool (unsigned long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_259(Sources &s)
{
  return invoke<Fn_eq_259, unsigned long, const sc_dt::sc_unsigned &>(&sc_dt::operator==, s);
}
using Fn_eq_260 = bool (*)(unsigned long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_eq_260>(&sc_dt::operator==)), Fn_eq_260>,
              "exact overload sc_dt::operator== bool (unsigned long, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq_260(Sources &s)
{
  return invoke<Fn_eq_260, unsigned long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator==, s);
}
inline unsigned count_eq = 261;
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_eq(unsigned index, Sources &s)
{
  switch (index)
  {
  case 0:
    return case_eq_000(s);
  case 1:
    return case_eq_001(s);
  case 2:
    return case_eq_002(s);
  case 3:
    return case_eq_003(s);
  case 4:
    return case_eq_004(s);
  case 5:
    return case_eq_005(s);
  case 6:
    return case_eq_006(s);
  case 7:
    return case_eq_007(s);
  case 8:
    return case_eq_008(s);
  case 9:
    return case_eq_009(s);
  case 10:
    return case_eq_010(s);
  case 11:
    return case_eq_011(s);
  case 12:
    return case_eq_012(s);
  case 13:
    return case_eq_013(s);
  case 14:
    return case_eq_014(s);
  case 15:
    return case_eq_015(s);
  case 16:
    return case_eq_016(s);
  case 17:
    return case_eq_017(s);
  case 18:
    return case_eq_018(s);
  case 19:
    return case_eq_019(s);
  case 20:
    return case_eq_020(s);
  case 21:
    return case_eq_021(s);
  case 22:
    return case_eq_022(s);
  case 23:
    return case_eq_023(s);
  case 24:
    return case_eq_024(s);
  case 25:
    return case_eq_025(s);
  case 26:
    return case_eq_026(s);
  case 27:
    return case_eq_027(s);
  case 28:
    return case_eq_028(s);
  case 29:
    return case_eq_029(s);
  case 30:
    return case_eq_030(s);
  case 31:
    return case_eq_031(s);
  case 32:
    return case_eq_032(s);
  case 33:
    return case_eq_033(s);
  case 34:
    return case_eq_034(s);
  case 35:
    return case_eq_035(s);
  case 36:
    return case_eq_036(s);
  case 37:
    return case_eq_037(s);
  case 38:
    return case_eq_038(s);
  case 39:
    return case_eq_039(s);
  case 40:
    return case_eq_040(s);
  case 41:
    return case_eq_041(s);
  case 42:
    return case_eq_042(s);
  case 43:
    return case_eq_043(s);
  case 44:
    return case_eq_044(s);
  case 45:
    return case_eq_045(s);
  case 46:
    return case_eq_046(s);
  case 47:
    return case_eq_047(s);
  case 48:
    return case_eq_048(s);
  case 49:
    return case_eq_049(s);
  case 50:
    return case_eq_050(s);
  case 51:
    return case_eq_051(s);
  case 52:
    return case_eq_052(s);
  case 53:
    return case_eq_053(s);
  case 54:
    return case_eq_054(s);
  case 55:
    return case_eq_055(s);
  case 56:
    return case_eq_056(s);
  case 57:
    return case_eq_057(s);
  case 58:
    return case_eq_058(s);
  case 59:
    return case_eq_059(s);
  case 60:
    return case_eq_060(s);
  case 61:
    return case_eq_061(s);
  case 62:
    return case_eq_062(s);
  case 63:
    return case_eq_063(s);
  case 64:
    return case_eq_064(s);
  case 65:
    return case_eq_065(s);
  case 66:
    return case_eq_066(s);
  case 67:
    return case_eq_067(s);
  case 68:
    return case_eq_068(s);
  case 69:
    return case_eq_069(s);
  case 70:
    return case_eq_070(s);
  case 71:
    return case_eq_071(s);
  case 72:
    return case_eq_072(s);
  case 73:
    return case_eq_073(s);
  case 74:
    return case_eq_074(s);
  case 75:
    return case_eq_075(s);
  case 76:
    return case_eq_076(s);
  case 77:
    return case_eq_077(s);
  case 78:
    return case_eq_078(s);
  case 79:
    return case_eq_079(s);
  case 80:
    return case_eq_080(s);
  case 81:
    return case_eq_081(s);
  case 82:
    return case_eq_082(s);
  case 83:
    return case_eq_083(s);
  case 84:
    return case_eq_084(s);
  case 85:
    return case_eq_085(s);
  case 86:
    return case_eq_086(s);
  case 87:
    return case_eq_087(s);
  case 88:
    return case_eq_088(s);
  case 89:
    return case_eq_089(s);
  case 90:
    return case_eq_090(s);
  case 91:
    return case_eq_091(s);
  case 92:
    return case_eq_092(s);
  case 93:
    return case_eq_093(s);
  case 94:
    return case_eq_094(s);
  case 95:
    return case_eq_095(s);
  case 96:
    return case_eq_096(s);
  case 97:
    return case_eq_097(s);
  case 98:
    return case_eq_098(s);
  case 99:
    return case_eq_099(s);
  case 100:
    return case_eq_100(s);
  case 101:
    return case_eq_101(s);
  case 102:
    return case_eq_102(s);
  case 103:
    return case_eq_103(s);
  case 104:
    return case_eq_104(s);
  case 105:
    return case_eq_105(s);
  case 106:
    return case_eq_106(s);
  case 107:
    return case_eq_107(s);
  case 108:
    return case_eq_108(s);
  case 109:
    return case_eq_109(s);
  case 110:
    return case_eq_110(s);
  case 111:
    return case_eq_111(s);
  case 112:
    return case_eq_112(s);
  case 113:
    return case_eq_113(s);
  case 114:
    return case_eq_114(s);
  case 115:
    return case_eq_115(s);
  case 116:
    return case_eq_116(s);
  case 117:
    return case_eq_117(s);
  case 118:
    return case_eq_118(s);
  case 119:
    return case_eq_119(s);
  case 120:
    return case_eq_120(s);
  case 121:
    return case_eq_121(s);
  case 122:
    return case_eq_122(s);
  case 123:
    return case_eq_123(s);
  case 124:
    return case_eq_124(s);
  case 125:
    return case_eq_125(s);
  case 126:
    return case_eq_126(s);
  case 127:
    return case_eq_127(s);
  case 128:
    return case_eq_128(s);
  case 129:
    return case_eq_129(s);
  case 130:
    return case_eq_130(s);
  case 131:
    return case_eq_131(s);
  case 132:
    return case_eq_132(s);
  case 133:
    return case_eq_133(s);
  case 134:
    return case_eq_134(s);
  case 135:
    return case_eq_135(s);
  case 136:
    return case_eq_136(s);
  case 137:
    return case_eq_137(s);
  case 138:
    return case_eq_138(s);
  case 139:
    return case_eq_139(s);
  case 140:
    return case_eq_140(s);
  case 141:
    return case_eq_141(s);
  case 142:
    return case_eq_142(s);
  case 143:
    return case_eq_143(s);
  case 144:
    return case_eq_144(s);
  case 145:
    return case_eq_145(s);
  case 146:
    return case_eq_146(s);
  case 147:
    return case_eq_147(s);
  case 148:
    return case_eq_148(s);
  case 149:
    return case_eq_149(s);
  case 150:
    return case_eq_150(s);
  case 151:
    return case_eq_151(s);
  case 152:
    return case_eq_152(s);
  case 153:
    return case_eq_153(s);
  case 154:
    return case_eq_154(s);
  case 155:
    return case_eq_155(s);
  case 156:
    return case_eq_156(s);
  case 157:
    return case_eq_157(s);
  case 158:
    return case_eq_158(s);
  case 159:
    return case_eq_159(s);
  case 160:
    return case_eq_160(s);
  case 161:
    return case_eq_161(s);
  case 162:
    return case_eq_162(s);
  case 163:
    return case_eq_163(s);
  case 164:
    return case_eq_164(s);
  case 165:
    return case_eq_165(s);
  case 166:
    return case_eq_166(s);
  case 167:
    return case_eq_167(s);
  case 168:
    return case_eq_168(s);
  case 169:
    return case_eq_169(s);
  case 170:
    return case_eq_170(s);
  case 171:
    return case_eq_171(s);
  case 172:
    return case_eq_172(s);
  case 173:
    return case_eq_173(s);
  case 174:
    return case_eq_174(s);
  case 175:
    return case_eq_175(s);
  case 176:
    return case_eq_176(s);
  case 177:
    return case_eq_177(s);
  case 178:
    return case_eq_178(s);
  case 179:
    return case_eq_179(s);
  case 180:
    return case_eq_180(s);
  case 181:
    return case_eq_181(s);
  case 182:
    return case_eq_182(s);
  case 183:
    return case_eq_183(s);
  case 184:
    return case_eq_184(s);
  case 185:
    return case_eq_185(s);
  case 186:
    return case_eq_186(s);
  case 187:
    return case_eq_187(s);
  case 188:
    return case_eq_188(s);
  case 189:
    return case_eq_189(s);
  case 190:
    return case_eq_190(s);
  case 191:
    return case_eq_191(s);
  case 192:
    return case_eq_192(s);
  case 193:
    return case_eq_193(s);
  case 194:
    return case_eq_194(s);
  case 195:
    return case_eq_195(s);
  case 196:
    return case_eq_196(s);
  case 197:
    return case_eq_197(s);
  case 198:
    return case_eq_198(s);
  case 199:
    return case_eq_199(s);
  case 200:
    return case_eq_200(s);
  case 201:
    return case_eq_201(s);
  case 202:
    return case_eq_202(s);
  case 203:
    return case_eq_203(s);
  case 204:
    return case_eq_204(s);
  case 205:
    return case_eq_205(s);
  case 206:
    return case_eq_206(s);
  case 207:
    return case_eq_207(s);
  case 208:
    return case_eq_208(s);
  case 209:
    return case_eq_209(s);
  case 210:
    return case_eq_210(s);
  case 211:
    return case_eq_211(s);
  case 212:
    return case_eq_212(s);
  case 213:
    return case_eq_213(s);
  case 214:
    return case_eq_214(s);
  case 215:
    return case_eq_215(s);
  case 216:
    return case_eq_216(s);
  case 217:
    return case_eq_217(s);
  case 218:
    return case_eq_218(s);
  case 219:
    return case_eq_219(s);
  case 220:
    return case_eq_220(s);
  case 221:
    return case_eq_221(s);
  case 222:
    return case_eq_222(s);
  case 223:
    return case_eq_223(s);
  case 224:
    return case_eq_224(s);
  case 225:
    return case_eq_225(s);
  case 226:
    return case_eq_226(s);
  case 227:
    return case_eq_227(s);
  case 228:
    return case_eq_228(s);
  case 229:
    return case_eq_229(s);
  case 230:
    return case_eq_230(s);
  case 231:
    return case_eq_231(s);
  case 232:
    return case_eq_232(s);
  case 233:
    return case_eq_233(s);
  case 234:
    return case_eq_234(s);
  case 235:
    return case_eq_235(s);
  case 236:
    return case_eq_236(s);
  case 237:
    return case_eq_237(s);
  case 238:
    return case_eq_238(s);
  case 239:
    return case_eq_239(s);
  case 240:
    return case_eq_240(s);
  case 241:
    return case_eq_241(s);
  case 242:
    return case_eq_242(s);
  case 243:
    return case_eq_243(s);
  case 244:
    return case_eq_244(s);
  case 245:
    return case_eq_245(s);
  case 246:
    return case_eq_246(s);
  case 247:
    return case_eq_247(s);
  case 248:
    return case_eq_248(s);
  case 249:
    return case_eq_249(s);
  case 250:
    return case_eq_250(s);
  case 251:
    return case_eq_251(s);
  case 252:
    return case_eq_252(s);
  case 253:
    return case_eq_253(s);
  case 254:
    return case_eq_254(s);
  case 255:
    return case_eq_255(s);
  case 256:
    return case_eq_256(s);
  case 257:
    return case_eq_257(s);
  case 258:
    return case_eq_258(s);
  case 259:
    return case_eq_259(s);
  case 260:
    return case_eq_260(s);
  default:
    throw std::out_of_range("unknown comparison case index");
  }
}
using Fn_ne_000 = bool (*)(bool, const sc_dt::sc_bit &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_000>(&sc_dt::operator!=)), Fn_ne_000>,
              "exact overload sc_dt::operator!= bool (bool, const sc_dt::sc_bit &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_000(Sources &s)
{
  return invoke<Fn_ne_000, bool, const sc_dt::sc_bit &>(&sc_dt::operator!=, s);
}
using Fn_ne_001 = bool (*)(bool, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_001>(&sc_dt::operator!=)), Fn_ne_001>,
              "exact overload sc_dt::operator!= bool (bool, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_001(Sources &s)
{
  return invoke<Fn_ne_001, bool, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_002 = bool (*)(bool, const sc_dt::sc_logic &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_002>(&sc_dt::operator!=)), Fn_ne_002>,
              "exact overload sc_dt::operator!= bool (bool, const sc_dt::sc_logic &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_002(Sources &s)
{
  return invoke<Fn_ne_002, bool, const sc_dt::sc_logic &>(&sc_dt::operator!=, s);
}
using Fn_ne_003 = bool (*)(char, const sc_dt::sc_bit &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_003>(&sc_dt::operator!=)), Fn_ne_003>,
              "exact overload sc_dt::operator!= bool (char, const sc_dt::sc_bit &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_003(Sources &s)
{
  return invoke<Fn_ne_003, char, const sc_dt::sc_bit &>(&sc_dt::operator!=, s);
}
using Fn_ne_004 = bool (*)(char, const sc_dt::sc_logic &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_004>(&sc_dt::operator!=)), Fn_ne_004>,
              "exact overload sc_dt::operator!= bool (char, const sc_dt::sc_logic &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_004(Sources &s)
{
  return invoke<Fn_ne_004, char, const sc_dt::sc_logic &>(&sc_dt::operator!=, s);
}
using Fn_ne_005 = bool (*)(const bool *, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_005>(&sc_dt::operator!=)), Fn_ne_005>,
              "exact overload sc_dt::operator!= bool (const bool *, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_005(Sources &s)
{
  return invoke<Fn_ne_005, const bool *, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_006 = bool (*)(const bool *, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_006>(&sc_dt::operator!=)), Fn_ne_006>,
              "exact overload sc_dt::operator!= bool (const bool *, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_006(Sources &s)
{
  return invoke<Fn_ne_006, const bool *, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_007 = bool (*)(const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_007>(&sc_dt::operator!=)), Fn_ne_007>,
              "exact overload sc_dt::operator!= bool (const bool *, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_007(Sources &s)
{
  return invoke<Fn_ne_007, const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=, s);
}
using Fn_ne_008 = bool (*)(const char *, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_008>(&sc_dt::operator!=)), Fn_ne_008>,
              "exact overload sc_dt::operator!= bool (const char *, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_008(Sources &s)
{
  return invoke<Fn_ne_008, const char *, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_009 = bool (*)(const char *, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_009>(&sc_dt::operator!=)), Fn_ne_009>,
              "exact overload sc_dt::operator!= bool (const char *, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_009(Sources &s)
{
  return invoke<Fn_ne_009, const char *, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_010 = bool (*)(const char *, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_010>(&sc_dt::operator!=)), Fn_ne_010>,
              "exact overload sc_dt::operator!= bool (const char *, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_010(Sources &s)
{
  return invoke<Fn_ne_010, const char *, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_011 = bool (*)(const char *, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_011>(&sc_dt::operator!=)), Fn_ne_011>,
              "exact overload sc_dt::operator!= bool (const char *, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_011(Sources &s)
{
  return invoke<Fn_ne_011, const char *, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_012 = bool (*)(const char *, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_012>(&sc_dt::operator!=)), Fn_ne_012>,
              "exact overload sc_dt::operator!= bool (const char *, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_012(Sources &s)
{
  return invoke<Fn_ne_012, const char *, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_013 = bool (*)(const char *, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_013>(&sc_dt::operator!=)), Fn_ne_013>,
              "exact overload sc_dt::operator!= bool (const char *, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_013(Sources &s)
{
  return invoke<Fn_ne_013, const char *, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_014 = bool (*)(const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_014>(&sc_dt::operator!=)), Fn_ne_014>,
              "exact overload sc_dt::operator!= bool (const char *, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_014(Sources &s)
{
  return invoke<Fn_ne_014, const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=, s);
}
using Fn_ne_015 = bool (*)(const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &,
                           const sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_015>(&sc_dt::operator!=)), Fn_ne_015>,
              "exact overload sc_dt::operator!= bool (const sc_bitref_r<T1> &, const sc_bitref_r<T2> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_015(Sources &s)
{
  return invoke<Fn_ne_015, const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &,
                const sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &>(&sc_dt::operator!=, s);
}
using Fn_ne_016 = bool (*)(const sc_dt::sc_bit &, bool);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_016>(&sc_dt::operator!=)), Fn_ne_016>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_bit &, bool)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_016(Sources &s)
{
  return invoke<Fn_ne_016, const sc_dt::sc_bit &, bool>(&sc_dt::operator!=, s);
}
using Fn_ne_017 = bool (*)(const sc_dt::sc_bit &, char);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_017>(&sc_dt::operator!=)), Fn_ne_017>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_bit &, char)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_017(Sources &s)
{
  return invoke<Fn_ne_017, const sc_dt::sc_bit &, char>(&sc_dt::operator!=, s);
}
using Fn_ne_018 = bool (*)(const sc_dt::sc_bit &, const sc_dt::sc_bit &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_018>(&sc_dt::operator!=)), Fn_ne_018>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_bit &, const sc_dt::sc_bit &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_018(Sources &s)
{
  return invoke<Fn_ne_018, const sc_dt::sc_bit &, const sc_dt::sc_bit &>(&sc_dt::operator!=, s);
}
using Fn_ne_019 = bool (*)(const sc_dt::sc_bit &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_019>(&sc_dt::operator!=)), Fn_ne_019>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_bit &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_019(Sources &s)
{
  return invoke<Fn_ne_019, const sc_dt::sc_bit &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_020 = bool (*)(const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_020>(&sc_dt::operator!=)), Fn_ne_020>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_020(Sources &s)
{
  return invoke<Fn_ne_020, const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_021 = bool (*)(const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_021>(&sc_dt::operator!=)), Fn_ne_021>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_021(Sources &s)
{
  return invoke<Fn_ne_021, const sc_dt::sc_bv_base &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_022 = bool (*)(const sc_dt::sc_concatref &, bool);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_022>(&sc_dt::operator!=)), Fn_ne_022>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, bool)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_022(Sources &s)
{
  return invoke<Fn_ne_022, const sc_dt::sc_concatref &, bool>(&sc_dt::operator!=, s);
}
using Fn_ne_023 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_023>(&sc_dt::operator!=)), Fn_ne_023>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_023(Sources &s)
{
  return invoke<Fn_ne_023, const sc_dt::sc_concatref &, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_024 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_024>(&sc_dt::operator!=)), Fn_ne_024>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_024(Sources &s)
{
  return invoke<Fn_ne_024, const sc_dt::sc_concatref &, const sc_dt::sc_int_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_025 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_025>(&sc_dt::operator!=)), Fn_ne_025>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_025(Sources &s)
{
  return invoke<Fn_ne_025, const sc_dt::sc_concatref &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_026 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_026>(&sc_dt::operator!=)), Fn_ne_026>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_026(Sources &s)
{
  return invoke<Fn_ne_026, const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_027 = bool (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_027>(&sc_dt::operator!=)), Fn_ne_027>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_027(Sources &s)
{
  return invoke<Fn_ne_027, const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_028 = bool (*)(const sc_dt::sc_concatref &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_028>(&sc_dt::operator!=)), Fn_ne_028>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_028(Sources &s)
{
  return invoke<Fn_ne_028, const sc_dt::sc_concatref &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_029 = bool (*)(const sc_dt::sc_concatref &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_029>(&sc_dt::operator!=)), Fn_ne_029>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_029(Sources &s)
{
  return invoke<Fn_ne_029, const sc_dt::sc_concatref &, long>(&sc_dt::operator!=, s);
}
using Fn_ne_030 = bool (*)(const sc_dt::sc_concatref &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_030>(&sc_dt::operator!=)), Fn_ne_030>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_030(Sources &s)
{
  return invoke<Fn_ne_030, const sc_dt::sc_concatref &, sc_dt::int64>(&sc_dt::operator!=, s);
}
using Fn_ne_031 = bool (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_031>(&sc_dt::operator!=)), Fn_ne_031>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_031(Sources &s)
{
  return invoke<Fn_ne_031, const sc_dt::sc_concatref &, sc_dt::uint64>(&sc_dt::operator!=, s);
}
using Fn_ne_032 = bool (*)(const sc_dt::sc_concatref &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_032>(&sc_dt::operator!=)), Fn_ne_032>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_032(Sources &s)
{
  return invoke<Fn_ne_032, const sc_dt::sc_concatref &, unsigned int>(&sc_dt::operator!=, s);
}
using Fn_ne_033 = bool (*)(const sc_dt::sc_concatref &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_033>(&sc_dt::operator!=)), Fn_ne_033>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_concatref &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_033(Sources &s)
{
  return invoke<Fn_ne_033, const sc_dt::sc_concatref &, unsigned long>(&sc_dt::operator!=, s);
}
using Fn_ne_034 = bool (*)(const sc_dt::sc_fxcast_switch &, const sc_dt::sc_fxcast_switch &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_034>(&sc_dt::operator!=)), Fn_ne_034>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxcast_switch &, const sc_dt::sc_fxcast_switch &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_034(Sources &s)
{
  return invoke<Fn_ne_034, const sc_dt::sc_fxcast_switch &, const sc_dt::sc_fxcast_switch &>(&sc_dt::operator!=, s);
}
using Fn_ne_035 = bool (*)(const sc_dt::sc_fxnum &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_035>(&sc_dt::operator!=)), Fn_ne_035>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_035(Sources &s)
{
  return invoke<Fn_ne_035, const sc_dt::sc_fxnum &, const char *>(&sc_dt::operator!=, s);
}
using Fn_ne_036 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_036>(&sc_dt::operator!=)), Fn_ne_036>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_036(Sources &s)
{
  return invoke<Fn_ne_036, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_037 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_037>(&sc_dt::operator!=)), Fn_ne_037>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_037(Sources &s)
{
  return invoke<Fn_ne_037, const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_038 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_038>(&sc_dt::operator!=)), Fn_ne_038>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_038(Sources &s)
{
  return invoke<Fn_ne_038, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_039 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_039>(&sc_dt::operator!=)), Fn_ne_039>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_039(Sources &s)
{
  return invoke<Fn_ne_039, const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_040 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_040>(&sc_dt::operator!=)), Fn_ne_040>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_040(Sources &s)
{
  return invoke<Fn_ne_040, const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_041 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_041>(&sc_dt::operator!=)), Fn_ne_041>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_041(Sources &s)
{
  return invoke<Fn_ne_041, const sc_dt::sc_fxnum &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_042 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_042>(&sc_dt::operator!=)), Fn_ne_042>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_042(Sources &s)
{
  return invoke<Fn_ne_042, const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_043 = bool (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_043>(&sc_dt::operator!=)), Fn_ne_043>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_043(Sources &s)
{
  return invoke<Fn_ne_043, const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_044 = bool (*)(const sc_dt::sc_fxnum &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_044>(&sc_dt::operator!=)), Fn_ne_044>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_044(Sources &s)
{
  return invoke<Fn_ne_044, const sc_dt::sc_fxnum &, double>(&sc_dt::operator!=, s);
}
using Fn_ne_045 = bool (*)(const sc_dt::sc_fxnum &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_045>(&sc_dt::operator!=)), Fn_ne_045>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_045(Sources &s)
{
  return invoke<Fn_ne_045, const sc_dt::sc_fxnum &, float>(&sc_dt::operator!=, s);
}
using Fn_ne_046 = bool (*)(const sc_dt::sc_fxnum &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_046>(&sc_dt::operator!=)), Fn_ne_046>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_046(Sources &s)
{
  return invoke<Fn_ne_046, const sc_dt::sc_fxnum &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_047 = bool (*)(const sc_dt::sc_fxnum &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_047>(&sc_dt::operator!=)), Fn_ne_047>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_047(Sources &s)
{
  return invoke<Fn_ne_047, const sc_dt::sc_fxnum &, long>(&sc_dt::operator!=, s);
}
using Fn_ne_048 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_048>(&sc_dt::operator!=)), Fn_ne_048>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_048(Sources &s)
{
  return invoke<Fn_ne_048, const sc_dt::sc_fxnum &, sc_dt::int64>(&sc_dt::operator!=, s);
}
using Fn_ne_049 = bool (*)(const sc_dt::sc_fxnum &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_049>(&sc_dt::operator!=)), Fn_ne_049>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_049(Sources &s)
{
  return invoke<Fn_ne_049, const sc_dt::sc_fxnum &, sc_dt::uint64>(&sc_dt::operator!=, s);
}
using Fn_ne_050 = bool (*)(const sc_dt::sc_fxnum &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_050>(&sc_dt::operator!=)), Fn_ne_050>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_050(Sources &s)
{
  return invoke<Fn_ne_050, const sc_dt::sc_fxnum &, unsigned int>(&sc_dt::operator!=, s);
}
using Fn_ne_051 = bool (*)(const sc_dt::sc_fxnum &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_051>(&sc_dt::operator!=)), Fn_ne_051>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_051(Sources &s)
{
  return invoke<Fn_ne_051, const sc_dt::sc_fxnum &, unsigned long>(&sc_dt::operator!=, s);
}
using Fn_ne_052 = bool (*)(const sc_dt::sc_fxnum_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_052>(&sc_dt::operator!=)), Fn_ne_052>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_052(Sources &s)
{
  return invoke<Fn_ne_052, const sc_dt::sc_fxnum_fast &, const char *>(&sc_dt::operator!=, s);
}
using Fn_ne_053 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_053>(&sc_dt::operator!=)), Fn_ne_053>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_053(Sources &s)
{
  return invoke<Fn_ne_053, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_054 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_054>(&sc_dt::operator!=)), Fn_ne_054>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_054(Sources &s)
{
  return invoke<Fn_ne_054, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_055 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_055>(&sc_dt::operator!=)), Fn_ne_055>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_055(Sources &s)
{
  return invoke<Fn_ne_055, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_056 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_056>(&sc_dt::operator!=)), Fn_ne_056>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_056(Sources &s)
{
  return invoke<Fn_ne_056, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_057 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_057>(&sc_dt::operator!=)), Fn_ne_057>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_057(Sources &s)
{
  return invoke<Fn_ne_057, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_058 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_058>(&sc_dt::operator!=)), Fn_ne_058>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_058(Sources &s)
{
  return invoke<Fn_ne_058, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_059 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_059>(&sc_dt::operator!=)), Fn_ne_059>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_059(Sources &s)
{
  return invoke<Fn_ne_059, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_060 = bool (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_060>(&sc_dt::operator!=)), Fn_ne_060>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_060(Sources &s)
{
  return invoke<Fn_ne_060, const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_061 = bool (*)(const sc_dt::sc_fxnum_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_061>(&sc_dt::operator!=)), Fn_ne_061>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_061(Sources &s)
{
  return invoke<Fn_ne_061, const sc_dt::sc_fxnum_fast &, double>(&sc_dt::operator!=, s);
}
using Fn_ne_062 = bool (*)(const sc_dt::sc_fxnum_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_062>(&sc_dt::operator!=)), Fn_ne_062>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_062(Sources &s)
{
  return invoke<Fn_ne_062, const sc_dt::sc_fxnum_fast &, float>(&sc_dt::operator!=, s);
}
using Fn_ne_063 = bool (*)(const sc_dt::sc_fxnum_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_063>(&sc_dt::operator!=)), Fn_ne_063>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_063(Sources &s)
{
  return invoke<Fn_ne_063, const sc_dt::sc_fxnum_fast &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_064 = bool (*)(const sc_dt::sc_fxnum_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_064>(&sc_dt::operator!=)), Fn_ne_064>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_064(Sources &s)
{
  return invoke<Fn_ne_064, const sc_dt::sc_fxnum_fast &, long>(&sc_dt::operator!=, s);
}
using Fn_ne_065 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_065>(&sc_dt::operator!=)), Fn_ne_065>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_065(Sources &s)
{
  return invoke<Fn_ne_065, const sc_dt::sc_fxnum_fast &, sc_dt::int64>(&sc_dt::operator!=, s);
}
using Fn_ne_066 = bool (*)(const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_066>(&sc_dt::operator!=)), Fn_ne_066>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_066(Sources &s)
{
  return invoke<Fn_ne_066, const sc_dt::sc_fxnum_fast &, sc_dt::uint64>(&sc_dt::operator!=, s);
}
using Fn_ne_067 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_067>(&sc_dt::operator!=)), Fn_ne_067>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_067(Sources &s)
{
  return invoke<Fn_ne_067, const sc_dt::sc_fxnum_fast &, unsigned int>(&sc_dt::operator!=, s);
}
using Fn_ne_068 = bool (*)(const sc_dt::sc_fxnum_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_068>(&sc_dt::operator!=)), Fn_ne_068>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_068(Sources &s)
{
  return invoke<Fn_ne_068, const sc_dt::sc_fxnum_fast &, unsigned long>(&sc_dt::operator!=, s);
}
using Fn_ne_069 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const bool *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_069>(&sc_dt::operator!=)), Fn_ne_069>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, const bool *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_069(Sources &s)
{
  return invoke<Fn_ne_069, const sc_dt::sc_fxnum_fast_subref_r &, const bool *>(&sc_dt::operator!=, s);
}
using Fn_ne_070 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_070>(&sc_dt::operator!=)), Fn_ne_070>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_070(Sources &s)
{
  return invoke<Fn_ne_070, const sc_dt::sc_fxnum_fast_subref_r &, const char *>(&sc_dt::operator!=, s);
}
using Fn_ne_071 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_bv_base &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_071>(&sc_dt::operator!=)), Fn_ne_071>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_bv_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_071(Sources &s)
{
  return invoke<Fn_ne_071, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_bv_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_072 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_072>(&sc_dt::operator!=)), Fn_ne_072>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, const "
              "sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_072(Sources &s)
{
  return invoke<Fn_ne_072, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_fxnum_fast_subref_r &>(
    &sc_dt::operator!=, s);
}
using Fn_ne_073 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_fxnum_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_073>(&sc_dt::operator!=)), Fn_ne_073>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_073(Sources &s)
{
  return invoke<Fn_ne_073, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=,
                                                                                                    s);
}
using Fn_ne_074 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_lv_base &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_074>(&sc_dt::operator!=)), Fn_ne_074>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_lv_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_074(Sources &s)
{
  return invoke<Fn_ne_074, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_lv_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_075 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_signed &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_075>(&sc_dt::operator!=)), Fn_ne_075>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_075(Sources &s)
{
  return invoke<Fn_ne_075, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_076 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_076>(&sc_dt::operator!=)), Fn_ne_076>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_076(Sources &s)
{
  return invoke<Fn_ne_076, const sc_dt::sc_fxnum_fast_subref_r &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_077 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_077>(&sc_dt::operator!=)), Fn_ne_077>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_077(Sources &s)
{
  return invoke<Fn_ne_077, const sc_dt::sc_fxnum_fast_subref_r &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_078 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_078>(&sc_dt::operator!=)), Fn_ne_078>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_078(Sources &s)
{
  return invoke<Fn_ne_078, const sc_dt::sc_fxnum_fast_subref_r &, long>(&sc_dt::operator!=, s);
}
using Fn_ne_079 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_079>(&sc_dt::operator!=)), Fn_ne_079>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_079(Sources &s)
{
  return invoke<Fn_ne_079, const sc_dt::sc_fxnum_fast_subref_r &, unsigned int>(&sc_dt::operator!=, s);
}
using Fn_ne_080 = bool (*)(const sc_dt::sc_fxnum_fast_subref_r &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_080>(&sc_dt::operator!=)), Fn_ne_080>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_fast_subref_r &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_080(Sources &s)
{
  return invoke<Fn_ne_080, const sc_dt::sc_fxnum_fast_subref_r &, unsigned long>(&sc_dt::operator!=, s);
}
using Fn_ne_081 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const bool *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_081>(&sc_dt::operator!=)), Fn_ne_081>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, const bool *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_081(Sources &s)
{
  return invoke<Fn_ne_081, const sc_dt::sc_fxnum_subref_r &, const bool *>(&sc_dt::operator!=, s);
}
using Fn_ne_082 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_082>(&sc_dt::operator!=)), Fn_ne_082>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_082(Sources &s)
{
  return invoke<Fn_ne_082, const sc_dt::sc_fxnum_subref_r &, const char *>(&sc_dt::operator!=, s);
}
using Fn_ne_083 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_bv_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_083>(&sc_dt::operator!=)), Fn_ne_083>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_bv_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_083(Sources &s)
{
  return invoke<Fn_ne_083, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_bv_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_084 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_084>(&sc_dt::operator!=)), Fn_ne_084>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_084(Sources &s)
{
  return invoke<Fn_ne_084, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator!=,
                                                                                                    s);
}
using Fn_ne_085 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_085>(&sc_dt::operator!=)), Fn_ne_085>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_085(Sources &s)
{
  return invoke<Fn_ne_085, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_086 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_lv_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_086>(&sc_dt::operator!=)), Fn_ne_086>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_lv_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_086(Sources &s)
{
  return invoke<Fn_ne_086, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_lv_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_087 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_087>(&sc_dt::operator!=)), Fn_ne_087>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_087(Sources &s)
{
  return invoke<Fn_ne_087, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_088 = bool (*)(const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_088>(&sc_dt::operator!=)), Fn_ne_088>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_088(Sources &s)
{
  return invoke<Fn_ne_088, const sc_dt::sc_fxnum_subref_r &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_089 = bool (*)(const sc_dt::sc_fxnum_subref_r &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_089>(&sc_dt::operator!=)), Fn_ne_089>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_089(Sources &s)
{
  return invoke<Fn_ne_089, const sc_dt::sc_fxnum_subref_r &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_090 = bool (*)(const sc_dt::sc_fxnum_subref_r &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_090>(&sc_dt::operator!=)), Fn_ne_090>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_090(Sources &s)
{
  return invoke<Fn_ne_090, const sc_dt::sc_fxnum_subref_r &, long>(&sc_dt::operator!=, s);
}
using Fn_ne_091 = bool (*)(const sc_dt::sc_fxnum_subref_r &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_091>(&sc_dt::operator!=)), Fn_ne_091>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_091(Sources &s)
{
  return invoke<Fn_ne_091, const sc_dt::sc_fxnum_subref_r &, unsigned int>(&sc_dt::operator!=, s);
}
using Fn_ne_092 = bool (*)(const sc_dt::sc_fxnum_subref_r &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_092>(&sc_dt::operator!=)), Fn_ne_092>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxnum_subref_r &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_092(Sources &s)
{
  return invoke<Fn_ne_092, const sc_dt::sc_fxnum_subref_r &, unsigned long>(&sc_dt::operator!=, s);
}
using Fn_ne_093 = bool (*)(const sc_dt::sc_fxtype_params &, const sc_dt::sc_fxtype_params &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_093>(&sc_dt::operator!=)), Fn_ne_093>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxtype_params &, const sc_dt::sc_fxtype_params &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_093(Sources &s)
{
  return invoke<Fn_ne_093, const sc_dt::sc_fxtype_params &, const sc_dt::sc_fxtype_params &>(&sc_dt::operator!=, s);
}
using Fn_ne_094 = bool (*)(const sc_dt::sc_fxval &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_094>(&sc_dt::operator!=)), Fn_ne_094>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_094(Sources &s)
{
  return invoke<Fn_ne_094, const sc_dt::sc_fxval &, const char *>(&sc_dt::operator!=, s);
}
using Fn_ne_095 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_095>(&sc_dt::operator!=)), Fn_ne_095>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_095(Sources &s)
{
  return invoke<Fn_ne_095, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_096 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_096>(&sc_dt::operator!=)), Fn_ne_096>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_096(Sources &s)
{
  return invoke<Fn_ne_096, const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_097 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_097>(&sc_dt::operator!=)), Fn_ne_097>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_097(Sources &s)
{
  return invoke<Fn_ne_097, const sc_dt::sc_fxval &, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_098 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_098>(&sc_dt::operator!=)), Fn_ne_098>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_098(Sources &s)
{
  return invoke<Fn_ne_098, const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_099 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_099>(&sc_dt::operator!=)), Fn_ne_099>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_099(Sources &s)
{
  return invoke<Fn_ne_099, const sc_dt::sc_fxval &, const sc_dt::sc_int_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_100 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_100>(&sc_dt::operator!=)), Fn_ne_100>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_100(Sources &s)
{
  return invoke<Fn_ne_100, const sc_dt::sc_fxval &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_101 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_101>(&sc_dt::operator!=)), Fn_ne_101>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_101(Sources &s)
{
  return invoke<Fn_ne_101, const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_102 = bool (*)(const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_102>(&sc_dt::operator!=)), Fn_ne_102>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_102(Sources &s)
{
  return invoke<Fn_ne_102, const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_103 = bool (*)(const sc_dt::sc_fxval &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_103>(&sc_dt::operator!=)), Fn_ne_103>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_103(Sources &s)
{
  return invoke<Fn_ne_103, const sc_dt::sc_fxval &, double>(&sc_dt::operator!=, s);
}
using Fn_ne_104 = bool (*)(const sc_dt::sc_fxval &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_104>(&sc_dt::operator!=)), Fn_ne_104>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_104(Sources &s)
{
  return invoke<Fn_ne_104, const sc_dt::sc_fxval &, float>(&sc_dt::operator!=, s);
}
using Fn_ne_105 = bool (*)(const sc_dt::sc_fxval &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_105>(&sc_dt::operator!=)), Fn_ne_105>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_105(Sources &s)
{
  return invoke<Fn_ne_105, const sc_dt::sc_fxval &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_106 = bool (*)(const sc_dt::sc_fxval &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_106>(&sc_dt::operator!=)), Fn_ne_106>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_106(Sources &s)
{
  return invoke<Fn_ne_106, const sc_dt::sc_fxval &, long>(&sc_dt::operator!=, s);
}
using Fn_ne_107 = bool (*)(const sc_dt::sc_fxval &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_107>(&sc_dt::operator!=)), Fn_ne_107>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_107(Sources &s)
{
  return invoke<Fn_ne_107, const sc_dt::sc_fxval &, sc_dt::int64>(&sc_dt::operator!=, s);
}
using Fn_ne_108 = bool (*)(const sc_dt::sc_fxval &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_108>(&sc_dt::operator!=)), Fn_ne_108>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_108(Sources &s)
{
  return invoke<Fn_ne_108, const sc_dt::sc_fxval &, sc_dt::uint64>(&sc_dt::operator!=, s);
}
using Fn_ne_109 = bool (*)(const sc_dt::sc_fxval &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_109>(&sc_dt::operator!=)), Fn_ne_109>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_109(Sources &s)
{
  return invoke<Fn_ne_109, const sc_dt::sc_fxval &, unsigned int>(&sc_dt::operator!=, s);
}
using Fn_ne_110 = bool (*)(const sc_dt::sc_fxval &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_110>(&sc_dt::operator!=)), Fn_ne_110>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_110(Sources &s)
{
  return invoke<Fn_ne_110, const sc_dt::sc_fxval &, unsigned long>(&sc_dt::operator!=, s);
}
using Fn_ne_111 = bool (*)(const sc_dt::sc_fxval_fast &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_111>(&sc_dt::operator!=)), Fn_ne_111>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_111(Sources &s)
{
  return invoke<Fn_ne_111, const sc_dt::sc_fxval_fast &, const char *>(&sc_dt::operator!=, s);
}
using Fn_ne_112 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_112>(&sc_dt::operator!=)), Fn_ne_112>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_112(Sources &s)
{
  return invoke<Fn_ne_112, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_113 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_113>(&sc_dt::operator!=)), Fn_ne_113>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_113(Sources &s)
{
  return invoke<Fn_ne_113, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_114 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_114>(&sc_dt::operator!=)), Fn_ne_114>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_114(Sources &s)
{
  return invoke<Fn_ne_114, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_115 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_115>(&sc_dt::operator!=)), Fn_ne_115>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_115(Sources &s)
{
  return invoke<Fn_ne_115, const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_116 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_116>(&sc_dt::operator!=)), Fn_ne_116>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_116(Sources &s)
{
  return invoke<Fn_ne_116, const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_117 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_117>(&sc_dt::operator!=)), Fn_ne_117>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_117(Sources &s)
{
  return invoke<Fn_ne_117, const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_118 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_118>(&sc_dt::operator!=)), Fn_ne_118>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_118(Sources &s)
{
  return invoke<Fn_ne_118, const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_119 = bool (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_119>(&sc_dt::operator!=)), Fn_ne_119>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_119(Sources &s)
{
  return invoke<Fn_ne_119, const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_120 = bool (*)(const sc_dt::sc_fxval_fast &, double);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_120>(&sc_dt::operator!=)), Fn_ne_120>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, double)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_120(Sources &s)
{
  return invoke<Fn_ne_120, const sc_dt::sc_fxval_fast &, double>(&sc_dt::operator!=, s);
}
using Fn_ne_121 = bool (*)(const sc_dt::sc_fxval_fast &, float);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_121>(&sc_dt::operator!=)), Fn_ne_121>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, float)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_121(Sources &s)
{
  return invoke<Fn_ne_121, const sc_dt::sc_fxval_fast &, float>(&sc_dt::operator!=, s);
}
using Fn_ne_122 = bool (*)(const sc_dt::sc_fxval_fast &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_122>(&sc_dt::operator!=)), Fn_ne_122>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_122(Sources &s)
{
  return invoke<Fn_ne_122, const sc_dt::sc_fxval_fast &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_123 = bool (*)(const sc_dt::sc_fxval_fast &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_123>(&sc_dt::operator!=)), Fn_ne_123>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_123(Sources &s)
{
  return invoke<Fn_ne_123, const sc_dt::sc_fxval_fast &, long>(&sc_dt::operator!=, s);
}
using Fn_ne_124 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_124>(&sc_dt::operator!=)), Fn_ne_124>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_124(Sources &s)
{
  return invoke<Fn_ne_124, const sc_dt::sc_fxval_fast &, sc_dt::int64>(&sc_dt::operator!=, s);
}
using Fn_ne_125 = bool (*)(const sc_dt::sc_fxval_fast &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_125>(&sc_dt::operator!=)), Fn_ne_125>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_125(Sources &s)
{
  return invoke<Fn_ne_125, const sc_dt::sc_fxval_fast &, sc_dt::uint64>(&sc_dt::operator!=, s);
}
using Fn_ne_126 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_126>(&sc_dt::operator!=)), Fn_ne_126>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_126(Sources &s)
{
  return invoke<Fn_ne_126, const sc_dt::sc_fxval_fast &, unsigned int>(&sc_dt::operator!=, s);
}
using Fn_ne_127 = bool (*)(const sc_dt::sc_fxval_fast &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_127>(&sc_dt::operator!=)), Fn_ne_127>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_fxval_fast &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_127(Sources &s)
{
  return invoke<Fn_ne_127, const sc_dt::sc_fxval_fast &, unsigned long>(&sc_dt::operator!=, s);
}
using Fn_ne_128 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_128>(&sc_dt::operator!=)), Fn_ne_128>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_128(Sources &s)
{
  return invoke<Fn_ne_128, const sc_dt::sc_int_base &, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_129 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_129>(&sc_dt::operator!=)), Fn_ne_129>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_129(Sources &s)
{
  return invoke<Fn_ne_129, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_130 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_130>(&sc_dt::operator!=)), Fn_ne_130>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_130(Sources &s)
{
  return invoke<Fn_ne_130, const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_131 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_131>(&sc_dt::operator!=)), Fn_ne_131>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_131(Sources &s)
{
  return invoke<Fn_ne_131, const sc_dt::sc_int_base &, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_132 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_132>(&sc_dt::operator!=)), Fn_ne_132>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_132(Sources &s)
{
  return invoke<Fn_ne_132, const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_133 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_133>(&sc_dt::operator!=)), Fn_ne_133>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_int_base &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_133(Sources &s)
{
  return invoke<Fn_ne_133, const sc_dt::sc_int_base &, const sc_dt::sc_int_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_134 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_134>(&sc_dt::operator!=)), Fn_ne_134>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_134(Sources &s)
{
  return invoke<Fn_ne_134, const sc_dt::sc_int_base &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_135 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_135>(&sc_dt::operator!=)), Fn_ne_135>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_135(Sources &s)
{
  return invoke<Fn_ne_135, const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_136 = bool (*)(const sc_dt::sc_int_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_136>(&sc_dt::operator!=)), Fn_ne_136>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_int_base &, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_136(Sources &s)
{
  return invoke<Fn_ne_136, const sc_dt::sc_int_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=,
                                                                                                   s);
}
using Fn_ne_137 = bool (*)(const sc_dt::sc_length_param &, const sc_dt::sc_length_param &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_137>(&sc_dt::operator!=)), Fn_ne_137>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_length_param &, const sc_dt::sc_length_param &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_137(Sources &s)
{
  return invoke<Fn_ne_137, const sc_dt::sc_length_param &, const sc_dt::sc_length_param &>(&sc_dt::operator!=, s);
}
using Fn_ne_138 = bool (*)(const sc_dt::sc_logic &, bool);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_138>(&sc_dt::operator!=)), Fn_ne_138>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_logic &, bool)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_138(Sources &s)
{
  return invoke<Fn_ne_138, const sc_dt::sc_logic &, bool>(&sc_dt::operator!=, s);
}
using Fn_ne_139 = bool (*)(const sc_dt::sc_logic &, char);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_139>(&sc_dt::operator!=)), Fn_ne_139>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_logic &, char)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_139(Sources &s)
{
  return invoke<Fn_ne_139, const sc_dt::sc_logic &, char>(&sc_dt::operator!=, s);
}
using Fn_ne_140 = bool (*)(const sc_dt::sc_logic &, const sc_dt::sc_logic &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_140>(&sc_dt::operator!=)), Fn_ne_140>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_logic &, const sc_dt::sc_logic &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_140(Sources &s)
{
  return invoke<Fn_ne_140, const sc_dt::sc_logic &, const sc_dt::sc_logic &>(&sc_dt::operator!=, s);
}
using Fn_ne_141 = bool (*)(const sc_dt::sc_logic &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_141>(&sc_dt::operator!=)), Fn_ne_141>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_logic &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_141(Sources &s)
{
  return invoke<Fn_ne_141, const sc_dt::sc_logic &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_142 = bool (*)(const sc_dt::sc_logic &, sc_dt::sc_logic_value_t);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_142>(&sc_dt::operator!=)), Fn_ne_142>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_logic &, sc_dt::sc_logic_value_t)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_142(Sources &s)
{
  return invoke<Fn_ne_142, const sc_dt::sc_logic &, sc_dt::sc_logic_value_t>(&sc_dt::operator!=, s);
}
using Fn_ne_143 = bool (*)(const sc_dt::sc_logic *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_143>(&sc_dt::operator!=)), Fn_ne_143>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_logic *, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_143(Sources &s)
{
  return invoke<Fn_ne_143, const sc_dt::sc_logic *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=, s);
}
using Fn_ne_144 = bool (*)(const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_144>(&sc_dt::operator!=)), Fn_ne_144>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_144(Sources &s)
{
  return invoke<Fn_ne_144, const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_145 = bool (*)(const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_145>(&sc_dt::operator!=)), Fn_ne_145>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_145(Sources &s)
{
  return invoke<Fn_ne_145, const sc_dt::sc_lv_base &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_146 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_146>(&sc_dt::operator!=)), Fn_ne_146>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_146(Sources &s)
{
  return invoke<Fn_ne_146, const sc_dt::sc_signed &, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_147 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_147>(&sc_dt::operator!=)), Fn_ne_147>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_147(Sources &s)
{
  return invoke<Fn_ne_147, const sc_dt::sc_signed &, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_148 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_148>(&sc_dt::operator!=)), Fn_ne_148>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_148(Sources &s)
{
  return invoke<Fn_ne_148, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_149 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_149>(&sc_dt::operator!=)), Fn_ne_149>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_149(Sources &s)
{
  return invoke<Fn_ne_149, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_150 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_150>(&sc_dt::operator!=)), Fn_ne_150>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_150(Sources &s)
{
  return invoke<Fn_ne_150, const sc_dt::sc_signed &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_151 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_151>(&sc_dt::operator!=)), Fn_ne_151>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_151(Sources &s)
{
  return invoke<Fn_ne_151, const sc_dt::sc_signed &, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_152 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_152>(&sc_dt::operator!=)), Fn_ne_152>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_152(Sources &s)
{
  return invoke<Fn_ne_152, const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_153 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_153>(&sc_dt::operator!=)), Fn_ne_153>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_153(Sources &s)
{
  return invoke<Fn_ne_153, const sc_dt::sc_signed &, const sc_dt::sc_int_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_154 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_154>(&sc_dt::operator!=)), Fn_ne_154>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_154(Sources &s)
{
  return invoke<Fn_ne_154, const sc_dt::sc_signed &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_155 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_155>(&sc_dt::operator!=)), Fn_ne_155>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_155(Sources &s)
{
  return invoke<Fn_ne_155, const sc_dt::sc_signed &, const sc_dt::sc_uint_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_156 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_156>(&sc_dt::operator!=)), Fn_ne_156>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_156(Sources &s)
{
  return invoke<Fn_ne_156, const sc_dt::sc_signed &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_157 = bool (*)(const sc_dt::sc_signed &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_157>(&sc_dt::operator!=)), Fn_ne_157>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_157(Sources &s)
{
  return invoke<Fn_ne_157, const sc_dt::sc_signed &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=, s);
}
using Fn_ne_158 = bool (*)(const sc_dt::sc_signed &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_158>(&sc_dt::operator!=)), Fn_ne_158>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_158(Sources &s)
{
  return invoke<Fn_ne_158, const sc_dt::sc_signed &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_159 = bool (*)(const sc_dt::sc_signed &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_159>(&sc_dt::operator!=)), Fn_ne_159>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_159(Sources &s)
{
  return invoke<Fn_ne_159, const sc_dt::sc_signed &, long>(&sc_dt::operator!=, s);
}
using Fn_ne_160 = bool (*)(const sc_dt::sc_signed &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_160>(&sc_dt::operator!=)), Fn_ne_160>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_160(Sources &s)
{
  return invoke<Fn_ne_160, const sc_dt::sc_signed &, sc_dt::int64>(&sc_dt::operator!=, s);
}
using Fn_ne_161 = bool (*)(const sc_dt::sc_signed &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_161>(&sc_dt::operator!=)), Fn_ne_161>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_161(Sources &s)
{
  return invoke<Fn_ne_161, const sc_dt::sc_signed &, sc_dt::uint64>(&sc_dt::operator!=, s);
}
using Fn_ne_162 = bool (*)(const sc_dt::sc_signed &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_162>(&sc_dt::operator!=)), Fn_ne_162>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_162(Sources &s)
{
  return invoke<Fn_ne_162, const sc_dt::sc_signed &, unsigned int>(&sc_dt::operator!=, s);
}
using Fn_ne_163 = bool (*)(const sc_dt::sc_signed &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_163>(&sc_dt::operator!=)), Fn_ne_163>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_signed &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_163(Sources &s)
{
  return invoke<Fn_ne_163, const sc_dt::sc_signed &, unsigned long>(&sc_dt::operator!=, s);
}
using Fn_ne_164 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_164>(&sc_dt::operator!=)), Fn_ne_164>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_164(Sources &s)
{
  return invoke<Fn_ne_164, const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_165 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_165>(&sc_dt::operator!=)), Fn_ne_165>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_165(Sources &s)
{
  return invoke<Fn_ne_165, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_166 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_166>(&sc_dt::operator!=)), Fn_ne_166>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_166(Sources &s)
{
  return invoke<Fn_ne_166, const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_167 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_167>(&sc_dt::operator!=)), Fn_ne_167>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_167(Sources &s)
{
  return invoke<Fn_ne_167, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_168 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_168>(&sc_dt::operator!=)), Fn_ne_168>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_168(Sources &s)
{
  return invoke<Fn_ne_168, const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_169 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_169>(&sc_dt::operator!=)), Fn_ne_169>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_169(Sources &s)
{
  return invoke<Fn_ne_169, const sc_dt::sc_uint_base &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_170 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_170>(&sc_dt::operator!=)), Fn_ne_170>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_170(Sources &s)
{
  return invoke<Fn_ne_170, const sc_dt::sc_uint_base &, const sc_dt::sc_uint_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_171 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_171>(&sc_dt::operator!=)), Fn_ne_171>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_171(Sources &s)
{
  return invoke<Fn_ne_171, const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_172 = bool (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_172>(&sc_dt::operator!=)), Fn_ne_172>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_uint_base &, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_172(Sources &s)
{
  return invoke<Fn_ne_172, const sc_dt::sc_uint_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=,
                                                                                                    s);
}
using Fn_ne_173 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_173>(&sc_dt::operator!=)), Fn_ne_173>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_173(Sources &s)
{
  return invoke<Fn_ne_173, const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_174 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_174>(&sc_dt::operator!=)), Fn_ne_174>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_174(Sources &s)
{
  return invoke<Fn_ne_174, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_175 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_175>(&sc_dt::operator!=)), Fn_ne_175>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_175(Sources &s)
{
  return invoke<Fn_ne_175, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_176 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_176>(&sc_dt::operator!=)), Fn_ne_176>,
  "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_176(Sources &s)
{
  return invoke<Fn_ne_176, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_177 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_177>(&sc_dt::operator!=)), Fn_ne_177>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_177(Sources &s)
{
  return invoke<Fn_ne_177, const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_178 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_178>(&sc_dt::operator!=)), Fn_ne_178>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_178(Sources &s)
{
  return invoke<Fn_ne_178, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_179 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_179>(&sc_dt::operator!=)), Fn_ne_179>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_179(Sources &s)
{
  return invoke<Fn_ne_179, const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_180 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_180>(&sc_dt::operator!=)), Fn_ne_180>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_180(Sources &s)
{
  return invoke<Fn_ne_180, const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_181 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_181>(&sc_dt::operator!=)), Fn_ne_181>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_181(Sources &s)
{
  return invoke<Fn_ne_181, const sc_dt::sc_unsigned &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_182 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_182>(&sc_dt::operator!=)), Fn_ne_182>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_182(Sources &s)
{
  return invoke<Fn_ne_182, const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &>(&sc_dt::operator!=, s);
}
using Fn_ne_183 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_183>(&sc_dt::operator!=)), Fn_ne_183>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_183(Sources &s)
{
  return invoke<Fn_ne_183, const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_184 = bool (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_184>(&sc_dt::operator!=)), Fn_ne_184>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_184(Sources &s)
{
  return invoke<Fn_ne_184, const sc_dt::sc_unsigned &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=,
                                                                                                   s);
}
using Fn_ne_185 = bool (*)(const sc_dt::sc_unsigned &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_185>(&sc_dt::operator!=)), Fn_ne_185>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_185(Sources &s)
{
  return invoke<Fn_ne_185, const sc_dt::sc_unsigned &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_186 = bool (*)(const sc_dt::sc_unsigned &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_186>(&sc_dt::operator!=)), Fn_ne_186>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_186(Sources &s)
{
  return invoke<Fn_ne_186, const sc_dt::sc_unsigned &, long>(&sc_dt::operator!=, s);
}
using Fn_ne_187 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_187>(&sc_dt::operator!=)), Fn_ne_187>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_187(Sources &s)
{
  return invoke<Fn_ne_187, const sc_dt::sc_unsigned &, sc_dt::int64>(&sc_dt::operator!=, s);
}
using Fn_ne_188 = bool (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_188>(&sc_dt::operator!=)), Fn_ne_188>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_188(Sources &s)
{
  return invoke<Fn_ne_188, const sc_dt::sc_unsigned &, sc_dt::uint64>(&sc_dt::operator!=, s);
}
using Fn_ne_189 = bool (*)(const sc_dt::sc_unsigned &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_189>(&sc_dt::operator!=)), Fn_ne_189>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_189(Sources &s)
{
  return invoke<Fn_ne_189, const sc_dt::sc_unsigned &, unsigned int>(&sc_dt::operator!=, s);
}
using Fn_ne_190 = bool (*)(const sc_dt::sc_unsigned &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_190>(&sc_dt::operator!=)), Fn_ne_190>,
              "exact overload sc_dt::operator!= bool (const sc_dt::sc_unsigned &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_190(Sources &s)
{
  return invoke<Fn_ne_190, const sc_dt::sc_unsigned &, unsigned long>(&sc_dt::operator!=, s);
}
using Fn_ne_191 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const bool *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_191>(&sc_dt::operator!=)), Fn_ne_191>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, const bool *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_191(Sources &s)
{
  return invoke<Fn_ne_191, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const bool *>(&sc_dt::operator!=, s);
}
using Fn_ne_192 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_192>(&sc_dt::operator!=)), Fn_ne_192>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_192(Sources &s)
{
  return invoke<Fn_ne_192, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const char *>(&sc_dt::operator!=, s);
}
using Fn_ne_193 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_int_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_193>(&sc_dt::operator!=)), Fn_ne_193>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, const sc_dt::sc_int_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_193(Sources &s)
{
  return invoke<Fn_ne_193, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_int_base &>(&sc_dt::operator!=,
                                                                                                   s);
}
using Fn_ne_194 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_logic *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_194>(&sc_dt::operator!=)), Fn_ne_194>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, const sc_dt::sc_logic *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_194(Sources &s)
{
  return invoke<Fn_ne_194, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_logic *>(&sc_dt::operator!=, s);
}
using Fn_ne_195 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_195>(&sc_dt::operator!=)), Fn_ne_195>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_195(Sources &s)
{
  return invoke<Fn_ne_195, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_196 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_uint_base &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_196>(&sc_dt::operator!=)), Fn_ne_196>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, const sc_dt::sc_uint_base &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_196(Sources &s)
{
  return invoke<Fn_ne_196, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_uint_base &>(&sc_dt::operator!=,
                                                                                                    s);
}
using Fn_ne_197 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_197>(&sc_dt::operator!=)), Fn_ne_197>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_197(Sources &s)
{
  return invoke<Fn_ne_197, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=,
                                                                                                   s);
}
using Fn_ne_198 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_198>(&sc_dt::operator!=)), Fn_ne_198>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, const sc_proxy<Y> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_198(Sources &s)
{
  return invoke<Fn_ne_198, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &>(
    &sc_dt::operator!=, s);
}
using Fn_ne_199 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_199>(&sc_dt::operator!=)), Fn_ne_199>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_199(Sources &s)
{
  return invoke<Fn_ne_199, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_200 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_200>(&sc_dt::operator!=)), Fn_ne_200>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_200(Sources &s)
{
  return invoke<Fn_ne_200, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, long>(&sc_dt::operator!=, s);
}
using Fn_ne_201 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_201>(&sc_dt::operator!=)), Fn_ne_201>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_201(Sources &s)
{
  return invoke<Fn_ne_201, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, sc_dt::int64>(&sc_dt::operator!=, s);
}
using Fn_ne_202 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_202>(&sc_dt::operator!=)), Fn_ne_202>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_202(Sources &s)
{
  return invoke<Fn_ne_202, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, sc_dt::uint64>(&sc_dt::operator!=, s);
}
using Fn_ne_203 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_203>(&sc_dt::operator!=)), Fn_ne_203>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_203(Sources &s)
{
  return invoke<Fn_ne_203, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, unsigned int>(&sc_dt::operator!=, s);
}
using Fn_ne_204 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_204>(&sc_dt::operator!=)), Fn_ne_204>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<X> &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_204(Sources &s)
{
  return invoke<Fn_ne_204, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, unsigned long>(&sc_dt::operator!=, s);
}
using Fn_ne_205 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const bool *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_205>(&sc_dt::operator!=)), Fn_ne_205>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, const bool *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_205(Sources &s)
{
  return invoke<Fn_ne_205, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const bool *>(&sc_dt::operator!=, s);
}
using Fn_ne_206 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const char *);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_206>(&sc_dt::operator!=)), Fn_ne_206>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, const char *)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_206(Sources &s)
{
  return invoke<Fn_ne_206, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const char *>(&sc_dt::operator!=, s);
}
using Fn_ne_207 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_207>(&sc_dt::operator!=)), Fn_ne_207>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_207(Sources &s)
{
  return invoke<Fn_ne_207, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_208 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_208>(&sc_dt::operator!=)), Fn_ne_208>,
  "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_208(Sources &s)
{
  return invoke<Fn_ne_208, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_unsigned &>(&sc_dt::operator!=,
                                                                                                   s);
}
using Fn_ne_209 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_209>(&sc_dt::operator!=)), Fn_ne_209>,
  "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, const sc_proxy<sc_dt::sc_bv_base> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_209(Sources &s)
{
  return invoke<Fn_ne_209, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    &sc_dt::operator!=, s);
}
using Fn_ne_210 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &);
static_assert(
  std::is_same_v<decltype(static_cast<Fn_ne_210>(&sc_dt::operator!=)), Fn_ne_210>,
  "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, const sc_proxy<sc_dt::sc_lv_base> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_210(Sources &s)
{
  return invoke<Fn_ne_210, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &>(
    &sc_dt::operator!=, s);
}
using Fn_ne_211 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_211>(&sc_dt::operator!=)), Fn_ne_211>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_211(Sources &s)
{
  return invoke<Fn_ne_211, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, int>(&sc_dt::operator!=, s);
}
using Fn_ne_212 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_212>(&sc_dt::operator!=)), Fn_ne_212>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_212(Sources &s)
{
  return invoke<Fn_ne_212, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, long>(&sc_dt::operator!=, s);
}
using Fn_ne_213 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, sc_dt::int64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_213>(&sc_dt::operator!=)), Fn_ne_213>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, sc_dt::int64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_213(Sources &s)
{
  return invoke<Fn_ne_213, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, sc_dt::int64>(&sc_dt::operator!=, s);
}
using Fn_ne_214 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, sc_dt::uint64);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_214>(&sc_dt::operator!=)), Fn_ne_214>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, sc_dt::uint64)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_214(Sources &s)
{
  return invoke<Fn_ne_214, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, sc_dt::uint64>(&sc_dt::operator!=, s);
}
using Fn_ne_215 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, unsigned int);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_215>(&sc_dt::operator!=)), Fn_ne_215>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, unsigned int)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_215(Sources &s)
{
  return invoke<Fn_ne_215, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, unsigned int>(&sc_dt::operator!=, s);
}
using Fn_ne_216 = bool (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, unsigned long);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_216>(&sc_dt::operator!=)), Fn_ne_216>,
              "exact overload sc_dt::operator!= bool (const sc_proxy<sc_dt::sc_bv_base> &, unsigned long)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_216(Sources &s)
{
  return invoke<Fn_ne_216, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, unsigned long>(&sc_dt::operator!=, s);
}
using Fn_ne_217 = bool (*)(double, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_217>(&sc_dt::operator!=)), Fn_ne_217>,
              "exact overload sc_dt::operator!= bool (double, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_217(Sources &s)
{
  return invoke<Fn_ne_217, double, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_218 = bool (*)(double, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_218>(&sc_dt::operator!=)), Fn_ne_218>,
              "exact overload sc_dt::operator!= bool (double, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_218(Sources &s)
{
  return invoke<Fn_ne_218, double, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_219 = bool (*)(double, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_219>(&sc_dt::operator!=)), Fn_ne_219>,
              "exact overload sc_dt::operator!= bool (double, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_219(Sources &s)
{
  return invoke<Fn_ne_219, double, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_220 = bool (*)(double, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_220>(&sc_dt::operator!=)), Fn_ne_220>,
              "exact overload sc_dt::operator!= bool (double, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_220(Sources &s)
{
  return invoke<Fn_ne_220, double, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_221 = bool (*)(float, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_221>(&sc_dt::operator!=)), Fn_ne_221>,
              "exact overload sc_dt::operator!= bool (float, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_221(Sources &s)
{
  return invoke<Fn_ne_221, float, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_222 = bool (*)(float, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_222>(&sc_dt::operator!=)), Fn_ne_222>,
              "exact overload sc_dt::operator!= bool (float, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_222(Sources &s)
{
  return invoke<Fn_ne_222, float, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_223 = bool (*)(float, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_223>(&sc_dt::operator!=)), Fn_ne_223>,
              "exact overload sc_dt::operator!= bool (float, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_223(Sources &s)
{
  return invoke<Fn_ne_223, float, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_224 = bool (*)(float, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_224>(&sc_dt::operator!=)), Fn_ne_224>,
              "exact overload sc_dt::operator!= bool (float, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_224(Sources &s)
{
  return invoke<Fn_ne_224, float, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_225 = bool (*)(int, const sc_dt::sc_bit &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_225>(&sc_dt::operator!=)), Fn_ne_225>,
              "exact overload sc_dt::operator!= bool (int, const sc_dt::sc_bit &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_225(Sources &s)
{
  return invoke<Fn_ne_225, int, const sc_dt::sc_bit &>(&sc_dt::operator!=, s);
}
using Fn_ne_226 = bool (*)(int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_226>(&sc_dt::operator!=)), Fn_ne_226>,
              "exact overload sc_dt::operator!= bool (int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_226(Sources &s)
{
  return invoke<Fn_ne_226, int, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_227 = bool (*)(int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_227>(&sc_dt::operator!=)), Fn_ne_227>,
              "exact overload sc_dt::operator!= bool (int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_227(Sources &s)
{
  return invoke<Fn_ne_227, int, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_228 = bool (*)(int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_228>(&sc_dt::operator!=)), Fn_ne_228>,
              "exact overload sc_dt::operator!= bool (int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_228(Sources &s)
{
  return invoke<Fn_ne_228, int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_229 = bool (*)(int, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_229>(&sc_dt::operator!=)), Fn_ne_229>,
              "exact overload sc_dt::operator!= bool (int, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_229(Sources &s)
{
  return invoke<Fn_ne_229, int, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_230 = bool (*)(int, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_230>(&sc_dt::operator!=)), Fn_ne_230>,
              "exact overload sc_dt::operator!= bool (int, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_230(Sources &s)
{
  return invoke<Fn_ne_230, int, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_231 = bool (*)(int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_231>(&sc_dt::operator!=)), Fn_ne_231>,
              "exact overload sc_dt::operator!= bool (int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_231(Sources &s)
{
  return invoke<Fn_ne_231, int, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_232 = bool (*)(int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_232>(&sc_dt::operator!=)), Fn_ne_232>,
              "exact overload sc_dt::operator!= bool (int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_232(Sources &s)
{
  return invoke<Fn_ne_232, int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_233 = bool (*)(int, const sc_dt::sc_logic &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_233>(&sc_dt::operator!=)), Fn_ne_233>,
              "exact overload sc_dt::operator!= bool (int, const sc_dt::sc_logic &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_233(Sources &s)
{
  return invoke<Fn_ne_233, int, const sc_dt::sc_logic &>(&sc_dt::operator!=, s);
}
using Fn_ne_234 = bool (*)(int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_234>(&sc_dt::operator!=)), Fn_ne_234>,
              "exact overload sc_dt::operator!= bool (int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_234(Sources &s)
{
  return invoke<Fn_ne_234, int, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_235 = bool (*)(int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_235>(&sc_dt::operator!=)), Fn_ne_235>,
              "exact overload sc_dt::operator!= bool (int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_235(Sources &s)
{
  return invoke<Fn_ne_235, int, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_236 = bool (*)(int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_236>(&sc_dt::operator!=)), Fn_ne_236>,
              "exact overload sc_dt::operator!= bool (int, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_236(Sources &s)
{
  return invoke<Fn_ne_236, int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=, s);
}
using Fn_ne_237 = bool (*)(long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_237>(&sc_dt::operator!=)), Fn_ne_237>,
              "exact overload sc_dt::operator!= bool (long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_237(Sources &s)
{
  return invoke<Fn_ne_237, long, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_238 = bool (*)(long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_238>(&sc_dt::operator!=)), Fn_ne_238>,
              "exact overload sc_dt::operator!= bool (long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_238(Sources &s)
{
  return invoke<Fn_ne_238, long, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_239 = bool (*)(long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_239>(&sc_dt::operator!=)), Fn_ne_239>,
              "exact overload sc_dt::operator!= bool (long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_239(Sources &s)
{
  return invoke<Fn_ne_239, long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_240 = bool (*)(long, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_240>(&sc_dt::operator!=)), Fn_ne_240>,
              "exact overload sc_dt::operator!= bool (long, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_240(Sources &s)
{
  return invoke<Fn_ne_240, long, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_241 = bool (*)(long, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_241>(&sc_dt::operator!=)), Fn_ne_241>,
              "exact overload sc_dt::operator!= bool (long, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_241(Sources &s)
{
  return invoke<Fn_ne_241, long, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_242 = bool (*)(long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_242>(&sc_dt::operator!=)), Fn_ne_242>,
              "exact overload sc_dt::operator!= bool (long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_242(Sources &s)
{
  return invoke<Fn_ne_242, long, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_243 = bool (*)(long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_243>(&sc_dt::operator!=)), Fn_ne_243>,
              "exact overload sc_dt::operator!= bool (long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_243(Sources &s)
{
  return invoke<Fn_ne_243, long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_244 = bool (*)(long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_244>(&sc_dt::operator!=)), Fn_ne_244>,
              "exact overload sc_dt::operator!= bool (long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_244(Sources &s)
{
  return invoke<Fn_ne_244, long, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_245 = bool (*)(long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_245>(&sc_dt::operator!=)), Fn_ne_245>,
              "exact overload sc_dt::operator!= bool (long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_245(Sources &s)
{
  return invoke<Fn_ne_245, long, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_246 = bool (*)(long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_246>(&sc_dt::operator!=)), Fn_ne_246>,
              "exact overload sc_dt::operator!= bool (long, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_246(Sources &s)
{
  return invoke<Fn_ne_246, long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=, s);
}
using Fn_ne_247 = bool (*)(sc_dt::int64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_247>(&sc_dt::operator!=)), Fn_ne_247>,
              "exact overload sc_dt::operator!= bool (sc_dt::int64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_247(Sources &s)
{
  return invoke<Fn_ne_247, sc_dt::int64, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_248 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_248>(&sc_dt::operator!=)), Fn_ne_248>,
              "exact overload sc_dt::operator!= bool (sc_dt::int64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_248(Sources &s)
{
  return invoke<Fn_ne_248, sc_dt::int64, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_249 = bool (*)(sc_dt::int64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_249>(&sc_dt::operator!=)), Fn_ne_249>,
              "exact overload sc_dt::operator!= bool (sc_dt::int64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_249(Sources &s)
{
  return invoke<Fn_ne_249, sc_dt::int64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_250 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_250>(&sc_dt::operator!=)), Fn_ne_250>,
              "exact overload sc_dt::operator!= bool (sc_dt::int64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_250(Sources &s)
{
  return invoke<Fn_ne_250, sc_dt::int64, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_251 = bool (*)(sc_dt::int64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_251>(&sc_dt::operator!=)), Fn_ne_251>,
              "exact overload sc_dt::operator!= bool (sc_dt::int64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_251(Sources &s)
{
  return invoke<Fn_ne_251, sc_dt::int64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_252 = bool (*)(sc_dt::int64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_252>(&sc_dt::operator!=)), Fn_ne_252>,
              "exact overload sc_dt::operator!= bool (sc_dt::int64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_252(Sources &s)
{
  return invoke<Fn_ne_252, sc_dt::int64, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_253 = bool (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_253>(&sc_dt::operator!=)), Fn_ne_253>,
              "exact overload sc_dt::operator!= bool (sc_dt::int64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_253(Sources &s)
{
  return invoke<Fn_ne_253, sc_dt::int64, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_254 = bool (*)(sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_254>(&sc_dt::operator!=)), Fn_ne_254>,
              "exact overload sc_dt::operator!= bool (sc_dt::int64, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_254(Sources &s)
{
  return invoke<Fn_ne_254, sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=, s);
}
using Fn_ne_255 = bool (*)(sc_dt::sc_logic_value_t, const sc_dt::sc_logic &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_255>(&sc_dt::operator!=)), Fn_ne_255>,
              "exact overload sc_dt::operator!= bool (sc_dt::sc_logic_value_t, const sc_dt::sc_logic &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_255(Sources &s)
{
  return invoke<Fn_ne_255, sc_dt::sc_logic_value_t, const sc_dt::sc_logic &>(&sc_dt::operator!=, s);
}
using Fn_ne_256 = bool (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_256>(&sc_dt::operator!=)), Fn_ne_256>,
              "exact overload sc_dt::operator!= bool (sc_dt::uint64, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_256(Sources &s)
{
  return invoke<Fn_ne_256, sc_dt::uint64, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_257 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_257>(&sc_dt::operator!=)), Fn_ne_257>,
              "exact overload sc_dt::operator!= bool (sc_dt::uint64, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_257(Sources &s)
{
  return invoke<Fn_ne_257, sc_dt::uint64, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_258 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_258>(&sc_dt::operator!=)), Fn_ne_258>,
              "exact overload sc_dt::operator!= bool (sc_dt::uint64, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_258(Sources &s)
{
  return invoke<Fn_ne_258, sc_dt::uint64, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_259 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_259>(&sc_dt::operator!=)), Fn_ne_259>,
              "exact overload sc_dt::operator!= bool (sc_dt::uint64, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_259(Sources &s)
{
  return invoke<Fn_ne_259, sc_dt::uint64, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_260 = bool (*)(sc_dt::uint64, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_260>(&sc_dt::operator!=)), Fn_ne_260>,
              "exact overload sc_dt::operator!= bool (sc_dt::uint64, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_260(Sources &s)
{
  return invoke<Fn_ne_260, sc_dt::uint64, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_261 = bool (*)(sc_dt::uint64, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_261>(&sc_dt::operator!=)), Fn_ne_261>,
              "exact overload sc_dt::operator!= bool (sc_dt::uint64, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_261(Sources &s)
{
  return invoke<Fn_ne_261, sc_dt::uint64, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_262 = bool (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_262>(&sc_dt::operator!=)), Fn_ne_262>,
              "exact overload sc_dt::operator!= bool (sc_dt::uint64, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_262(Sources &s)
{
  return invoke<Fn_ne_262, sc_dt::uint64, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_263 = bool (*)(sc_dt::uint64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_263>(&sc_dt::operator!=)), Fn_ne_263>,
              "exact overload sc_dt::operator!= bool (sc_dt::uint64, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_263(Sources &s)
{
  return invoke<Fn_ne_263, sc_dt::uint64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=, s);
}
using Fn_ne_264 = bool (*)(unsigned int, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_264>(&sc_dt::operator!=)), Fn_ne_264>,
              "exact overload sc_dt::operator!= bool (unsigned int, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_264(Sources &s)
{
  return invoke<Fn_ne_264, unsigned int, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_265 = bool (*)(unsigned int, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_265>(&sc_dt::operator!=)), Fn_ne_265>,
              "exact overload sc_dt::operator!= bool (unsigned int, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_265(Sources &s)
{
  return invoke<Fn_ne_265, unsigned int, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_266 = bool (*)(unsigned int, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_266>(&sc_dt::operator!=)), Fn_ne_266>,
              "exact overload sc_dt::operator!= bool (unsigned int, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_266(Sources &s)
{
  return invoke<Fn_ne_266, unsigned int, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_267 = bool (*)(unsigned int, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_267>(&sc_dt::operator!=)), Fn_ne_267>,
              "exact overload sc_dt::operator!= bool (unsigned int, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_267(Sources &s)
{
  return invoke<Fn_ne_267, unsigned int, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_268 = bool (*)(unsigned int, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_268>(&sc_dt::operator!=)), Fn_ne_268>,
              "exact overload sc_dt::operator!= bool (unsigned int, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_268(Sources &s)
{
  return invoke<Fn_ne_268, unsigned int, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_269 = bool (*)(unsigned int, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_269>(&sc_dt::operator!=)), Fn_ne_269>,
              "exact overload sc_dt::operator!= bool (unsigned int, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_269(Sources &s)
{
  return invoke<Fn_ne_269, unsigned int, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_270 = bool (*)(unsigned int, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_270>(&sc_dt::operator!=)), Fn_ne_270>,
              "exact overload sc_dt::operator!= bool (unsigned int, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_270(Sources &s)
{
  return invoke<Fn_ne_270, unsigned int, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_271 = bool (*)(unsigned int, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_271>(&sc_dt::operator!=)), Fn_ne_271>,
              "exact overload sc_dt::operator!= bool (unsigned int, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_271(Sources &s)
{
  return invoke<Fn_ne_271, unsigned int, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_272 = bool (*)(unsigned int, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_272>(&sc_dt::operator!=)), Fn_ne_272>,
              "exact overload sc_dt::operator!= bool (unsigned int, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_272(Sources &s)
{
  return invoke<Fn_ne_272, unsigned int, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_273 = bool (*)(unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_273>(&sc_dt::operator!=)), Fn_ne_273>,
              "exact overload sc_dt::operator!= bool (unsigned int, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_273(Sources &s)
{
  return invoke<Fn_ne_273, unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=, s);
}
using Fn_ne_274 = bool (*)(unsigned long, const sc_dt::sc_concatref &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_274>(&sc_dt::operator!=)), Fn_ne_274>,
              "exact overload sc_dt::operator!= bool (unsigned long, const sc_dt::sc_concatref &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_274(Sources &s)
{
  return invoke<Fn_ne_274, unsigned long, const sc_dt::sc_concatref &>(&sc_dt::operator!=, s);
}
using Fn_ne_275 = bool (*)(unsigned long, const sc_dt::sc_fxnum &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_275>(&sc_dt::operator!=)), Fn_ne_275>,
              "exact overload sc_dt::operator!= bool (unsigned long, const sc_dt::sc_fxnum &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_275(Sources &s)
{
  return invoke<Fn_ne_275, unsigned long, const sc_dt::sc_fxnum &>(&sc_dt::operator!=, s);
}
using Fn_ne_276 = bool (*)(unsigned long, const sc_dt::sc_fxnum_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_276>(&sc_dt::operator!=)), Fn_ne_276>,
              "exact overload sc_dt::operator!= bool (unsigned long, const sc_dt::sc_fxnum_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_276(Sources &s)
{
  return invoke<Fn_ne_276, unsigned long, const sc_dt::sc_fxnum_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_277 = bool (*)(unsigned long, const sc_dt::sc_fxnum_fast_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_277>(&sc_dt::operator!=)), Fn_ne_277>,
              "exact overload sc_dt::operator!= bool (unsigned long, const sc_dt::sc_fxnum_fast_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_277(Sources &s)
{
  return invoke<Fn_ne_277, unsigned long, const sc_dt::sc_fxnum_fast_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_278 = bool (*)(unsigned long, const sc_dt::sc_fxnum_subref_r &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_278>(&sc_dt::operator!=)), Fn_ne_278>,
              "exact overload sc_dt::operator!= bool (unsigned long, const sc_dt::sc_fxnum_subref_r &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_278(Sources &s)
{
  return invoke<Fn_ne_278, unsigned long, const sc_dt::sc_fxnum_subref_r &>(&sc_dt::operator!=, s);
}
using Fn_ne_279 = bool (*)(unsigned long, const sc_dt::sc_fxval &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_279>(&sc_dt::operator!=)), Fn_ne_279>,
              "exact overload sc_dt::operator!= bool (unsigned long, const sc_dt::sc_fxval &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_279(Sources &s)
{
  return invoke<Fn_ne_279, unsigned long, const sc_dt::sc_fxval &>(&sc_dt::operator!=, s);
}
using Fn_ne_280 = bool (*)(unsigned long, const sc_dt::sc_fxval_fast &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_280>(&sc_dt::operator!=)), Fn_ne_280>,
              "exact overload sc_dt::operator!= bool (unsigned long, const sc_dt::sc_fxval_fast &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_280(Sources &s)
{
  return invoke<Fn_ne_280, unsigned long, const sc_dt::sc_fxval_fast &>(&sc_dt::operator!=, s);
}
using Fn_ne_281 = bool (*)(unsigned long, const sc_dt::sc_signed &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_281>(&sc_dt::operator!=)), Fn_ne_281>,
              "exact overload sc_dt::operator!= bool (unsigned long, const sc_dt::sc_signed &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_281(Sources &s)
{
  return invoke<Fn_ne_281, unsigned long, const sc_dt::sc_signed &>(&sc_dt::operator!=, s);
}
using Fn_ne_282 = bool (*)(unsigned long, const sc_dt::sc_unsigned &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_282>(&sc_dt::operator!=)), Fn_ne_282>,
              "exact overload sc_dt::operator!= bool (unsigned long, const sc_dt::sc_unsigned &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_282(Sources &s)
{
  return invoke<Fn_ne_282, unsigned long, const sc_dt::sc_unsigned &>(&sc_dt::operator!=, s);
}
using Fn_ne_283 = bool (*)(unsigned long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same_v<decltype(static_cast<Fn_ne_283>(&sc_dt::operator!=)), Fn_ne_283>,
              "exact overload sc_dt::operator!= bool (unsigned long, const sc_proxy<X> &)");
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne_283(Sources &s)
{
  return invoke<Fn_ne_283, unsigned long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(&sc_dt::operator!=, s);
}
inline unsigned count_ne = 284;
/** @brief Invoke one exact inventoried native comparison overload.
 * @param s Owns each borrowed source operand through the synchronous call.
 * @return The comparison result copied to bool.
 */
inline bool case_ne(unsigned index, Sources &s)
{
  switch (index)
  {
  case 0:
    return case_ne_000(s);
  case 1:
    return case_ne_001(s);
  case 2:
    return case_ne_002(s);
  case 3:
    return case_ne_003(s);
  case 4:
    return case_ne_004(s);
  case 5:
    return case_ne_005(s);
  case 6:
    return case_ne_006(s);
  case 7:
    return case_ne_007(s);
  case 8:
    return case_ne_008(s);
  case 9:
    return case_ne_009(s);
  case 10:
    return case_ne_010(s);
  case 11:
    return case_ne_011(s);
  case 12:
    return case_ne_012(s);
  case 13:
    return case_ne_013(s);
  case 14:
    return case_ne_014(s);
  case 15:
    return case_ne_015(s);
  case 16:
    return case_ne_016(s);
  case 17:
    return case_ne_017(s);
  case 18:
    return case_ne_018(s);
  case 19:
    return case_ne_019(s);
  case 20:
    return case_ne_020(s);
  case 21:
    return case_ne_021(s);
  case 22:
    return case_ne_022(s);
  case 23:
    return case_ne_023(s);
  case 24:
    return case_ne_024(s);
  case 25:
    return case_ne_025(s);
  case 26:
    return case_ne_026(s);
  case 27:
    return case_ne_027(s);
  case 28:
    return case_ne_028(s);
  case 29:
    return case_ne_029(s);
  case 30:
    return case_ne_030(s);
  case 31:
    return case_ne_031(s);
  case 32:
    return case_ne_032(s);
  case 33:
    return case_ne_033(s);
  case 34:
    return case_ne_034(s);
  case 35:
    return case_ne_035(s);
  case 36:
    return case_ne_036(s);
  case 37:
    return case_ne_037(s);
  case 38:
    return case_ne_038(s);
  case 39:
    return case_ne_039(s);
  case 40:
    return case_ne_040(s);
  case 41:
    return case_ne_041(s);
  case 42:
    return case_ne_042(s);
  case 43:
    return case_ne_043(s);
  case 44:
    return case_ne_044(s);
  case 45:
    return case_ne_045(s);
  case 46:
    return case_ne_046(s);
  case 47:
    return case_ne_047(s);
  case 48:
    return case_ne_048(s);
  case 49:
    return case_ne_049(s);
  case 50:
    return case_ne_050(s);
  case 51:
    return case_ne_051(s);
  case 52:
    return case_ne_052(s);
  case 53:
    return case_ne_053(s);
  case 54:
    return case_ne_054(s);
  case 55:
    return case_ne_055(s);
  case 56:
    return case_ne_056(s);
  case 57:
    return case_ne_057(s);
  case 58:
    return case_ne_058(s);
  case 59:
    return case_ne_059(s);
  case 60:
    return case_ne_060(s);
  case 61:
    return case_ne_061(s);
  case 62:
    return case_ne_062(s);
  case 63:
    return case_ne_063(s);
  case 64:
    return case_ne_064(s);
  case 65:
    return case_ne_065(s);
  case 66:
    return case_ne_066(s);
  case 67:
    return case_ne_067(s);
  case 68:
    return case_ne_068(s);
  case 69:
    return case_ne_069(s);
  case 70:
    return case_ne_070(s);
  case 71:
    return case_ne_071(s);
  case 72:
    return case_ne_072(s);
  case 73:
    return case_ne_073(s);
  case 74:
    return case_ne_074(s);
  case 75:
    return case_ne_075(s);
  case 76:
    return case_ne_076(s);
  case 77:
    return case_ne_077(s);
  case 78:
    return case_ne_078(s);
  case 79:
    return case_ne_079(s);
  case 80:
    return case_ne_080(s);
  case 81:
    return case_ne_081(s);
  case 82:
    return case_ne_082(s);
  case 83:
    return case_ne_083(s);
  case 84:
    return case_ne_084(s);
  case 85:
    return case_ne_085(s);
  case 86:
    return case_ne_086(s);
  case 87:
    return case_ne_087(s);
  case 88:
    return case_ne_088(s);
  case 89:
    return case_ne_089(s);
  case 90:
    return case_ne_090(s);
  case 91:
    return case_ne_091(s);
  case 92:
    return case_ne_092(s);
  case 93:
    return case_ne_093(s);
  case 94:
    return case_ne_094(s);
  case 95:
    return case_ne_095(s);
  case 96:
    return case_ne_096(s);
  case 97:
    return case_ne_097(s);
  case 98:
    return case_ne_098(s);
  case 99:
    return case_ne_099(s);
  case 100:
    return case_ne_100(s);
  case 101:
    return case_ne_101(s);
  case 102:
    return case_ne_102(s);
  case 103:
    return case_ne_103(s);
  case 104:
    return case_ne_104(s);
  case 105:
    return case_ne_105(s);
  case 106:
    return case_ne_106(s);
  case 107:
    return case_ne_107(s);
  case 108:
    return case_ne_108(s);
  case 109:
    return case_ne_109(s);
  case 110:
    return case_ne_110(s);
  case 111:
    return case_ne_111(s);
  case 112:
    return case_ne_112(s);
  case 113:
    return case_ne_113(s);
  case 114:
    return case_ne_114(s);
  case 115:
    return case_ne_115(s);
  case 116:
    return case_ne_116(s);
  case 117:
    return case_ne_117(s);
  case 118:
    return case_ne_118(s);
  case 119:
    return case_ne_119(s);
  case 120:
    return case_ne_120(s);
  case 121:
    return case_ne_121(s);
  case 122:
    return case_ne_122(s);
  case 123:
    return case_ne_123(s);
  case 124:
    return case_ne_124(s);
  case 125:
    return case_ne_125(s);
  case 126:
    return case_ne_126(s);
  case 127:
    return case_ne_127(s);
  case 128:
    return case_ne_128(s);
  case 129:
    return case_ne_129(s);
  case 130:
    return case_ne_130(s);
  case 131:
    return case_ne_131(s);
  case 132:
    return case_ne_132(s);
  case 133:
    return case_ne_133(s);
  case 134:
    return case_ne_134(s);
  case 135:
    return case_ne_135(s);
  case 136:
    return case_ne_136(s);
  case 137:
    return case_ne_137(s);
  case 138:
    return case_ne_138(s);
  case 139:
    return case_ne_139(s);
  case 140:
    return case_ne_140(s);
  case 141:
    return case_ne_141(s);
  case 142:
    return case_ne_142(s);
  case 143:
    return case_ne_143(s);
  case 144:
    return case_ne_144(s);
  case 145:
    return case_ne_145(s);
  case 146:
    return case_ne_146(s);
  case 147:
    return case_ne_147(s);
  case 148:
    return case_ne_148(s);
  case 149:
    return case_ne_149(s);
  case 150:
    return case_ne_150(s);
  case 151:
    return case_ne_151(s);
  case 152:
    return case_ne_152(s);
  case 153:
    return case_ne_153(s);
  case 154:
    return case_ne_154(s);
  case 155:
    return case_ne_155(s);
  case 156:
    return case_ne_156(s);
  case 157:
    return case_ne_157(s);
  case 158:
    return case_ne_158(s);
  case 159:
    return case_ne_159(s);
  case 160:
    return case_ne_160(s);
  case 161:
    return case_ne_161(s);
  case 162:
    return case_ne_162(s);
  case 163:
    return case_ne_163(s);
  case 164:
    return case_ne_164(s);
  case 165:
    return case_ne_165(s);
  case 166:
    return case_ne_166(s);
  case 167:
    return case_ne_167(s);
  case 168:
    return case_ne_168(s);
  case 169:
    return case_ne_169(s);
  case 170:
    return case_ne_170(s);
  case 171:
    return case_ne_171(s);
  case 172:
    return case_ne_172(s);
  case 173:
    return case_ne_173(s);
  case 174:
    return case_ne_174(s);
  case 175:
    return case_ne_175(s);
  case 176:
    return case_ne_176(s);
  case 177:
    return case_ne_177(s);
  case 178:
    return case_ne_178(s);
  case 179:
    return case_ne_179(s);
  case 180:
    return case_ne_180(s);
  case 181:
    return case_ne_181(s);
  case 182:
    return case_ne_182(s);
  case 183:
    return case_ne_183(s);
  case 184:
    return case_ne_184(s);
  case 185:
    return case_ne_185(s);
  case 186:
    return case_ne_186(s);
  case 187:
    return case_ne_187(s);
  case 188:
    return case_ne_188(s);
  case 189:
    return case_ne_189(s);
  case 190:
    return case_ne_190(s);
  case 191:
    return case_ne_191(s);
  case 192:
    return case_ne_192(s);
  case 193:
    return case_ne_193(s);
  case 194:
    return case_ne_194(s);
  case 195:
    return case_ne_195(s);
  case 196:
    return case_ne_196(s);
  case 197:
    return case_ne_197(s);
  case 198:
    return case_ne_198(s);
  case 199:
    return case_ne_199(s);
  case 200:
    return case_ne_200(s);
  case 201:
    return case_ne_201(s);
  case 202:
    return case_ne_202(s);
  case 203:
    return case_ne_203(s);
  case 204:
    return case_ne_204(s);
  case 205:
    return case_ne_205(s);
  case 206:
    return case_ne_206(s);
  case 207:
    return case_ne_207(s);
  case 208:
    return case_ne_208(s);
  case 209:
    return case_ne_209(s);
  case 210:
    return case_ne_210(s);
  case 211:
    return case_ne_211(s);
  case 212:
    return case_ne_212(s);
  case 213:
    return case_ne_213(s);
  case 214:
    return case_ne_214(s);
  case 215:
    return case_ne_215(s);
  case 216:
    return case_ne_216(s);
  case 217:
    return case_ne_217(s);
  case 218:
    return case_ne_218(s);
  case 219:
    return case_ne_219(s);
  case 220:
    return case_ne_220(s);
  case 221:
    return case_ne_221(s);
  case 222:
    return case_ne_222(s);
  case 223:
    return case_ne_223(s);
  case 224:
    return case_ne_224(s);
  case 225:
    return case_ne_225(s);
  case 226:
    return case_ne_226(s);
  case 227:
    return case_ne_227(s);
  case 228:
    return case_ne_228(s);
  case 229:
    return case_ne_229(s);
  case 230:
    return case_ne_230(s);
  case 231:
    return case_ne_231(s);
  case 232:
    return case_ne_232(s);
  case 233:
    return case_ne_233(s);
  case 234:
    return case_ne_234(s);
  case 235:
    return case_ne_235(s);
  case 236:
    return case_ne_236(s);
  case 237:
    return case_ne_237(s);
  case 238:
    return case_ne_238(s);
  case 239:
    return case_ne_239(s);
  case 240:
    return case_ne_240(s);
  case 241:
    return case_ne_241(s);
  case 242:
    return case_ne_242(s);
  case 243:
    return case_ne_243(s);
  case 244:
    return case_ne_244(s);
  case 245:
    return case_ne_245(s);
  case 246:
    return case_ne_246(s);
  case 247:
    return case_ne_247(s);
  case 248:
    return case_ne_248(s);
  case 249:
    return case_ne_249(s);
  case 250:
    return case_ne_250(s);
  case 251:
    return case_ne_251(s);
  case 252:
    return case_ne_252(s);
  case 253:
    return case_ne_253(s);
  case 254:
    return case_ne_254(s);
  case 255:
    return case_ne_255(s);
  case 256:
    return case_ne_256(s);
  case 257:
    return case_ne_257(s);
  case 258:
    return case_ne_258(s);
  case 259:
    return case_ne_259(s);
  case 260:
    return case_ne_260(s);
  case 261:
    return case_ne_261(s);
  case 262:
    return case_ne_262(s);
  case 263:
    return case_ne_263(s);
  case 264:
    return case_ne_264(s);
  case 265:
    return case_ne_265(s);
  case 266:
    return case_ne_266(s);
  case 267:
    return case_ne_267(s);
  case 268:
    return case_ne_268(s);
  case 269:
    return case_ne_269(s);
  case 270:
    return case_ne_270(s);
  case 271:
    return case_ne_271(s);
  case 272:
    return case_ne_272(s);
  case 273:
    return case_ne_273(s);
  case 274:
    return case_ne_274(s);
  case 275:
    return case_ne_275(s);
  case 276:
    return case_ne_276(s);
  case 277:
    return case_ne_277(s);
  case 278:
    return case_ne_278(s);
  case 279:
    return case_ne_279(s);
  case 280:
    return case_ne_280(s);
  case 281:
    return case_ne_281(s);
  case 282:
    return case_ne_282(s);
  case 283:
    return case_ne_283(s);
  default:
    throw std::out_of_range("unknown comparison case index");
  }
}
inline unsigned native_case_count(const std::string &operation)
{
  if (operation == "lt") return count_lt;
  if (operation == "le") return count_le;
  if (operation == "gt") return count_gt;
  if (operation == "ge") return count_ge;
  if (operation == "eq") return count_eq;
  if (operation == "ne") return count_ne;
  throw std::out_of_range("unknown comparison operation");
}
inline bool native_case(const std::string &operation, unsigned index, Sources &s)
{
  if (operation == "lt") return case_lt(index, s);
  if (operation == "le") return case_le(index, s);
  if (operation == "gt") return case_gt(index, s);
  if (operation == "ge") return case_ge(index, s);
  if (operation == "eq") return case_eq(index, s);
  if (operation == "ne") return case_ne(index, s);
  throw std::out_of_range("unknown comparison operation");
}
inline constexpr unsigned exact_overload_count = 1269;
}  // namespace sc_dt_comparisons_probe

/** @file sc_dt_integer_binary_probe.h
 * @brief Exact overload probes and native fixtures for SystemC binary operators.
 * @details Fixtures retain every source owner for each synchronous call. The
 *          concatenation reference borrows its integer parents and is consumed
 *          during the call. Native results are rendered before fixture teardown.
 *          No Python callbacks or references are retained by this probe.
 */
#pragma once
#ifndef SC_INCLUDE_FX
#define SC_INCLUDE_FX
#endif
#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>
namespace sc_dt_integer_binary_probe
{
using Bool = bool;
using Char = char;
using SignedInt = int;
using UnsignedInt = unsigned int;
using SignedLong = long;
using UnsignedLong = unsigned long;
using SignedScalar = sc_dt::int64;
using UnsignedScalar = sc_dt::uint64;
using LogicState = sc_dt::sc_logic_value_t;

/** @brief Native owners and borrowed views used by every exact signature case. */
struct Sources
{
  sc_dt::sc_signed signed_value;
  sc_dt::sc_unsigned unsigned_value;
  sc_dt::sc_int_base signed_fixed;
  sc_dt::sc_uint_base unsigned_fixed;
  sc_dt::sc_fix fixed_value;
  sc_dt::sc_fix_fast fixed_fast_value;
  sc_dt::sc_ufix ufixed_value;
  sc_dt::sc_ufix_fast ufixed_fast_value;
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

  /** @brief Construct distinguishable values with live parents and fixed widths. */
  Sources()
    : signed_value(19)
    , unsigned_value(21)
    , signed_fixed(12)
    , unsigned_fixed(15)
    , fixed_value(3.25, 8, 4)
    , fixed_fast_value(2.75, 8, 4)
    , ufixed_value(5.25, 8, 4)
    , ufixed_fast_value(6.5, 8, 4)
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
  {
    signed_value = -12345;
    unsigned_value = 0x1a55a;
    signed_fixed = -311;
    unsigned_fixed = 0x4a31;
    concat_high = 0x12f;
    concat_low = 0x53;
    const bool bool_values[8] = {true, false, true, true, false, false, true, false};
    const char char_values[9] = {'1', '0', '1', '1', '0', '0', '1', '0', '\0'};
    const char logic_values[8] = {'1', '0', 'Z', 'X', '0', '1', '0', '1'};
    for (int index = 0; index < 8; ++index)
    {
      bool_bits[index] = bool_values[index];
      char_bits[index] = char_values[index];
      logic_bits[index] = sc_dt::sc_logic(logic_values[index]);
    }
    char_bits[8] = '\0';
  }

  /** @brief Return a borrowed integer concatref while both source integers live. */
  const sc_dt::sc_concatref &concat() const
  {
    return concat_value;
  }
};

template <class Value, class = void>
struct has_binary_string : std::false_type
{
};
template <class Value>
struct has_binary_string<Value, std::void_t<decltype(std::declval<const Value &>().to_string(sc_dt::SC_BIN))>>
  : std::true_type
{
};
template <class Value, class = void>
struct has_length : std::false_type
{
};
template <class Value>
struct has_length<Value, std::void_t<decltype(std::declval<const Value &>().length())>> : std::true_type
{
};
template <class Value, class = void>
struct has_wl : std::false_type
{
};
template <class Value>
struct has_wl<Value, std::void_t<decltype(std::declval<const Value &>().wl())>> : std::true_type
{
};
template <class Value, class = void>
struct has_iwl : std::false_type
{
};
template <class Value>
struct has_iwl<Value, std::void_t<decltype(std::declval<const Value &>().iwl())>> : std::true_type
{
};
template <class Value, class = void>
struct has_char : std::false_type
{
};
template <class Value>
struct has_char<Value, std::void_t<decltype(std::declval<const Value &>().to_char())>> : std::true_type
{
};

/** @brief Render a result value and native width when the type exposes one. */
template <class Value>
std::string describe(const Value &value)
{
  if constexpr (has_binary_string<Value>::value)
  {
    std::string result;
    if constexpr (std::is_base_of_v<sc_dt::sc_lv_base, Value> || std::is_base_of_v<sc_dt::sc_bv_base, Value>)
      result = value.to_string();
    else
      result = value.to_string(sc_dt::SC_BIN);
    if constexpr (has_length<Value>::value)
      result += ":" + std::to_string(value.length());
    else if constexpr (has_wl<Value>::value && has_iwl<Value>::value)
      result += ":" + std::to_string(value.wl()) + ":" + std::to_string(value.iwl());
    return result;
  }
  else if constexpr (has_char<Value>::value)
    return std::string(1, value.to_char());
  else
  {
    std::ostringstream stream;
    stream << value;
    return stream.str();
  }
}

template <class Argument>
decltype(auto) operand(Sources &source)
{
  using Value = std::remove_cv_t<std::remove_reference_t<Argument>>;
#define BINARY_VALUE_ARG(Type, expression)        \
  else if constexpr (std::is_same_v<Value, Type>) \
  {                                               \
    return static_cast<Argument>(expression);     \
  }
  if constexpr (std::is_same_v<Value, bool>)
  {
    return static_cast<Argument>(source.bool_bits[0]);
  }
  BINARY_VALUE_ARG(char, source.char_bits[0])
  BINARY_VALUE_ARG(int, 1)
  BINARY_VALUE_ARG(unsigned int, 29U)
  BINARY_VALUE_ARG(long, 12345L)
  BINARY_VALUE_ARG(unsigned long, 65535UL)
  BINARY_VALUE_ARG(sc_dt::int64, static_cast<sc_dt::int64>(-123456789))
  BINARY_VALUE_ARG(sc_dt::uint64, static_cast<sc_dt::uint64>(0xfedcba9876543210ULL))
  BINARY_VALUE_ARG(sc_dt::sc_logic_value_t, source.logic_state)
  BINARY_VALUE_ARG(const bool *, source.bool_bits.data())
  BINARY_VALUE_ARG(const char *, source.char_bits.data())
  BINARY_VALUE_ARG(const sc_dt::sc_logic *, source.logic_bits.data())
  BINARY_VALUE_ARG(sc_dt::sc_signed, source.signed_value)
  BINARY_VALUE_ARG(sc_dt::sc_unsigned, source.unsigned_value)
  BINARY_VALUE_ARG(sc_dt::sc_int_base, source.signed_fixed)
  BINARY_VALUE_ARG(sc_dt::sc_uint_base, source.unsigned_fixed)
  BINARY_VALUE_ARG(sc_dt::sc_fix, source.fixed_value)
  BINARY_VALUE_ARG(sc_dt::sc_fix_fast, source.fixed_fast_value)
  BINARY_VALUE_ARG(sc_dt::sc_ufix, source.ufixed_value)
  BINARY_VALUE_ARG(sc_dt::sc_ufix_fast, source.ufixed_fast_value)
  BINARY_VALUE_ARG(sc_dt::sc_bit, source.bit_value)
  BINARY_VALUE_ARG(sc_dt::sc_logic, source.logic_value)
  BINARY_VALUE_ARG(sc_dt::sc_concatref, source.concat_value)
  BINARY_VALUE_ARG(sc_dt::sc_proxy<sc_dt::sc_bv_base>, source.bv)
  BINARY_VALUE_ARG(sc_dt::sc_proxy<sc_dt::sc_lv_base>, source.lv)
  BINARY_VALUE_ARG(sc_dt::sc_bitref_r<sc_dt::sc_bv_base>, source.bv_ref)
  BINARY_VALUE_ARG(sc_dt::sc_bitref_r<sc_dt::sc_lv_base>, source.lv_ref)
  BINARY_VALUE_ARG(sc_dt::sc_bv_base, source.bv)
  BINARY_VALUE_ARG(sc_dt::sc_lv_base, source.lv)
  else
  {
    static_assert(!sizeof(Argument), "binary probe has no operand factory for this signature");
  }
#undef BINARY_VALUE_ARG
}

template <class Function, class Left, class Right>
std::string invoke(Function function, Sources &source)
{
  auto result = function(operand<Left>(source), operand<Right>(source));
  return describe(result);
}
using Fn_mod_000 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_000>(&sc_dt::operator%)), Fn_mod_000>::value,
  "exact overload sc_dt::operator% const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
using Fn_mod_001 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_001>(&sc_dt::operator%)), Fn_mod_001>::value,
  "exact overload sc_dt::operator% const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
using Fn_mod_002 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, int);
static_assert(std::is_same<decltype(static_cast<Fn_mod_002>(&sc_dt::operator%)), Fn_mod_002>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_signed (const sc_dt::sc_concatref &, int)");
using Fn_mod_003 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, long);
static_assert(std::is_same<decltype(static_cast<Fn_mod_003>(&sc_dt::operator%)), Fn_mod_003>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_signed (const sc_dt::sc_concatref &, long)");
using Fn_mod_004 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_mod_004>(&sc_dt::operator%)), Fn_mod_004>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_signed (const sc_dt::sc_concatref &, sc_dt::int64)");
using Fn_mod_005 = const sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_005>(&sc_dt::operator%)), Fn_mod_005>::value,
  "exact overload sc_dt::operator% const sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
using Fn_mod_006 = const sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_006>(&sc_dt::operator%)), Fn_mod_006>::value,
  "exact overload sc_dt::operator% const sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
using Fn_mod_007 = const sc_dt::sc_signed (*)(int, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_007>(&sc_dt::operator%)), Fn_mod_007>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_signed (int, const sc_dt::sc_concatref &)");
using Fn_mod_008 = const sc_dt::sc_signed (*)(long, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_008>(&sc_dt::operator%)), Fn_mod_008>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_signed (long, const sc_dt::sc_concatref &)");
using Fn_mod_009 = const sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_009>(&sc_dt::operator%)), Fn_mod_009>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_concatref &)");
using Fn_mod_010 = const sc_dt::sc_unsigned (*)(bool, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_010>(&sc_dt::operator%)), Fn_mod_010>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_unsigned (bool, const sc_dt::sc_concatref &)");
using Fn_mod_011 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, bool);
static_assert(std::is_same<decltype(static_cast<Fn_mod_011>(&sc_dt::operator%)), Fn_mod_011>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, bool)");
using Fn_mod_012 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_012>(&sc_dt::operator%)), Fn_mod_012>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
              "sc_dt::sc_concatref &)");
using Fn_mod_013 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_013>(&sc_dt::operator%)), Fn_mod_013>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
              "sc_dt::sc_uint_base &)");
using Fn_mod_014 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_014>(&sc_dt::operator%)), Fn_mod_014>::value,
  "exact overload sc_dt::operator% const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &)");
using Fn_mod_015 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_mod_015>(&sc_dt::operator%)), Fn_mod_015>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, sc_dt::uint64)");
using Fn_mod_016 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_mod_016>(&sc_dt::operator%)), Fn_mod_016>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned int)");
using Fn_mod_017 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_mod_017>(&sc_dt::operator%)), Fn_mod_017>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned long)");
using Fn_mod_018 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_018>(&sc_dt::operator%)), Fn_mod_018>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const "
              "sc_dt::sc_concatref &)");
using Fn_mod_019 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_019>(&sc_dt::operator%)), Fn_mod_019>::value,
  "exact overload sc_dt::operator% const sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &)");
using Fn_mod_020 = const sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_020>(&sc_dt::operator%)), Fn_mod_020>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_concatref &)");
using Fn_mod_021 = const sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_021>(&sc_dt::operator%)), Fn_mod_021>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_concatref &)");
using Fn_mod_022 = const sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_022>(&sc_dt::operator%)), Fn_mod_022>::value,
              "exact overload sc_dt::operator% const sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_concatref &)");
using Fn_mod_023 = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_023>(&sc_dt::operator%)), Fn_mod_023>::value,
  "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
using Fn_mod_024 = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_024>(&sc_dt::operator%)), Fn_mod_024>::value,
  "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
using Fn_mod_025 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_025>(&sc_dt::operator%)), Fn_mod_025>::value,
  "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
using Fn_mod_026 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_026>(&sc_dt::operator%)), Fn_mod_026>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
using Fn_mod_027 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_027>(&sc_dt::operator%)), Fn_mod_027>::value,
  "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
using Fn_mod_028 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_028>(&sc_dt::operator%)), Fn_mod_028>::value,
  "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
using Fn_mod_029 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, int);
static_assert(std::is_same<decltype(static_cast<Fn_mod_029>(&sc_dt::operator%)), Fn_mod_029>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_signed &, int)");
using Fn_mod_030 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, long);
static_assert(std::is_same<decltype(static_cast<Fn_mod_030>(&sc_dt::operator%)), Fn_mod_030>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_signed &, long)");
using Fn_mod_031 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_mod_031>(&sc_dt::operator%)), Fn_mod_031>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::int64)");
using Fn_mod_032 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_mod_032>(&sc_dt::operator%)), Fn_mod_032>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::uint64)");
using Fn_mod_033 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_mod_033>(&sc_dt::operator%)), Fn_mod_033>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned int)");
using Fn_mod_034 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_mod_034>(&sc_dt::operator%)), Fn_mod_034>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned long)");
using Fn_mod_035 = sc_dt::sc_signed (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_035>(&sc_dt::operator%)), Fn_mod_035>::value,
  "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
using Fn_mod_036 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_036>(&sc_dt::operator%)), Fn_mod_036>::value,
  "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
using Fn_mod_037 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_037>(&sc_dt::operator%)), Fn_mod_037>::value,
  "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
using Fn_mod_038 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, int);
static_assert(std::is_same<decltype(static_cast<Fn_mod_038>(&sc_dt::operator%)), Fn_mod_038>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_unsigned &, int)");
using Fn_mod_039 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, long);
static_assert(std::is_same<decltype(static_cast<Fn_mod_039>(&sc_dt::operator%)), Fn_mod_039>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_unsigned &, long)");
using Fn_mod_040 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_mod_040>(&sc_dt::operator%)), Fn_mod_040>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (const sc_dt::sc_unsigned &, sc_dt::int64)");
using Fn_mod_041 = sc_dt::sc_signed (*)(int, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_041>(&sc_dt::operator%)), Fn_mod_041>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (int, const sc_dt::sc_signed &)");
using Fn_mod_042 = sc_dt::sc_signed (*)(int, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_042>(&sc_dt::operator%)), Fn_mod_042>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (int, const sc_dt::sc_unsigned &)");
using Fn_mod_043 = sc_dt::sc_signed (*)(long, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_043>(&sc_dt::operator%)), Fn_mod_043>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (long, const sc_dt::sc_signed &)");
using Fn_mod_044 = sc_dt::sc_signed (*)(long, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_044>(&sc_dt::operator%)), Fn_mod_044>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (long, const sc_dt::sc_unsigned &)");
using Fn_mod_045 = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_045>(&sc_dt::operator%)), Fn_mod_045>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_signed &)");
using Fn_mod_046 = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_046>(&sc_dt::operator%)), Fn_mod_046>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_unsigned &)");
using Fn_mod_047 = sc_dt::sc_signed (*)(sc_dt::uint64, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_047>(&sc_dt::operator%)), Fn_mod_047>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (sc_dt::uint64, const sc_dt::sc_signed &)");
using Fn_mod_048 = sc_dt::sc_signed (*)(unsigned int, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_048>(&sc_dt::operator%)), Fn_mod_048>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (unsigned int, const sc_dt::sc_signed &)");
using Fn_mod_049 = sc_dt::sc_signed (*)(unsigned long, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_049>(&sc_dt::operator%)), Fn_mod_049>::value,
              "exact overload sc_dt::operator% sc_dt::sc_signed (unsigned long, const sc_dt::sc_signed &)");
using Fn_mod_050 = sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_050>(&sc_dt::operator%)), Fn_mod_050>::value,
  "exact overload sc_dt::operator% sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
using Fn_mod_051 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_051>(&sc_dt::operator%)), Fn_mod_051>::value,
  "exact overload sc_dt::operator% sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
using Fn_mod_052 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_mod_052>(&sc_dt::operator%)), Fn_mod_052>::value,
  "exact overload sc_dt::operator% sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
using Fn_mod_053 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_mod_053>(&sc_dt::operator%)), Fn_mod_053>::value,
              "exact overload sc_dt::operator% sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, sc_dt::uint64)");
using Fn_mod_054 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_mod_054>(&sc_dt::operator%)), Fn_mod_054>::value,
              "exact overload sc_dt::operator% sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned int)");
using Fn_mod_055 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_mod_055>(&sc_dt::operator%)), Fn_mod_055>::value,
              "exact overload sc_dt::operator% sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned long)");
using Fn_mod_056 = sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_056>(&sc_dt::operator%)), Fn_mod_056>::value,
              "exact overload sc_dt::operator% sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_unsigned &)");
using Fn_mod_057 = sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_057>(&sc_dt::operator%)), Fn_mod_057>::value,
              "exact overload sc_dt::operator% sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_unsigned &)");
using Fn_mod_058 = sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_mod_058>(&sc_dt::operator%)), Fn_mod_058>::value,
              "exact overload sc_dt::operator% sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_unsigned &)");
using Fn_and_000 = const sc_dt::sc_bit (*)(bool, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_and_000>(&sc_dt::operator&)), Fn_and_000>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_bit (bool, const sc_dt::sc_bit &)");
using Fn_and_001 = const sc_dt::sc_bit (*)(char, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_and_001>(&sc_dt::operator&)), Fn_and_001>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_bit (char, const sc_dt::sc_bit &)");
using Fn_and_002 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, bool);
static_assert(std::is_same<decltype(static_cast<Fn_and_002>(&sc_dt::operator&)), Fn_and_002>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_bit (const sc_dt::sc_bit &, bool)");
using Fn_and_003 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, char);
static_assert(std::is_same<decltype(static_cast<Fn_and_003>(&sc_dt::operator&)), Fn_and_003>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_bit (const sc_dt::sc_bit &, char)");
using Fn_and_004 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_and_004>(&sc_dt::operator&)), Fn_and_004>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_bit (const sc_dt::sc_bit &, const sc_dt::sc_bit &)");
using Fn_and_005 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, int);
static_assert(std::is_same<decltype(static_cast<Fn_and_005>(&sc_dt::operator&)), Fn_and_005>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_bit (const sc_dt::sc_bit &, int)");
using Fn_and_006 = const sc_dt::sc_bit (*)(int, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_and_006>(&sc_dt::operator&)), Fn_and_006>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_bit (int, const sc_dt::sc_bit &)");
using Fn_and_007 = const sc_dt::sc_fix (*)(const sc_dt::sc_fix &, const sc_dt::sc_fix &);
static_assert(std::is_same<decltype(static_cast<Fn_and_007>(&sc_dt::operator&)), Fn_and_007>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_fix (const sc_dt::sc_fix &, const sc_dt::sc_fix &)");
using Fn_and_008 = const sc_dt::sc_fix (*)(const sc_dt::sc_fix &, const sc_dt::sc_fix_fast &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_008>(&sc_dt::operator&)), Fn_and_008>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_fix (const sc_dt::sc_fix &, const sc_dt::sc_fix_fast &)");
using Fn_and_009 = const sc_dt::sc_fix (*)(const sc_dt::sc_fix_fast &, const sc_dt::sc_fix &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_009>(&sc_dt::operator&)), Fn_and_009>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_fix (const sc_dt::sc_fix_fast &, const sc_dt::sc_fix &)");
using Fn_and_010 = const sc_dt::sc_fix_fast (*)(const sc_dt::sc_fix_fast &, const sc_dt::sc_fix_fast &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_010>(&sc_dt::operator&)), Fn_and_010>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_fix_fast (const sc_dt::sc_fix_fast &, const sc_dt::sc_fix_fast &)");
using Fn_and_011 = const sc_dt::sc_logic (*)(bool, const sc_dt::sc_logic &);
static_assert(std::is_same<decltype(static_cast<Fn_and_011>(&sc_dt::operator&)), Fn_and_011>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_logic (bool, const sc_dt::sc_logic &)");
using Fn_and_012 = const sc_dt::sc_logic (*)(char, const sc_dt::sc_logic &);
static_assert(std::is_same<decltype(static_cast<Fn_and_012>(&sc_dt::operator&)), Fn_and_012>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_logic (char, const sc_dt::sc_logic &)");
using Fn_and_013 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, bool);
static_assert(std::is_same<decltype(static_cast<Fn_and_013>(&sc_dt::operator&)), Fn_and_013>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_logic (const sc_dt::sc_logic &, bool)");
using Fn_and_014 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, char);
static_assert(std::is_same<decltype(static_cast<Fn_and_014>(&sc_dt::operator&)), Fn_and_014>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_logic (const sc_dt::sc_logic &, char)");
using Fn_and_015 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, const sc_dt::sc_logic &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_015>(&sc_dt::operator&)), Fn_and_015>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_logic (const sc_dt::sc_logic &, const sc_dt::sc_logic &)");
using Fn_and_016 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, int);
static_assert(std::is_same<decltype(static_cast<Fn_and_016>(&sc_dt::operator&)), Fn_and_016>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_logic (const sc_dt::sc_logic &, int)");
using Fn_and_017 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, sc_dt::sc_logic_value_t);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_017>(&sc_dt::operator&)), Fn_and_017>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_logic (const sc_dt::sc_logic &, sc_dt::sc_logic_value_t)");
using Fn_and_018 = const sc_dt::sc_logic (*)(int, const sc_dt::sc_logic &);
static_assert(std::is_same<decltype(static_cast<Fn_and_018>(&sc_dt::operator&)), Fn_and_018>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_logic (int, const sc_dt::sc_logic &)");
using Fn_and_019 = const sc_dt::sc_logic (*)(sc_dt::sc_logic_value_t, const sc_dt::sc_logic &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_019>(&sc_dt::operator&)), Fn_and_019>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_logic (sc_dt::sc_logic_value_t, const sc_dt::sc_logic &)");
using Fn_and_020 = const sc_dt::sc_lv_base (*)(const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_020>(&sc_dt::operator&)), Fn_and_020>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_lv_base (const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_021 = const sc_dt::sc_lv_base (*)(const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_021>(&sc_dt::operator&)), Fn_and_021>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_lv_base (const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_022 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_int_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_and_022>(&sc_dt::operator&)), Fn_and_022>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_lv_base (const sc_dt::sc_int_base &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_023 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_logic *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_and_023>(&sc_dt::operator&)), Fn_and_023>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_lv_base (const sc_dt::sc_logic *, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_024 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &,
                                               const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_and_024>(&sc_dt::operator&)), Fn_and_024>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_lv_base (const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, "
              "const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_025 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &,
                                               const sc_dt::sc_proxy<sc_dt::sc_lv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_and_025>(&sc_dt::operator&)), Fn_and_025>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_lv_base (const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, "
              "const sc_dt::sc_proxy<sc_dt::sc_lv_base> &)");
using Fn_and_026 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_signed &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_and_026>(&sc_dt::operator&)), Fn_and_026>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_lv_base (const sc_dt::sc_signed &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_027 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_and_027>(&sc_dt::operator&)), Fn_and_027>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_lv_base (const sc_dt::sc_uint_base &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_028 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_and_028>(&sc_dt::operator&)), Fn_and_028>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_lv_base (const sc_dt::sc_unsigned &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_029 = const sc_dt::sc_lv_base (*)(int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_029>(&sc_dt::operator&)), Fn_and_029>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_lv_base (int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_030 = const sc_dt::sc_lv_base (*)(long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_030>(&sc_dt::operator&)), Fn_and_030>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_lv_base (long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_031 = const sc_dt::sc_lv_base (*)(sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_031>(&sc_dt::operator&)), Fn_and_031>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_lv_base (sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_032 = const sc_dt::sc_lv_base (*)(sc_dt::uint64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_and_032>(&sc_dt::operator&)), Fn_and_032>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_lv_base (sc_dt::uint64, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_033 = const sc_dt::sc_lv_base (*)(unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_033>(&sc_dt::operator&)), Fn_and_033>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_lv_base (unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_034 = const sc_dt::sc_lv_base (*)(unsigned long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_and_034>(&sc_dt::operator&)), Fn_and_034>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_lv_base (unsigned long, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_and_035 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_035>(&sc_dt::operator&)), Fn_and_035>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
using Fn_and_036 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_036>(&sc_dt::operator&)), Fn_and_036>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
using Fn_and_037 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, int);
static_assert(std::is_same<decltype(static_cast<Fn_and_037>(&sc_dt::operator&)), Fn_and_037>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_signed (const sc_dt::sc_concatref &, int)");
using Fn_and_038 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, long);
static_assert(std::is_same<decltype(static_cast<Fn_and_038>(&sc_dt::operator&)), Fn_and_038>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_signed (const sc_dt::sc_concatref &, long)");
using Fn_and_039 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_and_039>(&sc_dt::operator&)), Fn_and_039>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_signed (const sc_dt::sc_concatref &, sc_dt::int64)");
using Fn_and_040 = const sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_040>(&sc_dt::operator&)), Fn_and_040>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
using Fn_and_041 = const sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_041>(&sc_dt::operator&)), Fn_and_041>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
using Fn_and_042 = const sc_dt::sc_signed (*)(int, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_and_042>(&sc_dt::operator&)), Fn_and_042>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_signed (int, const sc_dt::sc_concatref &)");
using Fn_and_043 = const sc_dt::sc_signed (*)(long, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_and_043>(&sc_dt::operator&)), Fn_and_043>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_signed (long, const sc_dt::sc_concatref &)");
using Fn_and_044 = const sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_and_044>(&sc_dt::operator&)), Fn_and_044>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_concatref &)");
using Fn_and_045 = const sc_dt::sc_ufix (*)(const sc_dt::sc_ufix &, const sc_dt::sc_ufix &);
static_assert(std::is_same<decltype(static_cast<Fn_and_045>(&sc_dt::operator&)), Fn_and_045>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_ufix (const sc_dt::sc_ufix &, const sc_dt::sc_ufix &)");
using Fn_and_046 = const sc_dt::sc_ufix (*)(const sc_dt::sc_ufix &, const sc_dt::sc_ufix_fast &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_046>(&sc_dt::operator&)), Fn_and_046>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_ufix (const sc_dt::sc_ufix &, const sc_dt::sc_ufix_fast &)");
using Fn_and_047 = const sc_dt::sc_ufix (*)(const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_047>(&sc_dt::operator&)), Fn_and_047>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_ufix (const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix &)");
using Fn_and_048 = const sc_dt::sc_ufix_fast (*)(const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix_fast &);
static_assert(std::is_same<decltype(static_cast<Fn_and_048>(&sc_dt::operator&)), Fn_and_048>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_ufix_fast (const sc_dt::sc_ufix_fast &, const "
              "sc_dt::sc_ufix_fast &)");
using Fn_and_049 = const sc_dt::sc_unsigned (*)(bool, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_and_049>(&sc_dt::operator&)), Fn_and_049>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_unsigned (bool, const sc_dt::sc_concatref &)");
using Fn_and_050 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, bool);
static_assert(std::is_same<decltype(static_cast<Fn_and_050>(&sc_dt::operator&)), Fn_and_050>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, bool)");
using Fn_and_051 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_and_051>(&sc_dt::operator&)), Fn_and_051>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
              "sc_dt::sc_concatref &)");
using Fn_and_052 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
static_assert(std::is_same<decltype(static_cast<Fn_and_052>(&sc_dt::operator&)), Fn_and_052>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
              "sc_dt::sc_uint_base &)");
using Fn_and_053 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_053>(&sc_dt::operator&)), Fn_and_053>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &)");
using Fn_and_054 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_and_054>(&sc_dt::operator&)), Fn_and_054>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, sc_dt::uint64)");
using Fn_and_055 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_and_055>(&sc_dt::operator&)), Fn_and_055>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned int)");
using Fn_and_056 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_and_056>(&sc_dt::operator&)), Fn_and_056>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned long)");
using Fn_and_057 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_and_057>(&sc_dt::operator&)), Fn_and_057>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const "
              "sc_dt::sc_concatref &)");
using Fn_and_058 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_058>(&sc_dt::operator&)), Fn_and_058>::value,
  "exact overload sc_dt::operator& const sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &)");
using Fn_and_059 = const sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_and_059>(&sc_dt::operator&)), Fn_and_059>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_concatref &)");
using Fn_and_060 = const sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_and_060>(&sc_dt::operator&)), Fn_and_060>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_concatref &)");
using Fn_and_061 = const sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_and_061>(&sc_dt::operator&)), Fn_and_061>::value,
              "exact overload sc_dt::operator& const sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_concatref &)");
using Fn_and_062 = sc_dt::sc_logic (*)(const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &,
                                       const sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_and_062>(&sc_dt::operator&)), Fn_and_062>::value,
              "exact overload sc_dt::operator& sc_dt::sc_logic (const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &, const "
              "sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &)");
using Fn_and_063 = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_063>(&sc_dt::operator&)), Fn_and_063>::value,
  "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
using Fn_and_064 = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_064>(&sc_dt::operator&)), Fn_and_064>::value,
  "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
using Fn_and_065 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_065>(&sc_dt::operator&)), Fn_and_065>::value,
  "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
using Fn_and_066 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_and_066>(&sc_dt::operator&)), Fn_and_066>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
using Fn_and_067 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_067>(&sc_dt::operator&)), Fn_and_067>::value,
  "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
using Fn_and_068 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_068>(&sc_dt::operator&)), Fn_and_068>::value,
  "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
using Fn_and_069 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, int);
static_assert(std::is_same<decltype(static_cast<Fn_and_069>(&sc_dt::operator&)), Fn_and_069>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_signed &, int)");
using Fn_and_070 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, long);
static_assert(std::is_same<decltype(static_cast<Fn_and_070>(&sc_dt::operator&)), Fn_and_070>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_signed &, long)");
using Fn_and_071 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_and_071>(&sc_dt::operator&)), Fn_and_071>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::int64)");
using Fn_and_072 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_and_072>(&sc_dt::operator&)), Fn_and_072>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::uint64)");
using Fn_and_073 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_and_073>(&sc_dt::operator&)), Fn_and_073>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned int)");
using Fn_and_074 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_and_074>(&sc_dt::operator&)), Fn_and_074>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned long)");
using Fn_and_075 = sc_dt::sc_signed (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_075>(&sc_dt::operator&)), Fn_and_075>::value,
  "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
using Fn_and_076 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_076>(&sc_dt::operator&)), Fn_and_076>::value,
  "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
using Fn_and_077 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_077>(&sc_dt::operator&)), Fn_and_077>::value,
  "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
using Fn_and_078 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, int);
static_assert(std::is_same<decltype(static_cast<Fn_and_078>(&sc_dt::operator&)), Fn_and_078>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_unsigned &, int)");
using Fn_and_079 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, long);
static_assert(std::is_same<decltype(static_cast<Fn_and_079>(&sc_dt::operator&)), Fn_and_079>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_unsigned &, long)");
using Fn_and_080 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_and_080>(&sc_dt::operator&)), Fn_and_080>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (const sc_dt::sc_unsigned &, sc_dt::int64)");
using Fn_and_081 = sc_dt::sc_signed (*)(int, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_and_081>(&sc_dt::operator&)), Fn_and_081>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (int, const sc_dt::sc_signed &)");
using Fn_and_082 = sc_dt::sc_signed (*)(int, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_and_082>(&sc_dt::operator&)), Fn_and_082>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (int, const sc_dt::sc_unsigned &)");
using Fn_and_083 = sc_dt::sc_signed (*)(long, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_and_083>(&sc_dt::operator&)), Fn_and_083>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (long, const sc_dt::sc_signed &)");
using Fn_and_084 = sc_dt::sc_signed (*)(long, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_and_084>(&sc_dt::operator&)), Fn_and_084>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (long, const sc_dt::sc_unsigned &)");
using Fn_and_085 = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_and_085>(&sc_dt::operator&)), Fn_and_085>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_signed &)");
using Fn_and_086 = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_and_086>(&sc_dt::operator&)), Fn_and_086>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_unsigned &)");
using Fn_and_087 = sc_dt::sc_signed (*)(sc_dt::uint64, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_and_087>(&sc_dt::operator&)), Fn_and_087>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (sc_dt::uint64, const sc_dt::sc_signed &)");
using Fn_and_088 = sc_dt::sc_signed (*)(unsigned int, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_and_088>(&sc_dt::operator&)), Fn_and_088>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (unsigned int, const sc_dt::sc_signed &)");
using Fn_and_089 = sc_dt::sc_signed (*)(unsigned long, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_and_089>(&sc_dt::operator&)), Fn_and_089>::value,
              "exact overload sc_dt::operator& sc_dt::sc_signed (unsigned long, const sc_dt::sc_signed &)");
using Fn_and_090 = sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_090>(&sc_dt::operator&)), Fn_and_090>::value,
  "exact overload sc_dt::operator& sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
using Fn_and_091 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_091>(&sc_dt::operator&)), Fn_and_091>::value,
  "exact overload sc_dt::operator& sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
using Fn_and_092 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_and_092>(&sc_dt::operator&)), Fn_and_092>::value,
  "exact overload sc_dt::operator& sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
using Fn_and_093 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_and_093>(&sc_dt::operator&)), Fn_and_093>::value,
              "exact overload sc_dt::operator& sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, sc_dt::uint64)");
using Fn_and_094 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_and_094>(&sc_dt::operator&)), Fn_and_094>::value,
              "exact overload sc_dt::operator& sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned int)");
using Fn_and_095 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_and_095>(&sc_dt::operator&)), Fn_and_095>::value,
              "exact overload sc_dt::operator& sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned long)");
using Fn_and_096 = sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_and_096>(&sc_dt::operator&)), Fn_and_096>::value,
              "exact overload sc_dt::operator& sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_unsigned &)");
using Fn_and_097 = sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_and_097>(&sc_dt::operator&)), Fn_and_097>::value,
              "exact overload sc_dt::operator& sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_unsigned &)");
using Fn_and_098 = sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_and_098>(&sc_dt::operator&)), Fn_and_098>::value,
              "exact overload sc_dt::operator& sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_unsigned &)");
using Fn_or_000 = const sc_dt::sc_bit (*)(bool, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_or_000>(&sc_dt::operator|)), Fn_or_000>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_bit (bool, const sc_dt::sc_bit &)");
using Fn_or_001 = const sc_dt::sc_bit (*)(char, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_or_001>(&sc_dt::operator|)), Fn_or_001>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_bit (char, const sc_dt::sc_bit &)");
using Fn_or_002 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, bool);
static_assert(std::is_same<decltype(static_cast<Fn_or_002>(&sc_dt::operator|)), Fn_or_002>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_bit (const sc_dt::sc_bit &, bool)");
using Fn_or_003 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, char);
static_assert(std::is_same<decltype(static_cast<Fn_or_003>(&sc_dt::operator|)), Fn_or_003>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_bit (const sc_dt::sc_bit &, char)");
using Fn_or_004 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_or_004>(&sc_dt::operator|)), Fn_or_004>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_bit (const sc_dt::sc_bit &, const sc_dt::sc_bit &)");
using Fn_or_005 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, int);
static_assert(std::is_same<decltype(static_cast<Fn_or_005>(&sc_dt::operator|)), Fn_or_005>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_bit (const sc_dt::sc_bit &, int)");
using Fn_or_006 = const sc_dt::sc_bit (*)(int, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_or_006>(&sc_dt::operator|)), Fn_or_006>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_bit (int, const sc_dt::sc_bit &)");
using Fn_or_007 = const sc_dt::sc_fix (*)(const sc_dt::sc_fix &, const sc_dt::sc_fix &);
static_assert(std::is_same<decltype(static_cast<Fn_or_007>(&sc_dt::operator|)), Fn_or_007>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_fix (const sc_dt::sc_fix &, const sc_dt::sc_fix &)");
using Fn_or_008 = const sc_dt::sc_fix (*)(const sc_dt::sc_fix &, const sc_dt::sc_fix_fast &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_008>(&sc_dt::operator|)), Fn_or_008>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_fix (const sc_dt::sc_fix &, const sc_dt::sc_fix_fast &)");
using Fn_or_009 = const sc_dt::sc_fix (*)(const sc_dt::sc_fix_fast &, const sc_dt::sc_fix &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_009>(&sc_dt::operator|)), Fn_or_009>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_fix (const sc_dt::sc_fix_fast &, const sc_dt::sc_fix &)");
using Fn_or_010 = const sc_dt::sc_fix_fast (*)(const sc_dt::sc_fix_fast &, const sc_dt::sc_fix_fast &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_010>(&sc_dt::operator|)), Fn_or_010>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_fix_fast (const sc_dt::sc_fix_fast &, const sc_dt::sc_fix_fast &)");
using Fn_or_011 = const sc_dt::sc_logic (*)(bool, const sc_dt::sc_logic &);
static_assert(std::is_same<decltype(static_cast<Fn_or_011>(&sc_dt::operator|)), Fn_or_011>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_logic (bool, const sc_dt::sc_logic &)");
using Fn_or_012 = const sc_dt::sc_logic (*)(char, const sc_dt::sc_logic &);
static_assert(std::is_same<decltype(static_cast<Fn_or_012>(&sc_dt::operator|)), Fn_or_012>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_logic (char, const sc_dt::sc_logic &)");
using Fn_or_013 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, bool);
static_assert(std::is_same<decltype(static_cast<Fn_or_013>(&sc_dt::operator|)), Fn_or_013>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_logic (const sc_dt::sc_logic &, bool)");
using Fn_or_014 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, char);
static_assert(std::is_same<decltype(static_cast<Fn_or_014>(&sc_dt::operator|)), Fn_or_014>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_logic (const sc_dt::sc_logic &, char)");
using Fn_or_015 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, const sc_dt::sc_logic &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_015>(&sc_dt::operator|)), Fn_or_015>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_logic (const sc_dt::sc_logic &, const sc_dt::sc_logic &)");
using Fn_or_016 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, int);
static_assert(std::is_same<decltype(static_cast<Fn_or_016>(&sc_dt::operator|)), Fn_or_016>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_logic (const sc_dt::sc_logic &, int)");
using Fn_or_017 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, sc_dt::sc_logic_value_t);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_017>(&sc_dt::operator|)), Fn_or_017>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_logic (const sc_dt::sc_logic &, sc_dt::sc_logic_value_t)");
using Fn_or_018 = const sc_dt::sc_logic (*)(int, const sc_dt::sc_logic &);
static_assert(std::is_same<decltype(static_cast<Fn_or_018>(&sc_dt::operator|)), Fn_or_018>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_logic (int, const sc_dt::sc_logic &)");
using Fn_or_019 = const sc_dt::sc_logic (*)(sc_dt::sc_logic_value_t, const sc_dt::sc_logic &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_019>(&sc_dt::operator|)), Fn_or_019>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_logic (sc_dt::sc_logic_value_t, const sc_dt::sc_logic &)");
using Fn_or_020 = const sc_dt::sc_lv_base (*)(const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_020>(&sc_dt::operator|)), Fn_or_020>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_lv_base (const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_021 = const sc_dt::sc_lv_base (*)(const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_021>(&sc_dt::operator|)), Fn_or_021>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_lv_base (const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_022 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_int_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_or_022>(&sc_dt::operator|)), Fn_or_022>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_lv_base (const sc_dt::sc_int_base &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_023 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_logic *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_or_023>(&sc_dt::operator|)), Fn_or_023>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_lv_base (const sc_dt::sc_logic *, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_024 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &,
                                              const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_or_024>(&sc_dt::operator|)), Fn_or_024>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_lv_base (const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, "
              "const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_025 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &,
                                              const sc_dt::sc_proxy<sc_dt::sc_lv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_or_025>(&sc_dt::operator|)), Fn_or_025>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_lv_base (const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, "
              "const sc_dt::sc_proxy<sc_dt::sc_lv_base> &)");
using Fn_or_026 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_signed &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_or_026>(&sc_dt::operator|)), Fn_or_026>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_lv_base (const sc_dt::sc_signed &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_027 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_or_027>(&sc_dt::operator|)), Fn_or_027>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_lv_base (const sc_dt::sc_uint_base &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_028 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_or_028>(&sc_dt::operator|)), Fn_or_028>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_lv_base (const sc_dt::sc_unsigned &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_029 = const sc_dt::sc_lv_base (*)(int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_029>(&sc_dt::operator|)), Fn_or_029>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_lv_base (int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_030 = const sc_dt::sc_lv_base (*)(long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_030>(&sc_dt::operator|)), Fn_or_030>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_lv_base (long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_031 = const sc_dt::sc_lv_base (*)(sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_031>(&sc_dt::operator|)), Fn_or_031>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_lv_base (sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_032 = const sc_dt::sc_lv_base (*)(sc_dt::uint64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_or_032>(&sc_dt::operator|)), Fn_or_032>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_lv_base (sc_dt::uint64, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_033 = const sc_dt::sc_lv_base (*)(unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_033>(&sc_dt::operator|)), Fn_or_033>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_lv_base (unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_034 = const sc_dt::sc_lv_base (*)(unsigned long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_or_034>(&sc_dt::operator|)), Fn_or_034>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_lv_base (unsigned long, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_or_035 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_035>(&sc_dt::operator|)), Fn_or_035>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
using Fn_or_036 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_036>(&sc_dt::operator|)), Fn_or_036>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
using Fn_or_037 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, int);
static_assert(std::is_same<decltype(static_cast<Fn_or_037>(&sc_dt::operator|)), Fn_or_037>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_signed (const sc_dt::sc_concatref &, int)");
using Fn_or_038 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, long);
static_assert(std::is_same<decltype(static_cast<Fn_or_038>(&sc_dt::operator|)), Fn_or_038>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_signed (const sc_dt::sc_concatref &, long)");
using Fn_or_039 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_or_039>(&sc_dt::operator|)), Fn_or_039>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_signed (const sc_dt::sc_concatref &, sc_dt::int64)");
using Fn_or_040 = const sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_040>(&sc_dt::operator|)), Fn_or_040>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
using Fn_or_041 = const sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_041>(&sc_dt::operator|)), Fn_or_041>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
using Fn_or_042 = const sc_dt::sc_signed (*)(int, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_or_042>(&sc_dt::operator|)), Fn_or_042>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_signed (int, const sc_dt::sc_concatref &)");
using Fn_or_043 = const sc_dt::sc_signed (*)(long, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_or_043>(&sc_dt::operator|)), Fn_or_043>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_signed (long, const sc_dt::sc_concatref &)");
using Fn_or_044 = const sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_or_044>(&sc_dt::operator|)), Fn_or_044>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_concatref &)");
using Fn_or_045 = const sc_dt::sc_ufix (*)(const sc_dt::sc_ufix &, const sc_dt::sc_ufix &);
static_assert(std::is_same<decltype(static_cast<Fn_or_045>(&sc_dt::operator|)), Fn_or_045>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_ufix (const sc_dt::sc_ufix &, const sc_dt::sc_ufix &)");
using Fn_or_046 = const sc_dt::sc_ufix (*)(const sc_dt::sc_ufix &, const sc_dt::sc_ufix_fast &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_046>(&sc_dt::operator|)), Fn_or_046>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_ufix (const sc_dt::sc_ufix &, const sc_dt::sc_ufix_fast &)");
using Fn_or_047 = const sc_dt::sc_ufix (*)(const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_047>(&sc_dt::operator|)), Fn_or_047>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_ufix (const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix &)");
using Fn_or_048 = const sc_dt::sc_ufix_fast (*)(const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix_fast &);
static_assert(std::is_same<decltype(static_cast<Fn_or_048>(&sc_dt::operator|)), Fn_or_048>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_ufix_fast (const sc_dt::sc_ufix_fast &, const "
              "sc_dt::sc_ufix_fast &)");
using Fn_or_049 = const sc_dt::sc_unsigned (*)(bool, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_or_049>(&sc_dt::operator|)), Fn_or_049>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_unsigned (bool, const sc_dt::sc_concatref &)");
using Fn_or_050 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, bool);
static_assert(std::is_same<decltype(static_cast<Fn_or_050>(&sc_dt::operator|)), Fn_or_050>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, bool)");
using Fn_or_051 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_or_051>(&sc_dt::operator|)), Fn_or_051>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
              "sc_dt::sc_concatref &)");
using Fn_or_052 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
static_assert(std::is_same<decltype(static_cast<Fn_or_052>(&sc_dt::operator|)), Fn_or_052>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
              "sc_dt::sc_uint_base &)");
using Fn_or_053 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_053>(&sc_dt::operator|)), Fn_or_053>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &)");
using Fn_or_054 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_or_054>(&sc_dt::operator|)), Fn_or_054>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, sc_dt::uint64)");
using Fn_or_055 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_or_055>(&sc_dt::operator|)), Fn_or_055>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned int)");
using Fn_or_056 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_or_056>(&sc_dt::operator|)), Fn_or_056>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned long)");
using Fn_or_057 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_or_057>(&sc_dt::operator|)), Fn_or_057>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const "
              "sc_dt::sc_concatref &)");
using Fn_or_058 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_058>(&sc_dt::operator|)), Fn_or_058>::value,
  "exact overload sc_dt::operator| const sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &)");
using Fn_or_059 = const sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_or_059>(&sc_dt::operator|)), Fn_or_059>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_concatref &)");
using Fn_or_060 = const sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_or_060>(&sc_dt::operator|)), Fn_or_060>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_concatref &)");
using Fn_or_061 = const sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_or_061>(&sc_dt::operator|)), Fn_or_061>::value,
              "exact overload sc_dt::operator| const sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_concatref &)");
using Fn_or_062 = sc_dt::sc_logic (*)(const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &,
                                      const sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_or_062>(&sc_dt::operator|)), Fn_or_062>::value,
              "exact overload sc_dt::operator| sc_dt::sc_logic (const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &, const "
              "sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &)");
using Fn_or_063 = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_063>(&sc_dt::operator|)), Fn_or_063>::value,
  "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
using Fn_or_064 = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_064>(&sc_dt::operator|)), Fn_or_064>::value,
  "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
using Fn_or_065 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_065>(&sc_dt::operator|)), Fn_or_065>::value,
  "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
using Fn_or_066 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_or_066>(&sc_dt::operator|)), Fn_or_066>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
using Fn_or_067 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_067>(&sc_dt::operator|)), Fn_or_067>::value,
  "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
using Fn_or_068 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_068>(&sc_dt::operator|)), Fn_or_068>::value,
  "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
using Fn_or_069 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, int);
static_assert(std::is_same<decltype(static_cast<Fn_or_069>(&sc_dt::operator|)), Fn_or_069>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_signed &, int)");
using Fn_or_070 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, long);
static_assert(std::is_same<decltype(static_cast<Fn_or_070>(&sc_dt::operator|)), Fn_or_070>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_signed &, long)");
using Fn_or_071 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_or_071>(&sc_dt::operator|)), Fn_or_071>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::int64)");
using Fn_or_072 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_or_072>(&sc_dt::operator|)), Fn_or_072>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::uint64)");
using Fn_or_073 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_or_073>(&sc_dt::operator|)), Fn_or_073>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned int)");
using Fn_or_074 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_or_074>(&sc_dt::operator|)), Fn_or_074>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned long)");
using Fn_or_075 = sc_dt::sc_signed (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_075>(&sc_dt::operator|)), Fn_or_075>::value,
  "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
using Fn_or_076 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_076>(&sc_dt::operator|)), Fn_or_076>::value,
  "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
using Fn_or_077 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_077>(&sc_dt::operator|)), Fn_or_077>::value,
  "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
using Fn_or_078 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, int);
static_assert(std::is_same<decltype(static_cast<Fn_or_078>(&sc_dt::operator|)), Fn_or_078>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_unsigned &, int)");
using Fn_or_079 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, long);
static_assert(std::is_same<decltype(static_cast<Fn_or_079>(&sc_dt::operator|)), Fn_or_079>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_unsigned &, long)");
using Fn_or_080 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_or_080>(&sc_dt::operator|)), Fn_or_080>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (const sc_dt::sc_unsigned &, sc_dt::int64)");
using Fn_or_081 = sc_dt::sc_signed (*)(int, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_or_081>(&sc_dt::operator|)), Fn_or_081>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (int, const sc_dt::sc_signed &)");
using Fn_or_082 = sc_dt::sc_signed (*)(int, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_or_082>(&sc_dt::operator|)), Fn_or_082>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (int, const sc_dt::sc_unsigned &)");
using Fn_or_083 = sc_dt::sc_signed (*)(long, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_or_083>(&sc_dt::operator|)), Fn_or_083>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (long, const sc_dt::sc_signed &)");
using Fn_or_084 = sc_dt::sc_signed (*)(long, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_or_084>(&sc_dt::operator|)), Fn_or_084>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (long, const sc_dt::sc_unsigned &)");
using Fn_or_085 = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_or_085>(&sc_dt::operator|)), Fn_or_085>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_signed &)");
using Fn_or_086 = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_or_086>(&sc_dt::operator|)), Fn_or_086>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_unsigned &)");
using Fn_or_087 = sc_dt::sc_signed (*)(sc_dt::uint64, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_or_087>(&sc_dt::operator|)), Fn_or_087>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (sc_dt::uint64, const sc_dt::sc_signed &)");
using Fn_or_088 = sc_dt::sc_signed (*)(unsigned int, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_or_088>(&sc_dt::operator|)), Fn_or_088>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (unsigned int, const sc_dt::sc_signed &)");
using Fn_or_089 = sc_dt::sc_signed (*)(unsigned long, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_or_089>(&sc_dt::operator|)), Fn_or_089>::value,
              "exact overload sc_dt::operator| sc_dt::sc_signed (unsigned long, const sc_dt::sc_signed &)");
using Fn_or_090 = sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_090>(&sc_dt::operator|)), Fn_or_090>::value,
  "exact overload sc_dt::operator| sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
using Fn_or_091 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_091>(&sc_dt::operator|)), Fn_or_091>::value,
  "exact overload sc_dt::operator| sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
using Fn_or_092 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_or_092>(&sc_dt::operator|)), Fn_or_092>::value,
  "exact overload sc_dt::operator| sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
using Fn_or_093 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_or_093>(&sc_dt::operator|)), Fn_or_093>::value,
              "exact overload sc_dt::operator| sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, sc_dt::uint64)");
using Fn_or_094 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_or_094>(&sc_dt::operator|)), Fn_or_094>::value,
              "exact overload sc_dt::operator| sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned int)");
using Fn_or_095 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_or_095>(&sc_dt::operator|)), Fn_or_095>::value,
              "exact overload sc_dt::operator| sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned long)");
using Fn_or_096 = sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_or_096>(&sc_dt::operator|)), Fn_or_096>::value,
              "exact overload sc_dt::operator| sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_unsigned &)");
using Fn_or_097 = sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_or_097>(&sc_dt::operator|)), Fn_or_097>::value,
              "exact overload sc_dt::operator| sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_unsigned &)");
using Fn_or_098 = sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_or_098>(&sc_dt::operator|)), Fn_or_098>::value,
              "exact overload sc_dt::operator| sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_unsigned &)");
using Fn_xor_000 = const sc_dt::sc_bit (*)(bool, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_000>(&sc_dt::operator^)), Fn_xor_000>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_bit (bool, const sc_dt::sc_bit &)");
using Fn_xor_001 = const sc_dt::sc_bit (*)(char, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_001>(&sc_dt::operator^)), Fn_xor_001>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_bit (char, const sc_dt::sc_bit &)");
using Fn_xor_002 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, bool);
static_assert(std::is_same<decltype(static_cast<Fn_xor_002>(&sc_dt::operator^)), Fn_xor_002>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_bit (const sc_dt::sc_bit &, bool)");
using Fn_xor_003 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, char);
static_assert(std::is_same<decltype(static_cast<Fn_xor_003>(&sc_dt::operator^)), Fn_xor_003>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_bit (const sc_dt::sc_bit &, char)");
using Fn_xor_004 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_004>(&sc_dt::operator^)), Fn_xor_004>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_bit (const sc_dt::sc_bit &, const sc_dt::sc_bit &)");
using Fn_xor_005 = const sc_dt::sc_bit (*)(const sc_dt::sc_bit &, int);
static_assert(std::is_same<decltype(static_cast<Fn_xor_005>(&sc_dt::operator^)), Fn_xor_005>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_bit (const sc_dt::sc_bit &, int)");
using Fn_xor_006 = const sc_dt::sc_bit (*)(int, const sc_dt::sc_bit &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_006>(&sc_dt::operator^)), Fn_xor_006>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_bit (int, const sc_dt::sc_bit &)");
using Fn_xor_007 = const sc_dt::sc_fix (*)(const sc_dt::sc_fix &, const sc_dt::sc_fix &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_007>(&sc_dt::operator^)), Fn_xor_007>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_fix (const sc_dt::sc_fix &, const sc_dt::sc_fix &)");
using Fn_xor_008 = const sc_dt::sc_fix (*)(const sc_dt::sc_fix &, const sc_dt::sc_fix_fast &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_008>(&sc_dt::operator^)), Fn_xor_008>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_fix (const sc_dt::sc_fix &, const sc_dt::sc_fix_fast &)");
using Fn_xor_009 = const sc_dt::sc_fix (*)(const sc_dt::sc_fix_fast &, const sc_dt::sc_fix &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_009>(&sc_dt::operator^)), Fn_xor_009>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_fix (const sc_dt::sc_fix_fast &, const sc_dt::sc_fix &)");
using Fn_xor_010 = const sc_dt::sc_fix_fast (*)(const sc_dt::sc_fix_fast &, const sc_dt::sc_fix_fast &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_010>(&sc_dt::operator^)), Fn_xor_010>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_fix_fast (const sc_dt::sc_fix_fast &, const sc_dt::sc_fix_fast &)");
using Fn_xor_011 = const sc_dt::sc_logic (*)(bool, const sc_dt::sc_logic &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_011>(&sc_dt::operator^)), Fn_xor_011>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_logic (bool, const sc_dt::sc_logic &)");
using Fn_xor_012 = const sc_dt::sc_logic (*)(char, const sc_dt::sc_logic &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_012>(&sc_dt::operator^)), Fn_xor_012>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_logic (char, const sc_dt::sc_logic &)");
using Fn_xor_013 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, bool);
static_assert(std::is_same<decltype(static_cast<Fn_xor_013>(&sc_dt::operator^)), Fn_xor_013>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_logic (const sc_dt::sc_logic &, bool)");
using Fn_xor_014 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, char);
static_assert(std::is_same<decltype(static_cast<Fn_xor_014>(&sc_dt::operator^)), Fn_xor_014>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_logic (const sc_dt::sc_logic &, char)");
using Fn_xor_015 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, const sc_dt::sc_logic &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_015>(&sc_dt::operator^)), Fn_xor_015>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_logic (const sc_dt::sc_logic &, const sc_dt::sc_logic &)");
using Fn_xor_016 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, int);
static_assert(std::is_same<decltype(static_cast<Fn_xor_016>(&sc_dt::operator^)), Fn_xor_016>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_logic (const sc_dt::sc_logic &, int)");
using Fn_xor_017 = const sc_dt::sc_logic (*)(const sc_dt::sc_logic &, sc_dt::sc_logic_value_t);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_017>(&sc_dt::operator^)), Fn_xor_017>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_logic (const sc_dt::sc_logic &, sc_dt::sc_logic_value_t)");
using Fn_xor_018 = const sc_dt::sc_logic (*)(int, const sc_dt::sc_logic &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_018>(&sc_dt::operator^)), Fn_xor_018>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_logic (int, const sc_dt::sc_logic &)");
using Fn_xor_019 = const sc_dt::sc_logic (*)(sc_dt::sc_logic_value_t, const sc_dt::sc_logic &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_019>(&sc_dt::operator^)), Fn_xor_019>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_logic (sc_dt::sc_logic_value_t, const sc_dt::sc_logic &)");
using Fn_xor_020 = const sc_dt::sc_lv_base (*)(const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_020>(&sc_dt::operator^)), Fn_xor_020>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_021 = const sc_dt::sc_lv_base (*)(const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_021>(&sc_dt::operator^)), Fn_xor_021>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_022 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_int_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_022>(&sc_dt::operator^)), Fn_xor_022>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (const sc_dt::sc_int_base &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_023 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_logic *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_023>(&sc_dt::operator^)), Fn_xor_023>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (const sc_dt::sc_logic *, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_024 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &,
                                               const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_024>(&sc_dt::operator^)), Fn_xor_024>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, "
              "const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_025 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_proxy<sc_dt::sc_bv_base> &,
                                               const sc_dt::sc_proxy<sc_dt::sc_lv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_025>(&sc_dt::operator^)), Fn_xor_025>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, "
              "const sc_dt::sc_proxy<sc_dt::sc_lv_base> &)");
using Fn_xor_026 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_signed &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_026>(&sc_dt::operator^)), Fn_xor_026>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (const sc_dt::sc_signed &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_027 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_027>(&sc_dt::operator^)), Fn_xor_027>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (const sc_dt::sc_uint_base &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_028 = const sc_dt::sc_lv_base (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_028>(&sc_dt::operator^)), Fn_xor_028>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (const sc_dt::sc_unsigned &, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_029 = const sc_dt::sc_lv_base (*)(int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_029>(&sc_dt::operator^)), Fn_xor_029>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_030 = const sc_dt::sc_lv_base (*)(long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_030>(&sc_dt::operator^)), Fn_xor_030>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_031 = const sc_dt::sc_lv_base (*)(sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_031>(&sc_dt::operator^)), Fn_xor_031>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_032 = const sc_dt::sc_lv_base (*)(sc_dt::uint64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_032>(&sc_dt::operator^)), Fn_xor_032>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (sc_dt::uint64, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_033 = const sc_dt::sc_lv_base (*)(unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_033>(&sc_dt::operator^)), Fn_xor_033>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_034 = const sc_dt::sc_lv_base (*)(unsigned long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_034>(&sc_dt::operator^)), Fn_xor_034>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_lv_base (unsigned long, const "
              "sc_dt::sc_proxy<sc_dt::sc_bv_base> &)");
using Fn_xor_035 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_035>(&sc_dt::operator^)), Fn_xor_035>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
using Fn_xor_036 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_036>(&sc_dt::operator^)), Fn_xor_036>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
using Fn_xor_037 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, int);
static_assert(std::is_same<decltype(static_cast<Fn_xor_037>(&sc_dt::operator^)), Fn_xor_037>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_signed (const sc_dt::sc_concatref &, int)");
using Fn_xor_038 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, long);
static_assert(std::is_same<decltype(static_cast<Fn_xor_038>(&sc_dt::operator^)), Fn_xor_038>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_signed (const sc_dt::sc_concatref &, long)");
using Fn_xor_039 = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_xor_039>(&sc_dt::operator^)), Fn_xor_039>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_signed (const sc_dt::sc_concatref &, sc_dt::int64)");
using Fn_xor_040 = const sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_040>(&sc_dt::operator^)), Fn_xor_040>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
using Fn_xor_041 = const sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_041>(&sc_dt::operator^)), Fn_xor_041>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
using Fn_xor_042 = const sc_dt::sc_signed (*)(int, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_042>(&sc_dt::operator^)), Fn_xor_042>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_signed (int, const sc_dt::sc_concatref &)");
using Fn_xor_043 = const sc_dt::sc_signed (*)(long, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_043>(&sc_dt::operator^)), Fn_xor_043>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_signed (long, const sc_dt::sc_concatref &)");
using Fn_xor_044 = const sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_044>(&sc_dt::operator^)), Fn_xor_044>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_concatref &)");
using Fn_xor_045 = const sc_dt::sc_ufix (*)(const sc_dt::sc_ufix &, const sc_dt::sc_ufix &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_045>(&sc_dt::operator^)), Fn_xor_045>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_ufix (const sc_dt::sc_ufix &, const sc_dt::sc_ufix &)");
using Fn_xor_046 = const sc_dt::sc_ufix (*)(const sc_dt::sc_ufix &, const sc_dt::sc_ufix_fast &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_046>(&sc_dt::operator^)), Fn_xor_046>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_ufix (const sc_dt::sc_ufix &, const sc_dt::sc_ufix_fast &)");
using Fn_xor_047 = const sc_dt::sc_ufix (*)(const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_047>(&sc_dt::operator^)), Fn_xor_047>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_ufix (const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix &)");
using Fn_xor_048 = const sc_dt::sc_ufix_fast (*)(const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix_fast &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_048>(&sc_dt::operator^)), Fn_xor_048>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_ufix_fast (const sc_dt::sc_ufix_fast &, const "
              "sc_dt::sc_ufix_fast &)");
using Fn_xor_049 = const sc_dt::sc_unsigned (*)(bool, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_049>(&sc_dt::operator^)), Fn_xor_049>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (bool, const sc_dt::sc_concatref &)");
using Fn_xor_050 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, bool);
static_assert(std::is_same<decltype(static_cast<Fn_xor_050>(&sc_dt::operator^)), Fn_xor_050>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, bool)");
using Fn_xor_051 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_051>(&sc_dt::operator^)), Fn_xor_051>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
              "sc_dt::sc_concatref &)");
using Fn_xor_052 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_052>(&sc_dt::operator^)), Fn_xor_052>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
              "sc_dt::sc_uint_base &)");
using Fn_xor_053 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_053>(&sc_dt::operator^)), Fn_xor_053>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &)");
using Fn_xor_054 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_xor_054>(&sc_dt::operator^)), Fn_xor_054>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, sc_dt::uint64)");
using Fn_xor_055 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_xor_055>(&sc_dt::operator^)), Fn_xor_055>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned int)");
using Fn_xor_056 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_xor_056>(&sc_dt::operator^)), Fn_xor_056>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned long)");
using Fn_xor_057 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_057>(&sc_dt::operator^)), Fn_xor_057>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const "
              "sc_dt::sc_concatref &)");
using Fn_xor_058 = const sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_058>(&sc_dt::operator^)), Fn_xor_058>::value,
  "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &)");
using Fn_xor_059 = const sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_059>(&sc_dt::operator^)), Fn_xor_059>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_concatref &)");
using Fn_xor_060 = const sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_060>(&sc_dt::operator^)), Fn_xor_060>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_concatref &)");
using Fn_xor_061 = const sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_concatref &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_061>(&sc_dt::operator^)), Fn_xor_061>::value,
              "exact overload sc_dt::operator^ const sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_concatref &)");
using Fn_xor_062 = sc_dt::sc_logic (*)(const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &,
                                       const sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_062>(&sc_dt::operator^)), Fn_xor_062>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_logic (const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &, const "
              "sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &)");
using Fn_xor_063 = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_063>(&sc_dt::operator^)), Fn_xor_063>::value,
  "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
using Fn_xor_064 = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_064>(&sc_dt::operator^)), Fn_xor_064>::value,
  "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
using Fn_xor_065 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_065>(&sc_dt::operator^)), Fn_xor_065>::value,
  "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
using Fn_xor_066 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_066>(&sc_dt::operator^)), Fn_xor_066>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
using Fn_xor_067 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_067>(&sc_dt::operator^)), Fn_xor_067>::value,
  "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
using Fn_xor_068 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_068>(&sc_dt::operator^)), Fn_xor_068>::value,
  "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
using Fn_xor_069 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, int);
static_assert(std::is_same<decltype(static_cast<Fn_xor_069>(&sc_dt::operator^)), Fn_xor_069>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_signed &, int)");
using Fn_xor_070 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, long);
static_assert(std::is_same<decltype(static_cast<Fn_xor_070>(&sc_dt::operator^)), Fn_xor_070>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_signed &, long)");
using Fn_xor_071 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_xor_071>(&sc_dt::operator^)), Fn_xor_071>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::int64)");
using Fn_xor_072 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_xor_072>(&sc_dt::operator^)), Fn_xor_072>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::uint64)");
using Fn_xor_073 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_xor_073>(&sc_dt::operator^)), Fn_xor_073>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned int)");
using Fn_xor_074 = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_xor_074>(&sc_dt::operator^)), Fn_xor_074>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned long)");
using Fn_xor_075 = sc_dt::sc_signed (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_075>(&sc_dt::operator^)), Fn_xor_075>::value,
  "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
using Fn_xor_076 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_076>(&sc_dt::operator^)), Fn_xor_076>::value,
  "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
using Fn_xor_077 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_077>(&sc_dt::operator^)), Fn_xor_077>::value,
  "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
using Fn_xor_078 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, int);
static_assert(std::is_same<decltype(static_cast<Fn_xor_078>(&sc_dt::operator^)), Fn_xor_078>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_unsigned &, int)");
using Fn_xor_079 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, long);
static_assert(std::is_same<decltype(static_cast<Fn_xor_079>(&sc_dt::operator^)), Fn_xor_079>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_unsigned &, long)");
using Fn_xor_080 = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
static_assert(std::is_same<decltype(static_cast<Fn_xor_080>(&sc_dt::operator^)), Fn_xor_080>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (const sc_dt::sc_unsigned &, sc_dt::int64)");
using Fn_xor_081 = sc_dt::sc_signed (*)(int, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_081>(&sc_dt::operator^)), Fn_xor_081>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (int, const sc_dt::sc_signed &)");
using Fn_xor_082 = sc_dt::sc_signed (*)(int, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_082>(&sc_dt::operator^)), Fn_xor_082>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (int, const sc_dt::sc_unsigned &)");
using Fn_xor_083 = sc_dt::sc_signed (*)(long, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_083>(&sc_dt::operator^)), Fn_xor_083>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (long, const sc_dt::sc_signed &)");
using Fn_xor_084 = sc_dt::sc_signed (*)(long, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_084>(&sc_dt::operator^)), Fn_xor_084>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (long, const sc_dt::sc_unsigned &)");
using Fn_xor_085 = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_085>(&sc_dt::operator^)), Fn_xor_085>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_signed &)");
using Fn_xor_086 = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_086>(&sc_dt::operator^)), Fn_xor_086>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_unsigned &)");
using Fn_xor_087 = sc_dt::sc_signed (*)(sc_dt::uint64, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_087>(&sc_dt::operator^)), Fn_xor_087>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (sc_dt::uint64, const sc_dt::sc_signed &)");
using Fn_xor_088 = sc_dt::sc_signed (*)(unsigned int, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_088>(&sc_dt::operator^)), Fn_xor_088>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (unsigned int, const sc_dt::sc_signed &)");
using Fn_xor_089 = sc_dt::sc_signed (*)(unsigned long, const sc_dt::sc_signed &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_089>(&sc_dt::operator^)), Fn_xor_089>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_signed (unsigned long, const sc_dt::sc_signed &)");
using Fn_xor_090 = sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_090>(&sc_dt::operator^)), Fn_xor_090>::value,
  "exact overload sc_dt::operator^ sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
using Fn_xor_091 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_091>(&sc_dt::operator^)), Fn_xor_091>::value,
  "exact overload sc_dt::operator^ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
using Fn_xor_092 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
static_assert(
  std::is_same<decltype(static_cast<Fn_xor_092>(&sc_dt::operator^)), Fn_xor_092>::value,
  "exact overload sc_dt::operator^ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
using Fn_xor_093 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
static_assert(std::is_same<decltype(static_cast<Fn_xor_093>(&sc_dt::operator^)), Fn_xor_093>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, sc_dt::uint64)");
using Fn_xor_094 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned int);
static_assert(std::is_same<decltype(static_cast<Fn_xor_094>(&sc_dt::operator^)), Fn_xor_094>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned int)");
using Fn_xor_095 = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned long);
static_assert(std::is_same<decltype(static_cast<Fn_xor_095>(&sc_dt::operator^)), Fn_xor_095>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned long)");
using Fn_xor_096 = sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_096>(&sc_dt::operator^)), Fn_xor_096>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_unsigned &)");
using Fn_xor_097 = sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_097>(&sc_dt::operator^)), Fn_xor_097>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_unsigned &)");
using Fn_xor_098 = sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_unsigned &);
static_assert(std::is_same<decltype(static_cast<Fn_xor_098>(&sc_dt::operator^)), Fn_xor_098>::value,
              "exact overload sc_dt::operator^ sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_unsigned &)");

inline std::string case_mod_000(Sources &source)
{
  return invoke<Fn_mod_000, const sc_dt::sc_concatref &, const sc_dt::sc_int_base &>(
    static_cast<Fn_mod_000>(&sc_dt::operator%), source);
}
inline std::string case_mod_001(Sources &source)
{
  return invoke<Fn_mod_001, const sc_dt::sc_concatref &, const sc_dt::sc_signed &>(
    static_cast<Fn_mod_001>(&sc_dt::operator%), source);
}
inline std::string case_mod_002(Sources &source)
{
  return invoke<Fn_mod_002, const sc_dt::sc_concatref &, int>(static_cast<Fn_mod_002>(&sc_dt::operator%), source);
}
inline std::string case_mod_003(Sources &source)
{
  return invoke<Fn_mod_003, const sc_dt::sc_concatref &, long>(static_cast<Fn_mod_003>(&sc_dt::operator%), source);
}
inline std::string case_mod_004(Sources &source)
{
  return invoke<Fn_mod_004, const sc_dt::sc_concatref &, sc_dt::int64>(static_cast<Fn_mod_004>(&sc_dt::operator%),
                                                                       source);
}
inline std::string case_mod_005(Sources &source)
{
  return invoke<Fn_mod_005, const sc_dt::sc_int_base &, const sc_dt::sc_concatref &>(
    static_cast<Fn_mod_005>(&sc_dt::operator%), source);
}
inline std::string case_mod_006(Sources &source)
{
  return invoke<Fn_mod_006, const sc_dt::sc_signed &, const sc_dt::sc_concatref &>(
    static_cast<Fn_mod_006>(&sc_dt::operator%), source);
}
inline std::string case_mod_007(Sources &source)
{
  return invoke<Fn_mod_007, int, const sc_dt::sc_concatref &>(static_cast<Fn_mod_007>(&sc_dt::operator%), source);
}
inline std::string case_mod_008(Sources &source)
{
  return invoke<Fn_mod_008, long, const sc_dt::sc_concatref &>(static_cast<Fn_mod_008>(&sc_dt::operator%), source);
}
inline std::string case_mod_009(Sources &source)
{
  return invoke<Fn_mod_009, sc_dt::int64, const sc_dt::sc_concatref &>(static_cast<Fn_mod_009>(&sc_dt::operator%),
                                                                       source);
}
inline std::string case_mod_010(Sources &source)
{
  return invoke<Fn_mod_010, bool, const sc_dt::sc_concatref &>(static_cast<Fn_mod_010>(&sc_dt::operator%), source);
}
inline std::string case_mod_011(Sources &source)
{
  return invoke<Fn_mod_011, const sc_dt::sc_concatref &, bool>(static_cast<Fn_mod_011>(&sc_dt::operator%), source);
}
inline std::string case_mod_012(Sources &source)
{
  return invoke<Fn_mod_012, const sc_dt::sc_concatref &, const sc_dt::sc_concatref &>(
    static_cast<Fn_mod_012>(&sc_dt::operator%), source);
}
inline std::string case_mod_013(Sources &source)
{
  return invoke<Fn_mod_013, const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_mod_013>(&sc_dt::operator%), source);
}
inline std::string case_mod_014(Sources &source)
{
  return invoke<Fn_mod_014, const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_mod_014>(&sc_dt::operator%), source);
}
inline std::string case_mod_015(Sources &source)
{
  return invoke<Fn_mod_015, const sc_dt::sc_concatref &, sc_dt::uint64>(static_cast<Fn_mod_015>(&sc_dt::operator%),
                                                                        source);
}
inline std::string case_mod_016(Sources &source)
{
  return invoke<Fn_mod_016, const sc_dt::sc_concatref &, unsigned int>(static_cast<Fn_mod_016>(&sc_dt::operator%),
                                                                       source);
}
inline std::string case_mod_017(Sources &source)
{
  return invoke<Fn_mod_017, const sc_dt::sc_concatref &, unsigned long>(static_cast<Fn_mod_017>(&sc_dt::operator%),
                                                                        source);
}
inline std::string case_mod_018(Sources &source)
{
  return invoke<Fn_mod_018, const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &>(
    static_cast<Fn_mod_018>(&sc_dt::operator%), source);
}
inline std::string case_mod_019(Sources &source)
{
  return invoke<Fn_mod_019, const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &>(
    static_cast<Fn_mod_019>(&sc_dt::operator%), source);
}
inline std::string case_mod_020(Sources &source)
{
  return invoke<Fn_mod_020, sc_dt::uint64, const sc_dt::sc_concatref &>(static_cast<Fn_mod_020>(&sc_dt::operator%),
                                                                        source);
}
inline std::string case_mod_021(Sources &source)
{
  return invoke<Fn_mod_021, unsigned int, const sc_dt::sc_concatref &>(static_cast<Fn_mod_021>(&sc_dt::operator%),
                                                                       source);
}
inline std::string case_mod_022(Sources &source)
{
  return invoke<Fn_mod_022, unsigned long, const sc_dt::sc_concatref &>(static_cast<Fn_mod_022>(&sc_dt::operator%),
                                                                        source);
}
inline std::string case_mod_023(Sources &source)
{
  return invoke<Fn_mod_023, const sc_dt::sc_int_base &, const sc_dt::sc_signed &>(
    static_cast<Fn_mod_023>(&sc_dt::operator%), source);
}
inline std::string case_mod_024(Sources &source)
{
  return invoke<Fn_mod_024, const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_mod_024>(&sc_dt::operator%), source);
}
inline std::string case_mod_025(Sources &source)
{
  return invoke<Fn_mod_025, const sc_dt::sc_signed &, const sc_dt::sc_int_base &>(
    static_cast<Fn_mod_025>(&sc_dt::operator%), source);
}
inline std::string case_mod_026(Sources &source)
{
  return invoke<Fn_mod_026, const sc_dt::sc_signed &, const sc_dt::sc_signed &>(
    static_cast<Fn_mod_026>(&sc_dt::operator%), source);
}
inline std::string case_mod_027(Sources &source)
{
  return invoke<Fn_mod_027, const sc_dt::sc_signed &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_mod_027>(&sc_dt::operator%), source);
}
inline std::string case_mod_028(Sources &source)
{
  return invoke<Fn_mod_028, const sc_dt::sc_signed &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_mod_028>(&sc_dt::operator%), source);
}
inline std::string case_mod_029(Sources &source)
{
  return invoke<Fn_mod_029, const sc_dt::sc_signed &, int>(static_cast<Fn_mod_029>(&sc_dt::operator%), source);
}
inline std::string case_mod_030(Sources &source)
{
  return invoke<Fn_mod_030, const sc_dt::sc_signed &, long>(static_cast<Fn_mod_030>(&sc_dt::operator%), source);
}
inline std::string case_mod_031(Sources &source)
{
  return invoke<Fn_mod_031, const sc_dt::sc_signed &, sc_dt::int64>(static_cast<Fn_mod_031>(&sc_dt::operator%), source);
}
inline std::string case_mod_032(Sources &source)
{
  return invoke<Fn_mod_032, const sc_dt::sc_signed &, sc_dt::uint64>(static_cast<Fn_mod_032>(&sc_dt::operator%),
                                                                     source);
}
inline std::string case_mod_033(Sources &source)
{
  return invoke<Fn_mod_033, const sc_dt::sc_signed &, unsigned int>(static_cast<Fn_mod_033>(&sc_dt::operator%), source);
}
inline std::string case_mod_034(Sources &source)
{
  return invoke<Fn_mod_034, const sc_dt::sc_signed &, unsigned long>(static_cast<Fn_mod_034>(&sc_dt::operator%),
                                                                     source);
}
inline std::string case_mod_035(Sources &source)
{
  return invoke<Fn_mod_035, const sc_dt::sc_uint_base &, const sc_dt::sc_signed &>(
    static_cast<Fn_mod_035>(&sc_dt::operator%), source);
}
inline std::string case_mod_036(Sources &source)
{
  return invoke<Fn_mod_036, const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &>(
    static_cast<Fn_mod_036>(&sc_dt::operator%), source);
}
inline std::string case_mod_037(Sources &source)
{
  return invoke<Fn_mod_037, const sc_dt::sc_unsigned &, const sc_dt::sc_signed &>(
    static_cast<Fn_mod_037>(&sc_dt::operator%), source);
}
inline std::string case_mod_038(Sources &source)
{
  return invoke<Fn_mod_038, const sc_dt::sc_unsigned &, int>(static_cast<Fn_mod_038>(&sc_dt::operator%), source);
}
inline std::string case_mod_039(Sources &source)
{
  return invoke<Fn_mod_039, const sc_dt::sc_unsigned &, long>(static_cast<Fn_mod_039>(&sc_dt::operator%), source);
}
inline std::string case_mod_040(Sources &source)
{
  return invoke<Fn_mod_040, const sc_dt::sc_unsigned &, sc_dt::int64>(static_cast<Fn_mod_040>(&sc_dt::operator%),
                                                                      source);
}
inline std::string case_mod_041(Sources &source)
{
  return invoke<Fn_mod_041, int, const sc_dt::sc_signed &>(static_cast<Fn_mod_041>(&sc_dt::operator%), source);
}
inline std::string case_mod_042(Sources &source)
{
  return invoke<Fn_mod_042, int, const sc_dt::sc_unsigned &>(static_cast<Fn_mod_042>(&sc_dt::operator%), source);
}
inline std::string case_mod_043(Sources &source)
{
  return invoke<Fn_mod_043, long, const sc_dt::sc_signed &>(static_cast<Fn_mod_043>(&sc_dt::operator%), source);
}
inline std::string case_mod_044(Sources &source)
{
  return invoke<Fn_mod_044, long, const sc_dt::sc_unsigned &>(static_cast<Fn_mod_044>(&sc_dt::operator%), source);
}
inline std::string case_mod_045(Sources &source)
{
  return invoke<Fn_mod_045, sc_dt::int64, const sc_dt::sc_signed &>(static_cast<Fn_mod_045>(&sc_dt::operator%), source);
}
inline std::string case_mod_046(Sources &source)
{
  return invoke<Fn_mod_046, sc_dt::int64, const sc_dt::sc_unsigned &>(static_cast<Fn_mod_046>(&sc_dt::operator%),
                                                                      source);
}
inline std::string case_mod_047(Sources &source)
{
  return invoke<Fn_mod_047, sc_dt::uint64, const sc_dt::sc_signed &>(static_cast<Fn_mod_047>(&sc_dt::operator%),
                                                                     source);
}
inline std::string case_mod_048(Sources &source)
{
  return invoke<Fn_mod_048, unsigned int, const sc_dt::sc_signed &>(static_cast<Fn_mod_048>(&sc_dt::operator%), source);
}
inline std::string case_mod_049(Sources &source)
{
  return invoke<Fn_mod_049, unsigned long, const sc_dt::sc_signed &>(static_cast<Fn_mod_049>(&sc_dt::operator%),
                                                                     source);
}
inline std::string case_mod_050(Sources &source)
{
  return invoke<Fn_mod_050, const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_mod_050>(&sc_dt::operator%), source);
}
inline std::string case_mod_051(Sources &source)
{
  return invoke<Fn_mod_051, const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_mod_051>(&sc_dt::operator%), source);
}
inline std::string case_mod_052(Sources &source)
{
  return invoke<Fn_mod_052, const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_mod_052>(&sc_dt::operator%), source);
}
inline std::string case_mod_053(Sources &source)
{
  return invoke<Fn_mod_053, const sc_dt::sc_unsigned &, sc_dt::uint64>(static_cast<Fn_mod_053>(&sc_dt::operator%),
                                                                       source);
}
inline std::string case_mod_054(Sources &source)
{
  return invoke<Fn_mod_054, const sc_dt::sc_unsigned &, unsigned int>(static_cast<Fn_mod_054>(&sc_dt::operator%),
                                                                      source);
}
inline std::string case_mod_055(Sources &source)
{
  return invoke<Fn_mod_055, const sc_dt::sc_unsigned &, unsigned long>(static_cast<Fn_mod_055>(&sc_dt::operator%),
                                                                       source);
}
inline std::string case_mod_056(Sources &source)
{
  return invoke<Fn_mod_056, sc_dt::uint64, const sc_dt::sc_unsigned &>(static_cast<Fn_mod_056>(&sc_dt::operator%),
                                                                       source);
}
inline std::string case_mod_057(Sources &source)
{
  return invoke<Fn_mod_057, unsigned int, const sc_dt::sc_unsigned &>(static_cast<Fn_mod_057>(&sc_dt::operator%),
                                                                      source);
}
inline std::string case_mod_058(Sources &source)
{
  return invoke<Fn_mod_058, unsigned long, const sc_dt::sc_unsigned &>(static_cast<Fn_mod_058>(&sc_dt::operator%),
                                                                       source);
}
inline std::string case_and_000(Sources &source)
{
  return invoke<Fn_and_000, bool, const sc_dt::sc_bit &>(static_cast<Fn_and_000>(&sc_dt::operator&), source);
}
inline std::string case_and_001(Sources &source)
{
  return invoke<Fn_and_001, char, const sc_dt::sc_bit &>(static_cast<Fn_and_001>(&sc_dt::operator&), source);
}
inline std::string case_and_002(Sources &source)
{
  return invoke<Fn_and_002, const sc_dt::sc_bit &, bool>(static_cast<Fn_and_002>(&sc_dt::operator&), source);
}
inline std::string case_and_003(Sources &source)
{
  return invoke<Fn_and_003, const sc_dt::sc_bit &, char>(static_cast<Fn_and_003>(&sc_dt::operator&), source);
}
inline std::string case_and_004(Sources &source)
{
  return invoke<Fn_and_004, const sc_dt::sc_bit &, const sc_dt::sc_bit &>(static_cast<Fn_and_004>(&sc_dt::operator&),
                                                                          source);
}
inline std::string case_and_005(Sources &source)
{
  return invoke<Fn_and_005, const sc_dt::sc_bit &, int>(static_cast<Fn_and_005>(&sc_dt::operator&), source);
}
inline std::string case_and_006(Sources &source)
{
  return invoke<Fn_and_006, int, const sc_dt::sc_bit &>(static_cast<Fn_and_006>(&sc_dt::operator&), source);
}
inline std::string case_and_007(Sources &source)
{
  return invoke<Fn_and_007, const sc_dt::sc_fix &, const sc_dt::sc_fix &>(static_cast<Fn_and_007>(&sc_dt::operator&),
                                                                          source);
}
inline std::string case_and_008(Sources &source)
{
  return invoke<Fn_and_008, const sc_dt::sc_fix &, const sc_dt::sc_fix_fast &>(
    static_cast<Fn_and_008>(&sc_dt::operator&), source);
}
inline std::string case_and_009(Sources &source)
{
  return invoke<Fn_and_009, const sc_dt::sc_fix_fast &, const sc_dt::sc_fix &>(
    static_cast<Fn_and_009>(&sc_dt::operator&), source);
}
inline std::string case_and_010(Sources &source)
{
  return invoke<Fn_and_010, const sc_dt::sc_fix_fast &, const sc_dt::sc_fix_fast &>(
    static_cast<Fn_and_010>(&sc_dt::operator&), source);
}
inline std::string case_and_011(Sources &source)
{
  return invoke<Fn_and_011, bool, const sc_dt::sc_logic &>(static_cast<Fn_and_011>(&sc_dt::operator&), source);
}
inline std::string case_and_012(Sources &source)
{
  return invoke<Fn_and_012, char, const sc_dt::sc_logic &>(static_cast<Fn_and_012>(&sc_dt::operator&), source);
}
inline std::string case_and_013(Sources &source)
{
  return invoke<Fn_and_013, const sc_dt::sc_logic &, bool>(static_cast<Fn_and_013>(&sc_dt::operator&), source);
}
inline std::string case_and_014(Sources &source)
{
  return invoke<Fn_and_014, const sc_dt::sc_logic &, char>(static_cast<Fn_and_014>(&sc_dt::operator&), source);
}
inline std::string case_and_015(Sources &source)
{
  return invoke<Fn_and_015, const sc_dt::sc_logic &, const sc_dt::sc_logic &>(
    static_cast<Fn_and_015>(&sc_dt::operator&), source);
}
inline std::string case_and_016(Sources &source)
{
  return invoke<Fn_and_016, const sc_dt::sc_logic &, int>(static_cast<Fn_and_016>(&sc_dt::operator&), source);
}
inline std::string case_and_017(Sources &source)
{
  return invoke<Fn_and_017, const sc_dt::sc_logic &, sc_dt::sc_logic_value_t>(
    static_cast<Fn_and_017>(&sc_dt::operator&), source);
}
inline std::string case_and_018(Sources &source)
{
  return invoke<Fn_and_018, int, const sc_dt::sc_logic &>(static_cast<Fn_and_018>(&sc_dt::operator&), source);
}
inline std::string case_and_019(Sources &source)
{
  return invoke<Fn_and_019, sc_dt::sc_logic_value_t, const sc_dt::sc_logic &>(
    static_cast<Fn_and_019>(&sc_dt::operator&), source);
}
inline std::string case_and_020(Sources &source)
{
  return invoke<Fn_and_020, const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_020>(&sc_dt::operator&), source);
}
inline std::string case_and_021(Sources &source)
{
  return invoke<Fn_and_021, const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_021>(&sc_dt::operator&), source);
}
inline std::string case_and_022(Sources &source)
{
  return invoke<Fn_and_022, const sc_dt::sc_int_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_022>(&sc_dt::operator&), source);
}
inline std::string case_and_023(Sources &source)
{
  return invoke<Fn_and_023, const sc_dt::sc_logic *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_023>(&sc_dt::operator&), source);
}
inline std::string case_and_024(Sources &source)
{
  return invoke<Fn_and_026, const sc_dt::sc_signed &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_026>(&sc_dt::operator&), source);
}
inline std::string case_and_025(Sources &source)
{
  return invoke<Fn_and_027, const sc_dt::sc_uint_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_027>(&sc_dt::operator&), source);
}
inline std::string case_and_026(Sources &source)
{
  return invoke<Fn_and_028, const sc_dt::sc_unsigned &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_028>(&sc_dt::operator&), source);
}
inline std::string case_and_027(Sources &source)
{
  return invoke<Fn_and_025, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &>(
    static_cast<Fn_and_025>(&sc_dt::operator&), source);
}
inline std::string case_and_028(Sources &source)
{
  return invoke<Fn_and_024, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_024>(&sc_dt::operator&), source);
}
inline std::string case_and_029(Sources &source)
{
  return invoke<Fn_and_025, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &>(
    static_cast<Fn_and_025>(&sc_dt::operator&), source);
}
inline std::string case_and_030(Sources &source)
{
  return invoke<Fn_and_029, int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(static_cast<Fn_and_029>(&sc_dt::operator&),
                                                                             source);
}
inline std::string case_and_031(Sources &source)
{
  return invoke<Fn_and_030, long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_030>(&sc_dt::operator&), source);
}
inline std::string case_and_032(Sources &source)
{
  return invoke<Fn_and_031, sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_031>(&sc_dt::operator&), source);
}
inline std::string case_and_033(Sources &source)
{
  return invoke<Fn_and_032, sc_dt::uint64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_032>(&sc_dt::operator&), source);
}
inline std::string case_and_034(Sources &source)
{
  return invoke<Fn_and_033, unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_033>(&sc_dt::operator&), source);
}
inline std::string case_and_035(Sources &source)
{
  return invoke<Fn_and_034, unsigned long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_and_034>(&sc_dt::operator&), source);
}
inline std::string case_and_036(Sources &source)
{
  return invoke<Fn_and_035, const sc_dt::sc_concatref &, const sc_dt::sc_int_base &>(
    static_cast<Fn_and_035>(&sc_dt::operator&), source);
}
inline std::string case_and_037(Sources &source)
{
  return invoke<Fn_and_036, const sc_dt::sc_concatref &, const sc_dt::sc_signed &>(
    static_cast<Fn_and_036>(&sc_dt::operator&), source);
}
inline std::string case_and_038(Sources &source)
{
  return invoke<Fn_and_037, const sc_dt::sc_concatref &, int>(static_cast<Fn_and_037>(&sc_dt::operator&), source);
}
inline std::string case_and_039(Sources &source)
{
  return invoke<Fn_and_038, const sc_dt::sc_concatref &, long>(static_cast<Fn_and_038>(&sc_dt::operator&), source);
}
inline std::string case_and_040(Sources &source)
{
  return invoke<Fn_and_039, const sc_dt::sc_concatref &, sc_dt::int64>(static_cast<Fn_and_039>(&sc_dt::operator&),
                                                                       source);
}
inline std::string case_and_041(Sources &source)
{
  return invoke<Fn_and_040, const sc_dt::sc_int_base &, const sc_dt::sc_concatref &>(
    static_cast<Fn_and_040>(&sc_dt::operator&), source);
}
inline std::string case_and_042(Sources &source)
{
  return invoke<Fn_and_041, const sc_dt::sc_signed &, const sc_dt::sc_concatref &>(
    static_cast<Fn_and_041>(&sc_dt::operator&), source);
}
inline std::string case_and_043(Sources &source)
{
  return invoke<Fn_and_042, int, const sc_dt::sc_concatref &>(static_cast<Fn_and_042>(&sc_dt::operator&), source);
}
inline std::string case_and_044(Sources &source)
{
  return invoke<Fn_and_043, long, const sc_dt::sc_concatref &>(static_cast<Fn_and_043>(&sc_dt::operator&), source);
}
inline std::string case_and_045(Sources &source)
{
  return invoke<Fn_and_044, sc_dt::int64, const sc_dt::sc_concatref &>(static_cast<Fn_and_044>(&sc_dt::operator&),
                                                                       source);
}
inline std::string case_and_046(Sources &source)
{
  return invoke<Fn_and_045, const sc_dt::sc_ufix &, const sc_dt::sc_ufix &>(static_cast<Fn_and_045>(&sc_dt::operator&),
                                                                            source);
}
inline std::string case_and_047(Sources &source)
{
  return invoke<Fn_and_046, const sc_dt::sc_ufix &, const sc_dt::sc_ufix_fast &>(
    static_cast<Fn_and_046>(&sc_dt::operator&), source);
}
inline std::string case_and_048(Sources &source)
{
  return invoke<Fn_and_047, const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix &>(
    static_cast<Fn_and_047>(&sc_dt::operator&), source);
}
inline std::string case_and_049(Sources &source)
{
  return invoke<Fn_and_048, const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix_fast &>(
    static_cast<Fn_and_048>(&sc_dt::operator&), source);
}
inline std::string case_and_050(Sources &source)
{
  return invoke<Fn_and_049, bool, const sc_dt::sc_concatref &>(static_cast<Fn_and_049>(&sc_dt::operator&), source);
}
inline std::string case_and_051(Sources &source)
{
  return invoke<Fn_and_050, const sc_dt::sc_concatref &, bool>(static_cast<Fn_and_050>(&sc_dt::operator&), source);
}
inline std::string case_and_052(Sources &source)
{
  return invoke<Fn_and_051, const sc_dt::sc_concatref &, const sc_dt::sc_concatref &>(
    static_cast<Fn_and_051>(&sc_dt::operator&), source);
}
inline std::string case_and_053(Sources &source)
{
  return invoke<Fn_and_052, const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_and_052>(&sc_dt::operator&), source);
}
inline std::string case_and_054(Sources &source)
{
  return invoke<Fn_and_053, const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_and_053>(&sc_dt::operator&), source);
}
inline std::string case_and_055(Sources &source)
{
  return invoke<Fn_and_054, const sc_dt::sc_concatref &, sc_dt::uint64>(static_cast<Fn_and_054>(&sc_dt::operator&),
                                                                        source);
}
inline std::string case_and_056(Sources &source)
{
  return invoke<Fn_and_055, const sc_dt::sc_concatref &, unsigned int>(static_cast<Fn_and_055>(&sc_dt::operator&),
                                                                       source);
}
inline std::string case_and_057(Sources &source)
{
  return invoke<Fn_and_056, const sc_dt::sc_concatref &, unsigned long>(static_cast<Fn_and_056>(&sc_dt::operator&),
                                                                        source);
}
inline std::string case_and_058(Sources &source)
{
  return invoke<Fn_and_057, const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &>(
    static_cast<Fn_and_057>(&sc_dt::operator&), source);
}
inline std::string case_and_059(Sources &source)
{
  return invoke<Fn_and_058, const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &>(
    static_cast<Fn_and_058>(&sc_dt::operator&), source);
}
inline std::string case_and_060(Sources &source)
{
  return invoke<Fn_and_059, sc_dt::uint64, const sc_dt::sc_concatref &>(static_cast<Fn_and_059>(&sc_dt::operator&),
                                                                        source);
}
inline std::string case_and_061(Sources &source)
{
  return invoke<Fn_and_060, unsigned int, const sc_dt::sc_concatref &>(static_cast<Fn_and_060>(&sc_dt::operator&),
                                                                       source);
}
inline std::string case_and_062(Sources &source)
{
  return invoke<Fn_and_061, unsigned long, const sc_dt::sc_concatref &>(static_cast<Fn_and_061>(&sc_dt::operator&),
                                                                        source);
}
inline std::string case_and_063(Sources &source)
{
  return invoke<Fn_and_062, const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &,
                const sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &>(static_cast<Fn_and_062>(&sc_dt::operator&), source);
}
inline std::string case_and_064(Sources &source)
{
  return invoke<Fn_and_063, const sc_dt::sc_int_base &, const sc_dt::sc_signed &>(
    static_cast<Fn_and_063>(&sc_dt::operator&), source);
}
inline std::string case_and_065(Sources &source)
{
  return invoke<Fn_and_064, const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_and_064>(&sc_dt::operator&), source);
}
inline std::string case_and_066(Sources &source)
{
  return invoke<Fn_and_065, const sc_dt::sc_signed &, const sc_dt::sc_int_base &>(
    static_cast<Fn_and_065>(&sc_dt::operator&), source);
}
inline std::string case_and_067(Sources &source)
{
  return invoke<Fn_and_066, const sc_dt::sc_signed &, const sc_dt::sc_signed &>(
    static_cast<Fn_and_066>(&sc_dt::operator&), source);
}
inline std::string case_and_068(Sources &source)
{
  return invoke<Fn_and_067, const sc_dt::sc_signed &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_and_067>(&sc_dt::operator&), source);
}
inline std::string case_and_069(Sources &source)
{
  return invoke<Fn_and_068, const sc_dt::sc_signed &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_and_068>(&sc_dt::operator&), source);
}
inline std::string case_and_070(Sources &source)
{
  return invoke<Fn_and_069, const sc_dt::sc_signed &, int>(static_cast<Fn_and_069>(&sc_dt::operator&), source);
}
inline std::string case_and_071(Sources &source)
{
  return invoke<Fn_and_070, const sc_dt::sc_signed &, long>(static_cast<Fn_and_070>(&sc_dt::operator&), source);
}
inline std::string case_and_072(Sources &source)
{
  return invoke<Fn_and_071, const sc_dt::sc_signed &, sc_dt::int64>(static_cast<Fn_and_071>(&sc_dt::operator&), source);
}
inline std::string case_and_073(Sources &source)
{
  return invoke<Fn_and_072, const sc_dt::sc_signed &, sc_dt::uint64>(static_cast<Fn_and_072>(&sc_dt::operator&),
                                                                     source);
}
inline std::string case_and_074(Sources &source)
{
  return invoke<Fn_and_073, const sc_dt::sc_signed &, unsigned int>(static_cast<Fn_and_073>(&sc_dt::operator&), source);
}
inline std::string case_and_075(Sources &source)
{
  return invoke<Fn_and_074, const sc_dt::sc_signed &, unsigned long>(static_cast<Fn_and_074>(&sc_dt::operator&),
                                                                     source);
}
inline std::string case_and_076(Sources &source)
{
  return invoke<Fn_and_075, const sc_dt::sc_uint_base &, const sc_dt::sc_signed &>(
    static_cast<Fn_and_075>(&sc_dt::operator&), source);
}
inline std::string case_and_077(Sources &source)
{
  return invoke<Fn_and_076, const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &>(
    static_cast<Fn_and_076>(&sc_dt::operator&), source);
}
inline std::string case_and_078(Sources &source)
{
  return invoke<Fn_and_077, const sc_dt::sc_unsigned &, const sc_dt::sc_signed &>(
    static_cast<Fn_and_077>(&sc_dt::operator&), source);
}
inline std::string case_and_079(Sources &source)
{
  return invoke<Fn_and_078, const sc_dt::sc_unsigned &, int>(static_cast<Fn_and_078>(&sc_dt::operator&), source);
}
inline std::string case_and_080(Sources &source)
{
  return invoke<Fn_and_079, const sc_dt::sc_unsigned &, long>(static_cast<Fn_and_079>(&sc_dt::operator&), source);
}
inline std::string case_and_081(Sources &source)
{
  return invoke<Fn_and_080, const sc_dt::sc_unsigned &, sc_dt::int64>(static_cast<Fn_and_080>(&sc_dt::operator&),
                                                                      source);
}
inline std::string case_and_082(Sources &source)
{
  return invoke<Fn_and_081, int, const sc_dt::sc_signed &>(static_cast<Fn_and_081>(&sc_dt::operator&), source);
}
inline std::string case_and_083(Sources &source)
{
  return invoke<Fn_and_082, int, const sc_dt::sc_unsigned &>(static_cast<Fn_and_082>(&sc_dt::operator&), source);
}
inline std::string case_and_084(Sources &source)
{
  return invoke<Fn_and_083, long, const sc_dt::sc_signed &>(static_cast<Fn_and_083>(&sc_dt::operator&), source);
}
inline std::string case_and_085(Sources &source)
{
  return invoke<Fn_and_084, long, const sc_dt::sc_unsigned &>(static_cast<Fn_and_084>(&sc_dt::operator&), source);
}
inline std::string case_and_086(Sources &source)
{
  return invoke<Fn_and_085, sc_dt::int64, const sc_dt::sc_signed &>(static_cast<Fn_and_085>(&sc_dt::operator&), source);
}
inline std::string case_and_087(Sources &source)
{
  return invoke<Fn_and_086, sc_dt::int64, const sc_dt::sc_unsigned &>(static_cast<Fn_and_086>(&sc_dt::operator&),
                                                                      source);
}
inline std::string case_and_088(Sources &source)
{
  return invoke<Fn_and_087, sc_dt::uint64, const sc_dt::sc_signed &>(static_cast<Fn_and_087>(&sc_dt::operator&),
                                                                     source);
}
inline std::string case_and_089(Sources &source)
{
  return invoke<Fn_and_088, unsigned int, const sc_dt::sc_signed &>(static_cast<Fn_and_088>(&sc_dt::operator&), source);
}
inline std::string case_and_090(Sources &source)
{
  return invoke<Fn_and_089, unsigned long, const sc_dt::sc_signed &>(static_cast<Fn_and_089>(&sc_dt::operator&),
                                                                     source);
}
inline std::string case_and_091(Sources &source)
{
  return invoke<Fn_and_090, const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_and_090>(&sc_dt::operator&), source);
}
inline std::string case_and_092(Sources &source)
{
  return invoke<Fn_and_091, const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_and_091>(&sc_dt::operator&), source);
}
inline std::string case_and_093(Sources &source)
{
  return invoke<Fn_and_092, const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_and_092>(&sc_dt::operator&), source);
}
inline std::string case_and_094(Sources &source)
{
  return invoke<Fn_and_093, const sc_dt::sc_unsigned &, sc_dt::uint64>(static_cast<Fn_and_093>(&sc_dt::operator&),
                                                                       source);
}
inline std::string case_and_095(Sources &source)
{
  return invoke<Fn_and_094, const sc_dt::sc_unsigned &, unsigned int>(static_cast<Fn_and_094>(&sc_dt::operator&),
                                                                      source);
}
inline std::string case_and_096(Sources &source)
{
  return invoke<Fn_and_095, const sc_dt::sc_unsigned &, unsigned long>(static_cast<Fn_and_095>(&sc_dt::operator&),
                                                                       source);
}
inline std::string case_and_097(Sources &source)
{
  return invoke<Fn_and_096, sc_dt::uint64, const sc_dt::sc_unsigned &>(static_cast<Fn_and_096>(&sc_dt::operator&),
                                                                       source);
}
inline std::string case_and_098(Sources &source)
{
  return invoke<Fn_and_097, unsigned int, const sc_dt::sc_unsigned &>(static_cast<Fn_and_097>(&sc_dt::operator&),
                                                                      source);
}
inline std::string case_and_099(Sources &source)
{
  return invoke<Fn_and_098, unsigned long, const sc_dt::sc_unsigned &>(static_cast<Fn_and_098>(&sc_dt::operator&),
                                                                       source);
}
inline std::string case_or_000(Sources &source)
{
  return invoke<Fn_or_000, bool, const sc_dt::sc_bit &>(static_cast<Fn_or_000>(&sc_dt::operator|), source);
}
inline std::string case_or_001(Sources &source)
{
  return invoke<Fn_or_001, char, const sc_dt::sc_bit &>(static_cast<Fn_or_001>(&sc_dt::operator|), source);
}
inline std::string case_or_002(Sources &source)
{
  return invoke<Fn_or_002, const sc_dt::sc_bit &, bool>(static_cast<Fn_or_002>(&sc_dt::operator|), source);
}
inline std::string case_or_003(Sources &source)
{
  return invoke<Fn_or_003, const sc_dt::sc_bit &, char>(static_cast<Fn_or_003>(&sc_dt::operator|), source);
}
inline std::string case_or_004(Sources &source)
{
  return invoke<Fn_or_004, const sc_dt::sc_bit &, const sc_dt::sc_bit &>(static_cast<Fn_or_004>(&sc_dt::operator|),
                                                                         source);
}
inline std::string case_or_005(Sources &source)
{
  return invoke<Fn_or_005, const sc_dt::sc_bit &, int>(static_cast<Fn_or_005>(&sc_dt::operator|), source);
}
inline std::string case_or_006(Sources &source)
{
  return invoke<Fn_or_006, int, const sc_dt::sc_bit &>(static_cast<Fn_or_006>(&sc_dt::operator|), source);
}
inline std::string case_or_007(Sources &source)
{
  return invoke<Fn_or_007, const sc_dt::sc_fix &, const sc_dt::sc_fix &>(static_cast<Fn_or_007>(&sc_dt::operator|),
                                                                         source);
}
inline std::string case_or_008(Sources &source)
{
  return invoke<Fn_or_008, const sc_dt::sc_fix &, const sc_dt::sc_fix_fast &>(static_cast<Fn_or_008>(&sc_dt::operator|),
                                                                              source);
}
inline std::string case_or_009(Sources &source)
{
  return invoke<Fn_or_009, const sc_dt::sc_fix_fast &, const sc_dt::sc_fix &>(static_cast<Fn_or_009>(&sc_dt::operator|),
                                                                              source);
}
inline std::string case_or_010(Sources &source)
{
  return invoke<Fn_or_010, const sc_dt::sc_fix_fast &, const sc_dt::sc_fix_fast &>(
    static_cast<Fn_or_010>(&sc_dt::operator|), source);
}
inline std::string case_or_011(Sources &source)
{
  return invoke<Fn_or_011, bool, const sc_dt::sc_logic &>(static_cast<Fn_or_011>(&sc_dt::operator|), source);
}
inline std::string case_or_012(Sources &source)
{
  return invoke<Fn_or_012, char, const sc_dt::sc_logic &>(static_cast<Fn_or_012>(&sc_dt::operator|), source);
}
inline std::string case_or_013(Sources &source)
{
  return invoke<Fn_or_013, const sc_dt::sc_logic &, bool>(static_cast<Fn_or_013>(&sc_dt::operator|), source);
}
inline std::string case_or_014(Sources &source)
{
  return invoke<Fn_or_014, const sc_dt::sc_logic &, char>(static_cast<Fn_or_014>(&sc_dt::operator|), source);
}
inline std::string case_or_015(Sources &source)
{
  return invoke<Fn_or_015, const sc_dt::sc_logic &, const sc_dt::sc_logic &>(static_cast<Fn_or_015>(&sc_dt::operator|),
                                                                             source);
}
inline std::string case_or_016(Sources &source)
{
  return invoke<Fn_or_016, const sc_dt::sc_logic &, int>(static_cast<Fn_or_016>(&sc_dt::operator|), source);
}
inline std::string case_or_017(Sources &source)
{
  return invoke<Fn_or_017, const sc_dt::sc_logic &, sc_dt::sc_logic_value_t>(static_cast<Fn_or_017>(&sc_dt::operator|),
                                                                             source);
}
inline std::string case_or_018(Sources &source)
{
  return invoke<Fn_or_018, int, const sc_dt::sc_logic &>(static_cast<Fn_or_018>(&sc_dt::operator|), source);
}
inline std::string case_or_019(Sources &source)
{
  return invoke<Fn_or_019, sc_dt::sc_logic_value_t, const sc_dt::sc_logic &>(static_cast<Fn_or_019>(&sc_dt::operator|),
                                                                             source);
}
inline std::string case_or_020(Sources &source)
{
  return invoke<Fn_or_020, const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_020>(&sc_dt::operator|), source);
}
inline std::string case_or_021(Sources &source)
{
  return invoke<Fn_or_021, const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_021>(&sc_dt::operator|), source);
}
inline std::string case_or_022(Sources &source)
{
  return invoke<Fn_or_022, const sc_dt::sc_int_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_022>(&sc_dt::operator|), source);
}
inline std::string case_or_023(Sources &source)
{
  return invoke<Fn_or_023, const sc_dt::sc_logic *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_023>(&sc_dt::operator|), source);
}
inline std::string case_or_024(Sources &source)
{
  return invoke<Fn_or_026, const sc_dt::sc_signed &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_026>(&sc_dt::operator|), source);
}
inline std::string case_or_025(Sources &source)
{
  return invoke<Fn_or_027, const sc_dt::sc_uint_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_027>(&sc_dt::operator|), source);
}
inline std::string case_or_026(Sources &source)
{
  return invoke<Fn_or_028, const sc_dt::sc_unsigned &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_028>(&sc_dt::operator|), source);
}
inline std::string case_or_027(Sources &source)
{
  return invoke<Fn_or_025, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &>(
    static_cast<Fn_or_025>(&sc_dt::operator|), source);
}
inline std::string case_or_028(Sources &source)
{
  return invoke<Fn_or_024, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_024>(&sc_dt::operator|), source);
}
inline std::string case_or_029(Sources &source)
{
  return invoke<Fn_or_025, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &>(
    static_cast<Fn_or_025>(&sc_dt::operator|), source);
}
inline std::string case_or_030(Sources &source)
{
  return invoke<Fn_or_029, int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(static_cast<Fn_or_029>(&sc_dt::operator|),
                                                                            source);
}
inline std::string case_or_031(Sources &source)
{
  return invoke<Fn_or_030, long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(static_cast<Fn_or_030>(&sc_dt::operator|),
                                                                             source);
}
inline std::string case_or_032(Sources &source)
{
  return invoke<Fn_or_031, sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_031>(&sc_dt::operator|), source);
}
inline std::string case_or_033(Sources &source)
{
  return invoke<Fn_or_032, sc_dt::uint64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_032>(&sc_dt::operator|), source);
}
inline std::string case_or_034(Sources &source)
{
  return invoke<Fn_or_033, unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_033>(&sc_dt::operator|), source);
}
inline std::string case_or_035(Sources &source)
{
  return invoke<Fn_or_034, unsigned long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_or_034>(&sc_dt::operator|), source);
}
inline std::string case_or_036(Sources &source)
{
  return invoke<Fn_or_035, const sc_dt::sc_concatref &, const sc_dt::sc_int_base &>(
    static_cast<Fn_or_035>(&sc_dt::operator|), source);
}
inline std::string case_or_037(Sources &source)
{
  return invoke<Fn_or_036, const sc_dt::sc_concatref &, const sc_dt::sc_signed &>(
    static_cast<Fn_or_036>(&sc_dt::operator|), source);
}
inline std::string case_or_038(Sources &source)
{
  return invoke<Fn_or_037, const sc_dt::sc_concatref &, int>(static_cast<Fn_or_037>(&sc_dt::operator|), source);
}
inline std::string case_or_039(Sources &source)
{
  return invoke<Fn_or_038, const sc_dt::sc_concatref &, long>(static_cast<Fn_or_038>(&sc_dt::operator|), source);
}
inline std::string case_or_040(Sources &source)
{
  return invoke<Fn_or_039, const sc_dt::sc_concatref &, sc_dt::int64>(static_cast<Fn_or_039>(&sc_dt::operator|),
                                                                      source);
}
inline std::string case_or_041(Sources &source)
{
  return invoke<Fn_or_040, const sc_dt::sc_int_base &, const sc_dt::sc_concatref &>(
    static_cast<Fn_or_040>(&sc_dt::operator|), source);
}
inline std::string case_or_042(Sources &source)
{
  return invoke<Fn_or_041, const sc_dt::sc_signed &, const sc_dt::sc_concatref &>(
    static_cast<Fn_or_041>(&sc_dt::operator|), source);
}
inline std::string case_or_043(Sources &source)
{
  return invoke<Fn_or_042, int, const sc_dt::sc_concatref &>(static_cast<Fn_or_042>(&sc_dt::operator|), source);
}
inline std::string case_or_044(Sources &source)
{
  return invoke<Fn_or_043, long, const sc_dt::sc_concatref &>(static_cast<Fn_or_043>(&sc_dt::operator|), source);
}
inline std::string case_or_045(Sources &source)
{
  return invoke<Fn_or_044, sc_dt::int64, const sc_dt::sc_concatref &>(static_cast<Fn_or_044>(&sc_dt::operator|),
                                                                      source);
}
inline std::string case_or_046(Sources &source)
{
  return invoke<Fn_or_045, const sc_dt::sc_ufix &, const sc_dt::sc_ufix &>(static_cast<Fn_or_045>(&sc_dt::operator|),
                                                                           source);
}
inline std::string case_or_047(Sources &source)
{
  return invoke<Fn_or_046, const sc_dt::sc_ufix &, const sc_dt::sc_ufix_fast &>(
    static_cast<Fn_or_046>(&sc_dt::operator|), source);
}
inline std::string case_or_048(Sources &source)
{
  return invoke<Fn_or_047, const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix &>(
    static_cast<Fn_or_047>(&sc_dt::operator|), source);
}
inline std::string case_or_049(Sources &source)
{
  return invoke<Fn_or_048, const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix_fast &>(
    static_cast<Fn_or_048>(&sc_dt::operator|), source);
}
inline std::string case_or_050(Sources &source)
{
  return invoke<Fn_or_049, bool, const sc_dt::sc_concatref &>(static_cast<Fn_or_049>(&sc_dt::operator|), source);
}
inline std::string case_or_051(Sources &source)
{
  return invoke<Fn_or_050, const sc_dt::sc_concatref &, bool>(static_cast<Fn_or_050>(&sc_dt::operator|), source);
}
inline std::string case_or_052(Sources &source)
{
  return invoke<Fn_or_051, const sc_dt::sc_concatref &, const sc_dt::sc_concatref &>(
    static_cast<Fn_or_051>(&sc_dt::operator|), source);
}
inline std::string case_or_053(Sources &source)
{
  return invoke<Fn_or_052, const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_or_052>(&sc_dt::operator|), source);
}
inline std::string case_or_054(Sources &source)
{
  return invoke<Fn_or_053, const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_or_053>(&sc_dt::operator|), source);
}
inline std::string case_or_055(Sources &source)
{
  return invoke<Fn_or_054, const sc_dt::sc_concatref &, sc_dt::uint64>(static_cast<Fn_or_054>(&sc_dt::operator|),
                                                                       source);
}
inline std::string case_or_056(Sources &source)
{
  return invoke<Fn_or_055, const sc_dt::sc_concatref &, unsigned int>(static_cast<Fn_or_055>(&sc_dt::operator|),
                                                                      source);
}
inline std::string case_or_057(Sources &source)
{
  return invoke<Fn_or_056, const sc_dt::sc_concatref &, unsigned long>(static_cast<Fn_or_056>(&sc_dt::operator|),
                                                                       source);
}
inline std::string case_or_058(Sources &source)
{
  return invoke<Fn_or_057, const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &>(
    static_cast<Fn_or_057>(&sc_dt::operator|), source);
}
inline std::string case_or_059(Sources &source)
{
  return invoke<Fn_or_058, const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &>(
    static_cast<Fn_or_058>(&sc_dt::operator|), source);
}
inline std::string case_or_060(Sources &source)
{
  return invoke<Fn_or_059, sc_dt::uint64, const sc_dt::sc_concatref &>(static_cast<Fn_or_059>(&sc_dt::operator|),
                                                                       source);
}
inline std::string case_or_061(Sources &source)
{
  return invoke<Fn_or_060, unsigned int, const sc_dt::sc_concatref &>(static_cast<Fn_or_060>(&sc_dt::operator|),
                                                                      source);
}
inline std::string case_or_062(Sources &source)
{
  return invoke<Fn_or_061, unsigned long, const sc_dt::sc_concatref &>(static_cast<Fn_or_061>(&sc_dt::operator|),
                                                                       source);
}
inline std::string case_or_063(Sources &source)
{
  return invoke<Fn_or_062, const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &,
                const sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &>(static_cast<Fn_or_062>(&sc_dt::operator|), source);
}
inline std::string case_or_064(Sources &source)
{
  return invoke<Fn_or_063, const sc_dt::sc_int_base &, const sc_dt::sc_signed &>(
    static_cast<Fn_or_063>(&sc_dt::operator|), source);
}
inline std::string case_or_065(Sources &source)
{
  return invoke<Fn_or_064, const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_or_064>(&sc_dt::operator|), source);
}
inline std::string case_or_066(Sources &source)
{
  return invoke<Fn_or_065, const sc_dt::sc_signed &, const sc_dt::sc_int_base &>(
    static_cast<Fn_or_065>(&sc_dt::operator|), source);
}
inline std::string case_or_067(Sources &source)
{
  return invoke<Fn_or_066, const sc_dt::sc_signed &, const sc_dt::sc_signed &>(
    static_cast<Fn_or_066>(&sc_dt::operator|), source);
}
inline std::string case_or_068(Sources &source)
{
  return invoke<Fn_or_067, const sc_dt::sc_signed &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_or_067>(&sc_dt::operator|), source);
}
inline std::string case_or_069(Sources &source)
{
  return invoke<Fn_or_068, const sc_dt::sc_signed &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_or_068>(&sc_dt::operator|), source);
}
inline std::string case_or_070(Sources &source)
{
  return invoke<Fn_or_069, const sc_dt::sc_signed &, int>(static_cast<Fn_or_069>(&sc_dt::operator|), source);
}
inline std::string case_or_071(Sources &source)
{
  return invoke<Fn_or_070, const sc_dt::sc_signed &, long>(static_cast<Fn_or_070>(&sc_dt::operator|), source);
}
inline std::string case_or_072(Sources &source)
{
  return invoke<Fn_or_071, const sc_dt::sc_signed &, sc_dt::int64>(static_cast<Fn_or_071>(&sc_dt::operator|), source);
}
inline std::string case_or_073(Sources &source)
{
  return invoke<Fn_or_072, const sc_dt::sc_signed &, sc_dt::uint64>(static_cast<Fn_or_072>(&sc_dt::operator|), source);
}
inline std::string case_or_074(Sources &source)
{
  return invoke<Fn_or_073, const sc_dt::sc_signed &, unsigned int>(static_cast<Fn_or_073>(&sc_dt::operator|), source);
}
inline std::string case_or_075(Sources &source)
{
  return invoke<Fn_or_074, const sc_dt::sc_signed &, unsigned long>(static_cast<Fn_or_074>(&sc_dt::operator|), source);
}
inline std::string case_or_076(Sources &source)
{
  return invoke<Fn_or_075, const sc_dt::sc_uint_base &, const sc_dt::sc_signed &>(
    static_cast<Fn_or_075>(&sc_dt::operator|), source);
}
inline std::string case_or_077(Sources &source)
{
  return invoke<Fn_or_076, const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &>(
    static_cast<Fn_or_076>(&sc_dt::operator|), source);
}
inline std::string case_or_078(Sources &source)
{
  return invoke<Fn_or_077, const sc_dt::sc_unsigned &, const sc_dt::sc_signed &>(
    static_cast<Fn_or_077>(&sc_dt::operator|), source);
}
inline std::string case_or_079(Sources &source)
{
  return invoke<Fn_or_078, const sc_dt::sc_unsigned &, int>(static_cast<Fn_or_078>(&sc_dt::operator|), source);
}
inline std::string case_or_080(Sources &source)
{
  return invoke<Fn_or_079, const sc_dt::sc_unsigned &, long>(static_cast<Fn_or_079>(&sc_dt::operator|), source);
}
inline std::string case_or_081(Sources &source)
{
  return invoke<Fn_or_080, const sc_dt::sc_unsigned &, sc_dt::int64>(static_cast<Fn_or_080>(&sc_dt::operator|), source);
}
inline std::string case_or_082(Sources &source)
{
  return invoke<Fn_or_081, int, const sc_dt::sc_signed &>(static_cast<Fn_or_081>(&sc_dt::operator|), source);
}
inline std::string case_or_083(Sources &source)
{
  return invoke<Fn_or_082, int, const sc_dt::sc_unsigned &>(static_cast<Fn_or_082>(&sc_dt::operator|), source);
}
inline std::string case_or_084(Sources &source)
{
  return invoke<Fn_or_083, long, const sc_dt::sc_signed &>(static_cast<Fn_or_083>(&sc_dt::operator|), source);
}
inline std::string case_or_085(Sources &source)
{
  return invoke<Fn_or_084, long, const sc_dt::sc_unsigned &>(static_cast<Fn_or_084>(&sc_dt::operator|), source);
}
inline std::string case_or_086(Sources &source)
{
  return invoke<Fn_or_085, sc_dt::int64, const sc_dt::sc_signed &>(static_cast<Fn_or_085>(&sc_dt::operator|), source);
}
inline std::string case_or_087(Sources &source)
{
  return invoke<Fn_or_086, sc_dt::int64, const sc_dt::sc_unsigned &>(static_cast<Fn_or_086>(&sc_dt::operator|), source);
}
inline std::string case_or_088(Sources &source)
{
  return invoke<Fn_or_087, sc_dt::uint64, const sc_dt::sc_signed &>(static_cast<Fn_or_087>(&sc_dt::operator|), source);
}
inline std::string case_or_089(Sources &source)
{
  return invoke<Fn_or_088, unsigned int, const sc_dt::sc_signed &>(static_cast<Fn_or_088>(&sc_dt::operator|), source);
}
inline std::string case_or_090(Sources &source)
{
  return invoke<Fn_or_089, unsigned long, const sc_dt::sc_signed &>(static_cast<Fn_or_089>(&sc_dt::operator|), source);
}
inline std::string case_or_091(Sources &source)
{
  return invoke<Fn_or_090, const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_or_090>(&sc_dt::operator|), source);
}
inline std::string case_or_092(Sources &source)
{
  return invoke<Fn_or_091, const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_or_091>(&sc_dt::operator|), source);
}
inline std::string case_or_093(Sources &source)
{
  return invoke<Fn_or_092, const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_or_092>(&sc_dt::operator|), source);
}
inline std::string case_or_094(Sources &source)
{
  return invoke<Fn_or_093, const sc_dt::sc_unsigned &, sc_dt::uint64>(static_cast<Fn_or_093>(&sc_dt::operator|),
                                                                      source);
}
inline std::string case_or_095(Sources &source)
{
  return invoke<Fn_or_094, const sc_dt::sc_unsigned &, unsigned int>(static_cast<Fn_or_094>(&sc_dt::operator|), source);
}
inline std::string case_or_096(Sources &source)
{
  return invoke<Fn_or_095, const sc_dt::sc_unsigned &, unsigned long>(static_cast<Fn_or_095>(&sc_dt::operator|),
                                                                      source);
}
inline std::string case_or_097(Sources &source)
{
  return invoke<Fn_or_096, sc_dt::uint64, const sc_dt::sc_unsigned &>(static_cast<Fn_or_096>(&sc_dt::operator|),
                                                                      source);
}
inline std::string case_or_098(Sources &source)
{
  return invoke<Fn_or_097, unsigned int, const sc_dt::sc_unsigned &>(static_cast<Fn_or_097>(&sc_dt::operator|), source);
}
inline std::string case_or_099(Sources &source)
{
  return invoke<Fn_or_098, unsigned long, const sc_dt::sc_unsigned &>(static_cast<Fn_or_098>(&sc_dt::operator|),
                                                                      source);
}
inline std::string case_xor_000(Sources &source)
{
  return invoke<Fn_xor_000, bool, const sc_dt::sc_bit &>(static_cast<Fn_xor_000>(&sc_dt::operator^), source);
}
inline std::string case_xor_001(Sources &source)
{
  return invoke<Fn_xor_001, char, const sc_dt::sc_bit &>(static_cast<Fn_xor_001>(&sc_dt::operator^), source);
}
inline std::string case_xor_002(Sources &source)
{
  return invoke<Fn_xor_002, const sc_dt::sc_bit &, bool>(static_cast<Fn_xor_002>(&sc_dt::operator^), source);
}
inline std::string case_xor_003(Sources &source)
{
  return invoke<Fn_xor_003, const sc_dt::sc_bit &, char>(static_cast<Fn_xor_003>(&sc_dt::operator^), source);
}
inline std::string case_xor_004(Sources &source)
{
  return invoke<Fn_xor_004, const sc_dt::sc_bit &, const sc_dt::sc_bit &>(static_cast<Fn_xor_004>(&sc_dt::operator^),
                                                                          source);
}
inline std::string case_xor_005(Sources &source)
{
  return invoke<Fn_xor_005, const sc_dt::sc_bit &, int>(static_cast<Fn_xor_005>(&sc_dt::operator^), source);
}
inline std::string case_xor_006(Sources &source)
{
  return invoke<Fn_xor_006, int, const sc_dt::sc_bit &>(static_cast<Fn_xor_006>(&sc_dt::operator^), source);
}
inline std::string case_xor_007(Sources &source)
{
  return invoke<Fn_xor_007, const sc_dt::sc_fix &, const sc_dt::sc_fix &>(static_cast<Fn_xor_007>(&sc_dt::operator^),
                                                                          source);
}
inline std::string case_xor_008(Sources &source)
{
  return invoke<Fn_xor_008, const sc_dt::sc_fix &, const sc_dt::sc_fix_fast &>(
    static_cast<Fn_xor_008>(&sc_dt::operator^), source);
}
inline std::string case_xor_009(Sources &source)
{
  return invoke<Fn_xor_009, const sc_dt::sc_fix_fast &, const sc_dt::sc_fix &>(
    static_cast<Fn_xor_009>(&sc_dt::operator^), source);
}
inline std::string case_xor_010(Sources &source)
{
  return invoke<Fn_xor_010, const sc_dt::sc_fix_fast &, const sc_dt::sc_fix_fast &>(
    static_cast<Fn_xor_010>(&sc_dt::operator^), source);
}
inline std::string case_xor_011(Sources &source)
{
  return invoke<Fn_xor_011, bool, const sc_dt::sc_logic &>(static_cast<Fn_xor_011>(&sc_dt::operator^), source);
}
inline std::string case_xor_012(Sources &source)
{
  return invoke<Fn_xor_012, char, const sc_dt::sc_logic &>(static_cast<Fn_xor_012>(&sc_dt::operator^), source);
}
inline std::string case_xor_013(Sources &source)
{
  return invoke<Fn_xor_013, const sc_dt::sc_logic &, bool>(static_cast<Fn_xor_013>(&sc_dt::operator^), source);
}
inline std::string case_xor_014(Sources &source)
{
  return invoke<Fn_xor_014, const sc_dt::sc_logic &, char>(static_cast<Fn_xor_014>(&sc_dt::operator^), source);
}
inline std::string case_xor_015(Sources &source)
{
  return invoke<Fn_xor_015, const sc_dt::sc_logic &, const sc_dt::sc_logic &>(
    static_cast<Fn_xor_015>(&sc_dt::operator^), source);
}
inline std::string case_xor_016(Sources &source)
{
  return invoke<Fn_xor_016, const sc_dt::sc_logic &, int>(static_cast<Fn_xor_016>(&sc_dt::operator^), source);
}
inline std::string case_xor_017(Sources &source)
{
  return invoke<Fn_xor_017, const sc_dt::sc_logic &, sc_dt::sc_logic_value_t>(
    static_cast<Fn_xor_017>(&sc_dt::operator^), source);
}
inline std::string case_xor_018(Sources &source)
{
  return invoke<Fn_xor_018, int, const sc_dt::sc_logic &>(static_cast<Fn_xor_018>(&sc_dt::operator^), source);
}
inline std::string case_xor_019(Sources &source)
{
  return invoke<Fn_xor_019, sc_dt::sc_logic_value_t, const sc_dt::sc_logic &>(
    static_cast<Fn_xor_019>(&sc_dt::operator^), source);
}
inline std::string case_xor_020(Sources &source)
{
  return invoke<Fn_xor_020, const bool *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_020>(&sc_dt::operator^), source);
}
inline std::string case_xor_021(Sources &source)
{
  return invoke<Fn_xor_021, const char *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_021>(&sc_dt::operator^), source);
}
inline std::string case_xor_022(Sources &source)
{
  return invoke<Fn_xor_022, const sc_dt::sc_int_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_022>(&sc_dt::operator^), source);
}
inline std::string case_xor_023(Sources &source)
{
  return invoke<Fn_xor_023, const sc_dt::sc_logic *, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_023>(&sc_dt::operator^), source);
}
inline std::string case_xor_024(Sources &source)
{
  return invoke<Fn_xor_026, const sc_dt::sc_signed &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_026>(&sc_dt::operator^), source);
}
inline std::string case_xor_025(Sources &source)
{
  return invoke<Fn_xor_027, const sc_dt::sc_uint_base &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_027>(&sc_dt::operator^), source);
}
inline std::string case_xor_026(Sources &source)
{
  return invoke<Fn_xor_028, const sc_dt::sc_unsigned &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_028>(&sc_dt::operator^), source);
}
inline std::string case_xor_027(Sources &source)
{
  return invoke<Fn_xor_025, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &>(
    static_cast<Fn_xor_025>(&sc_dt::operator^), source);
}
inline std::string case_xor_028(Sources &source)
{
  return invoke<Fn_xor_024, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_024>(&sc_dt::operator^), source);
}
inline std::string case_xor_029(Sources &source)
{
  return invoke<Fn_xor_025, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &, const sc_dt::sc_proxy<sc_dt::sc_lv_base> &>(
    static_cast<Fn_xor_025>(&sc_dt::operator^), source);
}
inline std::string case_xor_030(Sources &source)
{
  return invoke<Fn_xor_029, int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(static_cast<Fn_xor_029>(&sc_dt::operator^),
                                                                             source);
}
inline std::string case_xor_031(Sources &source)
{
  return invoke<Fn_xor_030, long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_030>(&sc_dt::operator^), source);
}
inline std::string case_xor_032(Sources &source)
{
  return invoke<Fn_xor_031, sc_dt::int64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_031>(&sc_dt::operator^), source);
}
inline std::string case_xor_033(Sources &source)
{
  return invoke<Fn_xor_032, sc_dt::uint64, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_032>(&sc_dt::operator^), source);
}
inline std::string case_xor_034(Sources &source)
{
  return invoke<Fn_xor_033, unsigned int, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_033>(&sc_dt::operator^), source);
}
inline std::string case_xor_035(Sources &source)
{
  return invoke<Fn_xor_034, unsigned long, const sc_dt::sc_proxy<sc_dt::sc_bv_base> &>(
    static_cast<Fn_xor_034>(&sc_dt::operator^), source);
}
inline std::string case_xor_036(Sources &source)
{
  return invoke<Fn_xor_035, const sc_dt::sc_concatref &, const sc_dt::sc_int_base &>(
    static_cast<Fn_xor_035>(&sc_dt::operator^), source);
}
inline std::string case_xor_037(Sources &source)
{
  return invoke<Fn_xor_036, const sc_dt::sc_concatref &, const sc_dt::sc_signed &>(
    static_cast<Fn_xor_036>(&sc_dt::operator^), source);
}
inline std::string case_xor_038(Sources &source)
{
  return invoke<Fn_xor_037, const sc_dt::sc_concatref &, int>(static_cast<Fn_xor_037>(&sc_dt::operator^), source);
}
inline std::string case_xor_039(Sources &source)
{
  return invoke<Fn_xor_038, const sc_dt::sc_concatref &, long>(static_cast<Fn_xor_038>(&sc_dt::operator^), source);
}
inline std::string case_xor_040(Sources &source)
{
  return invoke<Fn_xor_039, const sc_dt::sc_concatref &, sc_dt::int64>(static_cast<Fn_xor_039>(&sc_dt::operator^),
                                                                       source);
}
inline std::string case_xor_041(Sources &source)
{
  return invoke<Fn_xor_040, const sc_dt::sc_int_base &, const sc_dt::sc_concatref &>(
    static_cast<Fn_xor_040>(&sc_dt::operator^), source);
}
inline std::string case_xor_042(Sources &source)
{
  return invoke<Fn_xor_041, const sc_dt::sc_signed &, const sc_dt::sc_concatref &>(
    static_cast<Fn_xor_041>(&sc_dt::operator^), source);
}
inline std::string case_xor_043(Sources &source)
{
  return invoke<Fn_xor_042, int, const sc_dt::sc_concatref &>(static_cast<Fn_xor_042>(&sc_dt::operator^), source);
}
inline std::string case_xor_044(Sources &source)
{
  return invoke<Fn_xor_043, long, const sc_dt::sc_concatref &>(static_cast<Fn_xor_043>(&sc_dt::operator^), source);
}
inline std::string case_xor_045(Sources &source)
{
  return invoke<Fn_xor_044, sc_dt::int64, const sc_dt::sc_concatref &>(static_cast<Fn_xor_044>(&sc_dt::operator^),
                                                                       source);
}
inline std::string case_xor_046(Sources &source)
{
  return invoke<Fn_xor_045, const sc_dt::sc_ufix &, const sc_dt::sc_ufix &>(static_cast<Fn_xor_045>(&sc_dt::operator^),
                                                                            source);
}
inline std::string case_xor_047(Sources &source)
{
  return invoke<Fn_xor_046, const sc_dt::sc_ufix &, const sc_dt::sc_ufix_fast &>(
    static_cast<Fn_xor_046>(&sc_dt::operator^), source);
}
inline std::string case_xor_048(Sources &source)
{
  return invoke<Fn_xor_047, const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix &>(
    static_cast<Fn_xor_047>(&sc_dt::operator^), source);
}
inline std::string case_xor_049(Sources &source)
{
  return invoke<Fn_xor_048, const sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix_fast &>(
    static_cast<Fn_xor_048>(&sc_dt::operator^), source);
}
inline std::string case_xor_050(Sources &source)
{
  return invoke<Fn_xor_049, bool, const sc_dt::sc_concatref &>(static_cast<Fn_xor_049>(&sc_dt::operator^), source);
}
inline std::string case_xor_051(Sources &source)
{
  return invoke<Fn_xor_050, const sc_dt::sc_concatref &, bool>(static_cast<Fn_xor_050>(&sc_dt::operator^), source);
}
inline std::string case_xor_052(Sources &source)
{
  return invoke<Fn_xor_051, const sc_dt::sc_concatref &, const sc_dt::sc_concatref &>(
    static_cast<Fn_xor_051>(&sc_dt::operator^), source);
}
inline std::string case_xor_053(Sources &source)
{
  return invoke<Fn_xor_052, const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_xor_052>(&sc_dt::operator^), source);
}
inline std::string case_xor_054(Sources &source)
{
  return invoke<Fn_xor_053, const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_xor_053>(&sc_dt::operator^), source);
}
inline std::string case_xor_055(Sources &source)
{
  return invoke<Fn_xor_054, const sc_dt::sc_concatref &, sc_dt::uint64>(static_cast<Fn_xor_054>(&sc_dt::operator^),
                                                                        source);
}
inline std::string case_xor_056(Sources &source)
{
  return invoke<Fn_xor_055, const sc_dt::sc_concatref &, unsigned int>(static_cast<Fn_xor_055>(&sc_dt::operator^),
                                                                       source);
}
inline std::string case_xor_057(Sources &source)
{
  return invoke<Fn_xor_056, const sc_dt::sc_concatref &, unsigned long>(static_cast<Fn_xor_056>(&sc_dt::operator^),
                                                                        source);
}
inline std::string case_xor_058(Sources &source)
{
  return invoke<Fn_xor_057, const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &>(
    static_cast<Fn_xor_057>(&sc_dt::operator^), source);
}
inline std::string case_xor_059(Sources &source)
{
  return invoke<Fn_xor_058, const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &>(
    static_cast<Fn_xor_058>(&sc_dt::operator^), source);
}
inline std::string case_xor_060(Sources &source)
{
  return invoke<Fn_xor_059, sc_dt::uint64, const sc_dt::sc_concatref &>(static_cast<Fn_xor_059>(&sc_dt::operator^),
                                                                        source);
}
inline std::string case_xor_061(Sources &source)
{
  return invoke<Fn_xor_060, unsigned int, const sc_dt::sc_concatref &>(static_cast<Fn_xor_060>(&sc_dt::operator^),
                                                                       source);
}
inline std::string case_xor_062(Sources &source)
{
  return invoke<Fn_xor_061, unsigned long, const sc_dt::sc_concatref &>(static_cast<Fn_xor_061>(&sc_dt::operator^),
                                                                        source);
}
inline std::string case_xor_063(Sources &source)
{
  return invoke<Fn_xor_062, const sc_dt::sc_bitref_r<sc_dt::sc_bv_base> &,
                const sc_dt::sc_bitref_r<sc_dt::sc_lv_base> &>(static_cast<Fn_xor_062>(&sc_dt::operator^), source);
}
inline std::string case_xor_064(Sources &source)
{
  return invoke<Fn_xor_063, const sc_dt::sc_int_base &, const sc_dt::sc_signed &>(
    static_cast<Fn_xor_063>(&sc_dt::operator^), source);
}
inline std::string case_xor_065(Sources &source)
{
  return invoke<Fn_xor_064, const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_xor_064>(&sc_dt::operator^), source);
}
inline std::string case_xor_066(Sources &source)
{
  return invoke<Fn_xor_065, const sc_dt::sc_signed &, const sc_dt::sc_int_base &>(
    static_cast<Fn_xor_065>(&sc_dt::operator^), source);
}
inline std::string case_xor_067(Sources &source)
{
  return invoke<Fn_xor_066, const sc_dt::sc_signed &, const sc_dt::sc_signed &>(
    static_cast<Fn_xor_066>(&sc_dt::operator^), source);
}
inline std::string case_xor_068(Sources &source)
{
  return invoke<Fn_xor_067, const sc_dt::sc_signed &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_xor_067>(&sc_dt::operator^), source);
}
inline std::string case_xor_069(Sources &source)
{
  return invoke<Fn_xor_068, const sc_dt::sc_signed &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_xor_068>(&sc_dt::operator^), source);
}
inline std::string case_xor_070(Sources &source)
{
  return invoke<Fn_xor_069, const sc_dt::sc_signed &, int>(static_cast<Fn_xor_069>(&sc_dt::operator^), source);
}
inline std::string case_xor_071(Sources &source)
{
  return invoke<Fn_xor_070, const sc_dt::sc_signed &, long>(static_cast<Fn_xor_070>(&sc_dt::operator^), source);
}
inline std::string case_xor_072(Sources &source)
{
  return invoke<Fn_xor_071, const sc_dt::sc_signed &, sc_dt::int64>(static_cast<Fn_xor_071>(&sc_dt::operator^), source);
}
inline std::string case_xor_073(Sources &source)
{
  return invoke<Fn_xor_072, const sc_dt::sc_signed &, sc_dt::uint64>(static_cast<Fn_xor_072>(&sc_dt::operator^),
                                                                     source);
}
inline std::string case_xor_074(Sources &source)
{
  return invoke<Fn_xor_073, const sc_dt::sc_signed &, unsigned int>(static_cast<Fn_xor_073>(&sc_dt::operator^), source);
}
inline std::string case_xor_075(Sources &source)
{
  return invoke<Fn_xor_074, const sc_dt::sc_signed &, unsigned long>(static_cast<Fn_xor_074>(&sc_dt::operator^),
                                                                     source);
}
inline std::string case_xor_076(Sources &source)
{
  return invoke<Fn_xor_075, const sc_dt::sc_uint_base &, const sc_dt::sc_signed &>(
    static_cast<Fn_xor_075>(&sc_dt::operator^), source);
}
inline std::string case_xor_077(Sources &source)
{
  return invoke<Fn_xor_076, const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &>(
    static_cast<Fn_xor_076>(&sc_dt::operator^), source);
}
inline std::string case_xor_078(Sources &source)
{
  return invoke<Fn_xor_077, const sc_dt::sc_unsigned &, const sc_dt::sc_signed &>(
    static_cast<Fn_xor_077>(&sc_dt::operator^), source);
}
inline std::string case_xor_079(Sources &source)
{
  return invoke<Fn_xor_078, const sc_dt::sc_unsigned &, int>(static_cast<Fn_xor_078>(&sc_dt::operator^), source);
}
inline std::string case_xor_080(Sources &source)
{
  return invoke<Fn_xor_079, const sc_dt::sc_unsigned &, long>(static_cast<Fn_xor_079>(&sc_dt::operator^), source);
}
inline std::string case_xor_081(Sources &source)
{
  return invoke<Fn_xor_080, const sc_dt::sc_unsigned &, sc_dt::int64>(static_cast<Fn_xor_080>(&sc_dt::operator^),
                                                                      source);
}
inline std::string case_xor_082(Sources &source)
{
  return invoke<Fn_xor_081, int, const sc_dt::sc_signed &>(static_cast<Fn_xor_081>(&sc_dt::operator^), source);
}
inline std::string case_xor_083(Sources &source)
{
  return invoke<Fn_xor_082, int, const sc_dt::sc_unsigned &>(static_cast<Fn_xor_082>(&sc_dt::operator^), source);
}
inline std::string case_xor_084(Sources &source)
{
  return invoke<Fn_xor_083, long, const sc_dt::sc_signed &>(static_cast<Fn_xor_083>(&sc_dt::operator^), source);
}
inline std::string case_xor_085(Sources &source)
{
  return invoke<Fn_xor_084, long, const sc_dt::sc_unsigned &>(static_cast<Fn_xor_084>(&sc_dt::operator^), source);
}
inline std::string case_xor_086(Sources &source)
{
  return invoke<Fn_xor_085, sc_dt::int64, const sc_dt::sc_signed &>(static_cast<Fn_xor_085>(&sc_dt::operator^), source);
}
inline std::string case_xor_087(Sources &source)
{
  return invoke<Fn_xor_086, sc_dt::int64, const sc_dt::sc_unsigned &>(static_cast<Fn_xor_086>(&sc_dt::operator^),
                                                                      source);
}
inline std::string case_xor_088(Sources &source)
{
  return invoke<Fn_xor_087, sc_dt::uint64, const sc_dt::sc_signed &>(static_cast<Fn_xor_087>(&sc_dt::operator^),
                                                                     source);
}
inline std::string case_xor_089(Sources &source)
{
  return invoke<Fn_xor_088, unsigned int, const sc_dt::sc_signed &>(static_cast<Fn_xor_088>(&sc_dt::operator^), source);
}
inline std::string case_xor_090(Sources &source)
{
  return invoke<Fn_xor_089, unsigned long, const sc_dt::sc_signed &>(static_cast<Fn_xor_089>(&sc_dt::operator^),
                                                                     source);
}
inline std::string case_xor_091(Sources &source)
{
  return invoke<Fn_xor_090, const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_xor_090>(&sc_dt::operator^), source);
}
inline std::string case_xor_092(Sources &source)
{
  return invoke<Fn_xor_091, const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &>(
    static_cast<Fn_xor_091>(&sc_dt::operator^), source);
}
inline std::string case_xor_093(Sources &source)
{
  return invoke<Fn_xor_092, const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &>(
    static_cast<Fn_xor_092>(&sc_dt::operator^), source);
}
inline std::string case_xor_094(Sources &source)
{
  return invoke<Fn_xor_093, const sc_dt::sc_unsigned &, sc_dt::uint64>(static_cast<Fn_xor_093>(&sc_dt::operator^),
                                                                       source);
}
inline std::string case_xor_095(Sources &source)
{
  return invoke<Fn_xor_094, const sc_dt::sc_unsigned &, unsigned int>(static_cast<Fn_xor_094>(&sc_dt::operator^),
                                                                      source);
}
inline std::string case_xor_096(Sources &source)
{
  return invoke<Fn_xor_095, const sc_dt::sc_unsigned &, unsigned long>(static_cast<Fn_xor_095>(&sc_dt::operator^),
                                                                       source);
}
inline std::string case_xor_097(Sources &source)
{
  return invoke<Fn_xor_096, sc_dt::uint64, const sc_dt::sc_unsigned &>(static_cast<Fn_xor_096>(&sc_dt::operator^),
                                                                       source);
}
inline std::string case_xor_098(Sources &source)
{
  return invoke<Fn_xor_097, unsigned int, const sc_dt::sc_unsigned &>(static_cast<Fn_xor_097>(&sc_dt::operator^),
                                                                      source);
}
inline std::string case_xor_099(Sources &source)
{
  return invoke<Fn_xor_098, unsigned long, const sc_dt::sc_unsigned &>(static_cast<Fn_xor_098>(&sc_dt::operator^),
                                                                       source);
}
/** @brief Invoke one exact native operator declaration by family and index. */
inline std::string native_case(const std::string &family, unsigned index, Sources &source)
{
  if (family == "sc_dt::operator%")
  {
    switch (index)
    {
    case 0:
      return case_mod_000(source);
    case 1:
      return case_mod_001(source);
    case 2:
      return case_mod_002(source);
    case 3:
      return case_mod_003(source);
    case 4:
      return case_mod_004(source);
    case 5:
      return case_mod_005(source);
    case 6:
      return case_mod_006(source);
    case 7:
      return case_mod_007(source);
    case 8:
      return case_mod_008(source);
    case 9:
      return case_mod_009(source);
    case 10:
      return case_mod_010(source);
    case 11:
      return case_mod_011(source);
    case 12:
      return case_mod_012(source);
    case 13:
      return case_mod_013(source);
    case 14:
      return case_mod_014(source);
    case 15:
      return case_mod_015(source);
    case 16:
      return case_mod_016(source);
    case 17:
      return case_mod_017(source);
    case 18:
      return case_mod_018(source);
    case 19:
      return case_mod_019(source);
    case 20:
      return case_mod_020(source);
    case 21:
      return case_mod_021(source);
    case 22:
      return case_mod_022(source);
    case 23:
      return case_mod_023(source);
    case 24:
      return case_mod_024(source);
    case 25:
      return case_mod_025(source);
    case 26:
      return case_mod_026(source);
    case 27:
      return case_mod_027(source);
    case 28:
      return case_mod_028(source);
    case 29:
      return case_mod_029(source);
    case 30:
      return case_mod_030(source);
    case 31:
      return case_mod_031(source);
    case 32:
      return case_mod_032(source);
    case 33:
      return case_mod_033(source);
    case 34:
      return case_mod_034(source);
    case 35:
      return case_mod_035(source);
    case 36:
      return case_mod_036(source);
    case 37:
      return case_mod_037(source);
    case 38:
      return case_mod_038(source);
    case 39:
      return case_mod_039(source);
    case 40:
      return case_mod_040(source);
    case 41:
      return case_mod_041(source);
    case 42:
      return case_mod_042(source);
    case 43:
      return case_mod_043(source);
    case 44:
      return case_mod_044(source);
    case 45:
      return case_mod_045(source);
    case 46:
      return case_mod_046(source);
    case 47:
      return case_mod_047(source);
    case 48:
      return case_mod_048(source);
    case 49:
      return case_mod_049(source);
    case 50:
      return case_mod_050(source);
    case 51:
      return case_mod_051(source);
    case 52:
      return case_mod_052(source);
    case 53:
      return case_mod_053(source);
    case 54:
      return case_mod_054(source);
    case 55:
      return case_mod_055(source);
    case 56:
      return case_mod_056(source);
    case 57:
      return case_mod_057(source);
    case 58:
      return case_mod_058(source);
    default:
      throw std::out_of_range("unknown binary case index");
    }
  }
  if (family == "sc_dt::operator&")
  {
    switch (index)
    {
    case 0:
      return case_and_000(source);
    case 1:
      return case_and_001(source);
    case 2:
      return case_and_002(source);
    case 3:
      return case_and_003(source);
    case 4:
      return case_and_004(source);
    case 5:
      return case_and_005(source);
    case 6:
      return case_and_006(source);
    case 7:
      return case_and_007(source);
    case 8:
      return case_and_008(source);
    case 9:
      return case_and_009(source);
    case 10:
      return case_and_010(source);
    case 11:
      return case_and_011(source);
    case 12:
      return case_and_012(source);
    case 13:
      return case_and_013(source);
    case 14:
      return case_and_014(source);
    case 15:
      return case_and_015(source);
    case 16:
      return case_and_016(source);
    case 17:
      return case_and_017(source);
    case 18:
      return case_and_018(source);
    case 19:
      return case_and_019(source);
    case 20:
      return case_and_020(source);
    case 21:
      return case_and_021(source);
    case 22:
      return case_and_022(source);
    case 23:
      return case_and_023(source);
    case 24:
      return case_and_024(source);
    case 25:
      return case_and_025(source);
    case 26:
      return case_and_026(source);
    case 27:
      return case_and_027(source);
    case 28:
      return case_and_028(source);
    case 29:
      return case_and_029(source);
    case 30:
      return case_and_030(source);
    case 31:
      return case_and_031(source);
    case 32:
      return case_and_032(source);
    case 33:
      return case_and_033(source);
    case 34:
      return case_and_034(source);
    case 35:
      return case_and_035(source);
    case 36:
      return case_and_036(source);
    case 37:
      return case_and_037(source);
    case 38:
      return case_and_038(source);
    case 39:
      return case_and_039(source);
    case 40:
      return case_and_040(source);
    case 41:
      return case_and_041(source);
    case 42:
      return case_and_042(source);
    case 43:
      return case_and_043(source);
    case 44:
      return case_and_044(source);
    case 45:
      return case_and_045(source);
    case 46:
      return case_and_046(source);
    case 47:
      return case_and_047(source);
    case 48:
      return case_and_048(source);
    case 49:
      return case_and_049(source);
    case 50:
      return case_and_050(source);
    case 51:
      return case_and_051(source);
    case 52:
      return case_and_052(source);
    case 53:
      return case_and_053(source);
    case 54:
      return case_and_054(source);
    case 55:
      return case_and_055(source);
    case 56:
      return case_and_056(source);
    case 57:
      return case_and_057(source);
    case 58:
      return case_and_058(source);
    case 59:
      return case_and_059(source);
    case 60:
      return case_and_060(source);
    case 61:
      return case_and_061(source);
    case 62:
      return case_and_062(source);
    case 63:
      return case_and_063(source);
    case 64:
      return case_and_064(source);
    case 65:
      return case_and_065(source);
    case 66:
      return case_and_066(source);
    case 67:
      return case_and_067(source);
    case 68:
      return case_and_068(source);
    case 69:
      return case_and_069(source);
    case 70:
      return case_and_070(source);
    case 71:
      return case_and_071(source);
    case 72:
      return case_and_072(source);
    case 73:
      return case_and_073(source);
    case 74:
      return case_and_074(source);
    case 75:
      return case_and_075(source);
    case 76:
      return case_and_076(source);
    case 77:
      return case_and_077(source);
    case 78:
      return case_and_078(source);
    case 79:
      return case_and_079(source);
    case 80:
      return case_and_080(source);
    case 81:
      return case_and_081(source);
    case 82:
      return case_and_082(source);
    case 83:
      return case_and_083(source);
    case 84:
      return case_and_084(source);
    case 85:
      return case_and_085(source);
    case 86:
      return case_and_086(source);
    case 87:
      return case_and_087(source);
    case 88:
      return case_and_088(source);
    case 89:
      return case_and_089(source);
    case 90:
      return case_and_090(source);
    case 91:
      return case_and_091(source);
    case 92:
      return case_and_092(source);
    case 93:
      return case_and_093(source);
    case 94:
      return case_and_094(source);
    case 95:
      return case_and_095(source);
    case 96:
      return case_and_096(source);
    case 97:
      return case_and_097(source);
    case 98:
      return case_and_098(source);
    case 99:
      return case_and_099(source);
    default:
      throw std::out_of_range("unknown binary case index");
    }
  }
  if (family == "sc_dt::operator|")
  {
    switch (index)
    {
    case 0:
      return case_or_000(source);
    case 1:
      return case_or_001(source);
    case 2:
      return case_or_002(source);
    case 3:
      return case_or_003(source);
    case 4:
      return case_or_004(source);
    case 5:
      return case_or_005(source);
    case 6:
      return case_or_006(source);
    case 7:
      return case_or_007(source);
    case 8:
      return case_or_008(source);
    case 9:
      return case_or_009(source);
    case 10:
      return case_or_010(source);
    case 11:
      return case_or_011(source);
    case 12:
      return case_or_012(source);
    case 13:
      return case_or_013(source);
    case 14:
      return case_or_014(source);
    case 15:
      return case_or_015(source);
    case 16:
      return case_or_016(source);
    case 17:
      return case_or_017(source);
    case 18:
      return case_or_018(source);
    case 19:
      return case_or_019(source);
    case 20:
      return case_or_020(source);
    case 21:
      return case_or_021(source);
    case 22:
      return case_or_022(source);
    case 23:
      return case_or_023(source);
    case 24:
      return case_or_024(source);
    case 25:
      return case_or_025(source);
    case 26:
      return case_or_026(source);
    case 27:
      return case_or_027(source);
    case 28:
      return case_or_028(source);
    case 29:
      return case_or_029(source);
    case 30:
      return case_or_030(source);
    case 31:
      return case_or_031(source);
    case 32:
      return case_or_032(source);
    case 33:
      return case_or_033(source);
    case 34:
      return case_or_034(source);
    case 35:
      return case_or_035(source);
    case 36:
      return case_or_036(source);
    case 37:
      return case_or_037(source);
    case 38:
      return case_or_038(source);
    case 39:
      return case_or_039(source);
    case 40:
      return case_or_040(source);
    case 41:
      return case_or_041(source);
    case 42:
      return case_or_042(source);
    case 43:
      return case_or_043(source);
    case 44:
      return case_or_044(source);
    case 45:
      return case_or_045(source);
    case 46:
      return case_or_046(source);
    case 47:
      return case_or_047(source);
    case 48:
      return case_or_048(source);
    case 49:
      return case_or_049(source);
    case 50:
      return case_or_050(source);
    case 51:
      return case_or_051(source);
    case 52:
      return case_or_052(source);
    case 53:
      return case_or_053(source);
    case 54:
      return case_or_054(source);
    case 55:
      return case_or_055(source);
    case 56:
      return case_or_056(source);
    case 57:
      return case_or_057(source);
    case 58:
      return case_or_058(source);
    case 59:
      return case_or_059(source);
    case 60:
      return case_or_060(source);
    case 61:
      return case_or_061(source);
    case 62:
      return case_or_062(source);
    case 63:
      return case_or_063(source);
    case 64:
      return case_or_064(source);
    case 65:
      return case_or_065(source);
    case 66:
      return case_or_066(source);
    case 67:
      return case_or_067(source);
    case 68:
      return case_or_068(source);
    case 69:
      return case_or_069(source);
    case 70:
      return case_or_070(source);
    case 71:
      return case_or_071(source);
    case 72:
      return case_or_072(source);
    case 73:
      return case_or_073(source);
    case 74:
      return case_or_074(source);
    case 75:
      return case_or_075(source);
    case 76:
      return case_or_076(source);
    case 77:
      return case_or_077(source);
    case 78:
      return case_or_078(source);
    case 79:
      return case_or_079(source);
    case 80:
      return case_or_080(source);
    case 81:
      return case_or_081(source);
    case 82:
      return case_or_082(source);
    case 83:
      return case_or_083(source);
    case 84:
      return case_or_084(source);
    case 85:
      return case_or_085(source);
    case 86:
      return case_or_086(source);
    case 87:
      return case_or_087(source);
    case 88:
      return case_or_088(source);
    case 89:
      return case_or_089(source);
    case 90:
      return case_or_090(source);
    case 91:
      return case_or_091(source);
    case 92:
      return case_or_092(source);
    case 93:
      return case_or_093(source);
    case 94:
      return case_or_094(source);
    case 95:
      return case_or_095(source);
    case 96:
      return case_or_096(source);
    case 97:
      return case_or_097(source);
    case 98:
      return case_or_098(source);
    case 99:
      return case_or_099(source);
    default:
      throw std::out_of_range("unknown binary case index");
    }
  }
  if (family == "sc_dt::operator^")
  {
    switch (index)
    {
    case 0:
      return case_xor_000(source);
    case 1:
      return case_xor_001(source);
    case 2:
      return case_xor_002(source);
    case 3:
      return case_xor_003(source);
    case 4:
      return case_xor_004(source);
    case 5:
      return case_xor_005(source);
    case 6:
      return case_xor_006(source);
    case 7:
      return case_xor_007(source);
    case 8:
      return case_xor_008(source);
    case 9:
      return case_xor_009(source);
    case 10:
      return case_xor_010(source);
    case 11:
      return case_xor_011(source);
    case 12:
      return case_xor_012(source);
    case 13:
      return case_xor_013(source);
    case 14:
      return case_xor_014(source);
    case 15:
      return case_xor_015(source);
    case 16:
      return case_xor_016(source);
    case 17:
      return case_xor_017(source);
    case 18:
      return case_xor_018(source);
    case 19:
      return case_xor_019(source);
    case 20:
      return case_xor_020(source);
    case 21:
      return case_xor_021(source);
    case 22:
      return case_xor_022(source);
    case 23:
      return case_xor_023(source);
    case 24:
      return case_xor_024(source);
    case 25:
      return case_xor_025(source);
    case 26:
      return case_xor_026(source);
    case 27:
      return case_xor_027(source);
    case 28:
      return case_xor_028(source);
    case 29:
      return case_xor_029(source);
    case 30:
      return case_xor_030(source);
    case 31:
      return case_xor_031(source);
    case 32:
      return case_xor_032(source);
    case 33:
      return case_xor_033(source);
    case 34:
      return case_xor_034(source);
    case 35:
      return case_xor_035(source);
    case 36:
      return case_xor_036(source);
    case 37:
      return case_xor_037(source);
    case 38:
      return case_xor_038(source);
    case 39:
      return case_xor_039(source);
    case 40:
      return case_xor_040(source);
    case 41:
      return case_xor_041(source);
    case 42:
      return case_xor_042(source);
    case 43:
      return case_xor_043(source);
    case 44:
      return case_xor_044(source);
    case 45:
      return case_xor_045(source);
    case 46:
      return case_xor_046(source);
    case 47:
      return case_xor_047(source);
    case 48:
      return case_xor_048(source);
    case 49:
      return case_xor_049(source);
    case 50:
      return case_xor_050(source);
    case 51:
      return case_xor_051(source);
    case 52:
      return case_xor_052(source);
    case 53:
      return case_xor_053(source);
    case 54:
      return case_xor_054(source);
    case 55:
      return case_xor_055(source);
    case 56:
      return case_xor_056(source);
    case 57:
      return case_xor_057(source);
    case 58:
      return case_xor_058(source);
    case 59:
      return case_xor_059(source);
    case 60:
      return case_xor_060(source);
    case 61:
      return case_xor_061(source);
    case 62:
      return case_xor_062(source);
    case 63:
      return case_xor_063(source);
    case 64:
      return case_xor_064(source);
    case 65:
      return case_xor_065(source);
    case 66:
      return case_xor_066(source);
    case 67:
      return case_xor_067(source);
    case 68:
      return case_xor_068(source);
    case 69:
      return case_xor_069(source);
    case 70:
      return case_xor_070(source);
    case 71:
      return case_xor_071(source);
    case 72:
      return case_xor_072(source);
    case 73:
      return case_xor_073(source);
    case 74:
      return case_xor_074(source);
    case 75:
      return case_xor_075(source);
    case 76:
      return case_xor_076(source);
    case 77:
      return case_xor_077(source);
    case 78:
      return case_xor_078(source);
    case 79:
      return case_xor_079(source);
    case 80:
      return case_xor_080(source);
    case 81:
      return case_xor_081(source);
    case 82:
      return case_xor_082(source);
    case 83:
      return case_xor_083(source);
    case 84:
      return case_xor_084(source);
    case 85:
      return case_xor_085(source);
    case 86:
      return case_xor_086(source);
    case 87:
      return case_xor_087(source);
    case 88:
      return case_xor_088(source);
    case 89:
      return case_xor_089(source);
    case 90:
      return case_xor_090(source);
    case 91:
      return case_xor_091(source);
    case 92:
      return case_xor_092(source);
    case 93:
      return case_xor_093(source);
    case 94:
      return case_xor_094(source);
    case 95:
      return case_xor_095(source);
    case 96:
      return case_xor_096(source);
    case 97:
      return case_xor_097(source);
    case 98:
      return case_xor_098(source);
    case 99:
      return case_xor_099(source);
    default:
      throw std::out_of_range("unknown binary case index");
    }
  }
  throw std::out_of_range("unknown binary operator family");
}
inline unsigned native_case_count(const std::string &family)
{
  if (family == "sc_dt::operator%") return 59;
  if (family == "sc_dt::operator&") return 100;
  if (family == "sc_dt::operator|") return 100;
  if (family == "sc_dt::operator^") return 100;
  throw std::out_of_range("unknown binary operator family");
}
inline constexpr unsigned exact_overload_count = 359;
}  // namespace sc_dt_integer_binary_probe

/** @file sc_dt_arithmetic_operators_probe.h
 * @brief Exact function-pointer references for public sc_dt arithmetic operators.
 * @details Each selected call borrows its live operands for one GIL-held
 * expression and returns an independently owned native value. Concatenation
 * views retain their parent integers through the call; fast fixed-point values
 * are rendered through their exact fast API. No callback, scheduler or kernel
 * state is retained.
 */
#pragma once
#ifndef SC_INCLUDE_FX
#define SC_INCLUDE_FX
#endif
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>

#include "sc_dt_integer_binary_probe.h"

namespace sc_dt_arithmetic_operators_probe
{
using Bool = bool;
using SignedInt = int;
using UnsignedInt = unsigned int;
using SignedLong = long;
using UnsignedLong = unsigned long;
using RealFloat = float;
using RealDouble = double;
using Signed64 = sc_dt::int64;
using Unsigned64 = sc_dt::uint64;

/** @brief Own wide numeric sources and all borrowed operator inputs. */
struct Sources
{
  sc_dt::sc_fxval value;
  sc_dt::sc_fxval_fast value_fast;
  sc_dt::sc_fixed<191, 130> number;
  sc_dt::sc_fixed_fast<191, 130> number_fast;
  sc_dt::sc_int<63> sint;
  sc_dt::sc_uint<63> uint;
  sc_dt::sc_bigint<191> signed_value;
  sc_dt::sc_biguint<191> unsigned_value;
  sc_dt::sc_string_old old_string;
  sc_dt_integer_binary_probe::Sources bits;

  /** @brief Construct nonzero, wide operands without relying on cppyy field initialization. */
  Sources()
    : value("123.125")
    , value_fast("2.718281828459045")
    , number("1267650600228229401496703205376.125")
    , number_fast("3.125")
    , sint("-9007199254740993")
    , uint("9007199254740995")
    , signed_value("-1267650600228229401496703205377")
    , unsigned_value("1393796574908163946345982392040522594123781")
    , old_string("old-systemc")
  {
  }

  /** @brief Return signed SystemC integers while retaining their 191-bit owners. */
  static const sc_dt::sc_signed &signed_source(Sources &s)
  {
    return s.signed_value;
  }
  /** @brief Return unsigned SystemC integers while retaining their 191-bit owners. */
  static const sc_dt::sc_unsigned &unsigned_source(Sources &s)
  {
    return s.unsigned_value;
  }
  /** @brief Return the borrowed concatenation with both parents kept live. */
  static const sc_dt::sc_concatref &concat_source(Sources &s)
  {
    return s.bits.concat_value;
  }
};

/** @brief Python-callable typed source views for native base classes and scalars. */
struct SourceView
{
  static const sc_dt::sc_fxnum &fxnum(Sources &s)
  {
    return s.number;
  }
  static const sc_dt::sc_fxnum_fast &fxnum_fast(Sources &s)
  {
    return s.number_fast;
  }
  static const sc_dt::sc_fxval &fxval(Sources &s)
  {
    return s.value;
  }
  static const sc_dt::sc_fxval_fast &fxval_fast(Sources &s)
  {
    return s.value_fast;
  }
  static const sc_dt::sc_int_base &sint(Sources &s)
  {
    return s.sint;
  }
  static const sc_dt::sc_uint_base &uint(Sources &s)
  {
    return s.uint;
  }
  static const sc_dt::sc_signed &signed_value(Sources &s)
  {
    return s.signed_value;
  }
  static const sc_dt::sc_unsigned &unsigned_value(Sources &s)
  {
    return s.unsigned_value;
  }
  static const sc_dt::sc_concatref &concat(Sources &s)
  {
    return s.bits.concat_value;
  }
  static const sc_dt::sc_string_old &old_string(Sources &s)
  {
    return s.old_string;
  }
  static Bool boolean()
  {
    return true;
  }
  static const char *text()
  {
    return "7.8125";
  }
  static RealDouble real_double()
  {
    return 2.625;
  }
  static RealFloat real_float()
  {
    return 1.375F;
  }
  static SignedInt integer()
  {
    return 23;
  }
  static SignedLong long_integer()
  {
    return 12345L;
  }
  static Signed64 signed_64()
  {
    return static_cast<sc_dt::int64>(-1234567890123LL);
  }
  static Unsigned64 unsigned_64()
  {
    return static_cast<sc_dt::uint64>(0xfedcba9876543210ULL);
  }
  static UnsignedInt unsigned_integer()
  {
    return 29U;
  }
  static UnsignedLong unsigned_long_integer()
  {
    return 65535UL;
  }
};

template <class T>
using bare_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <class Argument>
decltype(auto) operand(Sources &s)
{
  using T = bare_t<Argument>;
  if constexpr (std::is_same_v<T, sc_dt::sc_fxnum>)
    return static_cast<Argument>(s.number);
  else if constexpr (std::is_same_v<T, sc_dt::sc_fxnum_fast>)
    return static_cast<Argument>(s.number_fast);
  else if constexpr (std::is_same_v<T, sc_dt::sc_fxval>)
    return static_cast<Argument>(s.value);
  else if constexpr (std::is_same_v<T, sc_dt::sc_fxval_fast>)
    return static_cast<Argument>(s.value_fast);
  else if constexpr (std::is_same_v<T, sc_dt::sc_int_base>)
    return static_cast<Argument>(s.sint);
  else if constexpr (std::is_same_v<T, sc_dt::sc_uint_base>)
    return static_cast<Argument>(s.uint);
  else if constexpr (std::is_same_v<T, sc_dt::sc_signed>)
    return static_cast<Argument>(s.signed_value);
  else if constexpr (std::is_same_v<T, sc_dt::sc_unsigned>)
    return static_cast<Argument>(s.unsigned_value);
  else if constexpr (std::is_same_v<T, sc_dt::sc_concatref>)
    return static_cast<Argument>(s.bits.concat_value);
  else if constexpr (std::is_same_v<T, sc_dt::sc_string_old>)
    return static_cast<Argument>(s.old_string);
  else if constexpr (std::is_same_v<T, const char *>)
    return "7.8125";
  else if constexpr (std::is_same_v<T, bool>)
    return true;
  else if constexpr (std::is_same_v<T, int>)
    return 23;
  else if constexpr (std::is_same_v<T, unsigned int>)
    return 29U;
  else if constexpr (std::is_same_v<T, long>)
    return 12345L;
  else if constexpr (std::is_same_v<T, unsigned long>)
    return 65535UL;
  else if constexpr (std::is_same_v<T, float>)
    return 1.375F;
  else if constexpr (std::is_same_v<T, double>)
    return 2.625;
  else if constexpr (std::is_same_v<T, sc_dt::int64>)
    return static_cast<sc_dt::int64>(-1234567890123LL);
  else if constexpr (std::is_same_v<T, sc_dt::uint64>)
    return static_cast<sc_dt::uint64>(0xfedcba9876543210ULL);
  else
    static_assert(!sizeof(T), "arithmetic operator probe lacks a typed source factory");
}

template <class T>
std::string type_name()
{
  using V = bare_t<T>;
  if constexpr (std::is_same_v<V, sc_dt::sc_fxval>)
    return "sc_dt::sc_fxval";
  else if constexpr (std::is_same_v<V, sc_dt::sc_fxval_fast>)
    return "sc_dt::sc_fxval_fast";
  else if constexpr (std::is_same_v<V, sc_dt::sc_signed>)
    return "sc_dt::sc_signed";
  else if constexpr (std::is_same_v<V, sc_dt::sc_unsigned>)
    return "sc_dt::sc_unsigned";
  else if constexpr (std::is_same_v<V, sc_dt::sc_string_old>)
    return "sc_dt::sc_string_old";
  else
    return "<unexpected-native-result-type>";
}

template <class T>
std::string render(const T &value)
{
  using V = bare_t<T>;
  if constexpr (std::is_same_v<V, sc_dt::sc_fxval> || std::is_same_v<V, sc_dt::sc_fxval_fast>)
  {
    std::ostringstream out;
    out << std::setprecision(17) << value.to_double() << '|' << value.to_string(sc_dt::SC_DEC) << '|'
        << int(value.is_neg()) << '|' << int(value.is_zero()) << '|' << int(value.is_nan()) << '|'
        << int(value.is_inf()) << '|' << int(value.is_normal()) << '|' << int(value.rounding_flag());
    return out.str();
  }
  else if constexpr (std::is_same_v<V, sc_dt::sc_signed> || std::is_same_v<V, sc_dt::sc_unsigned>)
    return value.to_string(sc_dt::SC_BIN) + ":" + std::to_string(value.length());
  else if constexpr (std::is_same_v<V, sc_dt::sc_string_old>)
    return value.c_str();
  else
    static_assert(!sizeof(V), "arithmetic operator probe lacks a result renderer");
}

template <int Operation, std::size_t Index>
struct NativeCase;

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 0 && SC_DT_ARITHMETIC_END > 0
template <>
struct NativeCase<0, 0>
{
  using Function = const sc_dt::sc_fxval (*)(const char *, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const char *, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 1 && SC_DT_ARITHMETIC_END > 1
template <>
struct NativeCase<0, 1>
{
  using Function = const sc_dt::sc_fxval (*)(const char *, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const char *, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 2 && SC_DT_ARITHMETIC_END > 2
template <>
struct NativeCase<0, 2>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 3 && SC_DT_ARITHMETIC_END > 3
template <>
struct NativeCase<0, 3>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 4 && SC_DT_ARITHMETIC_END > 4
template <>
struct NativeCase<0, 4>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 5 && SC_DT_ARITHMETIC_END > 5
template <>
struct NativeCase<0, 5>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 6 && SC_DT_ARITHMETIC_END > 6
template <>
struct NativeCase<0, 6>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 7 && SC_DT_ARITHMETIC_END > 7
template <>
struct NativeCase<0, 7>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 8 && SC_DT_ARITHMETIC_END > 8
template <>
struct NativeCase<0, 8>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 9 && SC_DT_ARITHMETIC_END > 9
template <>
struct NativeCase<0, 9>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 10 && SC_DT_ARITHMETIC_END > 10
template <>
struct NativeCase<0, 10>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 11 && SC_DT_ARITHMETIC_END > 11
template <>
struct NativeCase<0, 11>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 12 && SC_DT_ARITHMETIC_END > 12
template <>
struct NativeCase<0, 12>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 13 && SC_DT_ARITHMETIC_END > 13
template <>
struct NativeCase<0, 13>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 14 && SC_DT_ARITHMETIC_END > 14
template <>
struct NativeCase<0, 14>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 15 && SC_DT_ARITHMETIC_END > 15
template <>
struct NativeCase<0, 15>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 16 && SC_DT_ARITHMETIC_END > 16
template <>
struct NativeCase<0, 16>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 17 && SC_DT_ARITHMETIC_END > 17
template <>
struct NativeCase<0, 17>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 18 && SC_DT_ARITHMETIC_END > 18
template <>
struct NativeCase<0, 18>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 19 && SC_DT_ARITHMETIC_END > 19
template <>
struct NativeCase<0, 19>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 20 && SC_DT_ARITHMETIC_END > 20
template <>
struct NativeCase<0, 20>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 21 && SC_DT_ARITHMETIC_END > 21
template <>
struct NativeCase<0, 21>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 22 && SC_DT_ARITHMETIC_END > 22
template <>
struct NativeCase<0, 22>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 23 && SC_DT_ARITHMETIC_END > 23
template <>
struct NativeCase<0, 23>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 24 && SC_DT_ARITHMETIC_END > 24
template <>
struct NativeCase<0, 24>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 25 && SC_DT_ARITHMETIC_END > 25
template <>
struct NativeCase<0, 25>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 26 && SC_DT_ARITHMETIC_END > 26
template <>
struct NativeCase<0, 26>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 27 && SC_DT_ARITHMETIC_END > 27
template <>
struct NativeCase<0, 27>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 28 && SC_DT_ARITHMETIC_END > 28
template <>
struct NativeCase<0, 28>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 29 && SC_DT_ARITHMETIC_END > 29
template <>
struct NativeCase<0, 29>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 30 && SC_DT_ARITHMETIC_END > 30
template <>
struct NativeCase<0, 30>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 31 && SC_DT_ARITHMETIC_END > 31
template <>
struct NativeCase<0, 31>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 32 && SC_DT_ARITHMETIC_END > 32
template <>
struct NativeCase<0, 32>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 33 && SC_DT_ARITHMETIC_END > 33
template <>
struct NativeCase<0, 33>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 34 && SC_DT_ARITHMETIC_END > 34
template <>
struct NativeCase<0, 34>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 35 && SC_DT_ARITHMETIC_END > 35
template <>
struct NativeCase<0, 35>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 36 && SC_DT_ARITHMETIC_END > 36
template <>
struct NativeCase<0, 36>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 37 && SC_DT_ARITHMETIC_END > 37
template <>
struct NativeCase<0, 37>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 38 && SC_DT_ARITHMETIC_END > 38
template <>
struct NativeCase<0, 38>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 39 && SC_DT_ARITHMETIC_END > 39
template <>
struct NativeCase<0, 39>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 40 && SC_DT_ARITHMETIC_END > 40
template <>
struct NativeCase<0, 40>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 41 && SC_DT_ARITHMETIC_END > 41
template <>
struct NativeCase<0, 41>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_int_base &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 42 && SC_DT_ARITHMETIC_END > 42
template <>
struct NativeCase<0, 42>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_signed &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 43 && SC_DT_ARITHMETIC_END > 43
template <>
struct NativeCase<0, 43>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_signed &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 44 && SC_DT_ARITHMETIC_END > 44
template <>
struct NativeCase<0, 44>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 45 && SC_DT_ARITHMETIC_END > 45
template <>
struct NativeCase<0, 45>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 46 && SC_DT_ARITHMETIC_END > 46
template <>
struct NativeCase<0, 46>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 47 && SC_DT_ARITHMETIC_END > 47
template <>
struct NativeCase<0, 47>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 48 && SC_DT_ARITHMETIC_END > 48
template <>
struct NativeCase<0, 48>
{
  using Function = const sc_dt::sc_fxval (*)(double, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (double, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 49 && SC_DT_ARITHMETIC_END > 49
template <>
struct NativeCase<0, 49>
{
  using Function = const sc_dt::sc_fxval (*)(double, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (double, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 50 && SC_DT_ARITHMETIC_END > 50
template <>
struct NativeCase<0, 50>
{
  using Function = const sc_dt::sc_fxval (*)(float, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (float, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 51 && SC_DT_ARITHMETIC_END > 51
template <>
struct NativeCase<0, 51>
{
  using Function = const sc_dt::sc_fxval (*)(float, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (float, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 52 && SC_DT_ARITHMETIC_END > 52
template <>
struct NativeCase<0, 52>
{
  using Function = const sc_dt::sc_fxval (*)(int, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (int, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 53 && SC_DT_ARITHMETIC_END > 53
template <>
struct NativeCase<0, 53>
{
  using Function = const sc_dt::sc_fxval (*)(int, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (int, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 54 && SC_DT_ARITHMETIC_END > 54
template <>
struct NativeCase<0, 54>
{
  using Function = const sc_dt::sc_fxval (*)(long, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (long, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 55 && SC_DT_ARITHMETIC_END > 55
template <>
struct NativeCase<0, 55>
{
  using Function = const sc_dt::sc_fxval (*)(long, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (long, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 56 && SC_DT_ARITHMETIC_END > 56
template <>
struct NativeCase<0, 56>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::int64, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (sc_dt::int64, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 57 && SC_DT_ARITHMETIC_END > 57
template <>
struct NativeCase<0, 57>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::int64, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (sc_dt::int64, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 58 && SC_DT_ARITHMETIC_END > 58
template <>
struct NativeCase<0, 58>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::uint64, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (sc_dt::uint64, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 59 && SC_DT_ARITHMETIC_END > 59
template <>
struct NativeCase<0, 59>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::uint64, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (sc_dt::uint64, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 60 && SC_DT_ARITHMETIC_END > 60
template <>
struct NativeCase<0, 60>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned int, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (unsigned int, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 61 && SC_DT_ARITHMETIC_END > 61
template <>
struct NativeCase<0, 61>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned int, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (unsigned int, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 62 && SC_DT_ARITHMETIC_END > 62
template <>
struct NativeCase<0, 62>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned long, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (unsigned long, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 63 && SC_DT_ARITHMETIC_END > 63
template <>
struct NativeCase<0, 63>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned long, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval (unsigned long, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 64 && SC_DT_ARITHMETIC_END > 64
template <>
struct NativeCase<0, 64>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const char *, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const char *, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 65 && SC_DT_ARITHMETIC_END > 65
template <>
struct NativeCase<0, 65>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const char *, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const char *, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 66 && SC_DT_ARITHMETIC_END > 66
template <>
struct NativeCase<0, 66>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 67 && SC_DT_ARITHMETIC_END > 67
template <>
struct NativeCase<0, 67>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 68 && SC_DT_ARITHMETIC_END > 68
template <>
struct NativeCase<0, 68>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 69 && SC_DT_ARITHMETIC_END > 69
template <>
struct NativeCase<0, 69>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 70 && SC_DT_ARITHMETIC_END > 70
template <>
struct NativeCase<0, 70>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 71 && SC_DT_ARITHMETIC_END > 71
template <>
struct NativeCase<0, 71>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 72 && SC_DT_ARITHMETIC_END > 72
template <>
struct NativeCase<0, 72>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 73 && SC_DT_ARITHMETIC_END > 73
template <>
struct NativeCase<0, 73>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 74 && SC_DT_ARITHMETIC_END > 74
template <>
struct NativeCase<0, 74>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 75 && SC_DT_ARITHMETIC_END > 75
template <>
struct NativeCase<0, 75>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 76 && SC_DT_ARITHMETIC_END > 76
template <>
struct NativeCase<0, 76>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 77 && SC_DT_ARITHMETIC_END > 77
template <>
struct NativeCase<0, 77>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 78 && SC_DT_ARITHMETIC_END > 78
template <>
struct NativeCase<0, 78>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 79 && SC_DT_ARITHMETIC_END > 79
template <>
struct NativeCase<0, 79>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 80 && SC_DT_ARITHMETIC_END > 80
template <>
struct NativeCase<0, 80>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 81 && SC_DT_ARITHMETIC_END > 81
template <>
struct NativeCase<0, 81>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 82 && SC_DT_ARITHMETIC_END > 82
template <>
struct NativeCase<0, 82>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 83 && SC_DT_ARITHMETIC_END > 83
template <>
struct NativeCase<0, 83>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 84 && SC_DT_ARITHMETIC_END > 84
template <>
struct NativeCase<0, 84>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 85 && SC_DT_ARITHMETIC_END > 85
template <>
struct NativeCase<0, 85>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 86 && SC_DT_ARITHMETIC_END > 86
template <>
struct NativeCase<0, 86>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 87 && SC_DT_ARITHMETIC_END > 87
template <>
struct NativeCase<0, 87>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 88 && SC_DT_ARITHMETIC_END > 88
template <>
struct NativeCase<0, 88>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 89 && SC_DT_ARITHMETIC_END > 89
template <>
struct NativeCase<0, 89>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 90 && SC_DT_ARITHMETIC_END > 90
template <>
struct NativeCase<0, 90>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 91 && SC_DT_ARITHMETIC_END > 91
template <>
struct NativeCase<0, 91>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 92 && SC_DT_ARITHMETIC_END > 92
template <>
struct NativeCase<0, 92>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 93 && SC_DT_ARITHMETIC_END > 93
template <>
struct NativeCase<0, 93>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 94 && SC_DT_ARITHMETIC_END > 94
template <>
struct NativeCase<0, 94>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 95 && SC_DT_ARITHMETIC_END > 95
template <>
struct NativeCase<0, 95>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 96 && SC_DT_ARITHMETIC_END > 96
template <>
struct NativeCase<0, 96>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_int_base &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 97 && SC_DT_ARITHMETIC_END > 97
template <>
struct NativeCase<0, 97>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_int_base &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 98 && SC_DT_ARITHMETIC_END > 98
template <>
struct NativeCase<0, 98>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_signed &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 99 && SC_DT_ARITHMETIC_END > 99
template <>
struct NativeCase<0, 99>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_signed &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 100 && SC_DT_ARITHMETIC_END > 100
template <>
struct NativeCase<0, 100>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 101 && SC_DT_ARITHMETIC_END > 101
template <>
struct NativeCase<0, 101>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 102 && SC_DT_ARITHMETIC_END > 102
template <>
struct NativeCase<0, 102>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_unsigned &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 103 && SC_DT_ARITHMETIC_END > 103
template <>
struct NativeCase<0, 103>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (const sc_dt::sc_unsigned &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 104 && SC_DT_ARITHMETIC_END > 104
template <>
struct NativeCase<0, 104>
{
  using Function = const sc_dt::sc_fxval_fast (*)(double, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (double, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 105 && SC_DT_ARITHMETIC_END > 105
template <>
struct NativeCase<0, 105>
{
  using Function = const sc_dt::sc_fxval_fast (*)(double, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (double, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 106 && SC_DT_ARITHMETIC_END > 106
template <>
struct NativeCase<0, 106>
{
  using Function = const sc_dt::sc_fxval_fast (*)(float, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (float, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 107 && SC_DT_ARITHMETIC_END > 107
template <>
struct NativeCase<0, 107>
{
  using Function = const sc_dt::sc_fxval_fast (*)(float, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (float, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 108 && SC_DT_ARITHMETIC_END > 108
template <>
struct NativeCase<0, 108>
{
  using Function = const sc_dt::sc_fxval_fast (*)(int, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (int, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 109 && SC_DT_ARITHMETIC_END > 109
template <>
struct NativeCase<0, 109>
{
  using Function = const sc_dt::sc_fxval_fast (*)(int, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (int, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 110 && SC_DT_ARITHMETIC_END > 110
template <>
struct NativeCase<0, 110>
{
  using Function = const sc_dt::sc_fxval_fast (*)(long, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (long, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 111 && SC_DT_ARITHMETIC_END > 111
template <>
struct NativeCase<0, 111>
{
  using Function = const sc_dt::sc_fxval_fast (*)(long, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (long, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 112 && SC_DT_ARITHMETIC_END > 112
template <>
struct NativeCase<0, 112>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::int64, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (sc_dt::int64, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 113 && SC_DT_ARITHMETIC_END > 113
template <>
struct NativeCase<0, 113>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::int64, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (sc_dt::int64, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 114 && SC_DT_ARITHMETIC_END > 114
template <>
struct NativeCase<0, 114>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (sc_dt::uint64, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 115 && SC_DT_ARITHMETIC_END > 115
template <>
struct NativeCase<0, 115>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::uint64, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (sc_dt::uint64, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 116 && SC_DT_ARITHMETIC_END > 116
template <>
struct NativeCase<0, 116>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned int, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (unsigned int, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 117 && SC_DT_ARITHMETIC_END > 117
template <>
struct NativeCase<0, 117>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned int, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (unsigned int, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 118 && SC_DT_ARITHMETIC_END > 118
template <>
struct NativeCase<0, 118>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned long, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (unsigned long, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 119 && SC_DT_ARITHMETIC_END > 119
template <>
struct NativeCase<0, 119>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned long, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_fxval_fast (unsigned long, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 120 && SC_DT_ARITHMETIC_END > 120
template <>
struct NativeCase<0, 120>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 121 && SC_DT_ARITHMETIC_END > 121
template <>
struct NativeCase<0, 121>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 122 && SC_DT_ARITHMETIC_END > 122
template <>
struct NativeCase<0, 122>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_signed (const sc_dt::sc_concatref &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 123 && SC_DT_ARITHMETIC_END > 123
template <>
struct NativeCase<0, 123>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_signed (const sc_dt::sc_concatref &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 124 && SC_DT_ARITHMETIC_END > 124
template <>
struct NativeCase<0, 124>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_signed (const sc_dt::sc_concatref &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 125 && SC_DT_ARITHMETIC_END > 125
template <>
struct NativeCase<0, 125>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 126 && SC_DT_ARITHMETIC_END > 126
template <>
struct NativeCase<0, 126>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 127 && SC_DT_ARITHMETIC_END > 127
template <>
struct NativeCase<0, 127>
{
  using Function = const sc_dt::sc_signed (*)(int, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_signed (int, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 128 && SC_DT_ARITHMETIC_END > 128
template <>
struct NativeCase<0, 128>
{
  using Function = const sc_dt::sc_signed (*)(long, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_signed (long, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 129 && SC_DT_ARITHMETIC_END > 129
template <>
struct NativeCase<0, 129>
{
  using Function = const sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 130 && SC_DT_ARITHMETIC_END > 130
template <>
struct NativeCase<0, 130>
{
  using Function = const sc_dt::sc_unsigned (*)(bool, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (bool, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<bool>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 131 && SC_DT_ARITHMETIC_END > 131
template <>
struct NativeCase<0, 131>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, bool);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, bool)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<bool>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 132 && SC_DT_ARITHMETIC_END > 132
template <>
struct NativeCase<0, 132>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
                "sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 133 && SC_DT_ARITHMETIC_END > 133
template <>
struct NativeCase<0, 133>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 134 && SC_DT_ARITHMETIC_END > 134
template <>
struct NativeCase<0, 134>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
                "sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 135 && SC_DT_ARITHMETIC_END > 135
template <>
struct NativeCase<0, 135>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 136 && SC_DT_ARITHMETIC_END > 136
template <>
struct NativeCase<0, 136>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 137 && SC_DT_ARITHMETIC_END > 137
template <>
struct NativeCase<0, 137>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 138 && SC_DT_ARITHMETIC_END > 138
template <>
struct NativeCase<0, 138>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 139 && SC_DT_ARITHMETIC_END > 139
template <>
struct NativeCase<0, 139>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const "
                "sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 140 && SC_DT_ARITHMETIC_END > 140
template <>
struct NativeCase<0, 140>
{
  using Function = const sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 141 && SC_DT_ARITHMETIC_END > 141
template <>
struct NativeCase<0, 141>
{
  using Function = const sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 142 && SC_DT_ARITHMETIC_END > 142
template <>
struct NativeCase<0, 142>
{
  using Function = const sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ const sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 143 && SC_DT_ARITHMETIC_END > 143
template <>
struct NativeCase<0, 143>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 144 && SC_DT_ARITHMETIC_END > 144
template <>
struct NativeCase<0, 144>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 145 && SC_DT_ARITHMETIC_END > 145
template <>
struct NativeCase<0, 145>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 146 && SC_DT_ARITHMETIC_END > 146
template <>
struct NativeCase<0, 146>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 147 && SC_DT_ARITHMETIC_END > 147
template <>
struct NativeCase<0, 147>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 148 && SC_DT_ARITHMETIC_END > 148
template <>
struct NativeCase<0, 148>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 149 && SC_DT_ARITHMETIC_END > 149
template <>
struct NativeCase<0, 149>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 150 && SC_DT_ARITHMETIC_END > 150
template <>
struct NativeCase<0, 150>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_signed &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 151 && SC_DT_ARITHMETIC_END > 151
template <>
struct NativeCase<0, 151>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_signed &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 152 && SC_DT_ARITHMETIC_END > 152
template <>
struct NativeCase<0, 152>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<sc_dt::int64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 153 && SC_DT_ARITHMETIC_END > 153
template <>
struct NativeCase<0, 153>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 154 && SC_DT_ARITHMETIC_END > 154
template <>
struct NativeCase<0, 154>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<unsigned int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 155 && SC_DT_ARITHMETIC_END > 155
template <>
struct NativeCase<0, 155>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<unsigned long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 156 && SC_DT_ARITHMETIC_END > 156
template <>
struct NativeCase<0, 156>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 157 && SC_DT_ARITHMETIC_END > 157
template <>
struct NativeCase<0, 157>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 158 && SC_DT_ARITHMETIC_END > 158
template <>
struct NativeCase<0, 158>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 159 && SC_DT_ARITHMETIC_END > 159
template <>
struct NativeCase<0, 159>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_unsigned &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 160 && SC_DT_ARITHMETIC_END > 160
template <>
struct NativeCase<0, 160>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_unsigned &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 161 && SC_DT_ARITHMETIC_END > 161
template <>
struct NativeCase<0, 161>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (const sc_dt::sc_unsigned &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<sc_dt::int64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 162 && SC_DT_ARITHMETIC_END > 162
template <>
struct NativeCase<0, 162>
{
  using Function = sc_dt::sc_signed (*)(int, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (int, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 163 && SC_DT_ARITHMETIC_END > 163
template <>
struct NativeCase<0, 163>
{
  using Function = sc_dt::sc_signed (*)(int, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (int, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 164 && SC_DT_ARITHMETIC_END > 164
template <>
struct NativeCase<0, 164>
{
  using Function = sc_dt::sc_signed (*)(long, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (long, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 165 && SC_DT_ARITHMETIC_END > 165
template <>
struct NativeCase<0, 165>
{
  using Function = sc_dt::sc_signed (*)(long, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (long, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 166 && SC_DT_ARITHMETIC_END > 166
template <>
struct NativeCase<0, 166>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 167 && SC_DT_ARITHMETIC_END > 167
template <>
struct NativeCase<0, 167>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 168 && SC_DT_ARITHMETIC_END > 168
template <>
struct NativeCase<0, 168>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::uint64, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (sc_dt::uint64, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 169 && SC_DT_ARITHMETIC_END > 169
template <>
struct NativeCase<0, 169>
{
  using Function = sc_dt::sc_signed (*)(unsigned int, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (unsigned int, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 170 && SC_DT_ARITHMETIC_END > 170
template <>
struct NativeCase<0, 170>
{
  using Function = sc_dt::sc_signed (*)(unsigned long, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_signed (unsigned long, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 171 && SC_DT_ARITHMETIC_END > 171
template <>
struct NativeCase<0, 171>
{
  using Function = sc_dt::sc_string_old (*)(const char *, const sc_dt::sc_string_old &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_string_old (const char *, const sc_dt::sc_string_old &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_string_old &>(sources));
    return type_name<sc_dt::sc_string_old>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 172 && SC_DT_ARITHMETIC_END > 172
template <>
struct NativeCase<0, 172>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 173 && SC_DT_ARITHMETIC_END > 173
template <>
struct NativeCase<0, 173>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 174 && SC_DT_ARITHMETIC_END > 174
template <>
struct NativeCase<0, 174>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 175 && SC_DT_ARITHMETIC_END > 175
template <>
struct NativeCase<0, 175>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
    "exact overload sc_dt::operator+ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 176 && SC_DT_ARITHMETIC_END > 176
template <>
struct NativeCase<0, 176>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 177 && SC_DT_ARITHMETIC_END > 177
template <>
struct NativeCase<0, 177>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<unsigned int>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 178 && SC_DT_ARITHMETIC_END > 178
template <>
struct NativeCase<0, 178>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<unsigned long>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 179 && SC_DT_ARITHMETIC_END > 179
template <>
struct NativeCase<0, 179>
{
  using Function = sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 180 && SC_DT_ARITHMETIC_END > 180
template <>
struct NativeCase<0, 180>
{
  using Function = sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 0 && SC_DT_ARITHMETIC_START <= 181 && SC_DT_ARITHMETIC_END > 181
template <>
struct NativeCase<0, 181>
{
  using Function = sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator+);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator+)), Function>,
                "exact overload sc_dt::operator+ sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 0 && SC_DT_ARITHMETIC_END > 0
template <>
struct NativeCase<1, 0>
{
  using Function = const sc_dt::sc_fxval (*)(const char *, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const char *, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 1 && SC_DT_ARITHMETIC_END > 1
template <>
struct NativeCase<1, 1>
{
  using Function = const sc_dt::sc_fxval (*)(const char *, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const char *, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 2 && SC_DT_ARITHMETIC_END > 2
template <>
struct NativeCase<1, 2>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 3 && SC_DT_ARITHMETIC_END > 3
template <>
struct NativeCase<1, 3>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 4 && SC_DT_ARITHMETIC_END > 4
template <>
struct NativeCase<1, 4>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 5 && SC_DT_ARITHMETIC_END > 5
template <>
struct NativeCase<1, 5>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 6 && SC_DT_ARITHMETIC_END > 6
template <>
struct NativeCase<1, 6>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 7 && SC_DT_ARITHMETIC_END > 7
template <>
struct NativeCase<1, 7>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 8 && SC_DT_ARITHMETIC_END > 8
template <>
struct NativeCase<1, 8>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 9 && SC_DT_ARITHMETIC_END > 9
template <>
struct NativeCase<1, 9>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 10 && SC_DT_ARITHMETIC_END > 10
template <>
struct NativeCase<1, 10>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 11 && SC_DT_ARITHMETIC_END > 11
template <>
struct NativeCase<1, 11>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 12 && SC_DT_ARITHMETIC_END > 12
template <>
struct NativeCase<1, 12>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 13 && SC_DT_ARITHMETIC_END > 13
template <>
struct NativeCase<1, 13>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 14 && SC_DT_ARITHMETIC_END > 14
template <>
struct NativeCase<1, 14>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 15 && SC_DT_ARITHMETIC_END > 15
template <>
struct NativeCase<1, 15>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 16 && SC_DT_ARITHMETIC_END > 16
template <>
struct NativeCase<1, 16>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 17 && SC_DT_ARITHMETIC_END > 17
template <>
struct NativeCase<1, 17>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 18 && SC_DT_ARITHMETIC_END > 18
template <>
struct NativeCase<1, 18>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 19 && SC_DT_ARITHMETIC_END > 19
template <>
struct NativeCase<1, 19>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 20 && SC_DT_ARITHMETIC_END > 20
template <>
struct NativeCase<1, 20>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 21 && SC_DT_ARITHMETIC_END > 21
template <>
struct NativeCase<1, 21>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 22 && SC_DT_ARITHMETIC_END > 22
template <>
struct NativeCase<1, 22>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 23 && SC_DT_ARITHMETIC_END > 23
template <>
struct NativeCase<1, 23>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 24 && SC_DT_ARITHMETIC_END > 24
template <>
struct NativeCase<1, 24>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 25 && SC_DT_ARITHMETIC_END > 25
template <>
struct NativeCase<1, 25>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 26 && SC_DT_ARITHMETIC_END > 26
template <>
struct NativeCase<1, 26>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 27 && SC_DT_ARITHMETIC_END > 27
template <>
struct NativeCase<1, 27>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 28 && SC_DT_ARITHMETIC_END > 28
template <>
struct NativeCase<1, 28>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 29 && SC_DT_ARITHMETIC_END > 29
template <>
struct NativeCase<1, 29>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 30 && SC_DT_ARITHMETIC_END > 30
template <>
struct NativeCase<1, 30>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 31 && SC_DT_ARITHMETIC_END > 31
template <>
struct NativeCase<1, 31>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 32 && SC_DT_ARITHMETIC_END > 32
template <>
struct NativeCase<1, 32>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 33 && SC_DT_ARITHMETIC_END > 33
template <>
struct NativeCase<1, 33>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 34 && SC_DT_ARITHMETIC_END > 34
template <>
struct NativeCase<1, 34>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 35 && SC_DT_ARITHMETIC_END > 35
template <>
struct NativeCase<1, 35>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 36 && SC_DT_ARITHMETIC_END > 36
template <>
struct NativeCase<1, 36>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 37 && SC_DT_ARITHMETIC_END > 37
template <>
struct NativeCase<1, 37>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 38 && SC_DT_ARITHMETIC_END > 38
template <>
struct NativeCase<1, 38>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 39 && SC_DT_ARITHMETIC_END > 39
template <>
struct NativeCase<1, 39>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 40 && SC_DT_ARITHMETIC_END > 40
template <>
struct NativeCase<1, 40>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 41 && SC_DT_ARITHMETIC_END > 41
template <>
struct NativeCase<1, 41>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_int_base &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 42 && SC_DT_ARITHMETIC_END > 42
template <>
struct NativeCase<1, 42>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_signed &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 43 && SC_DT_ARITHMETIC_END > 43
template <>
struct NativeCase<1, 43>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_signed &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 44 && SC_DT_ARITHMETIC_END > 44
template <>
struct NativeCase<1, 44>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 45 && SC_DT_ARITHMETIC_END > 45
template <>
struct NativeCase<1, 45>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 46 && SC_DT_ARITHMETIC_END > 46
template <>
struct NativeCase<1, 46>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 47 && SC_DT_ARITHMETIC_END > 47
template <>
struct NativeCase<1, 47>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 48 && SC_DT_ARITHMETIC_END > 48
template <>
struct NativeCase<1, 48>
{
  using Function = const sc_dt::sc_fxval (*)(double, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (double, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 49 && SC_DT_ARITHMETIC_END > 49
template <>
struct NativeCase<1, 49>
{
  using Function = const sc_dt::sc_fxval (*)(double, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (double, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 50 && SC_DT_ARITHMETIC_END > 50
template <>
struct NativeCase<1, 50>
{
  using Function = const sc_dt::sc_fxval (*)(float, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (float, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 51 && SC_DT_ARITHMETIC_END > 51
template <>
struct NativeCase<1, 51>
{
  using Function = const sc_dt::sc_fxval (*)(float, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (float, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 52 && SC_DT_ARITHMETIC_END > 52
template <>
struct NativeCase<1, 52>
{
  using Function = const sc_dt::sc_fxval (*)(int, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (int, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 53 && SC_DT_ARITHMETIC_END > 53
template <>
struct NativeCase<1, 53>
{
  using Function = const sc_dt::sc_fxval (*)(int, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (int, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 54 && SC_DT_ARITHMETIC_END > 54
template <>
struct NativeCase<1, 54>
{
  using Function = const sc_dt::sc_fxval (*)(long, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (long, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 55 && SC_DT_ARITHMETIC_END > 55
template <>
struct NativeCase<1, 55>
{
  using Function = const sc_dt::sc_fxval (*)(long, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (long, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 56 && SC_DT_ARITHMETIC_END > 56
template <>
struct NativeCase<1, 56>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::int64, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (sc_dt::int64, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 57 && SC_DT_ARITHMETIC_END > 57
template <>
struct NativeCase<1, 57>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::int64, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (sc_dt::int64, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 58 && SC_DT_ARITHMETIC_END > 58
template <>
struct NativeCase<1, 58>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::uint64, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (sc_dt::uint64, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 59 && SC_DT_ARITHMETIC_END > 59
template <>
struct NativeCase<1, 59>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::uint64, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (sc_dt::uint64, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 60 && SC_DT_ARITHMETIC_END > 60
template <>
struct NativeCase<1, 60>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned int, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (unsigned int, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 61 && SC_DT_ARITHMETIC_END > 61
template <>
struct NativeCase<1, 61>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned int, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (unsigned int, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 62 && SC_DT_ARITHMETIC_END > 62
template <>
struct NativeCase<1, 62>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned long, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (unsigned long, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 63 && SC_DT_ARITHMETIC_END > 63
template <>
struct NativeCase<1, 63>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned long, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval (unsigned long, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 64 && SC_DT_ARITHMETIC_END > 64
template <>
struct NativeCase<1, 64>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const char *, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const char *, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 65 && SC_DT_ARITHMETIC_END > 65
template <>
struct NativeCase<1, 65>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const char *, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const char *, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 66 && SC_DT_ARITHMETIC_END > 66
template <>
struct NativeCase<1, 66>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 67 && SC_DT_ARITHMETIC_END > 67
template <>
struct NativeCase<1, 67>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 68 && SC_DT_ARITHMETIC_END > 68
template <>
struct NativeCase<1, 68>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 69 && SC_DT_ARITHMETIC_END > 69
template <>
struct NativeCase<1, 69>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 70 && SC_DT_ARITHMETIC_END > 70
template <>
struct NativeCase<1, 70>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 71 && SC_DT_ARITHMETIC_END > 71
template <>
struct NativeCase<1, 71>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 72 && SC_DT_ARITHMETIC_END > 72
template <>
struct NativeCase<1, 72>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 73 && SC_DT_ARITHMETIC_END > 73
template <>
struct NativeCase<1, 73>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 74 && SC_DT_ARITHMETIC_END > 74
template <>
struct NativeCase<1, 74>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 75 && SC_DT_ARITHMETIC_END > 75
template <>
struct NativeCase<1, 75>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 76 && SC_DT_ARITHMETIC_END > 76
template <>
struct NativeCase<1, 76>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 77 && SC_DT_ARITHMETIC_END > 77
template <>
struct NativeCase<1, 77>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 78 && SC_DT_ARITHMETIC_END > 78
template <>
struct NativeCase<1, 78>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 79 && SC_DT_ARITHMETIC_END > 79
template <>
struct NativeCase<1, 79>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 80 && SC_DT_ARITHMETIC_END > 80
template <>
struct NativeCase<1, 80>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 81 && SC_DT_ARITHMETIC_END > 81
template <>
struct NativeCase<1, 81>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 82 && SC_DT_ARITHMETIC_END > 82
template <>
struct NativeCase<1, 82>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 83 && SC_DT_ARITHMETIC_END > 83
template <>
struct NativeCase<1, 83>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 84 && SC_DT_ARITHMETIC_END > 84
template <>
struct NativeCase<1, 84>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 85 && SC_DT_ARITHMETIC_END > 85
template <>
struct NativeCase<1, 85>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 86 && SC_DT_ARITHMETIC_END > 86
template <>
struct NativeCase<1, 86>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 87 && SC_DT_ARITHMETIC_END > 87
template <>
struct NativeCase<1, 87>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 88 && SC_DT_ARITHMETIC_END > 88
template <>
struct NativeCase<1, 88>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 89 && SC_DT_ARITHMETIC_END > 89
template <>
struct NativeCase<1, 89>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 90 && SC_DT_ARITHMETIC_END > 90
template <>
struct NativeCase<1, 90>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 91 && SC_DT_ARITHMETIC_END > 91
template <>
struct NativeCase<1, 91>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 92 && SC_DT_ARITHMETIC_END > 92
template <>
struct NativeCase<1, 92>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 93 && SC_DT_ARITHMETIC_END > 93
template <>
struct NativeCase<1, 93>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 94 && SC_DT_ARITHMETIC_END > 94
template <>
struct NativeCase<1, 94>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 95 && SC_DT_ARITHMETIC_END > 95
template <>
struct NativeCase<1, 95>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 96 && SC_DT_ARITHMETIC_END > 96
template <>
struct NativeCase<1, 96>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_int_base &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 97 && SC_DT_ARITHMETIC_END > 97
template <>
struct NativeCase<1, 97>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_int_base &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 98 && SC_DT_ARITHMETIC_END > 98
template <>
struct NativeCase<1, 98>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_signed &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 99 && SC_DT_ARITHMETIC_END > 99
template <>
struct NativeCase<1, 99>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_signed &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 100 && SC_DT_ARITHMETIC_END > 100
template <>
struct NativeCase<1, 100>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 101 && SC_DT_ARITHMETIC_END > 101
template <>
struct NativeCase<1, 101>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 102 && SC_DT_ARITHMETIC_END > 102
template <>
struct NativeCase<1, 102>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_unsigned &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 103 && SC_DT_ARITHMETIC_END > 103
template <>
struct NativeCase<1, 103>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (const sc_dt::sc_unsigned &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 104 && SC_DT_ARITHMETIC_END > 104
template <>
struct NativeCase<1, 104>
{
  using Function = const sc_dt::sc_fxval_fast (*)(double, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (double, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 105 && SC_DT_ARITHMETIC_END > 105
template <>
struct NativeCase<1, 105>
{
  using Function = const sc_dt::sc_fxval_fast (*)(double, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (double, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 106 && SC_DT_ARITHMETIC_END > 106
template <>
struct NativeCase<1, 106>
{
  using Function = const sc_dt::sc_fxval_fast (*)(float, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (float, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 107 && SC_DT_ARITHMETIC_END > 107
template <>
struct NativeCase<1, 107>
{
  using Function = const sc_dt::sc_fxval_fast (*)(float, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (float, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 108 && SC_DT_ARITHMETIC_END > 108
template <>
struct NativeCase<1, 108>
{
  using Function = const sc_dt::sc_fxval_fast (*)(int, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (int, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 109 && SC_DT_ARITHMETIC_END > 109
template <>
struct NativeCase<1, 109>
{
  using Function = const sc_dt::sc_fxval_fast (*)(int, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (int, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 110 && SC_DT_ARITHMETIC_END > 110
template <>
struct NativeCase<1, 110>
{
  using Function = const sc_dt::sc_fxval_fast (*)(long, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (long, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 111 && SC_DT_ARITHMETIC_END > 111
template <>
struct NativeCase<1, 111>
{
  using Function = const sc_dt::sc_fxval_fast (*)(long, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (long, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 112 && SC_DT_ARITHMETIC_END > 112
template <>
struct NativeCase<1, 112>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::int64, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (sc_dt::int64, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 113 && SC_DT_ARITHMETIC_END > 113
template <>
struct NativeCase<1, 113>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::int64, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (sc_dt::int64, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 114 && SC_DT_ARITHMETIC_END > 114
template <>
struct NativeCase<1, 114>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (sc_dt::uint64, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 115 && SC_DT_ARITHMETIC_END > 115
template <>
struct NativeCase<1, 115>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::uint64, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (sc_dt::uint64, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 116 && SC_DT_ARITHMETIC_END > 116
template <>
struct NativeCase<1, 116>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned int, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (unsigned int, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 117 && SC_DT_ARITHMETIC_END > 117
template <>
struct NativeCase<1, 117>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned int, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (unsigned int, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 118 && SC_DT_ARITHMETIC_END > 118
template <>
struct NativeCase<1, 118>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned long, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (unsigned long, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 119 && SC_DT_ARITHMETIC_END > 119
template <>
struct NativeCase<1, 119>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned long, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_fxval_fast (unsigned long, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 120 && SC_DT_ARITHMETIC_END > 120
template <>
struct NativeCase<1, 120>
{
  using Function = const sc_dt::sc_signed (*)(bool, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (bool, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<bool>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 121 && SC_DT_ARITHMETIC_END > 121
template <>
struct NativeCase<1, 121>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, bool);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, bool)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<bool>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 122 && SC_DT_ARITHMETIC_END > 122
template <>
struct NativeCase<1, 122>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, const "
                "sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 123 && SC_DT_ARITHMETIC_END > 123
template <>
struct NativeCase<1, 123>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 124 && SC_DT_ARITHMETIC_END > 124
template <>
struct NativeCase<1, 124>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 125 && SC_DT_ARITHMETIC_END > 125
template <>
struct NativeCase<1, 125>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 126 && SC_DT_ARITHMETIC_END > 126
template <>
struct NativeCase<1, 126>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 127 && SC_DT_ARITHMETIC_END > 127
template <>
struct NativeCase<1, 127>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 128 && SC_DT_ARITHMETIC_END > 128
template <>
struct NativeCase<1, 128>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 129 && SC_DT_ARITHMETIC_END > 129
template <>
struct NativeCase<1, 129>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 130 && SC_DT_ARITHMETIC_END > 130
template <>
struct NativeCase<1, 130>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 131 && SC_DT_ARITHMETIC_END > 131
template <>
struct NativeCase<1, 131>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 132 && SC_DT_ARITHMETIC_END > 132
template <>
struct NativeCase<1, 132>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_concatref &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 133 && SC_DT_ARITHMETIC_END > 133
template <>
struct NativeCase<1, 133>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 134 && SC_DT_ARITHMETIC_END > 134
template <>
struct NativeCase<1, 134>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 135 && SC_DT_ARITHMETIC_END > 135
template <>
struct NativeCase<1, 135>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 136 && SC_DT_ARITHMETIC_END > 136
template <>
struct NativeCase<1, 136>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- const sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 137 && SC_DT_ARITHMETIC_END > 137
template <>
struct NativeCase<1, 137>
{
  using Function = const sc_dt::sc_signed (*)(int, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (int, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 138 && SC_DT_ARITHMETIC_END > 138
template <>
struct NativeCase<1, 138>
{
  using Function = const sc_dt::sc_signed (*)(long, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (long, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 139 && SC_DT_ARITHMETIC_END > 139
template <>
struct NativeCase<1, 139>
{
  using Function = const sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 140 && SC_DT_ARITHMETIC_END > 140
template <>
struct NativeCase<1, 140>
{
  using Function = const sc_dt::sc_signed (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (sc_dt::uint64, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 141 && SC_DT_ARITHMETIC_END > 141
template <>
struct NativeCase<1, 141>
{
  using Function = const sc_dt::sc_signed (*)(unsigned int, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (unsigned int, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 142 && SC_DT_ARITHMETIC_END > 142
template <>
struct NativeCase<1, 142>
{
  using Function = const sc_dt::sc_signed (*)(unsigned long, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- const sc_dt::sc_signed (unsigned long, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 143 && SC_DT_ARITHMETIC_END > 143
template <>
struct NativeCase<1, 143>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 144 && SC_DT_ARITHMETIC_END > 144
template <>
struct NativeCase<1, 144>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 145 && SC_DT_ARITHMETIC_END > 145
template <>
struct NativeCase<1, 145>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 146 && SC_DT_ARITHMETIC_END > 146
template <>
struct NativeCase<1, 146>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 147 && SC_DT_ARITHMETIC_END > 147
template <>
struct NativeCase<1, 147>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 148 && SC_DT_ARITHMETIC_END > 148
template <>
struct NativeCase<1, 148>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 149 && SC_DT_ARITHMETIC_END > 149
template <>
struct NativeCase<1, 149>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 150 && SC_DT_ARITHMETIC_END > 150
template <>
struct NativeCase<1, 150>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_signed &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 151 && SC_DT_ARITHMETIC_END > 151
template <>
struct NativeCase<1, 151>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_signed &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 152 && SC_DT_ARITHMETIC_END > 152
template <>
struct NativeCase<1, 152>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<sc_dt::int64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 153 && SC_DT_ARITHMETIC_END > 153
template <>
struct NativeCase<1, 153>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 154 && SC_DT_ARITHMETIC_END > 154
template <>
struct NativeCase<1, 154>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<unsigned int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 155 && SC_DT_ARITHMETIC_END > 155
template <>
struct NativeCase<1, 155>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<unsigned long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 156 && SC_DT_ARITHMETIC_END > 156
template <>
struct NativeCase<1, 156>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 157 && SC_DT_ARITHMETIC_END > 157
template <>
struct NativeCase<1, 157>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 158 && SC_DT_ARITHMETIC_END > 158
template <>
struct NativeCase<1, 158>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 159 && SC_DT_ARITHMETIC_END > 159
template <>
struct NativeCase<1, 159>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 160 && SC_DT_ARITHMETIC_END > 160
template <>
struct NativeCase<1, 160>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 161 && SC_DT_ARITHMETIC_END > 161
template <>
struct NativeCase<1, 161>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 162 && SC_DT_ARITHMETIC_END > 162
template <>
struct NativeCase<1, 162>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
    "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 163 && SC_DT_ARITHMETIC_END > 163
template <>
struct NativeCase<1, 163>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_unsigned &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 164 && SC_DT_ARITHMETIC_END > 164
template <>
struct NativeCase<1, 164>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_unsigned &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 165 && SC_DT_ARITHMETIC_END > 165
template <>
struct NativeCase<1, 165>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_unsigned &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<sc_dt::int64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 166 && SC_DT_ARITHMETIC_END > 166
template <>
struct NativeCase<1, 166>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_unsigned &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 167 && SC_DT_ARITHMETIC_END > 167
template <>
struct NativeCase<1, 167>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_unsigned &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<unsigned int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 168 && SC_DT_ARITHMETIC_END > 168
template <>
struct NativeCase<1, 168>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (const sc_dt::sc_unsigned &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<unsigned long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 169 && SC_DT_ARITHMETIC_END > 169
template <>
struct NativeCase<1, 169>
{
  using Function = sc_dt::sc_signed (*)(int, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (int, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 170 && SC_DT_ARITHMETIC_END > 170
template <>
struct NativeCase<1, 170>
{
  using Function = sc_dt::sc_signed (*)(int, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (int, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 171 && SC_DT_ARITHMETIC_END > 171
template <>
struct NativeCase<1, 171>
{
  using Function = sc_dt::sc_signed (*)(long, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (long, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 172 && SC_DT_ARITHMETIC_END > 172
template <>
struct NativeCase<1, 172>
{
  using Function = sc_dt::sc_signed (*)(long, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (long, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 173 && SC_DT_ARITHMETIC_END > 173
template <>
struct NativeCase<1, 173>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 174 && SC_DT_ARITHMETIC_END > 174
template <>
struct NativeCase<1, 174>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 175 && SC_DT_ARITHMETIC_END > 175
template <>
struct NativeCase<1, 175>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::uint64, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (sc_dt::uint64, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 176 && SC_DT_ARITHMETIC_END > 176
template <>
struct NativeCase<1, 176>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (sc_dt::uint64, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 177 && SC_DT_ARITHMETIC_END > 177
template <>
struct NativeCase<1, 177>
{
  using Function = sc_dt::sc_signed (*)(unsigned int, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (unsigned int, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 178 && SC_DT_ARITHMETIC_END > 178
template <>
struct NativeCase<1, 178>
{
  using Function = sc_dt::sc_signed (*)(unsigned int, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (unsigned int, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 179 && SC_DT_ARITHMETIC_END > 179
template <>
struct NativeCase<1, 179>
{
  using Function = sc_dt::sc_signed (*)(unsigned long, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (unsigned long, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 1 && SC_DT_ARITHMETIC_START <= 180 && SC_DT_ARITHMETIC_END > 180
template <>
struct NativeCase<1, 180>
{
  using Function = sc_dt::sc_signed (*)(unsigned long, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator-);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator-)), Function>,
                "exact overload sc_dt::operator- sc_dt::sc_signed (unsigned long, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 0 && SC_DT_ARITHMETIC_END > 0
template <>
struct NativeCase<2, 0>
{
  using Function = const sc_dt::sc_fxval (*)(const char *, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const char *, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 1 && SC_DT_ARITHMETIC_END > 1
template <>
struct NativeCase<2, 1>
{
  using Function = const sc_dt::sc_fxval (*)(const char *, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const char *, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 2 && SC_DT_ARITHMETIC_END > 2
template <>
struct NativeCase<2, 2>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 3 && SC_DT_ARITHMETIC_END > 3
template <>
struct NativeCase<2, 3>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 4 && SC_DT_ARITHMETIC_END > 4
template <>
struct NativeCase<2, 4>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 5 && SC_DT_ARITHMETIC_END > 5
template <>
struct NativeCase<2, 5>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 6 && SC_DT_ARITHMETIC_END > 6
template <>
struct NativeCase<2, 6>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 7 && SC_DT_ARITHMETIC_END > 7
template <>
struct NativeCase<2, 7>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 8 && SC_DT_ARITHMETIC_END > 8
template <>
struct NativeCase<2, 8>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 9 && SC_DT_ARITHMETIC_END > 9
template <>
struct NativeCase<2, 9>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 10 && SC_DT_ARITHMETIC_END > 10
template <>
struct NativeCase<2, 10>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 11 && SC_DT_ARITHMETIC_END > 11
template <>
struct NativeCase<2, 11>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 12 && SC_DT_ARITHMETIC_END > 12
template <>
struct NativeCase<2, 12>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 13 && SC_DT_ARITHMETIC_END > 13
template <>
struct NativeCase<2, 13>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 14 && SC_DT_ARITHMETIC_END > 14
template <>
struct NativeCase<2, 14>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 15 && SC_DT_ARITHMETIC_END > 15
template <>
struct NativeCase<2, 15>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 16 && SC_DT_ARITHMETIC_END > 16
template <>
struct NativeCase<2, 16>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 17 && SC_DT_ARITHMETIC_END > 17
template <>
struct NativeCase<2, 17>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 18 && SC_DT_ARITHMETIC_END > 18
template <>
struct NativeCase<2, 18>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 19 && SC_DT_ARITHMETIC_END > 19
template <>
struct NativeCase<2, 19>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 20 && SC_DT_ARITHMETIC_END > 20
template <>
struct NativeCase<2, 20>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 21 && SC_DT_ARITHMETIC_END > 21
template <>
struct NativeCase<2, 21>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 22 && SC_DT_ARITHMETIC_END > 22
template <>
struct NativeCase<2, 22>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 23 && SC_DT_ARITHMETIC_END > 23
template <>
struct NativeCase<2, 23>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 24 && SC_DT_ARITHMETIC_END > 24
template <>
struct NativeCase<2, 24>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 25 && SC_DT_ARITHMETIC_END > 25
template <>
struct NativeCase<2, 25>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 26 && SC_DT_ARITHMETIC_END > 26
template <>
struct NativeCase<2, 26>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 27 && SC_DT_ARITHMETIC_END > 27
template <>
struct NativeCase<2, 27>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 28 && SC_DT_ARITHMETIC_END > 28
template <>
struct NativeCase<2, 28>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 29 && SC_DT_ARITHMETIC_END > 29
template <>
struct NativeCase<2, 29>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 30 && SC_DT_ARITHMETIC_END > 30
template <>
struct NativeCase<2, 30>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 31 && SC_DT_ARITHMETIC_END > 31
template <>
struct NativeCase<2, 31>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 32 && SC_DT_ARITHMETIC_END > 32
template <>
struct NativeCase<2, 32>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 33 && SC_DT_ARITHMETIC_END > 33
template <>
struct NativeCase<2, 33>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 34 && SC_DT_ARITHMETIC_END > 34
template <>
struct NativeCase<2, 34>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 35 && SC_DT_ARITHMETIC_END > 35
template <>
struct NativeCase<2, 35>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 36 && SC_DT_ARITHMETIC_END > 36
template <>
struct NativeCase<2, 36>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 37 && SC_DT_ARITHMETIC_END > 37
template <>
struct NativeCase<2, 37>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 38 && SC_DT_ARITHMETIC_END > 38
template <>
struct NativeCase<2, 38>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 39 && SC_DT_ARITHMETIC_END > 39
template <>
struct NativeCase<2, 39>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 40 && SC_DT_ARITHMETIC_END > 40
template <>
struct NativeCase<2, 40>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 41 && SC_DT_ARITHMETIC_END > 41
template <>
struct NativeCase<2, 41>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_int_base &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 42 && SC_DT_ARITHMETIC_END > 42
template <>
struct NativeCase<2, 42>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_signed &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 43 && SC_DT_ARITHMETIC_END > 43
template <>
struct NativeCase<2, 43>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_signed &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 44 && SC_DT_ARITHMETIC_END > 44
template <>
struct NativeCase<2, 44>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 45 && SC_DT_ARITHMETIC_END > 45
template <>
struct NativeCase<2, 45>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 46 && SC_DT_ARITHMETIC_END > 46
template <>
struct NativeCase<2, 46>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 47 && SC_DT_ARITHMETIC_END > 47
template <>
struct NativeCase<2, 47>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 48 && SC_DT_ARITHMETIC_END > 48
template <>
struct NativeCase<2, 48>
{
  using Function = const sc_dt::sc_fxval (*)(double, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (double, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 49 && SC_DT_ARITHMETIC_END > 49
template <>
struct NativeCase<2, 49>
{
  using Function = const sc_dt::sc_fxval (*)(double, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (double, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 50 && SC_DT_ARITHMETIC_END > 50
template <>
struct NativeCase<2, 50>
{
  using Function = const sc_dt::sc_fxval (*)(float, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (float, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 51 && SC_DT_ARITHMETIC_END > 51
template <>
struct NativeCase<2, 51>
{
  using Function = const sc_dt::sc_fxval (*)(float, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (float, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 52 && SC_DT_ARITHMETIC_END > 52
template <>
struct NativeCase<2, 52>
{
  using Function = const sc_dt::sc_fxval (*)(int, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (int, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 53 && SC_DT_ARITHMETIC_END > 53
template <>
struct NativeCase<2, 53>
{
  using Function = const sc_dt::sc_fxval (*)(int, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (int, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 54 && SC_DT_ARITHMETIC_END > 54
template <>
struct NativeCase<2, 54>
{
  using Function = const sc_dt::sc_fxval (*)(long, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (long, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 55 && SC_DT_ARITHMETIC_END > 55
template <>
struct NativeCase<2, 55>
{
  using Function = const sc_dt::sc_fxval (*)(long, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (long, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 56 && SC_DT_ARITHMETIC_END > 56
template <>
struct NativeCase<2, 56>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::int64, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (sc_dt::int64, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 57 && SC_DT_ARITHMETIC_END > 57
template <>
struct NativeCase<2, 57>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::int64, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (sc_dt::int64, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 58 && SC_DT_ARITHMETIC_END > 58
template <>
struct NativeCase<2, 58>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::uint64, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (sc_dt::uint64, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 59 && SC_DT_ARITHMETIC_END > 59
template <>
struct NativeCase<2, 59>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::uint64, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (sc_dt::uint64, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 60 && SC_DT_ARITHMETIC_END > 60
template <>
struct NativeCase<2, 60>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned int, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (unsigned int, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 61 && SC_DT_ARITHMETIC_END > 61
template <>
struct NativeCase<2, 61>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned int, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (unsigned int, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 62 && SC_DT_ARITHMETIC_END > 62
template <>
struct NativeCase<2, 62>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned long, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (unsigned long, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 63 && SC_DT_ARITHMETIC_END > 63
template <>
struct NativeCase<2, 63>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned long, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval (unsigned long, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 64 && SC_DT_ARITHMETIC_END > 64
template <>
struct NativeCase<2, 64>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const char *, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const char *, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 65 && SC_DT_ARITHMETIC_END > 65
template <>
struct NativeCase<2, 65>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const char *, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const char *, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 66 && SC_DT_ARITHMETIC_END > 66
template <>
struct NativeCase<2, 66>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 67 && SC_DT_ARITHMETIC_END > 67
template <>
struct NativeCase<2, 67>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 68 && SC_DT_ARITHMETIC_END > 68
template <>
struct NativeCase<2, 68>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 69 && SC_DT_ARITHMETIC_END > 69
template <>
struct NativeCase<2, 69>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 70 && SC_DT_ARITHMETIC_END > 70
template <>
struct NativeCase<2, 70>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 71 && SC_DT_ARITHMETIC_END > 71
template <>
struct NativeCase<2, 71>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 72 && SC_DT_ARITHMETIC_END > 72
template <>
struct NativeCase<2, 72>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 73 && SC_DT_ARITHMETIC_END > 73
template <>
struct NativeCase<2, 73>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 74 && SC_DT_ARITHMETIC_END > 74
template <>
struct NativeCase<2, 74>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 75 && SC_DT_ARITHMETIC_END > 75
template <>
struct NativeCase<2, 75>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 76 && SC_DT_ARITHMETIC_END > 76
template <>
struct NativeCase<2, 76>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 77 && SC_DT_ARITHMETIC_END > 77
template <>
struct NativeCase<2, 77>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 78 && SC_DT_ARITHMETIC_END > 78
template <>
struct NativeCase<2, 78>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 79 && SC_DT_ARITHMETIC_END > 79
template <>
struct NativeCase<2, 79>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 80 && SC_DT_ARITHMETIC_END > 80
template <>
struct NativeCase<2, 80>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 81 && SC_DT_ARITHMETIC_END > 81
template <>
struct NativeCase<2, 81>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 82 && SC_DT_ARITHMETIC_END > 82
template <>
struct NativeCase<2, 82>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 83 && SC_DT_ARITHMETIC_END > 83
template <>
struct NativeCase<2, 83>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 84 && SC_DT_ARITHMETIC_END > 84
template <>
struct NativeCase<2, 84>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 85 && SC_DT_ARITHMETIC_END > 85
template <>
struct NativeCase<2, 85>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 86 && SC_DT_ARITHMETIC_END > 86
template <>
struct NativeCase<2, 86>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 87 && SC_DT_ARITHMETIC_END > 87
template <>
struct NativeCase<2, 87>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 88 && SC_DT_ARITHMETIC_END > 88
template <>
struct NativeCase<2, 88>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 89 && SC_DT_ARITHMETIC_END > 89
template <>
struct NativeCase<2, 89>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 90 && SC_DT_ARITHMETIC_END > 90
template <>
struct NativeCase<2, 90>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 91 && SC_DT_ARITHMETIC_END > 91
template <>
struct NativeCase<2, 91>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 92 && SC_DT_ARITHMETIC_END > 92
template <>
struct NativeCase<2, 92>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 93 && SC_DT_ARITHMETIC_END > 93
template <>
struct NativeCase<2, 93>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 94 && SC_DT_ARITHMETIC_END > 94
template <>
struct NativeCase<2, 94>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 95 && SC_DT_ARITHMETIC_END > 95
template <>
struct NativeCase<2, 95>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 96 && SC_DT_ARITHMETIC_END > 96
template <>
struct NativeCase<2, 96>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_int_base &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 97 && SC_DT_ARITHMETIC_END > 97
template <>
struct NativeCase<2, 97>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_int_base &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 98 && SC_DT_ARITHMETIC_END > 98
template <>
struct NativeCase<2, 98>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_signed &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 99 && SC_DT_ARITHMETIC_END > 99
template <>
struct NativeCase<2, 99>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_signed &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 100 && SC_DT_ARITHMETIC_END > 100
template <>
struct NativeCase<2, 100>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 101 && SC_DT_ARITHMETIC_END > 101
template <>
struct NativeCase<2, 101>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 102 && SC_DT_ARITHMETIC_END > 102
template <>
struct NativeCase<2, 102>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_unsigned &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 103 && SC_DT_ARITHMETIC_END > 103
template <>
struct NativeCase<2, 103>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (const sc_dt::sc_unsigned &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 104 && SC_DT_ARITHMETIC_END > 104
template <>
struct NativeCase<2, 104>
{
  using Function = const sc_dt::sc_fxval_fast (*)(double, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (double, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 105 && SC_DT_ARITHMETIC_END > 105
template <>
struct NativeCase<2, 105>
{
  using Function = const sc_dt::sc_fxval_fast (*)(double, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (double, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 106 && SC_DT_ARITHMETIC_END > 106
template <>
struct NativeCase<2, 106>
{
  using Function = const sc_dt::sc_fxval_fast (*)(float, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (float, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 107 && SC_DT_ARITHMETIC_END > 107
template <>
struct NativeCase<2, 107>
{
  using Function = const sc_dt::sc_fxval_fast (*)(float, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (float, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 108 && SC_DT_ARITHMETIC_END > 108
template <>
struct NativeCase<2, 108>
{
  using Function = const sc_dt::sc_fxval_fast (*)(int, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (int, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 109 && SC_DT_ARITHMETIC_END > 109
template <>
struct NativeCase<2, 109>
{
  using Function = const sc_dt::sc_fxval_fast (*)(int, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (int, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 110 && SC_DT_ARITHMETIC_END > 110
template <>
struct NativeCase<2, 110>
{
  using Function = const sc_dt::sc_fxval_fast (*)(long, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (long, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 111 && SC_DT_ARITHMETIC_END > 111
template <>
struct NativeCase<2, 111>
{
  using Function = const sc_dt::sc_fxval_fast (*)(long, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (long, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 112 && SC_DT_ARITHMETIC_END > 112
template <>
struct NativeCase<2, 112>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::int64, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (sc_dt::int64, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 113 && SC_DT_ARITHMETIC_END > 113
template <>
struct NativeCase<2, 113>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::int64, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (sc_dt::int64, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 114 && SC_DT_ARITHMETIC_END > 114
template <>
struct NativeCase<2, 114>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (sc_dt::uint64, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 115 && SC_DT_ARITHMETIC_END > 115
template <>
struct NativeCase<2, 115>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::uint64, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (sc_dt::uint64, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 116 && SC_DT_ARITHMETIC_END > 116
template <>
struct NativeCase<2, 116>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned int, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (unsigned int, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 117 && SC_DT_ARITHMETIC_END > 117
template <>
struct NativeCase<2, 117>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned int, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (unsigned int, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 118 && SC_DT_ARITHMETIC_END > 118
template <>
struct NativeCase<2, 118>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned long, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (unsigned long, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 119 && SC_DT_ARITHMETIC_END > 119
template <>
struct NativeCase<2, 119>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned long, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_fxval_fast (unsigned long, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 120 && SC_DT_ARITHMETIC_END > 120
template <>
struct NativeCase<2, 120>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 121 && SC_DT_ARITHMETIC_END > 121
template <>
struct NativeCase<2, 121>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 122 && SC_DT_ARITHMETIC_END > 122
template <>
struct NativeCase<2, 122>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_signed (const sc_dt::sc_concatref &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 123 && SC_DT_ARITHMETIC_END > 123
template <>
struct NativeCase<2, 123>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_signed (const sc_dt::sc_concatref &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 124 && SC_DT_ARITHMETIC_END > 124
template <>
struct NativeCase<2, 124>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_signed (const sc_dt::sc_concatref &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 125 && SC_DT_ARITHMETIC_END > 125
template <>
struct NativeCase<2, 125>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 126 && SC_DT_ARITHMETIC_END > 126
template <>
struct NativeCase<2, 126>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 127 && SC_DT_ARITHMETIC_END > 127
template <>
struct NativeCase<2, 127>
{
  using Function = const sc_dt::sc_signed (*)(int, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_signed (int, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 128 && SC_DT_ARITHMETIC_END > 128
template <>
struct NativeCase<2, 128>
{
  using Function = const sc_dt::sc_signed (*)(long, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_signed (long, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 129 && SC_DT_ARITHMETIC_END > 129
template <>
struct NativeCase<2, 129>
{
  using Function = const sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 130 && SC_DT_ARITHMETIC_END > 130
template <>
struct NativeCase<2, 130>
{
  using Function = const sc_dt::sc_unsigned (*)(bool, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_unsigned (bool, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<bool>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 131 && SC_DT_ARITHMETIC_END > 131
template <>
struct NativeCase<2, 131>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, bool);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, bool)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<bool>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 132 && SC_DT_ARITHMETIC_END > 132
template <>
struct NativeCase<2, 132>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
                "sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 133 && SC_DT_ARITHMETIC_END > 133
template <>
struct NativeCase<2, 133>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 134 && SC_DT_ARITHMETIC_END > 134
template <>
struct NativeCase<2, 134>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
                "sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 135 && SC_DT_ARITHMETIC_END > 135
template <>
struct NativeCase<2, 135>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 136 && SC_DT_ARITHMETIC_END > 136
template <>
struct NativeCase<2, 136>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 137 && SC_DT_ARITHMETIC_END > 137
template <>
struct NativeCase<2, 137>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 138 && SC_DT_ARITHMETIC_END > 138
template <>
struct NativeCase<2, 138>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 139 && SC_DT_ARITHMETIC_END > 139
template <>
struct NativeCase<2, 139>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const "
                "sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 140 && SC_DT_ARITHMETIC_END > 140
template <>
struct NativeCase<2, 140>
{
  using Function = const sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 141 && SC_DT_ARITHMETIC_END > 141
template <>
struct NativeCase<2, 141>
{
  using Function = const sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* const sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 142 && SC_DT_ARITHMETIC_END > 142
template <>
struct NativeCase<2, 142>
{
  using Function = const sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* const sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 143 && SC_DT_ARITHMETIC_END > 143
template <>
struct NativeCase<2, 143>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 144 && SC_DT_ARITHMETIC_END > 144
template <>
struct NativeCase<2, 144>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 145 && SC_DT_ARITHMETIC_END > 145
template <>
struct NativeCase<2, 145>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 146 && SC_DT_ARITHMETIC_END > 146
template <>
struct NativeCase<2, 146>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 147 && SC_DT_ARITHMETIC_END > 147
template <>
struct NativeCase<2, 147>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 148 && SC_DT_ARITHMETIC_END > 148
template <>
struct NativeCase<2, 148>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 149 && SC_DT_ARITHMETIC_END > 149
template <>
struct NativeCase<2, 149>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_signed &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 150 && SC_DT_ARITHMETIC_END > 150
template <>
struct NativeCase<2, 150>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_signed &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 151 && SC_DT_ARITHMETIC_END > 151
template <>
struct NativeCase<2, 151>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<sc_dt::int64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 152 && SC_DT_ARITHMETIC_END > 152
template <>
struct NativeCase<2, 152>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 153 && SC_DT_ARITHMETIC_END > 153
template <>
struct NativeCase<2, 153>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<unsigned int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 154 && SC_DT_ARITHMETIC_END > 154
template <>
struct NativeCase<2, 154>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<unsigned long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 155 && SC_DT_ARITHMETIC_END > 155
template <>
struct NativeCase<2, 155>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 156 && SC_DT_ARITHMETIC_END > 156
template <>
struct NativeCase<2, 156>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 157 && SC_DT_ARITHMETIC_END > 157
template <>
struct NativeCase<2, 157>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 158 && SC_DT_ARITHMETIC_END > 158
template <>
struct NativeCase<2, 158>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_unsigned &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 159 && SC_DT_ARITHMETIC_END > 159
template <>
struct NativeCase<2, 159>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_unsigned &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 160 && SC_DT_ARITHMETIC_END > 160
template <>
struct NativeCase<2, 160>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (const sc_dt::sc_unsigned &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<sc_dt::int64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 161 && SC_DT_ARITHMETIC_END > 161
template <>
struct NativeCase<2, 161>
{
  using Function = sc_dt::sc_signed (*)(int, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (int, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 162 && SC_DT_ARITHMETIC_END > 162
template <>
struct NativeCase<2, 162>
{
  using Function = sc_dt::sc_signed (*)(int, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (int, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 163 && SC_DT_ARITHMETIC_END > 163
template <>
struct NativeCase<2, 163>
{
  using Function = sc_dt::sc_signed (*)(long, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (long, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 164 && SC_DT_ARITHMETIC_END > 164
template <>
struct NativeCase<2, 164>
{
  using Function = sc_dt::sc_signed (*)(long, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (long, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 165 && SC_DT_ARITHMETIC_END > 165
template <>
struct NativeCase<2, 165>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 166 && SC_DT_ARITHMETIC_END > 166
template <>
struct NativeCase<2, 166>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 167 && SC_DT_ARITHMETIC_END > 167
template <>
struct NativeCase<2, 167>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::uint64, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (sc_dt::uint64, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 168 && SC_DT_ARITHMETIC_END > 168
template <>
struct NativeCase<2, 168>
{
  using Function = sc_dt::sc_signed (*)(unsigned int, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (unsigned int, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 169 && SC_DT_ARITHMETIC_END > 169
template <>
struct NativeCase<2, 169>
{
  using Function = sc_dt::sc_signed (*)(unsigned long, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_signed (unsigned long, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 170 && SC_DT_ARITHMETIC_END > 170
template <>
struct NativeCase<2, 170>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 171 && SC_DT_ARITHMETIC_END > 171
template <>
struct NativeCase<2, 171>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 172 && SC_DT_ARITHMETIC_END > 172
template <>
struct NativeCase<2, 172>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
    "exact overload sc_dt::operator* sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 173 && SC_DT_ARITHMETIC_END > 173
template <>
struct NativeCase<2, 173>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 174 && SC_DT_ARITHMETIC_END > 174
template <>
struct NativeCase<2, 174>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<unsigned int>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 175 && SC_DT_ARITHMETIC_END > 175
template <>
struct NativeCase<2, 175>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<unsigned long>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 176 && SC_DT_ARITHMETIC_END > 176
template <>
struct NativeCase<2, 176>
{
  using Function = sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 177 && SC_DT_ARITHMETIC_END > 177
template <>
struct NativeCase<2, 177>
{
  using Function = sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 2 && SC_DT_ARITHMETIC_START <= 178 && SC_DT_ARITHMETIC_END > 178
template <>
struct NativeCase<2, 178>
{
  using Function = sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator*);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator*)), Function>,
                "exact overload sc_dt::operator* sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 0 && SC_DT_ARITHMETIC_END > 0
template <>
struct NativeCase<3, 0>
{
  using Function = const sc_dt::sc_fxval (*)(const char *, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const char *, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 1 && SC_DT_ARITHMETIC_END > 1
template <>
struct NativeCase<3, 1>
{
  using Function = const sc_dt::sc_fxval (*)(const char *, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const char *, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 2 && SC_DT_ARITHMETIC_END > 2
template <>
struct NativeCase<3, 2>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 3 && SC_DT_ARITHMETIC_END > 3
template <>
struct NativeCase<3, 3>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 4 && SC_DT_ARITHMETIC_END > 4
template <>
struct NativeCase<3, 4>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 5 && SC_DT_ARITHMETIC_END > 5
template <>
struct NativeCase<3, 5>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 6 && SC_DT_ARITHMETIC_END > 6
template <>
struct NativeCase<3, 6>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 7 && SC_DT_ARITHMETIC_END > 7
template <>
struct NativeCase<3, 7>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 8 && SC_DT_ARITHMETIC_END > 8
template <>
struct NativeCase<3, 8>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 9 && SC_DT_ARITHMETIC_END > 9
template <>
struct NativeCase<3, 9>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 10 && SC_DT_ARITHMETIC_END > 10
template <>
struct NativeCase<3, 10>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 11 && SC_DT_ARITHMETIC_END > 11
template <>
struct NativeCase<3, 11>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 12 && SC_DT_ARITHMETIC_END > 12
template <>
struct NativeCase<3, 12>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 13 && SC_DT_ARITHMETIC_END > 13
template <>
struct NativeCase<3, 13>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 14 && SC_DT_ARITHMETIC_END > 14
template <>
struct NativeCase<3, 14>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 15 && SC_DT_ARITHMETIC_END > 15
template <>
struct NativeCase<3, 15>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 16 && SC_DT_ARITHMETIC_END > 16
template <>
struct NativeCase<3, 16>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 17 && SC_DT_ARITHMETIC_END > 17
template <>
struct NativeCase<3, 17>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 18 && SC_DT_ARITHMETIC_END > 18
template <>
struct NativeCase<3, 18>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 19 && SC_DT_ARITHMETIC_END > 19
template <>
struct NativeCase<3, 19>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 20 && SC_DT_ARITHMETIC_END > 20
template <>
struct NativeCase<3, 20>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 21 && SC_DT_ARITHMETIC_END > 21
template <>
struct NativeCase<3, 21>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 22 && SC_DT_ARITHMETIC_END > 22
template <>
struct NativeCase<3, 22>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 23 && SC_DT_ARITHMETIC_END > 23
template <>
struct NativeCase<3, 23>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 24 && SC_DT_ARITHMETIC_END > 24
template <>
struct NativeCase<3, 24>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 25 && SC_DT_ARITHMETIC_END > 25
template <>
struct NativeCase<3, 25>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 26 && SC_DT_ARITHMETIC_END > 26
template <>
struct NativeCase<3, 26>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 27 && SC_DT_ARITHMETIC_END > 27
template <>
struct NativeCase<3, 27>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 28 && SC_DT_ARITHMETIC_END > 28
template <>
struct NativeCase<3, 28>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 29 && SC_DT_ARITHMETIC_END > 29
template <>
struct NativeCase<3, 29>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 30 && SC_DT_ARITHMETIC_END > 30
template <>
struct NativeCase<3, 30>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 31 && SC_DT_ARITHMETIC_END > 31
template <>
struct NativeCase<3, 31>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 32 && SC_DT_ARITHMETIC_END > 32
template <>
struct NativeCase<3, 32>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 33 && SC_DT_ARITHMETIC_END > 33
template <>
struct NativeCase<3, 33>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 34 && SC_DT_ARITHMETIC_END > 34
template <>
struct NativeCase<3, 34>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 35 && SC_DT_ARITHMETIC_END > 35
template <>
struct NativeCase<3, 35>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 36 && SC_DT_ARITHMETIC_END > 36
template <>
struct NativeCase<3, 36>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 37 && SC_DT_ARITHMETIC_END > 37
template <>
struct NativeCase<3, 37>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 38 && SC_DT_ARITHMETIC_END > 38
template <>
struct NativeCase<3, 38>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 39 && SC_DT_ARITHMETIC_END > 39
template <>
struct NativeCase<3, 39>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 40 && SC_DT_ARITHMETIC_END > 40
template <>
struct NativeCase<3, 40>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_int_base &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 41 && SC_DT_ARITHMETIC_END > 41
template <>
struct NativeCase<3, 41>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_int_base &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 42 && SC_DT_ARITHMETIC_END > 42
template <>
struct NativeCase<3, 42>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_signed &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 43 && SC_DT_ARITHMETIC_END > 43
template <>
struct NativeCase<3, 43>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_signed &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 44 && SC_DT_ARITHMETIC_END > 44
template <>
struct NativeCase<3, 44>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 45 && SC_DT_ARITHMETIC_END > 45
template <>
struct NativeCase<3, 45>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_uint_base &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 46 && SC_DT_ARITHMETIC_END > 46
template <>
struct NativeCase<3, 46>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 47 && SC_DT_ARITHMETIC_END > 47
template <>
struct NativeCase<3, 47>
{
  using Function = const sc_dt::sc_fxval (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval (const sc_dt::sc_unsigned &, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 48 && SC_DT_ARITHMETIC_END > 48
template <>
struct NativeCase<3, 48>
{
  using Function = const sc_dt::sc_fxval (*)(double, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (double, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 49 && SC_DT_ARITHMETIC_END > 49
template <>
struct NativeCase<3, 49>
{
  using Function = const sc_dt::sc_fxval (*)(double, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (double, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 50 && SC_DT_ARITHMETIC_END > 50
template <>
struct NativeCase<3, 50>
{
  using Function = const sc_dt::sc_fxval (*)(float, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (float, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 51 && SC_DT_ARITHMETIC_END > 51
template <>
struct NativeCase<3, 51>
{
  using Function = const sc_dt::sc_fxval (*)(float, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (float, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 52 && SC_DT_ARITHMETIC_END > 52
template <>
struct NativeCase<3, 52>
{
  using Function = const sc_dt::sc_fxval (*)(int, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (int, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 53 && SC_DT_ARITHMETIC_END > 53
template <>
struct NativeCase<3, 53>
{
  using Function = const sc_dt::sc_fxval (*)(int, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (int, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 54 && SC_DT_ARITHMETIC_END > 54
template <>
struct NativeCase<3, 54>
{
  using Function = const sc_dt::sc_fxval (*)(long, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (long, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 55 && SC_DT_ARITHMETIC_END > 55
template <>
struct NativeCase<3, 55>
{
  using Function = const sc_dt::sc_fxval (*)(long, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (long, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 56 && SC_DT_ARITHMETIC_END > 56
template <>
struct NativeCase<3, 56>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::int64, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (sc_dt::int64, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 57 && SC_DT_ARITHMETIC_END > 57
template <>
struct NativeCase<3, 57>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::int64, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (sc_dt::int64, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 58 && SC_DT_ARITHMETIC_END > 58
template <>
struct NativeCase<3, 58>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::uint64, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (sc_dt::uint64, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 59 && SC_DT_ARITHMETIC_END > 59
template <>
struct NativeCase<3, 59>
{
  using Function = const sc_dt::sc_fxval (*)(sc_dt::uint64, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (sc_dt::uint64, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 60 && SC_DT_ARITHMETIC_END > 60
template <>
struct NativeCase<3, 60>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned int, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (unsigned int, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 61 && SC_DT_ARITHMETIC_END > 61
template <>
struct NativeCase<3, 61>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned int, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (unsigned int, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 62 && SC_DT_ARITHMETIC_END > 62
template <>
struct NativeCase<3, 62>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned long, const sc_dt::sc_fxnum &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (unsigned long, const sc_dt::sc_fxnum &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxnum &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 63 && SC_DT_ARITHMETIC_END > 63
template <>
struct NativeCase<3, 63>
{
  using Function = const sc_dt::sc_fxval (*)(unsigned long, const sc_dt::sc_fxval &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval (unsigned long, const sc_dt::sc_fxval &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxval &>(sources));
    return type_name<const sc_dt::sc_fxval>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 64 && SC_DT_ARITHMETIC_END > 64
template <>
struct NativeCase<3, 64>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const char *, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const char *, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 65 && SC_DT_ARITHMETIC_END > 65
template <>
struct NativeCase<3, 65>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const char *, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const char *, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const char *>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 66 && SC_DT_ARITHMETIC_END > 66
template <>
struct NativeCase<3, 66>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 67 && SC_DT_ARITHMETIC_END > 67
template <>
struct NativeCase<3, 67>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 68 && SC_DT_ARITHMETIC_END > 68
template <>
struct NativeCase<3, 68>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 69 && SC_DT_ARITHMETIC_END > 69
template <>
struct NativeCase<3, 69>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 70 && SC_DT_ARITHMETIC_END > 70
template <>
struct NativeCase<3, 70>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 71 && SC_DT_ARITHMETIC_END > 71
template <>
struct NativeCase<3, 71>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 72 && SC_DT_ARITHMETIC_END > 72
template <>
struct NativeCase<3, 72>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, const "
                "sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 73 && SC_DT_ARITHMETIC_END > 73
template <>
struct NativeCase<3, 73>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 74 && SC_DT_ARITHMETIC_END > 74
template <>
struct NativeCase<3, 74>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 75 && SC_DT_ARITHMETIC_END > 75
template <>
struct NativeCase<3, 75>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 76 && SC_DT_ARITHMETIC_END > 76
template <>
struct NativeCase<3, 76>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 77 && SC_DT_ARITHMETIC_END > 77
template <>
struct NativeCase<3, 77>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 78 && SC_DT_ARITHMETIC_END > 78
template <>
struct NativeCase<3, 78>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 79 && SC_DT_ARITHMETIC_END > 79
template <>
struct NativeCase<3, 79>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 80 && SC_DT_ARITHMETIC_END > 80
template <>
struct NativeCase<3, 80>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxnum_fast &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxnum_fast &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxnum_fast &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 81 && SC_DT_ARITHMETIC_END > 81
template <>
struct NativeCase<3, 81>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const char *);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const char *)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const char *>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 82 && SC_DT_ARITHMETIC_END > 82
template <>
struct NativeCase<3, 82>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 83 && SC_DT_ARITHMETIC_END > 83
template <>
struct NativeCase<3, 83>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 84 && SC_DT_ARITHMETIC_END > 84
template <>
struct NativeCase<3, 84>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 85 && SC_DT_ARITHMETIC_END > 85
template <>
struct NativeCase<3, 85>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 86 && SC_DT_ARITHMETIC_END > 86
template <>
struct NativeCase<3, 86>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 87 && SC_DT_ARITHMETIC_END > 87
template <>
struct NativeCase<3, 87>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, const "
                "sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 88 && SC_DT_ARITHMETIC_END > 88
template <>
struct NativeCase<3, 88>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, double);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, double)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<double>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 89 && SC_DT_ARITHMETIC_END > 89
template <>
struct NativeCase<3, 89>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, float);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, float)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<float>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 90 && SC_DT_ARITHMETIC_END > 90
template <>
struct NativeCase<3, 90>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 91 && SC_DT_ARITHMETIC_END > 91
template <>
struct NativeCase<3, 91>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 92 && SC_DT_ARITHMETIC_END > 92
template <>
struct NativeCase<3, 92>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 93 && SC_DT_ARITHMETIC_END > 93
template <>
struct NativeCase<3, 93>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 94 && SC_DT_ARITHMETIC_END > 94
template <>
struct NativeCase<3, 94>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 95 && SC_DT_ARITHMETIC_END > 95
template <>
struct NativeCase<3, 95>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_fxval_fast &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_fxval_fast &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_fxval_fast &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 96 && SC_DT_ARITHMETIC_END > 96
template <>
struct NativeCase<3, 96>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_int_base &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 97 && SC_DT_ARITHMETIC_END > 97
template <>
struct NativeCase<3, 97>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_int_base &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_int_base &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 98 && SC_DT_ARITHMETIC_END > 98
template <>
struct NativeCase<3, 98>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_signed &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 99 && SC_DT_ARITHMETIC_END > 99
template <>
struct NativeCase<3, 99>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_signed &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_signed &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 100 && SC_DT_ARITHMETIC_END > 100
template <>
struct NativeCase<3, 100>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 101 && SC_DT_ARITHMETIC_END > 101
template <>
struct NativeCase<3, 101>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 102 && SC_DT_ARITHMETIC_END > 102
template <>
struct NativeCase<3, 102>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_unsigned &, const "
                "sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 103 && SC_DT_ARITHMETIC_END > 103
template <>
struct NativeCase<3, 103>
{
  using Function = const sc_dt::sc_fxval_fast (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (const sc_dt::sc_unsigned &, const "
                "sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 104 && SC_DT_ARITHMETIC_END > 104
template <>
struct NativeCase<3, 104>
{
  using Function = const sc_dt::sc_fxval_fast (*)(double, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (double, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 105 && SC_DT_ARITHMETIC_END > 105
template <>
struct NativeCase<3, 105>
{
  using Function = const sc_dt::sc_fxval_fast (*)(double, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (double, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<double>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 106 && SC_DT_ARITHMETIC_END > 106
template <>
struct NativeCase<3, 106>
{
  using Function = const sc_dt::sc_fxval_fast (*)(float, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (float, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 107 && SC_DT_ARITHMETIC_END > 107
template <>
struct NativeCase<3, 107>
{
  using Function = const sc_dt::sc_fxval_fast (*)(float, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (float, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<float>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 108 && SC_DT_ARITHMETIC_END > 108
template <>
struct NativeCase<3, 108>
{
  using Function = const sc_dt::sc_fxval_fast (*)(int, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (int, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 109 && SC_DT_ARITHMETIC_END > 109
template <>
struct NativeCase<3, 109>
{
  using Function = const sc_dt::sc_fxval_fast (*)(int, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (int, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 110 && SC_DT_ARITHMETIC_END > 110
template <>
struct NativeCase<3, 110>
{
  using Function = const sc_dt::sc_fxval_fast (*)(long, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (long, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 111 && SC_DT_ARITHMETIC_END > 111
template <>
struct NativeCase<3, 111>
{
  using Function = const sc_dt::sc_fxval_fast (*)(long, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (long, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 112 && SC_DT_ARITHMETIC_END > 112
template <>
struct NativeCase<3, 112>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::int64, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (sc_dt::int64, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 113 && SC_DT_ARITHMETIC_END > 113
template <>
struct NativeCase<3, 113>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::int64, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (sc_dt::int64, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 114 && SC_DT_ARITHMETIC_END > 114
template <>
struct NativeCase<3, 114>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::uint64, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (sc_dt::uint64, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 115 && SC_DT_ARITHMETIC_END > 115
template <>
struct NativeCase<3, 115>
{
  using Function = const sc_dt::sc_fxval_fast (*)(sc_dt::uint64, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (sc_dt::uint64, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 116 && SC_DT_ARITHMETIC_END > 116
template <>
struct NativeCase<3, 116>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned int, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (unsigned int, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 117 && SC_DT_ARITHMETIC_END > 117
template <>
struct NativeCase<3, 117>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned int, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (unsigned int, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 118 && SC_DT_ARITHMETIC_END > 118
template <>
struct NativeCase<3, 118>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned long, const sc_dt::sc_fxnum_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (unsigned long, const sc_dt::sc_fxnum_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxnum_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 119 && SC_DT_ARITHMETIC_END > 119
template <>
struct NativeCase<3, 119>
{
  using Function = const sc_dt::sc_fxval_fast (*)(unsigned long, const sc_dt::sc_fxval_fast &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_fxval_fast (unsigned long, const sc_dt::sc_fxval_fast &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_fxval_fast &>(sources));
    return type_name<const sc_dt::sc_fxval_fast>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 120 && SC_DT_ARITHMETIC_END > 120
template <>
struct NativeCase<3, 120>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 121 && SC_DT_ARITHMETIC_END > 121
template <>
struct NativeCase<3, 121>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_signed (const sc_dt::sc_concatref &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 122 && SC_DT_ARITHMETIC_END > 122
template <>
struct NativeCase<3, 122>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_signed (const sc_dt::sc_concatref &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<int>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 123 && SC_DT_ARITHMETIC_END > 123
template <>
struct NativeCase<3, 123>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_signed (const sc_dt::sc_concatref &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<long>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 124 && SC_DT_ARITHMETIC_END > 124
template <>
struct NativeCase<3, 124>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_concatref &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_signed (const sc_dt::sc_concatref &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<sc_dt::int64>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 125 && SC_DT_ARITHMETIC_END > 125
template <>
struct NativeCase<3, 125>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 126 && SC_DT_ARITHMETIC_END > 126
template <>
struct NativeCase<3, 126>
{
  using Function = const sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 127 && SC_DT_ARITHMETIC_END > 127
template <>
struct NativeCase<3, 127>
{
  using Function = const sc_dt::sc_signed (*)(int, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_signed (int, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 128 && SC_DT_ARITHMETIC_END > 128
template <>
struct NativeCase<3, 128>
{
  using Function = const sc_dt::sc_signed (*)(long, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_signed (long, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 129 && SC_DT_ARITHMETIC_END > 129
template <>
struct NativeCase<3, 129>
{
  using Function = const sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 130 && SC_DT_ARITHMETIC_END > 130
template <>
struct NativeCase<3, 130>
{
  using Function = const sc_dt::sc_unsigned (*)(bool, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (bool, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<bool>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 131 && SC_DT_ARITHMETIC_END > 131
template <>
struct NativeCase<3, 131>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, bool);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, bool)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<bool>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 132 && SC_DT_ARITHMETIC_END > 132
template <>
struct NativeCase<3, 132>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
                "sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 133 && SC_DT_ARITHMETIC_END > 133
template <>
struct NativeCase<3, 133>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
                "sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 134 && SC_DT_ARITHMETIC_END > 134
template <>
struct NativeCase<3, 134>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, const "
                "sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_concatref &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 135 && SC_DT_ARITHMETIC_END > 135
template <>
struct NativeCase<3, 135>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 136 && SC_DT_ARITHMETIC_END > 136
template <>
struct NativeCase<3, 136>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<unsigned int>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 137 && SC_DT_ARITHMETIC_END > 137
template <>
struct NativeCase<3, 137>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_concatref &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (const sc_dt::sc_concatref &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_concatref &>(sources), operand<unsigned long>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 138 && SC_DT_ARITHMETIC_END > 138
template <>
struct NativeCase<3, 138>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const "
                "sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 139 && SC_DT_ARITHMETIC_END > 139
template <>
struct NativeCase<3, 139>
{
  using Function = const sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const "
                "sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 140 && SC_DT_ARITHMETIC_END > 140
template <>
struct NativeCase<3, 140>
{
  using Function = const sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 141 && SC_DT_ARITHMETIC_END > 141
template <>
struct NativeCase<3, 141>
{
  using Function = const sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 142 && SC_DT_ARITHMETIC_END > 142
template <>
struct NativeCase<3, 142>
{
  using Function = const sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_concatref &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ const sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_concatref &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_concatref &>(sources));
    return type_name<const sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 143 && SC_DT_ARITHMETIC_END > 143
template <>
struct NativeCase<3, 143>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 144 && SC_DT_ARITHMETIC_END > 144
template <>
struct NativeCase<3, 144>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_int_base &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_int_base &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 145 && SC_DT_ARITHMETIC_END > 145
template <>
struct NativeCase<3, 145>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 146 && SC_DT_ARITHMETIC_END > 146
template <>
struct NativeCase<3, 146>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 147 && SC_DT_ARITHMETIC_END > 147
template <>
struct NativeCase<3, 147>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 148 && SC_DT_ARITHMETIC_END > 148
template <>
struct NativeCase<3, 148>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_signed &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_signed &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 149 && SC_DT_ARITHMETIC_END > 149
template <>
struct NativeCase<3, 149>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_signed &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 150 && SC_DT_ARITHMETIC_END > 150
template <>
struct NativeCase<3, 150>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_signed &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 151 && SC_DT_ARITHMETIC_END > 151
template <>
struct NativeCase<3, 151>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<sc_dt::int64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 152 && SC_DT_ARITHMETIC_END > 152
template <>
struct NativeCase<3, 152>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_signed &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 153 && SC_DT_ARITHMETIC_END > 153
template <>
struct NativeCase<3, 153>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<unsigned int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 154 && SC_DT_ARITHMETIC_END > 154
template <>
struct NativeCase<3, 154>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_signed &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_signed &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_signed &>(sources), operand<unsigned long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 155 && SC_DT_ARITHMETIC_END > 155
template <>
struct NativeCase<3, 155>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_uint_base &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 156 && SC_DT_ARITHMETIC_END > 156
template <>
struct NativeCase<3, 156>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_int_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_int_base &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 157 && SC_DT_ARITHMETIC_END > 157
template <>
struct NativeCase<3, 157>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_unsigned &, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 158 && SC_DT_ARITHMETIC_END > 158
template <>
struct NativeCase<3, 158>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_unsigned &, int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<int>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 159 && SC_DT_ARITHMETIC_END > 159
template <>
struct NativeCase<3, 159>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_unsigned &, long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<long>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 160 && SC_DT_ARITHMETIC_END > 160
template <>
struct NativeCase<3, 160>
{
  using Function = sc_dt::sc_signed (*)(const sc_dt::sc_unsigned &, sc_dt::int64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (const sc_dt::sc_unsigned &, sc_dt::int64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<sc_dt::int64>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 161 && SC_DT_ARITHMETIC_END > 161
template <>
struct NativeCase<3, 161>
{
  using Function = sc_dt::sc_signed (*)(int, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (int, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 162 && SC_DT_ARITHMETIC_END > 162
template <>
struct NativeCase<3, 162>
{
  using Function = sc_dt::sc_signed (*)(int, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (int, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<int>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 163 && SC_DT_ARITHMETIC_END > 163
template <>
struct NativeCase<3, 163>
{
  using Function = sc_dt::sc_signed (*)(long, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (long, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 164 && SC_DT_ARITHMETIC_END > 164
template <>
struct NativeCase<3, 164>
{
  using Function = sc_dt::sc_signed (*)(long, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (long, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<long>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 165 && SC_DT_ARITHMETIC_END > 165
template <>
struct NativeCase<3, 165>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 166 && SC_DT_ARITHMETIC_END > 166
template <>
struct NativeCase<3, 166>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::int64, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (sc_dt::int64, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::int64>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 167 && SC_DT_ARITHMETIC_END > 167
template <>
struct NativeCase<3, 167>
{
  using Function = sc_dt::sc_signed (*)(sc_dt::uint64, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (sc_dt::uint64, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 168 && SC_DT_ARITHMETIC_END > 168
template <>
struct NativeCase<3, 168>
{
  using Function = sc_dt::sc_signed (*)(unsigned int, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (unsigned int, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 169 && SC_DT_ARITHMETIC_END > 169
template <>
struct NativeCase<3, 169>
{
  using Function = sc_dt::sc_signed (*)(unsigned long, const sc_dt::sc_signed &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_signed (unsigned long, const sc_dt::sc_signed &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_signed &>(sources));
    return type_name<sc_dt::sc_signed>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 170 && SC_DT_ARITHMETIC_END > 170
template <>
struct NativeCase<3, 170>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_unsigned (const sc_dt::sc_uint_base &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_uint_base &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 171 && SC_DT_ARITHMETIC_END > 171
template <>
struct NativeCase<3, 171>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_uint_base &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_uint_base &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 172 && SC_DT_ARITHMETIC_END > 172
template <>
struct NativeCase<3, 172>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(
    std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
    "exact overload sc_dt::operator/ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result =
      function(operand<const sc_dt::sc_unsigned &>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 173 && SC_DT_ARITHMETIC_END > 173
template <>
struct NativeCase<3, 173>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, sc_dt::uint64);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, sc_dt::uint64)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<sc_dt::uint64>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 174 && SC_DT_ARITHMETIC_END > 174
template <>
struct NativeCase<3, 174>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned int);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned int)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<unsigned int>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 175 && SC_DT_ARITHMETIC_END > 175
template <>
struct NativeCase<3, 175>
{
  using Function = sc_dt::sc_unsigned (*)(const sc_dt::sc_unsigned &, unsigned long);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_unsigned (const sc_dt::sc_unsigned &, unsigned long)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<const sc_dt::sc_unsigned &>(sources), operand<unsigned long>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 176 && SC_DT_ARITHMETIC_END > 176
template <>
struct NativeCase<3, 176>
{
  using Function = sc_dt::sc_unsigned (*)(sc_dt::uint64, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_unsigned (sc_dt::uint64, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<sc_dt::uint64>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 177 && SC_DT_ARITHMETIC_END > 177
template <>
struct NativeCase<3, 177>
{
  using Function = sc_dt::sc_unsigned (*)(unsigned int, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_unsigned (unsigned int, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned int>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

#if SC_DT_ARITHMETIC_FAMILY == 3 && SC_DT_ARITHMETIC_START <= 178 && SC_DT_ARITHMETIC_END > 178
template <>
struct NativeCase<3, 178>
{
  using Function = sc_dt::sc_unsigned (*)(unsigned long, const sc_dt::sc_unsigned &);
  static constexpr Function function = static_cast<Function>(&sc_dt::operator/);
  static_assert(std::is_same_v<decltype(static_cast<Function>(&sc_dt::operator/)), Function>,
                "exact overload sc_dt::operator/ sc_dt::sc_unsigned (unsigned long, const sc_dt::sc_unsigned &)");
  static std::string call(Sources &sources)
  {
    const auto result = function(operand<unsigned long>(sources), operand<const sc_dt::sc_unsigned &>(sources));
    return type_name<sc_dt::sc_unsigned>() + "|" + render(result);
  }
};
#endif

}  // namespace sc_dt_arithmetic_operators_probe

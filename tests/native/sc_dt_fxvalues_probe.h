/** @file sc_dt_fxvalues_probe.h
 * @brief Exact typed probes for sc_fxval and sc_fxval_fast public contracts.
 * @details Sources and results are native values owned by the caller. Arithmetic
 *          helpers borrow their operands synchronously and retain no callbacks,
 *          Python objects or pointers. All calls hold the GIL and do not enter
 *          the SystemC scheduler; native reports propagate unchanged.
 */
#ifndef PYSYSTEMC_TEST_SC_DT_FXVALUES_PROBE_H
#define PYSYSTEMC_TEST_SC_DT_FXVALUES_PROBE_H

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>

namespace sc_dt_fxvalues_probe
{

template <typename T, bool Value = std::is_arithmetic<T>::value || std::is_pointer<T>::value>
struct parameter
{
  using type = const T &;
};
template <typename T>
struct parameter<T, true>
{
  using type = T;
};
template <typename T>
using parameter_t = typename parameter<T>::type;

/** @brief Representative live objects that preserve native operand types. */
struct Sources
{
  sc_dt::sc_fxval value{3.5};
  sc_dt::sc_fxval_fast value_fast{3.5};
  sc_dt::sc_fixed<23, 11> number{3.5};
  sc_dt::sc_fixed_fast<23, 11> number_fast{3.5};
  sc_dt::sc_int<19> sint{3};
  sc_dt::sc_uint<19> uint{3};
  sc_dt::sc_signed signed_value{32};
  sc_dt::sc_unsigned unsigned_value{32};

  /** @brief Initialize dynamic integer operands to exact nonzero values. */
  Sources()
  {
    signed_value = -3;
    unsigned_value = 5;
  }
};

/** @brief Return base-typed references for exact public overload selection. */
struct SourceView
{
  static const sc_dt::sc_fxnum &number(const Sources &s)
  {
    return s.number;
  }
  static const sc_dt::sc_fxnum_fast &number_fast(const Sources &s)
  {
    return s.number_fast;
  }
  static const sc_dt::sc_fxval &value(const Sources &s)
  {
    return s.value;
  }
  static const sc_dt::sc_fxval_fast &value_fast(const Sources &s)
  {
    return s.value_fast;
  }
  static const sc_dt::sc_int_base &sint(const Sources &s)
  {
    return s.sint;
  }
  static const sc_dt::sc_uint_base &uint(const Sources &s)
  {
    return s.uint;
  }
  static const sc_dt::sc_signed &signed_value(const Sources &s)
  {
    return s.signed_value;
  }
  static const sc_dt::sc_unsigned &unsigned_value(const Sources &s)
  {
    return s.unsigned_value;
  }
};

/** @brief Return exact scalar types without Python's automatic integer narrowing. */
struct Scalars
{
  static int integer()
  {
    return -3;
  }
  static unsigned int unsigned_integer()
  {
    return 5U;
  }
  static long long_integer()
  {
    return -300L;
  }
  static unsigned long unsigned_long_integer()
  {
    return 500UL;
  }
  static float real_float()
  {
    return 1.25F;
  }
  static double real_double()
  {
    return 2.75;
  }
  static sc_dt::int64 signed_64()
  {
    return static_cast<sc_dt::int64>(-1234567890123LL);
  }
  static sc_dt::uint64 unsigned_64()
  {
    return static_cast<sc_dt::uint64>(1234567890123ULL);
  }
  static const char *text()
  {
    return "2.5";
  }
};

/** @brief Create a decimal string through the value class's native overload. */
template <typename Value>
std::string decimal(const Value &value)
{
  return value.to_string(sc_dt::SC_DEC);
}

/** @brief Return the stable native signature of a representative value. */
template <typename Value>
std::string observe(const Value &value)
{
  std::ostringstream out;
  out << std::setprecision(17) << value.to_double() << '|' << decimal(value) << '|' << value.is_neg() << '|'
      << value.is_zero() << '|' << value.is_nan() << '|' << value.is_inf() << '|' << value.is_normal() << '|'
      << value.rounding_flag();
  return out.str();
}

/** @brief Invoke four exact typed native arithmetic-function overloads. */
template <typename Output, typename Left, typename Right>
std::string binary(const Left &left, const Right &right)
{
  using Add = void (*)(Output &, parameter_t<Left>, parameter_t<Right>);
  using Sub = Add;
  using Mult = Add;
  using Div = Add;
  static_assert(std::is_same<decltype(static_cast<Add>(&sc_dt::add)), Add>::value, "exact add overload");
  static_assert(std::is_same<decltype(static_cast<Sub>(&sc_dt::sub)), Sub>::value, "exact sub overload");
  static_assert(std::is_same<decltype(static_cast<Mult>(&sc_dt::mult)), Mult>::value, "exact mult overload");
  static_assert(std::is_same<decltype(static_cast<Div>(&sc_dt::div)), Div>::value, "exact div overload");
  Output result(0);
  const Add add = static_cast<Add>(&sc_dt::add);
  const Sub sub = static_cast<Sub>(&sc_dt::sub);
  const Mult mult = static_cast<Mult>(&sc_dt::mult);
  const Div div = static_cast<Div>(&sc_dt::div);
  std::ostringstream out;
  add(result, left, right);
  out << decimal(result) << '|';
  sub(result, left, right);
  out << decimal(result) << '|';
  mult(result, left, right);
  out << decimal(result) << '|';
  div(result, left, right);
  out << decimal(result);
  return out.str();
}

/** @brief Invoke exact class-static formatting overloads without template deduction. */
struct Formatting
{
  static std::string stream_value(const sc_dt::sc_fxval &value)
  {
    std::ostringstream out;
    value.print(out);
    return out.str();
  }
  static std::string stream_fast(const sc_dt::sc_fxval_fast &value)
  {
    std::ostringstream out;
    value.print(out);
    return out.str();
  }
  static std::string dump_value(const sc_dt::sc_fxval &value)
  {
    std::ostringstream out;
    value.dump(out);
    return out.str();
  }
  static std::string dump_fast(const sc_dt::sc_fxval_fast &value)
  {
    std::ostringstream out;
    value.dump(out);
    return out.str();
  }
  static double precision_value(const sc_dt::sc_fxval &value)
  {
    return value.to_double();
  }
  static double precision_fast(const sc_dt::sc_fxval_fast &value)
  {
    return value.to_double();
  }
};

#define SC_FXVAL_CTOR(T, OBS, ARG) static_assert(std::is_constructible<T, ARG, OBS *>::value, "constructor source type")
#define SC_FXVAL_CTORS(T, OBS, OTHER, OTHER_NUM, OTHER_FAST) \
  SC_FXVAL_CTOR(T, OBS, int);                                \
  SC_FXVAL_CTOR(T, OBS, unsigned int);                       \
  SC_FXVAL_CTOR(T, OBS, long);                               \
  SC_FXVAL_CTOR(T, OBS, unsigned long);                      \
  SC_FXVAL_CTOR(T, OBS, float);                              \
  SC_FXVAL_CTOR(T, OBS, double);                             \
  SC_FXVAL_CTOR(T, OBS, const char *);                       \
  SC_FXVAL_CTOR(T, OBS, const T &);                          \
  SC_FXVAL_CTOR(T, OBS, const OTHER &);                      \
  SC_FXVAL_CTOR(T, OBS, const OTHER_NUM &);                  \
  SC_FXVAL_CTOR(T, OBS, const OTHER_FAST &);                 \
  SC_FXVAL_CTOR(T, OBS, sc_dt::int64);                       \
  SC_FXVAL_CTOR(T, OBS, sc_dt::uint64);                      \
  SC_FXVAL_CTOR(T, OBS, const sc_dt::sc_int_base &);         \
  SC_FXVAL_CTOR(T, OBS, const sc_dt::sc_uint_base &);        \
  SC_FXVAL_CTOR(T, OBS, const sc_dt::sc_signed &);           \
  SC_FXVAL_CTOR(T, OBS, const sc_dt::sc_unsigned &);         \
  static_assert(std::is_constructible<T, OBS *>::value, "observer constructor")
SC_FXVAL_CTORS(sc_dt::sc_fxval, sc_dt::sc_fxval_observer, sc_dt::sc_fxval_fast, sc_dt::sc_fxnum, sc_dt::sc_fxnum_fast);
SC_FXVAL_CTORS(sc_dt::sc_fxval_fast, sc_dt::sc_fxval_fast_observer, sc_dt::sc_fxval, sc_dt::sc_fxnum,
               sc_dt::sc_fxnum_fast);
static_assert(std::is_destructible<sc_dt::sc_fxval>::value, "sc_fxval destructor");
static_assert(std::is_destructible<sc_dt::sc_fxval_fast>::value, "sc_fxval_fast destructor");
static_assert(std::is_same<decltype(static_cast<double>(std::declval<const sc_dt::sc_fxval &>())), double>::value,
              "sc_fxval double conversion");
static_assert(std::is_same<decltype(static_cast<double>(std::declval<const sc_dt::sc_fxval_fast &>())), double>::value,
              "sc_fxval_fast double conversion");
#undef SC_FXVAL_CTORS
#undef SC_FXVAL_CTOR

// Every distinct public method/name/signature pair in the 133 inventoried method rows.
static_assert(
  std::is_same<decltype(static_cast<void (sc_dt::sc_fxval::*)(::std::ostream &) const>(&sc_dt::sc_fxval::dump)),
               void (sc_dt::sc_fxval::*)(::std::ostream &) const>::value,
  "sc_fxval::dump exact member signature");
static_assert(std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::is_inf)),
                           bool (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::is_inf exact member signature");
static_assert(std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::is_nan)),
                           bool (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::is_nan exact member signature");
static_assert(std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::is_neg)),
                           bool (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::is_neg exact member signature");
static_assert(std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::is_normal)),
                           bool (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::is_normal exact member signature");
static_assert(std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::is_zero)),
                           bool (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::is_zero exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum &)>(
                             &sc_dt::sc_fxval::operator*=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum &)>::value,
              "sc_fxval::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_signed &)>(
                             &sc_dt::sc_fxval::operator*=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_signed &)>::value,
              "sc_fxval::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_uint_base &)>(
                             &sc_dt::sc_fxval::operator*=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_uint_base &)>::value,
              "sc_fxval::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned int)>(&sc_dt::sc_fxval::operator*=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned int)>::value,
  "sc_fxval::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned long)>(
                             &sc_dt::sc_fxval::operator*=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned long)>::value,
              "sc_fxval::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const char *)>(&sc_dt::sc_fxval::operator*=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const char *)>::value,
  "sc_fxval::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::int64)>(&sc_dt::sc_fxval::operator*=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::int64)>::value,
  "sc_fxval::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::uint64)>(
                             &sc_dt::sc_fxval::operator*=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::uint64)>::value,
              "sc_fxval::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>(&sc_dt::sc_fxval::operator*=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>::value,
  "sc_fxval::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval &)>(
                             &sc_dt::sc_fxval::operator*=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval &)>::value,
              "sc_fxval::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum_fast &)>(
                             &sc_dt::sc_fxval::operator*=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum_fast &)>::value,
              "sc_fxval::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval_fast &)>(
                             &sc_dt::sc_fxval::operator*=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval_fast &)>::value,
              "sc_fxval::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(float)>(&sc_dt::sc_fxval::operator*=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(float)>::value,
  "sc_fxval::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_int_base &)>(
                             &sc_dt::sc_fxval::operator*=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_int_base &)>::value,
              "sc_fxval::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(double)>(&sc_dt::sc_fxval::operator*=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(double)>::value,
  "sc_fxval::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(long)>(&sc_dt::sc_fxval::operator*=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(long)>::value,
  "sc_fxval::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_unsigned &)>(
                             &sc_dt::sc_fxval::operator*=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_unsigned &)>::value,
              "sc_fxval::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<const sc_dt::sc_fxval &(sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::operator+)),
               const sc_dt::sc_fxval &(sc_dt::sc_fxval::*)() const>::value,
  "sc_fxval::operator+ exact member signature");
static_assert(
  std::is_same<decltype(static_cast<const sc_dt::sc_fxval (sc_dt::sc_fxval::*)(int)>(&sc_dt::sc_fxval::operator++)),
               const sc_dt::sc_fxval (sc_dt::sc_fxval::*)(int)>::value,
  "sc_fxval::operator++ exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)()>(&sc_dt::sc_fxval::operator++)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)()>::value,
              "sc_fxval::operator++ exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum &)>(
                             &sc_dt::sc_fxval::operator+=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum &)>::value,
              "sc_fxval::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_uint_base &)>(
                             &sc_dt::sc_fxval::operator+=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_uint_base &)>::value,
              "sc_fxval::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum_fast &)>(
                             &sc_dt::sc_fxval::operator+=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum_fast &)>::value,
              "sc_fxval::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>(&sc_dt::sc_fxval::operator+=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>::value,
  "sc_fxval::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::int64)>(&sc_dt::sc_fxval::operator+=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::int64)>::value,
  "sc_fxval::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(float)>(&sc_dt::sc_fxval::operator+=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(float)>::value,
  "sc_fxval::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_signed &)>(
                             &sc_dt::sc_fxval::operator+=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_signed &)>::value,
              "sc_fxval::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(long)>(&sc_dt::sc_fxval::operator+=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(long)>::value,
  "sc_fxval::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const char *)>(&sc_dt::sc_fxval::operator+=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const char *)>::value,
  "sc_fxval::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::uint64)>(
                             &sc_dt::sc_fxval::operator+=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::uint64)>::value,
              "sc_fxval::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval &)>(
                             &sc_dt::sc_fxval::operator+=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval &)>::value,
              "sc_fxval::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval_fast &)>(
                             &sc_dt::sc_fxval::operator+=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval_fast &)>::value,
              "sc_fxval::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_unsigned &)>(
                             &sc_dt::sc_fxval::operator+=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_unsigned &)>::value,
              "sc_fxval::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(double)>(&sc_dt::sc_fxval::operator+=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(double)>::value,
  "sc_fxval::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_int_base &)>(
                             &sc_dt::sc_fxval::operator+=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_int_base &)>::value,
              "sc_fxval::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned long)>(
                             &sc_dt::sc_fxval::operator+=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned long)>::value,
              "sc_fxval::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned int)>(&sc_dt::sc_fxval::operator+=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned int)>::value,
  "sc_fxval::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<const sc_dt::sc_fxval (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::operator-)),
               const sc_dt::sc_fxval (sc_dt::sc_fxval::*)() const>::value,
  "sc_fxval::operator- exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)()>(&sc_dt::sc_fxval::operator--)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)()>::value,
              "sc_fxval::operator-- exact member signature");
static_assert(
  std::is_same<decltype(static_cast<const sc_dt::sc_fxval (sc_dt::sc_fxval::*)(int)>(&sc_dt::sc_fxval::operator--)),
               const sc_dt::sc_fxval (sc_dt::sc_fxval::*)(int)>::value,
  "sc_fxval::operator-- exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(double)>(&sc_dt::sc_fxval::operator-=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(double)>::value,
  "sc_fxval::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_uint_base &)>(
                             &sc_dt::sc_fxval::operator-=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_uint_base &)>::value,
              "sc_fxval::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval &)>(
                             &sc_dt::sc_fxval::operator-=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval &)>::value,
              "sc_fxval::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_int_base &)>(
                             &sc_dt::sc_fxval::operator-=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_int_base &)>::value,
              "sc_fxval::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum_fast &)>(
                             &sc_dt::sc_fxval::operator-=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum_fast &)>::value,
              "sc_fxval::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_signed &)>(
                             &sc_dt::sc_fxval::operator-=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_signed &)>::value,
              "sc_fxval::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const char *)>(&sc_dt::sc_fxval::operator-=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const char *)>::value,
  "sc_fxval::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum &)>(
                             &sc_dt::sc_fxval::operator-=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum &)>::value,
              "sc_fxval::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval_fast &)>(
                             &sc_dt::sc_fxval::operator-=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval_fast &)>::value,
              "sc_fxval::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::int64)>(&sc_dt::sc_fxval::operator-=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::int64)>::value,
  "sc_fxval::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(long)>(&sc_dt::sc_fxval::operator-=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(long)>::value,
  "sc_fxval::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::uint64)>(
                             &sc_dt::sc_fxval::operator-=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::uint64)>::value,
              "sc_fxval::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>(&sc_dt::sc_fxval::operator-=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>::value,
  "sc_fxval::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_unsigned &)>(
                             &sc_dt::sc_fxval::operator-=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_unsigned &)>::value,
              "sc_fxval::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(float)>(&sc_dt::sc_fxval::operator-=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(float)>::value,
  "sc_fxval::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned long)>(
                             &sc_dt::sc_fxval::operator-=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned long)>::value,
              "sc_fxval::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned int)>(&sc_dt::sc_fxval::operator-=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned int)>::value,
  "sc_fxval::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>(&sc_dt::sc_fxval::operator/=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>::value,
  "sc_fxval::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval &)>(
                             &sc_dt::sc_fxval::operator/=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval &)>::value,
              "sc_fxval::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_signed &)>(
                             &sc_dt::sc_fxval::operator/=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_signed &)>::value,
              "sc_fxval::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(long)>(&sc_dt::sc_fxval::operator/=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(long)>::value,
  "sc_fxval::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const char *)>(&sc_dt::sc_fxval::operator/=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const char *)>::value,
  "sc_fxval::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum_fast &)>(
                             &sc_dt::sc_fxval::operator/=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum_fast &)>::value,
              "sc_fxval::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(float)>(&sc_dt::sc_fxval::operator/=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(float)>::value,
  "sc_fxval::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_uint_base &)>(
                             &sc_dt::sc_fxval::operator/=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_uint_base &)>::value,
              "sc_fxval::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned int)>(&sc_dt::sc_fxval::operator/=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned int)>::value,
  "sc_fxval::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::int64)>(&sc_dt::sc_fxval::operator/=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::int64)>::value,
  "sc_fxval::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval_fast &)>(
                             &sc_dt::sc_fxval::operator/=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval_fast &)>::value,
              "sc_fxval::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::uint64)>(
                             &sc_dt::sc_fxval::operator/=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::uint64)>::value,
              "sc_fxval::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum &)>(
                             &sc_dt::sc_fxval::operator/=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum &)>::value,
              "sc_fxval::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_int_base &)>(
                             &sc_dt::sc_fxval::operator/=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_int_base &)>::value,
              "sc_fxval::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned long)>(
                             &sc_dt::sc_fxval::operator/=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned long)>::value,
              "sc_fxval::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(double)>(&sc_dt::sc_fxval::operator/=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(double)>::value,
  "sc_fxval::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_unsigned &)>(
                             &sc_dt::sc_fxval::operator/=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_unsigned &)>::value,
              "sc_fxval::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>(&sc_dt::sc_fxval::operator<<=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>::value,
  "sc_fxval::operator<<= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::int64)>(&sc_dt::sc_fxval::operator=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::int64)>::value,
  "sc_fxval::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_signed &)>(
                             &sc_dt::sc_fxval::operator=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_signed &)>::value,
              "sc_fxval::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum &)>(
                             &sc_dt::sc_fxval::operator=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum &)>::value,
              "sc_fxval::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned int)>(&sc_dt::sc_fxval::operator=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned int)>::value,
  "sc_fxval::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned long)>(&sc_dt::sc_fxval::operator=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(unsigned long)>::value,
  "sc_fxval::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const char *)>(&sc_dt::sc_fxval::operator=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const char *)>::value,
  "sc_fxval::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(double)>(&sc_dt::sc_fxval::operator=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(double)>::value,
  "sc_fxval::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::uint64)>(&sc_dt::sc_fxval::operator=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(sc_dt::uint64)>::value,
  "sc_fxval::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>(&sc_dt::sc_fxval::operator=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>::value,
  "sc_fxval::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum_fast &)>(
                             &sc_dt::sc_fxval::operator=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxnum_fast &)>::value,
              "sc_fxval::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_int_base &)>(
                             &sc_dt::sc_fxval::operator=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_int_base &)>::value,
              "sc_fxval::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval_fast &)>(
                             &sc_dt::sc_fxval::operator=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval_fast &)>::value,
              "sc_fxval::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_uint_base &)>(
                             &sc_dt::sc_fxval::operator=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_uint_base &)>::value,
              "sc_fxval::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(long)>(&sc_dt::sc_fxval::operator=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(long)>::value,
  "sc_fxval::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(float)>(&sc_dt::sc_fxval::operator=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(float)>::value,
  "sc_fxval::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_unsigned &)>(
                             &sc_dt::sc_fxval::operator=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_unsigned &)>::value,
              "sc_fxval::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval &)>(
                             &sc_dt::sc_fxval::operator=)),
                           sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(const sc_dt::sc_fxval &)>::value,
              "sc_fxval::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>(&sc_dt::sc_fxval::operator>>=)),
               sc_dt::sc_fxval &(sc_dt::sc_fxval::*)(int)>::value,
  "sc_fxval::operator>>= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<void (sc_dt::sc_fxval::*)(::std::ostream &) const>(&sc_dt::sc_fxval::print)),
               void (sc_dt::sc_fxval::*)(::std::ostream &) const>::value,
  "sc_fxval::print exact member signature");
static_assert(std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::rounding_flag)),
                           bool (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::rounding_flag exact member signature");
static_assert(std::is_same<decltype(static_cast<void (sc_dt::sc_fxval::*)(::std::istream &)>(&sc_dt::sc_fxval::scan)),
                           void (sc_dt::sc_fxval::*)(::std::istream &)>::value,
              "sc_fxval::scan exact member signature");
static_assert(
  std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_bin)),
               const std::string (sc_dt::sc_fxval::*)() const>::value,
  "sc_fxval::to_bin exact member signature");
static_assert(
  std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_dec)),
               const std::string (sc_dt::sc_fxval::*)() const>::value,
  "sc_fxval::to_dec exact member signature");
static_assert(std::is_same<decltype(static_cast<double (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_double)),
                           double (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::to_double exact member signature");
static_assert(std::is_same<decltype(static_cast<float (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_float)),
                           float (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::to_float exact member signature");
static_assert(
  std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_hex)),
               const std::string (sc_dt::sc_fxval::*)() const>::value,
  "sc_fxval::to_hex exact member signature");
static_assert(std::is_same<decltype(static_cast<int (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_int)),
                           int (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::to_int exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::int64 (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_int64)),
                           sc_dt::int64 (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::to_int64 exact member signature");
static_assert(std::is_same<decltype(static_cast<long (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_long)),
                           long (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::to_long exact member signature");
static_assert(
  std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_oct)),
               const std::string (sc_dt::sc_fxval::*)() const>::value,
  "sc_fxval::to_oct exact member signature");
static_assert(std::is_same<decltype(static_cast<short (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_short)),
                           short (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::to_short exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval::*)(sc_dt::sc_numrep, bool) const>(
                             &sc_dt::sc_fxval::to_string)),
                           const std::string (sc_dt::sc_fxval::*)(sc_dt::sc_numrep, bool) const>::value,
              "sc_fxval::to_string exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval::*)(sc_dt::sc_numrep) const>(
                             &sc_dt::sc_fxval::to_string)),
                           const std::string (sc_dt::sc_fxval::*)(sc_dt::sc_numrep) const>::value,
              "sc_fxval::to_string exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval::*)(sc_dt::sc_numrep, sc_dt::sc_fmt)
                                                  const>(&sc_dt::sc_fxval::to_string)),
                           const std::string (sc_dt::sc_fxval::*)(sc_dt::sc_numrep, sc_dt::sc_fmt) const>::value,
              "sc_fxval::to_string exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval::*)(
                                      sc_dt::sc_numrep, bool, sc_dt::sc_fmt) const>(&sc_dt::sc_fxval::to_string)),
                           const std::string (sc_dt::sc_fxval::*)(sc_dt::sc_numrep, bool, sc_dt::sc_fmt) const>::value,
              "sc_fxval::to_string exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval::*)(sc_dt::sc_fmt) const>(
                             &sc_dt::sc_fxval::to_string)),
                           const std::string (sc_dt::sc_fxval::*)(sc_dt::sc_fmt) const>::value,
              "sc_fxval::to_string exact member signature");
static_assert(
  std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_string)),
               const std::string (sc_dt::sc_fxval::*)() const>::value,
  "sc_fxval::to_string exact member signature");
static_assert(std::is_same<decltype(static_cast<unsigned int (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_uint)),
                           unsigned int (sc_dt::sc_fxval::*)() const>::value,
              "sc_fxval::to_uint exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::uint64 (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_uint64)),
               sc_dt::uint64 (sc_dt::sc_fxval::*)() const>::value,
  "sc_fxval::to_uint64 exact member signature");
static_assert(
  std::is_same<decltype(static_cast<unsigned long (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_ulong)),
               unsigned long (sc_dt::sc_fxval::*)() const>::value,
  "sc_fxval::to_ulong exact member signature");
static_assert(
  std::is_same<decltype(static_cast<unsigned short (sc_dt::sc_fxval::*)() const>(&sc_dt::sc_fxval::to_ushort)),
               unsigned short (sc_dt::sc_fxval::*)() const>::value,
  "sc_fxval::to_ushort exact member signature");
static_assert(std::is_same<decltype(static_cast<void (sc_dt::sc_fxval_fast::*)(::std::ostream &) const>(
                             &sc_dt::sc_fxval_fast::dump)),
                           void (sc_dt::sc_fxval_fast::*)(::std::ostream &) const>::value,
              "sc_fxval_fast::dump exact member signature");
static_assert(std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::is_inf)),
                           bool (sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::is_inf exact member signature");
static_assert(std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::is_nan)),
                           bool (sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::is_nan exact member signature");
static_assert(std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::is_neg)),
                           bool (sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::is_neg exact member signature");
static_assert(
  std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::is_normal)),
               bool (sc_dt::sc_fxval_fast::*)() const>::value,
  "sc_fxval_fast::is_normal exact member signature");
static_assert(
  std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::is_zero)),
               bool (sc_dt::sc_fxval_fast::*)() const>::value,
  "sc_fxval_fast::is_zero exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_uint_base &)>(
                 &sc_dt::sc_fxval_fast::operator*=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_uint_base &)>::value,
  "sc_fxval_fast::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::uint64)>(
                             &sc_dt::sc_fxval_fast::operator*=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::uint64)>::value,
              "sc_fxval_fast::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::int64)>(
                             &sc_dt::sc_fxval_fast::operator*=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::int64)>::value,
              "sc_fxval_fast::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum_fast &)>(
                 &sc_dt::sc_fxval_fast::operator*=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum_fast &)>::value,
  "sc_fxval_fast::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const char *)>(
                             &sc_dt::sc_fxval_fast::operator*=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const char *)>::value,
              "sc_fxval_fast::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_int_base &)>(
                 &sc_dt::sc_fxval_fast::operator*=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_int_base &)>::value,
  "sc_fxval_fast::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>(
                             &sc_dt::sc_fxval_fast::operator*=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>::value,
              "sc_fxval_fast::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum &)>(
                 &sc_dt::sc_fxval_fast::operator*=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum &)>::value,
  "sc_fxval_fast::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned int)>(
                             &sc_dt::sc_fxval_fast::operator*=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned int)>::value,
              "sc_fxval_fast::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned long)>(
                             &sc_dt::sc_fxval_fast::operator*=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned long)>::value,
              "sc_fxval_fast::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(float)>(
                             &sc_dt::sc_fxval_fast::operator*=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(float)>::value,
              "sc_fxval_fast::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval &)>(
                 &sc_dt::sc_fxval_fast::operator*=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval &)>::value,
  "sc_fxval_fast::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval_fast &)>(
                 &sc_dt::sc_fxval_fast::operator*=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval_fast &)>::value,
  "sc_fxval_fast::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(double)>(
                             &sc_dt::sc_fxval_fast::operator*=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(double)>::value,
              "sc_fxval_fast::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_signed &)>(
                 &sc_dt::sc_fxval_fast::operator*=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_signed &)>::value,
  "sc_fxval_fast::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(long)>(
                             &sc_dt::sc_fxval_fast::operator*=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(long)>::value,
              "sc_fxval_fast::operator*= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_unsigned &)>(
                 &sc_dt::sc_fxval_fast::operator*=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_unsigned &)>::value,
  "sc_fxval_fast::operator*= exact member signature");
static_assert(std::is_same<decltype(static_cast<const sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)() const>(
                             &sc_dt::sc_fxval_fast::operator+)),
                           const sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::operator+ exact member signature");
static_assert(std::is_same<decltype(static_cast<const sc_dt::sc_fxval_fast (sc_dt::sc_fxval_fast::*)(int)>(
                             &sc_dt::sc_fxval_fast::operator++)),
                           const sc_dt::sc_fxval_fast (sc_dt::sc_fxval_fast::*)(int)>::value,
              "sc_fxval_fast::operator++ exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)()>(
                             &sc_dt::sc_fxval_fast::operator++)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)()>::value,
              "sc_fxval_fast::operator++ exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval_fast &)>(
                 &sc_dt::sc_fxval_fast::operator+=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval_fast &)>::value,
  "sc_fxval_fast::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_int_base &)>(
                 &sc_dt::sc_fxval_fast::operator+=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_int_base &)>::value,
  "sc_fxval_fast::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(long)>(
                             &sc_dt::sc_fxval_fast::operator+=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(long)>::value,
              "sc_fxval_fast::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_unsigned &)>(
                 &sc_dt::sc_fxval_fast::operator+=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_unsigned &)>::value,
  "sc_fxval_fast::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::uint64)>(
                             &sc_dt::sc_fxval_fast::operator+=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::uint64)>::value,
              "sc_fxval_fast::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum_fast &)>(
                 &sc_dt::sc_fxval_fast::operator+=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum_fast &)>::value,
  "sc_fxval_fast::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_uint_base &)>(
                 &sc_dt::sc_fxval_fast::operator+=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_uint_base &)>::value,
  "sc_fxval_fast::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_signed &)>(
                 &sc_dt::sc_fxval_fast::operator+=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_signed &)>::value,
  "sc_fxval_fast::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned long)>(
                             &sc_dt::sc_fxval_fast::operator+=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned long)>::value,
              "sc_fxval_fast::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum &)>(
                 &sc_dt::sc_fxval_fast::operator+=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum &)>::value,
  "sc_fxval_fast::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(double)>(
                             &sc_dt::sc_fxval_fast::operator+=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(double)>::value,
              "sc_fxval_fast::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>(
                             &sc_dt::sc_fxval_fast::operator+=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>::value,
              "sc_fxval_fast::operator+= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval &)>(
                 &sc_dt::sc_fxval_fast::operator+=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval &)>::value,
  "sc_fxval_fast::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned int)>(
                             &sc_dt::sc_fxval_fast::operator+=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned int)>::value,
              "sc_fxval_fast::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(float)>(
                             &sc_dt::sc_fxval_fast::operator+=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(float)>::value,
              "sc_fxval_fast::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const char *)>(
                             &sc_dt::sc_fxval_fast::operator+=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const char *)>::value,
              "sc_fxval_fast::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::int64)>(
                             &sc_dt::sc_fxval_fast::operator+=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::int64)>::value,
              "sc_fxval_fast::operator+= exact member signature");
static_assert(std::is_same<decltype(static_cast<const sc_dt::sc_fxval_fast (sc_dt::sc_fxval_fast::*)() const>(
                             &sc_dt::sc_fxval_fast::operator-)),
                           const sc_dt::sc_fxval_fast (sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::operator- exact member signature");
static_assert(std::is_same<decltype(static_cast<const sc_dt::sc_fxval_fast (sc_dt::sc_fxval_fast::*)(int)>(
                             &sc_dt::sc_fxval_fast::operator--)),
                           const sc_dt::sc_fxval_fast (sc_dt::sc_fxval_fast::*)(int)>::value,
              "sc_fxval_fast::operator-- exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)()>(
                             &sc_dt::sc_fxval_fast::operator--)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)()>::value,
              "sc_fxval_fast::operator-- exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(long)>(
                             &sc_dt::sc_fxval_fast::operator-=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(long)>::value,
              "sc_fxval_fast::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_int_base &)>(
                 &sc_dt::sc_fxval_fast::operator-=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_int_base &)>::value,
  "sc_fxval_fast::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const char *)>(
                             &sc_dt::sc_fxval_fast::operator-=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const char *)>::value,
              "sc_fxval_fast::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval_fast &)>(
                 &sc_dt::sc_fxval_fast::operator-=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval_fast &)>::value,
  "sc_fxval_fast::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_unsigned &)>(
                 &sc_dt::sc_fxval_fast::operator-=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_unsigned &)>::value,
  "sc_fxval_fast::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_signed &)>(
                 &sc_dt::sc_fxval_fast::operator-=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_signed &)>::value,
  "sc_fxval_fast::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>(
                             &sc_dt::sc_fxval_fast::operator-=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>::value,
              "sc_fxval_fast::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(float)>(
                             &sc_dt::sc_fxval_fast::operator-=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(float)>::value,
              "sc_fxval_fast::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::uint64)>(
                             &sc_dt::sc_fxval_fast::operator-=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::uint64)>::value,
              "sc_fxval_fast::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_uint_base &)>(
                 &sc_dt::sc_fxval_fast::operator-=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_uint_base &)>::value,
  "sc_fxval_fast::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval &)>(
                 &sc_dt::sc_fxval_fast::operator-=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval &)>::value,
  "sc_fxval_fast::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum &)>(
                 &sc_dt::sc_fxval_fast::operator-=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum &)>::value,
  "sc_fxval_fast::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned int)>(
                             &sc_dt::sc_fxval_fast::operator-=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned int)>::value,
              "sc_fxval_fast::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned long)>(
                             &sc_dt::sc_fxval_fast::operator-=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned long)>::value,
              "sc_fxval_fast::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(double)>(
                             &sc_dt::sc_fxval_fast::operator-=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(double)>::value,
              "sc_fxval_fast::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum_fast &)>(
                 &sc_dt::sc_fxval_fast::operator-=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum_fast &)>::value,
  "sc_fxval_fast::operator-= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::int64)>(
                             &sc_dt::sc_fxval_fast::operator-=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::int64)>::value,
              "sc_fxval_fast::operator-= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_int_base &)>(
                 &sc_dt::sc_fxval_fast::operator/=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_int_base &)>::value,
  "sc_fxval_fast::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(float)>(
                             &sc_dt::sc_fxval_fast::operator/=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(float)>::value,
              "sc_fxval_fast::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval_fast &)>(
                 &sc_dt::sc_fxval_fast::operator/=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval_fast &)>::value,
  "sc_fxval_fast::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(double)>(
                             &sc_dt::sc_fxval_fast::operator/=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(double)>::value,
              "sc_fxval_fast::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned long)>(
                             &sc_dt::sc_fxval_fast::operator/=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned long)>::value,
              "sc_fxval_fast::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_unsigned &)>(
                 &sc_dt::sc_fxval_fast::operator/=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_unsigned &)>::value,
  "sc_fxval_fast::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_signed &)>(
                 &sc_dt::sc_fxval_fast::operator/=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_signed &)>::value,
  "sc_fxval_fast::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned int)>(
                             &sc_dt::sc_fxval_fast::operator/=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned int)>::value,
              "sc_fxval_fast::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_uint_base &)>(
                 &sc_dt::sc_fxval_fast::operator/=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_uint_base &)>::value,
  "sc_fxval_fast::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum_fast &)>(
                 &sc_dt::sc_fxval_fast::operator/=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum_fast &)>::value,
  "sc_fxval_fast::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum &)>(
                 &sc_dt::sc_fxval_fast::operator/=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum &)>::value,
  "sc_fxval_fast::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(long)>(
                             &sc_dt::sc_fxval_fast::operator/=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(long)>::value,
              "sc_fxval_fast::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>(
                             &sc_dt::sc_fxval_fast::operator/=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>::value,
              "sc_fxval_fast::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const char *)>(
                             &sc_dt::sc_fxval_fast::operator/=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const char *)>::value,
              "sc_fxval_fast::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::int64)>(
                             &sc_dt::sc_fxval_fast::operator/=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::int64)>::value,
              "sc_fxval_fast::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::uint64)>(
                             &sc_dt::sc_fxval_fast::operator/=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::uint64)>::value,
              "sc_fxval_fast::operator/= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval &)>(
                 &sc_dt::sc_fxval_fast::operator/=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval &)>::value,
  "sc_fxval_fast::operator/= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>(
                             &sc_dt::sc_fxval_fast::operator<<=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>::value,
              "sc_fxval_fast::operator<<= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum_fast &)>(
                 &sc_dt::sc_fxval_fast::operator=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum_fast &)>::value,
  "sc_fxval_fast::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(long)>(
                             &sc_dt::sc_fxval_fast::operator=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(long)>::value,
              "sc_fxval_fast::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(float)>(
                             &sc_dt::sc_fxval_fast::operator=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(float)>::value,
              "sc_fxval_fast::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(double)>(
                             &sc_dt::sc_fxval_fast::operator=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(double)>::value,
              "sc_fxval_fast::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned long)>(
                             &sc_dt::sc_fxval_fast::operator=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned long)>::value,
              "sc_fxval_fast::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum &)>(
                 &sc_dt::sc_fxval_fast::operator=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxnum &)>::value,
  "sc_fxval_fast::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::uint64)>(
                             &sc_dt::sc_fxval_fast::operator=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::uint64)>::value,
              "sc_fxval_fast::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>(
                             &sc_dt::sc_fxval_fast::operator=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>::value,
              "sc_fxval_fast::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_int_base &)>(
                 &sc_dt::sc_fxval_fast::operator=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_int_base &)>::value,
  "sc_fxval_fast::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned int)>(
                             &sc_dt::sc_fxval_fast::operator=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(unsigned int)>::value,
              "sc_fxval_fast::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval_fast &)>(
                 &sc_dt::sc_fxval_fast::operator=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval_fast &)>::value,
  "sc_fxval_fast::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::int64)>(
                             &sc_dt::sc_fxval_fast::operator=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(sc_dt::int64)>::value,
              "sc_fxval_fast::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval &)>(
                 &sc_dt::sc_fxval_fast::operator=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_fxval &)>::value,
  "sc_fxval_fast::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_uint_base &)>(
                 &sc_dt::sc_fxval_fast::operator=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_uint_base &)>::value,
  "sc_fxval_fast::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const char *)>(
                             &sc_dt::sc_fxval_fast::operator=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const char *)>::value,
              "sc_fxval_fast::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_unsigned &)>(
                 &sc_dt::sc_fxval_fast::operator=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_unsigned &)>::value,
  "sc_fxval_fast::operator= exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_signed &)>(
                 &sc_dt::sc_fxval_fast::operator=)),
               sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(const sc_dt::sc_signed &)>::value,
  "sc_fxval_fast::operator= exact member signature");
static_assert(std::is_same<decltype(static_cast<sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>(
                             &sc_dt::sc_fxval_fast::operator>>=)),
                           sc_dt::sc_fxval_fast &(sc_dt::sc_fxval_fast::*)(int)>::value,
              "sc_fxval_fast::operator>>= exact member signature");
static_assert(std::is_same<decltype(static_cast<void (sc_dt::sc_fxval_fast::*)(::std::ostream &) const>(
                             &sc_dt::sc_fxval_fast::print)),
                           void (sc_dt::sc_fxval_fast::*)(::std::ostream &) const>::value,
              "sc_fxval_fast::print exact member signature");
static_assert(
  std::is_same<decltype(static_cast<bool (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::rounding_flag)),
               bool (sc_dt::sc_fxval_fast::*)() const>::value,
  "sc_fxval_fast::rounding_flag exact member signature");
static_assert(
  std::is_same<decltype(static_cast<void (sc_dt::sc_fxval_fast::*)(::std::istream &)>(&sc_dt::sc_fxval_fast::scan)),
               void (sc_dt::sc_fxval_fast::*)(::std::istream &)>::value,
  "sc_fxval_fast::scan exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval_fast::*)() const>(
                             &sc_dt::sc_fxval_fast::to_bin)),
                           const std::string (sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::to_bin exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval_fast::*)() const>(
                             &sc_dt::sc_fxval_fast::to_dec)),
                           const std::string (sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::to_dec exact member signature");
static_assert(
  std::is_same<decltype(static_cast<double (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::to_double)),
               double (sc_dt::sc_fxval_fast::*)() const>::value,
  "sc_fxval_fast::to_double exact member signature");
static_assert(
  std::is_same<decltype(static_cast<float (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::to_float)),
               float (sc_dt::sc_fxval_fast::*)() const>::value,
  "sc_fxval_fast::to_float exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval_fast::*)() const>(
                             &sc_dt::sc_fxval_fast::to_hex)),
                           const std::string (sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::to_hex exact member signature");
static_assert(std::is_same<decltype(static_cast<int (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::to_int)),
                           int (sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::to_int exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::int64 (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::to_int64)),
               sc_dt::int64 (sc_dt::sc_fxval_fast::*)() const>::value,
  "sc_fxval_fast::to_int64 exact member signature");
static_assert(
  std::is_same<decltype(static_cast<long (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::to_long)),
               long (sc_dt::sc_fxval_fast::*)() const>::value,
  "sc_fxval_fast::to_long exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval_fast::*)() const>(
                             &sc_dt::sc_fxval_fast::to_oct)),
                           const std::string (sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::to_oct exact member signature");
static_assert(
  std::is_same<decltype(static_cast<short (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::to_short)),
               short (sc_dt::sc_fxval_fast::*)() const>::value,
  "sc_fxval_fast::to_short exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval_fast::*)() const>(
                             &sc_dt::sc_fxval_fast::to_string)),
                           const std::string (sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::to_string exact member signature");
static_assert(
  std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval_fast::*)(sc_dt::sc_numrep, bool, sc_dt::sc_fmt)
                                      const>(&sc_dt::sc_fxval_fast::to_string)),
               const std::string (sc_dt::sc_fxval_fast::*)(sc_dt::sc_numrep, bool, sc_dt::sc_fmt) const>::value,
  "sc_fxval_fast::to_string exact member signature");
static_assert(
  std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval_fast::*)(sc_dt::sc_numrep, sc_dt::sc_fmt) const>(
                 &sc_dt::sc_fxval_fast::to_string)),
               const std::string (sc_dt::sc_fxval_fast::*)(sc_dt::sc_numrep, sc_dt::sc_fmt) const>::value,
  "sc_fxval_fast::to_string exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval_fast::*)(sc_dt::sc_numrep) const>(
                             &sc_dt::sc_fxval_fast::to_string)),
                           const std::string (sc_dt::sc_fxval_fast::*)(sc_dt::sc_numrep) const>::value,
              "sc_fxval_fast::to_string exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval_fast::*)(sc_dt::sc_numrep, bool)
                                                  const>(&sc_dt::sc_fxval_fast::to_string)),
                           const std::string (sc_dt::sc_fxval_fast::*)(sc_dt::sc_numrep, bool) const>::value,
              "sc_fxval_fast::to_string exact member signature");
static_assert(std::is_same<decltype(static_cast<const std::string (sc_dt::sc_fxval_fast::*)(sc_dt::sc_fmt) const>(
                             &sc_dt::sc_fxval_fast::to_string)),
                           const std::string (sc_dt::sc_fxval_fast::*)(sc_dt::sc_fmt) const>::value,
              "sc_fxval_fast::to_string exact member signature");
static_assert(
  std::is_same<decltype(static_cast<unsigned int (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::to_uint)),
               unsigned int (sc_dt::sc_fxval_fast::*)() const>::value,
  "sc_fxval_fast::to_uint exact member signature");
static_assert(
  std::is_same<decltype(static_cast<sc_dt::uint64 (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::to_uint64)),
               sc_dt::uint64 (sc_dt::sc_fxval_fast::*)() const>::value,
  "sc_fxval_fast::to_uint64 exact member signature");
static_assert(
  std::is_same<decltype(static_cast<unsigned long (sc_dt::sc_fxval_fast::*)() const>(&sc_dt::sc_fxval_fast::to_ulong)),
               unsigned long (sc_dt::sc_fxval_fast::*)() const>::value,
  "sc_fxval_fast::to_ulong exact member signature");
static_assert(std::is_same<decltype(static_cast<unsigned short (sc_dt::sc_fxval_fast::*)() const>(
                             &sc_dt::sc_fxval_fast::to_ushort)),
                           unsigned short (sc_dt::sc_fxval_fast::*)() const>::value,
              "sc_fxval_fast::to_ushort exact member signature");

/** @brief Invoke exact friend operator overloads when they are declared. */
template <typename Output, typename Left, typename Right>
std::string operators(const Left &left, const Right &right)
{
  using L = parameter_t<Left>;
  using R = parameter_t<Right>;
  static_assert(std::is_same<decltype(std::declval<L>() + std::declval<R>()), const Output>::value,
                "exact plus friend result");
  static_assert(std::is_same<decltype(std::declval<L>() - std::declval<R>()), const Output>::value,
                "exact minus friend result");
  static_assert(std::is_same<decltype(std::declval<L>() * std::declval<R>()), const Output>::value,
                "exact multiply friend result");
  static_assert(std::is_same<decltype(std::declval<L>() / std::declval<R>()), const Output>::value,
                "exact divide friend result");
  const Output add = left + right;
  const Output sub = left - right;
  const Output mult = left * right;
  const Output div = left / right;
  std::ostringstream out;
  out << decimal(add) << '|' << decimal(sub) << '|' << decimal(mult) << '|' << decimal(div);
  return out.str();
}

/** @brief Run one typed arithmetic matrix case and emit four native results. */
template <typename Output, typename Left, typename Right>
void emit_binary(const char *label, const Left &left, const Right &right)
{
  std::cout << "FXBIN\t" << label << '\t' << binary<Output>(left, right) << '\n';
}

/** @brief Run one exact friend operator matrix case and emit four results. */
template <typename Output, typename Left, typename Right>
void emit_operators(const char *label, const Left &left, const Right &right)
{
  std::cout << "FXOP\t" << label << '\t' << operators<Output>(left, right) << '\n';
}

/** @brief Exercise ordinary native expressions used by Python numeric wrappers. */
template <typename Output, typename Source>
void emit_python_expression(const char *label, const Source &source)
{
  const Output left(3.5);
  const Output add = left + source;
  const Output sub = left - source;
  const Output mult = left * source;
  const Output div = left / source;
  std::cout << "FXPY\t" << label << '\t' << decimal(add) << '|' << decimal(sub) << '|' << decimal(mult) << '|'
            << decimal(div) << '\n';
}

/** @brief Construct through one exact native constructor and emit its value. */
template <typename Output, typename Observer, typename Source>
void emit_constructor(const char *label, parameter_t<Source> source)
{
  Output value(source, static_cast<Observer *>(nullptr));
  std::cout << "FXCTOR\t" << label << '\t' << observe(value) << '\n';
}

/** @brief Exercise the observer-only constructor with an explicit null observer. */
template <typename Output, typename Observer>
void emit_empty_constructor(const char *label)
{
  Output value(static_cast<Observer *>(nullptr));
  std::cout << "FXCTOR\t" << label << '\t' << observe(value) << '\n';
}

/** @brief Run every distinct source and operand constructor signature. */
void run_constructor_matrix();

/** @brief Assign through one exact public member overload. */
template <typename Output, typename Source>
void emit_assignment(const char *label, parameter_t<Source> source)
{
  using Fn = Output &(Output::*)(parameter_t<Source>);
  const Fn assign = static_cast<Fn>(&Output::operator=);
  Output value(0);
  (value.*assign)(source);
  std::cout << "FXASSIGN\t" << label << '\t' << observe(value) << '\n';
}

/** @brief Apply an exact public arithmetic or shift compound overload. */
template <typename Output, typename Source, int Operation>
void emit_compound(const char *label, parameter_t<Source> source)
{
  using Fn = Output &(Output::*)(parameter_t<Source>);
  Output value(3.5);
  if constexpr (Operation == 0)
  {
    const Fn apply = static_cast<Fn>(&Output::operator+=);
    (value.*apply)(source);
  }
  else if constexpr (Operation == 1)
  {
    const Fn apply = static_cast<Fn>(&Output::operator-=);
    (value.*apply)(source);
  }
  else if constexpr (Operation == 2)
  {
    const Fn apply = static_cast<Fn>(&Output::operator*=);
    (value.*apply)(source);
  }
  else if constexpr (Operation == 3)
  {
    const Fn apply = static_cast<Fn>(&Output::operator/=);
    (value.*apply)(source);
  }
  else
  {
    const Fn apply = Operation == 4 ? static_cast<Fn>(&Output::operator<<=) : static_cast<Fn>(&Output::operator>>=);
    const int count = 2;
    (value.*apply)(count);
  }
  std::cout << "FXCOMPOUND\t" << label << '\t' << observe(value) << '\n';
}

/** @brief Run every assignment, compound and shift assignment source. */
void run_assignment_matrix();
void run_compound_matrix();

/** @brief Run exact binary functions for the inventoried operand pairs. */
void run_binary_matrix();
void run_operator_matrix();

}  // namespace sc_dt_fxvalues_probe

#endif  // PYSYSTEMC_TEST_SC_DT_FXVALUES_PROBE_H

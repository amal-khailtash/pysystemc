/** @file sc_dt_fxnums_probe.h
 * @brief Native probes for the protected fixed-point number bases.
 * @details The derived test facades expose only native protected constructors
 *          and the observer getter. Number storage owns its representation;
 *          source operands and observers are borrowed during calls. Helpers
 *          are synchronous, retain no Python references, and require the GIL
 *          when called from cppyy. Native reports and exceptions propagate.
 */
#pragma once

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>
#include <utility>

namespace sc_dt_fxnums_probe
{
using Float = float;
using Int = int;
using Double = double;
using UnsignedInt = unsigned int;
using Long = long;
using UnsignedLong = unsigned long;
using Int64 = sc_dt::int64;
using UInt64 = sc_dt::uint64;

template <class T, bool ByValue = std::is_arithmetic_v<T> || std::is_pointer_v<T>>
struct parameter
{
  using type = const T&;
};
template <class T>
struct parameter<T, true>
{
  using type = T;
};
template <class T>
using parameter_t = typename parameter<T>::type;

#define DECL_FXNUM_CONSTRUCTOR(Name, Base, Observer, Source)                                                         \
  Name(Source source, const sc_dt::sc_fxtype_params& params, sc_dt::sc_enc enc, const sc_dt::sc_fxcast_switch& cast, \
       Observer* observer)                                                                                           \
    : Base(source, params, enc, cast, observer)                                                                      \
  {                                                                                                                  \
  }

/** @brief Expose protected construction while preserving the native base. */
struct Fxnum : sc_dt::sc_fxnum
{
  using sc_dt::sc_fxnum::operator=;
  Fxnum(const sc_dt::sc_fxtype_params& p, sc_dt::sc_enc e, const sc_dt::sc_fxcast_switch& c,
        sc_dt::sc_fxnum_observer* o)
    : sc_dt::sc_fxnum(p, e, c, o)
  {
  }
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, int)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, unsigned int)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, long)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, unsigned long)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, float)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, double)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, const char*)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, const sc_dt::sc_fxval&)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, const sc_dt::sc_fxval_fast&)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, const sc_dt::sc_fxnum&)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, const sc_dt::sc_fxnum_fast&)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, sc_dt::int64)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, sc_dt::uint64)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, const sc_dt::sc_int_base&)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, const sc_dt::sc_uint_base&)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, const sc_dt::sc_signed&)
  DECL_FXNUM_CONSTRUCTOR(Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer, const sc_dt::sc_unsigned&)
  ~Fxnum() = default;
  /** @brief Return the protected getter's exact member pointer. */
  static auto observer_getter()
  {
    return &Fxnum::observer;
  }
};

/** @brief Expose protected construction while preserving the native fast base. */
struct FxnumFast : sc_dt::sc_fxnum_fast
{
  using sc_dt::sc_fxnum_fast::operator=;
  FxnumFast(const sc_dt::sc_fxtype_params& p, sc_dt::sc_enc e, const sc_dt::sc_fxcast_switch& c,
            sc_dt::sc_fxnum_fast_observer* o)
    : sc_dt::sc_fxnum_fast(p, e, c, o)
  {
  }
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, int)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, unsigned int)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, long)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, unsigned long)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, float)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, double)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, const char*)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, const sc_dt::sc_fxval&)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, const sc_dt::sc_fxval_fast&)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, const sc_dt::sc_fxnum&)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, const sc_dt::sc_fxnum_fast&)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, sc_dt::int64)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, sc_dt::uint64)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, const sc_dt::sc_int_base&)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, const sc_dt::sc_uint_base&)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, const sc_dt::sc_signed&)
  DECL_FXNUM_CONSTRUCTOR(FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer, const sc_dt::sc_unsigned&)
  ~FxnumFast() = default;
  /** @brief Return the protected getter's exact member pointer. */
  static auto observer_getter()
  {
    return &FxnumFast::observer;
  }
};
#undef DECL_FXNUM_CONSTRUCTOR

/** @brief Native observer whose lifetime can be counted by the parity model. */
struct Observer : sc_dt::sc_fxnum_observer
{
  inline static int destroyed = 0;
  /** @brief Count release after every borrowed number has been destroyed. */
  ~Observer() override
  {
    ++destroyed;
  }
};

/** @brief Native fast observer whose lifetime can be counted by the parity model. */
struct FastObserver : sc_dt::sc_fxnum_fast_observer
{
  inline static int destroyed = 0;
  /** @brief Count release after every borrowed number has been destroyed. */
  ~FastObserver() override
  {
    ++destroyed;
  }
};

/** @brief Own representative native operands of every declared constructor type. */
struct Sources
{
  sc_dt::sc_fxval value;
  sc_dt::sc_fxval_fast value_fast;
  sc_dt::sc_fixed<20, 8> number;
  sc_dt::sc_fixed_fast<20, 8> number_fast;
  sc_dt::sc_int<59> sint;
  sc_dt::sc_uint<59> uint;
  sc_dt::sc_bigint<137> signed_value;
  sc_dt::sc_biguint<191> unsigned_value;

  /** @brief Initialize exact live sources before borrowed overload calls. */
  Sources()
    : value(1.9375)
    , value_fast(2.8125)
    , number("-3.0625")
    , number_fast("4.1875")
    , sint("-9007199254740993")
    , uint("9007199254740995")
    , signed_value("-9007199254740993")
    , unsigned_value("9007199254740995")
  {
  }

  /** @brief Return exact scalar values for typed constructor and operator probes. */
  /** @brief Return the exact signed-int source. */
  static int integer()
  {
    return -3;
  }
  /** @brief Return the exact unsigned-int source. */
  static UnsignedInt unsigned_integer()
  {
    return 5U;
  }
  /** @brief Return the exact long source. */
  static Long long_integer()
  {
    return -30L;
  }
  /** @brief Return the exact unsigned-long source. */
  static UnsignedLong unsigned_long_integer()
  {
    return 31UL;
  }
  /** @brief Return the exact single-precision source. */
  static Float real_float()
  {
    return 1.25F;
  }
  /** @brief Return the exact double-precision source. */
  static double real_double()
  {
    return 2.625;
  }
  /** @brief Return the exact SystemC signed-64 source. */
  static sc_dt::int64 signed_64()
  {
    return static_cast<sc_dt::int64>(-12345);
  }
  /** @brief Return the exact SystemC unsigned-64 source. */
  static sc_dt::uint64 unsigned_64()
  {
    return static_cast<sc_dt::uint64>(54321);
  }
  /** @brief Return a borrowed decimal string source. */
  static const char* text()
  {
    return "3.1875";
  }
};

/** @brief Return a stable value, conversion, parameter, and status record. */
template <class Number>
std::string observe(const Number& value)
{
  std::ostringstream out;
  out << std::setprecision(17) << value.to_double() << '|' << value.to_string(sc_dt::SC_DEC) << '|' << value.to_int()
      << '|' << value.to_uint() << '|' << value.wl() << '|' << value.iwl() << '|' << static_cast<int>(value.q_mode())
      << '|' << static_cast<int>(value.o_mode()) << '|' << value.n_bits() << '|' << value.is_neg() << '|'
      << value.is_zero() << '|' << value.quantization_flag() << '|' << value.overflow_flag() << '|'
      << (value.cast_switch() == sc_dt::SC_ON);
  return out.str();
}

/** @brief Render wide precision without narrowing through primitive integers. */
template <class Number>
std::string observe_precision(const Number& value)
{
  std::ostringstream out;
  out << std::setprecision(17) << value.to_double() << '|' << value.to_string(sc_dt::SC_DEC) << '|' << value.wl() << '|'
      << value.iwl();
  return out.str();
}

/** @brief Check exact protected constructor signatures through public facades. */
template <class Facade, class Base, class Observer>
struct ConstructorSignatures
{
  using Params = sc_dt::sc_fxtype_params;
  using Cast = sc_dt::sc_fxcast_switch;
  /** @brief Check one exact typed constructor signature. */
  template <class Source>
  static constexpr bool accepts()
  {
    return std::is_constructible_v<Facade, Source, const Params&, sc_dt::sc_enc, const Cast&, Observer*>;
  }
  /** @brief Assert the supported protected surface and actual private gaps. */
  static constexpr bool valid()
  {
    static_assert(std::is_constructible_v<Facade, const Params&, sc_dt::sc_enc, const Cast&, Observer*>);
    static_assert(accepts<int>() && accepts<unsigned int>() && accepts<long>() && accepts<unsigned long>());
    static_assert(accepts<float>() && accepts<double>() && accepts<const char*>());
    static_assert(accepts<const sc_dt::sc_fxval&>() && accepts<const sc_dt::sc_fxval_fast&>());
    static_assert(accepts<const sc_dt::sc_fxnum&>() && accepts<const sc_dt::sc_fxnum_fast&>());
    static_assert(accepts<sc_dt::int64>() && accepts<sc_dt::uint64>());
    static_assert(accepts<const sc_dt::sc_int_base&>() && accepts<const sc_dt::sc_uint_base&>());
    static_assert(accepts<const sc_dt::sc_signed&>() && accepts<const sc_dt::sc_unsigned&>());
    static_assert(!std::is_constructible_v<Base> && !std::is_constructible_v<Base, const Base&>);
    static_assert(!std::is_constructible_v<Facade> && !std::is_constructible_v<Facade, const Facade&>);
    static_assert(!std::is_destructible_v<Base> && std::is_destructible_v<Facade>);
    return true;
  }
};

#define SC_FXNUM_SOURCE_SIGNATURES(Base)                                                                               \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(int)>(&Base::operator=)), Base& (Base::*)(int)>);  \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(unsigned int)>(&Base::operator=)),                 \
                               Base& (Base::*)(unsigned int)>);                                                        \
  static_assert(                                                                                                       \
    std::is_same_v<decltype(static_cast<Base& (Base::*)(long)>(&Base::operator=)), Base& (Base::*)(long)>);            \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(unsigned long)>(&Base::operator=)),                \
                               Base& (Base::*)(unsigned long)>);                                                       \
  static_assert(                                                                                                       \
    std::is_same_v<decltype(static_cast<Base& (Base::*)(float)>(&Base::operator=)), Base& (Base::*)(float)>);          \
  static_assert(                                                                                                       \
    std::is_same_v<decltype(static_cast<Base& (Base::*)(double)>(&Base::operator=)), Base& (Base::*)(double)>);        \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(const char*)>(&Base::operator=)),                  \
                               Base& (Base::*)(const char*)>);                                                         \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(const sc_dt::sc_fxval&)>(&Base::operator=)),       \
                               Base& (Base::*)(const sc_dt::sc_fxval&)>);                                              \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(const sc_dt::sc_fxval_fast&)>(&Base::operator=)),  \
                               Base& (Base::*)(const sc_dt::sc_fxval_fast&)>);                                         \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(const sc_dt::sc_fxnum&)>(&Base::operator=)),       \
                               Base& (Base::*)(const sc_dt::sc_fxnum&)>);                                              \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(const sc_dt::sc_fxnum_fast&)>(&Base::operator=)),  \
                               Base& (Base::*)(const sc_dt::sc_fxnum_fast&)>);                                         \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(sc_dt::int64)>(&Base::operator=)),                 \
                               Base& (Base::*)(sc_dt::int64)>);                                                        \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(sc_dt::uint64)>(&Base::operator=)),                \
                               Base& (Base::*)(sc_dt::uint64)>);                                                       \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(const sc_dt::sc_int_base&)>(&Base::operator=)),    \
                               Base& (Base::*)(const sc_dt::sc_int_base&)>);                                           \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(const sc_dt::sc_uint_base&)>(&Base::operator=)),   \
                               Base& (Base::*)(const sc_dt::sc_uint_base&)>);                                          \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(const sc_dt::sc_signed&)>(&Base::operator=)),      \
                               Base& (Base::*)(const sc_dt::sc_signed&)>);                                             \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(const sc_dt::sc_unsigned&)>(&Base::operator=)),    \
                               Base& (Base::*)(const sc_dt::sc_unsigned&)>);                                           \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(int)>(&Base::operator+=)), Base& (Base::*)(int)>); \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(int)>(&Base::operator-=)), Base& (Base::*)(int)>); \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(int)>(&Base::operator*=)), Base& (Base::*)(int)>); \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(int)>(&Base::operator/=)), Base& (Base::*)(int)>); \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, unsigned int);                                                                   \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, long);                                                                           \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, unsigned long);                                                                  \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, float);                                                                          \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, double);                                                                         \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, const char*);                                                                    \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, const sc_dt::sc_fxval&);                                                         \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, const sc_dt::sc_fxval_fast&);                                                    \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, const sc_dt::sc_fxnum&);                                                         \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, const sc_dt::sc_fxnum_fast&);                                                    \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, sc_dt::int64);                                                                   \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, sc_dt::uint64);                                                                  \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, const sc_dt::sc_int_base&);                                                      \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, const sc_dt::sc_uint_base&);                                                     \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, const sc_dt::sc_signed&);                                                        \
  SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, const sc_dt::sc_unsigned&)

#define SC_FXNUM_ASSERT_ONE_COMPOUND(Base, Op, Source) \
  static_assert(std::is_same_v<decltype(static_cast<Base& (Base::*)(Source)>(&Base::Op)), Base& (Base::*)(Source)>)
#define SC_FXNUM_ASSERT_ALL_COMPOUNDS(Base, Source)       \
  SC_FXNUM_ASSERT_ONE_COMPOUND(Base, operator+=, Source); \
  SC_FXNUM_ASSERT_ONE_COMPOUND(Base, operator-=, Source); \
  SC_FXNUM_ASSERT_ONE_COMPOUND(Base, operator*=, Source); \
  SC_FXNUM_ASSERT_ONE_COMPOUND(Base, operator/=, Source)

SC_FXNUM_SOURCE_SIGNATURES(sc_dt::sc_fxnum);
SC_FXNUM_SOURCE_SIGNATURES(sc_dt::sc_fxnum_fast);
static_assert(
  std::is_same_v<decltype(static_cast<sc_dt::sc_fxnum& (sc_dt::sc_fxnum::*)(int)>(&sc_dt::sc_fxnum::operator<<=)),
                 sc_dt::sc_fxnum& (sc_dt::sc_fxnum::*)(int)>);
static_assert(std::is_same_v<decltype(static_cast<sc_dt::sc_fxnum_fast& (sc_dt::sc_fxnum_fast::*)(int)>(
                               &sc_dt::sc_fxnum_fast::operator>>=)),
                             sc_dt::sc_fxnum_fast& (sc_dt::sc_fxnum_fast::*)(int)>);

#undef SC_FXNUM_SOURCE_SIGNATURES
#undef SC_FXNUM_ASSERT_ALL_COMPOUNDS
#undef SC_FXNUM_ASSERT_ONE_COMPOUND

using FxnumSignatures = ConstructorSignatures<Fxnum, sc_dt::sc_fxnum, sc_dt::sc_fxnum_observer>;
using FxnumFastSignatures = ConstructorSignatures<FxnumFast, sc_dt::sc_fxnum_fast, sc_dt::sc_fxnum_fast_observer>;
static_assert(FxnumSignatures::valid());
static_assert(FxnumFastSignatures::valid());

/** @brief Return the observed pointer without copying or retaining it. */
inline bool observer_matches(const sc_dt::sc_fxnum& value, const sc_dt::sc_fxnum_observer* observer)
{
  return (value.*Fxnum::observer_getter())() == observer;
}

/** @brief Return the observed fast pointer without copying or retaining it. */
inline bool observer_matches(const sc_dt::sc_fxnum_fast& value, const sc_dt::sc_fxnum_fast_observer* observer)
{
  return (value.*FxnumFast::observer_getter())() == observer;
}
}  // namespace sc_dt_fxnums_probe

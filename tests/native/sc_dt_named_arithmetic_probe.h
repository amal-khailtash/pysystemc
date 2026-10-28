/** @file sc_dt_named_arithmetic_probe.h
 * @brief Exact native references for the public fixed-point named functions.
 * @details Sources and outputs own native storage; function arguments borrow
 * them only for a single call. Numeric outputs borrow an observer kept alive
 * by the caller. Helpers hold the GIL when called through cppyy, retain no
 * Python objects, and do not wait or modify kernel state.
 */
#pragma once

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>

namespace sc_dt_named_arithmetic_probe
{
using Float = float;
using UnsignedInt = unsigned int;
using Long = long;
using UnsignedLong = unsigned long;
/** @brief Observer with local counters for output lifetime and callback checks. */
struct Observer : sc_dt::sc_fxnum_observer
{
  inline static int destroyed = 0;
  int constructs = 0, reads = 0, writes = 0, destructs = 0;

  /** @brief Count native destruction while no fixed-point number borrows this observer. */
  ~Observer() override
  {
    ++destroyed;
  }
  /** @brief Count the native construction callback without recursively reading. */
  void construct(const sc_dt::sc_fxnum&) override
  {
    ++constructs;
  }
  /** @brief Count native reads without retaining the observed number. */
  void read(const sc_dt::sc_fxnum&) override
  {
    ++reads;
  }
  /** @brief Count native writes without retaining the observed number. */
  void write(const sc_dt::sc_fxnum&) override
  {
    ++writes;
  }
  /** @brief Count native number destruction without retaining its reference. */
  void destruct(const sc_dt::sc_fxnum&) override
  {
    ++destructs;
  }
};

/** @brief Fast-number observer with local counters and no Python callback state. */
struct FastObserver : sc_dt::sc_fxnum_fast_observer
{
  inline static int destroyed = 0;
  int constructs = 0, reads = 0, writes = 0, destructs = 0;

  /** @brief Count native destruction while no fast number borrows this observer. */
  ~FastObserver() override
  {
    ++destroyed;
  }
  /** @brief Count the native construction callback without recursively reading. */
  void construct(const sc_dt::sc_fxnum_fast&) override
  {
    ++constructs;
  }
  /** @brief Count native reads without retaining the observed number. */
  void read(const sc_dt::sc_fxnum_fast&) override
  {
    ++reads;
  }
  /** @brief Count native writes without retaining the observed number. */
  void write(const sc_dt::sc_fxnum_fast&) override
  {
    ++writes;
  }
  /** @brief Count native number destruction without retaining its reference. */
  void destruct(const sc_dt::sc_fxnum_fast&) override
  {
    ++destructs;
  }
};

/** @brief Legal access shim for the protected ordinary-number observer getter. */
struct ObserverAccess : sc_dt::sc_fxnum
{
  /** @brief Return the native observer member pointer without fabricating an object. */
  static auto getter()
  {
    return &ObserverAccess::observer;
  }
};

/** @brief Legal access shim for the protected fast-number observer getter. */
struct FastObserverAccess : sc_dt::sc_fxnum_fast
{
  /** @brief Return the native observer member pointer without fabricating an object. */
  static auto getter()
  {
    return &FastObserverAccess::observer;
  }
};

/** @brief Check that a numeric output still borrows its exact observer pointer. */
inline bool observer_matches(const sc_dt::sc_fxnum& number, const Observer* observer)
{
  return (number.*ObserverAccess::getter())() == observer;
}

/** @brief Check that a fast numeric output still borrows its exact observer pointer. */
inline bool fast_observer_matches(const sc_dt::sc_fxnum_fast& number, const FastObserver* observer)
{
  return (number.*FastObserverAccess::getter())() == observer;
}

/** @brief Report native observer callback support without claiming callbacks ran. */
inline bool observers_enabled()
{
#ifdef SC_ENABLE_OBSERVERS
  return true;
#else
  return false;
#endif
}

/** @brief Own all native source categories used by the inventoried overloads. */
struct Sources
{
  sc_dt::sc_fxval value;
  sc_dt::sc_fxval_fast value_fast;
  sc_dt::sc_fixed<137, 100> number;
  sc_dt::sc_fixed_fast<137, 100> number_fast;
  sc_dt::sc_int<59> sint;
  sc_dt::sc_uint<59> uint;
  sc_dt::sc_bigint<137> signed_value;
  sc_dt::sc_biguint<191> unsigned_value;

  /** @brief Initialize borrowed native inputs before parity calls. */
  Sources()
    : value("1.23456789012345678901234567890123456789")
    , value_fast("2.7182818284590452353602874713527")
    , number("9007199254740992.125")
    , number_fast("9007199254740992.125")
    , sint("-9007199254740993")
    , uint("9007199254740995")
    , signed_value("-9007199254740993")
    , unsigned_value("9007199254740995")
  {
  }
};

/** @brief Return one exact borrowed source or a typed scalar value.
 * @tparam Argument The exact function-pointer parameter type from SystemC.
 * @param side Select the first or second deterministic scalar where relevant.
 * @return A reference for native object parameters, otherwise a value.
 * @details The source remains alive throughout the GIL-held call. No value or
 * reference escapes the caller's expression, and no callback is registered.
 */
template <class Argument>
decltype(auto) source(int side)
{
  using T = std::remove_cv_t<std::remove_reference_t<Argument>>;
  static Sources sources;
  if constexpr (std::is_same_v<T, sc_dt::sc_fxnum>)
    return static_cast<Argument>(side == 0 ? static_cast<const sc_dt::sc_fxnum&>(sources.number)
                                           : static_cast<const sc_dt::sc_fxnum&>(sources.number));
  else if constexpr (std::is_same_v<T, sc_dt::sc_fxnum_fast>)
    return static_cast<Argument>(static_cast<const sc_dt::sc_fxnum_fast&>(sources.number_fast));
  else if constexpr (std::is_same_v<T, sc_dt::sc_fxval>)
    return static_cast<Argument>(sources.value);
  else if constexpr (std::is_same_v<T, sc_dt::sc_fxval_fast>)
    return static_cast<Argument>(sources.value_fast);
  else if constexpr (std::is_same_v<T, sc_dt::sc_int_base>)
    return static_cast<Argument>(sources.sint);
  else if constexpr (std::is_same_v<T, sc_dt::sc_uint_base>)
    return static_cast<Argument>(sources.uint);
  else if constexpr (std::is_same_v<T, sc_dt::sc_signed>)
    return static_cast<Argument>(sources.signed_value);
  else if constexpr (std::is_same_v<T, sc_dt::sc_unsigned>)
    return static_cast<Argument>(sources.unsigned_value);
  else if constexpr (std::is_same_v<T, const char*>)
    return side == 0 ? "7.8125" : "3.25";
  else if constexpr (std::is_same_v<T, int>)
    return side == 0 ? -7 : 3;
  else if constexpr (std::is_same_v<T, unsigned int>)
    return side == 0 ? 0xA5A55A5AU : 37U;
  else if constexpr (std::is_same_v<T, long>)
    return static_cast<long>(side == 0 ? -0x123456789ABCDEFLL : 17LL);
  else if constexpr (std::is_same_v<T, unsigned long>)
    return side == 0 ? 0x123456789ABCDEFUL : 19UL;
  else if constexpr (std::is_same_v<T, float>)
    return side == 0 ? 1.375F : 3.25F;
  else if constexpr (std::is_same_v<T, double>)
    return side == 0 ? 2.625 : 5.5;
  else if constexpr (std::is_same_v<T, sc_dt::int64>)
    return static_cast<sc_dt::int64>(side == 0 ? -0x123456789ABCDEFLL : 11LL);
  else if constexpr (std::is_same_v<T, sc_dt::uint64>)
    return side == 0 ? sc_dt::uint64(0xFEDCBA9876543210ULL) : sc_dt::uint64(13);
  else
    static_assert(!sizeof(T), "unexpected named arithmetic source parameter");
}

/** @brief Serialize the exact value-oriented output state for parity. */
inline std::string render_value(const sc_dt::sc_fxval& value)
{
  std::ostringstream result;
  result << std::setprecision(17) << value.to_double() << '|' << value.to_string(sc_dt::SC_DEC) << '|'
         << int(value.is_neg()) << '|' << int(value.is_zero()) << '|' << int(value.is_nan()) << '|'
         << int(value.is_inf()) << '|' << int(value.is_normal()) << '|' << int(value.rounding_flag());
  return result.str();
}

/** @brief Serialize a fast value without converting it to arbitrary precision. */
inline std::string render_value(const sc_dt::sc_fxval_fast& value)
{
  std::ostringstream result;
  result << std::setprecision(17) << value.to_double() << '|' << value.to_string(sc_dt::SC_DEC) << '|'
         << int(value.is_neg()) << '|' << int(value.is_zero()) << '|' << int(value.is_nan()) << '|'
         << int(value.is_inf()) << '|' << int(value.is_normal()) << '|' << int(value.rounding_flag());
  return result.str();
}

/** @brief Serialize the exact fixed-number value, policy and flag state. */
template <class Number>
std::string render_number(const Number& value)
{
  std::ostringstream result;
  result << std::setprecision(17) << value.to_double() << '|' << value.to_string(sc_dt::SC_DEC) << '|' << value.to_int()
         << '|' << value.to_uint() << '|' << value.wl() << '|' << value.iwl() << '|' << int(value.q_mode()) << '|'
         << int(value.o_mode()) << '|' << value.n_bits() << '|' << int(value.is_neg()) << '|' << int(value.is_zero())
         << '|' << int(value.quantization_flag()) << '|' << int(value.overflow_flag()) << '|'
         << int(value.cast_switch() == sc_dt::SC_ON);
  return result.str();
}

/** @brief Supply an owning concrete object for one native output-reference base. */
template <class Base>
struct Output;

/** @brief Concrete storage and value rendering for arbitrary precision output. */
template <>
struct Output<sc_dt::sc_fxval>
{
  using Storage = sc_dt::sc_fxval;
  static Storage make()
  {
    return Storage("0.125");
  }
  static std::string render(const Storage& value)
  {
    return render_value(value);
  }
  static bool observer_matches(const Storage&, const Observer*)
  {
    return false;
  }
};

/** @brief Concrete storage and value rendering for fast output. */
template <>
struct Output<sc_dt::sc_fxval_fast>
{
  using Storage = sc_dt::sc_fxval_fast;
  static Storage make()
  {
    return Storage("0.125");
  }
  static std::string render(const Storage& value)
  {
    return render_value(value);
  }
  static bool observer_matches(const Storage&, const Observer*)
  {
    return false;
  }
};

/** @brief Wide arbitrary fixed-number owner used behind the native base output. */
template <>
struct Output<sc_dt::sc_fxnum>
{
  using Storage = sc_dt::sc_fixed<191, 140>;
  static Storage make(Observer* observer)
  {
    return Storage("0.125", observer);
  }
  static std::string render(const Storage& value)
  {
    return render_number(value);
  }
  static bool observer_matches(const Storage& value, const Observer* observer)
  {
    return sc_dt_named_arithmetic_probe::observer_matches(value, observer);
  }
};

/** @brief Wide fast fixed-number owner used behind the native base output. */
template <>
struct Output<sc_dt::sc_fxnum_fast>
{
  using Storage = sc_dt::sc_fixed_fast<191, 140>;
  static Storage make(FastObserver* observer)
  {
    return Storage("0.125", observer);
  }
  static std::string render(const Storage& value)
  {
    return render_number(value);
  }
  static bool observer_matches(const Storage& value, const FastObserver* observer)
  {
    return fast_observer_matches(value, observer);
  }
};

/** @brief Run all four exact native function pointers for one operand signature.
 * @tparam Base Exact native output base class.
 * @tparam Left Exact first operand parameter type.
 * @tparam Right Exact second operand parameter type.
 * @param output_name Stable native output name from the independent matrix.
 * @param left_name Stable native first operand name from the independent matrix.
 * @param right_name Stable native second operand name from the independent matrix.
 * @details Each call uses a fresh owning output, borrowed live inputs, and a
 * compile-time exact overload pointer. Number targets retain an observer for
 * their complete lifetime. Calls hold the GIL through cppyy and retain no
 * Python references, callbacks, or kernel state.
 */
template <class Base, class Left, class Right>
void run_case(const char* output_name, const char* left_name, const char* right_name)
{
  using Factory = Output<Base>;
  using Fn = void (*)(Base&, Left, Right);
  static Sources sources;
  auto run = [&](const char* operation, Fn function)
  {
    if constexpr (std::is_same_v<Base, sc_dt::sc_fxnum>)
    {
      Observer observer;
      auto target = Factory::make(&observer);
      decltype(auto) left = source<Left>(0);
      decltype(auto) right = source<Right>(1);
      const auto* identity = &target;
      function(target, left, right);
      std::cout << "NAR\t" << output_name << '\t' << operation << '\t' << left_name << '\t' << right_name << '\t'
                << Factory::render(target) << '\t' << int(&target == identity) << '\t'
                << int(Factory::observer_matches(target, &observer)) << '\t' << int(observers_enabled()) << '\n';
    }
    else if constexpr (std::is_same_v<Base, sc_dt::sc_fxnum_fast>)
    {
      FastObserver observer;
      auto target = Factory::make(&observer);
      decltype(auto) left = source<Left>(0);
      decltype(auto) right = source<Right>(1);
      const auto* identity = &target;
      function(target, left, right);
      std::cout << "NAR\t" << output_name << '\t' << operation << '\t' << left_name << '\t' << right_name << '\t'
                << Factory::render(target) << '\t' << int(&target == identity) << '\t'
                << int(Factory::observer_matches(target, &observer)) << '\t' << int(observers_enabled()) << '\n';
    }
    else
    {
      auto target = Factory::make();
      decltype(auto) left = source<Left>(0);
      decltype(auto) right = source<Right>(1);
      const auto* identity = &target;
      function(target, left, right);
      std::cout << "NAR\t" << output_name << '\t' << operation << '\t' << left_name << '\t' << right_name << '\t'
                << Factory::render(target) << '\t' << int(&target == identity) << "\t0\t0\n";
    }
  };

  run("add", static_cast<Fn>(&sc_dt::add));
  run("sub", static_cast<Fn>(&sc_dt::sub));
  run("mult", static_cast<Fn>(&sc_dt::mult));
  run("div", static_cast<Fn>(&sc_dt::div));
  (void)sources;
}

/** @brief Exercise fast-output/input alias directions through exact native pointers.
 * @tparam Base The native output and input base type.
 * @param output_name Stable native output name from the independent matrix.
 * @details Each result is compared with a separate native reference process.
 * Only fast outputs are permitted: arbitrary-precision sc_fxval and sc_fxnum
 * functions delete or replace output representation before completing reads.
 * The target owns its storage while the other operand remains borrowed.
 * The native observer remains live until the target is destroyed.
 */
template <class Base>
void run_alias_cases(const char* output_name)
{
  static_assert(std::is_same_v<Base, sc_dt::sc_fxval_fast> || std::is_same_v<Base, sc_dt::sc_fxnum_fast>,
                "output/input alias probes are supported only for fast fixed-point outputs");
  using Factory = Output<Base>;
  using Fn = void (*)(Base&, const Base&, const Base&);
  auto run = [&](const char* operation, const char* direction, Fn function)
  {
    if constexpr (std::is_same_v<Base, sc_dt::sc_fxnum_fast>)
    {
      FastObserver observer;
      auto target = Factory::make(&observer);
      const Base& alias = target;
      const Base& independent = source<const Base&>(1);
      const auto* identity = &target;
      if (direction[0] == 'l')
        function(target, alias, independent);
      else
        function(target, independent, alias);
      std::cout << "NARA\t" << output_name << '\t' << operation << '\t' << direction << '\t' << Factory::render(target)
                << '\t' << int(&target == identity) << '\t' << int(Factory::observer_matches(target, &observer)) << '\t'
                << int(observers_enabled()) << '\n';
    }
    else
    {
      auto target = Factory::make();
      const Base& alias = target;
      const Base& independent = source<const Base&>(1);
      const auto* identity = &target;
      if (direction[0] == 'l')
        function(target, alias, independent);
      else
        function(target, independent, alias);
      std::cout << "NARA\t" << output_name << '\t' << operation << '\t' << direction << '\t' << Factory::render(target)
                << '\t' << int(&target == identity) << "\t0\t0\n";
    }
  };
  run("add", "left", static_cast<Fn>(&sc_dt::add));
  run("add", "right", static_cast<Fn>(&sc_dt::add));
  run("sub", "left", static_cast<Fn>(&sc_dt::sub));
  run("sub", "right", static_cast<Fn>(&sc_dt::sub));
  run("mult", "left", static_cast<Fn>(&sc_dt::mult));
  run("mult", "right", static_cast<Fn>(&sc_dt::mult));
  run("div", "left", static_cast<Fn>(&sc_dt::div));
  run("div", "right", static_cast<Fn>(&sc_dt::div));
}
}  // namespace sc_dt_named_arithmetic_probe

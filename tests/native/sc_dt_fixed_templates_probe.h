/** @file sc_dt_fixed_templates_probe.h
 * @brief Exact fixed-template source, observer and signature probes.
 * @details Sources own static storage. Numbers borrow observer pointers, whose
 * owners must remain live until every number using them is destroyed. Helpers
 * hold the GIL, do not wait or retain Python references, and propagate native
 * allocation/report errors. Observer callbacks follow native build settings.
 */
#pragma once
#include <systemc>
#include <type_traits>
#include <utility>

namespace fixed_templates_probe
{
using Float = float;
using UnsignedInt = unsigned int;
using Long = long;
using UnsignedLong = unsigned long;
/** @brief Own native observer state without borrowing a number or Python callback. */
struct Observer : sc_dt::sc_fxnum_observer
{
  inline static int destroyed = 0;
  int constructed = 0, reads = 0, writes = 0, destructed = 0;
  /** @brief Expose the protected observer constructor. */
  Observer() = default;
  /** @brief Count destruction after every borrowing number has been released. */
  ~Observer() override
  {
    ++destroyed;
  }
  /** @brief Record native construction without reading the observed number. */
  void construct(const sc_dt::sc_fxnum&) override
  {
    ++constructed;
  }
  /** @brief Record native reads without recursive observer dispatch. */
  void read(const sc_dt::sc_fxnum&) override
  {
    ++reads;
  }
  /** @brief Record native writes without recursive observer dispatch. */
  void write(const sc_dt::sc_fxnum&) override
  {
    ++writes;
  }
  /** @brief Record native destruction without retaining the borrowed number. */
  void destruct(const sc_dt::sc_fxnum&) override
  {
    ++destructed;
  }
};
/** @brief Own fast observer state without borrowing a number or Python callback. */
struct FastObserver : sc_dt::sc_fxnum_fast_observer
{
  inline static int destroyed = 0;
  int constructed = 0, reads = 0, writes = 0, destructed = 0;
  /** @brief Expose the protected observer constructor. */
  FastObserver() = default;
  /** @brief Count destruction after every borrowing number has been released. */
  ~FastObserver() override
  {
    ++destroyed;
  }
  /** @brief Record native construction without reading the observed number. */
  void construct(const sc_dt::sc_fxnum_fast&) override
  {
    ++constructed;
  }
  /** @brief Record native reads without recursive observer dispatch. */
  void read(const sc_dt::sc_fxnum_fast&) override
  {
    ++reads;
  }
  /** @brief Record native writes without recursive observer dispatch. */
  void write(const sc_dt::sc_fxnum_fast&) override
  {
    ++writes;
  }
  /** @brief Record native destruction without retaining the borrowed number. */
  void destruct(const sc_dt::sc_fxnum_fast&) override
  {
    ++destructed;
  }
};
/** @brief Expose a legal protected-member pointer without inventing a derived owner. */
struct ObserverAccess : sc_dt::sc_fxnum
{
  /** @brief Return the native base observer getter's pointer-to-member. */
  static auto getter()
  {
    return &ObserverAccess::observer;
  }
};
/** @brief Expose a legal protected-member pointer without inventing a derived owner. */
struct FastObserverAccess : sc_dt::sc_fxnum_fast
{
  /** @brief Return the native fast base observer getter's pointer-to-member. */
  static auto getter()
  {
    return &FastObserverAccess::observer;
  }
};
/** @brief Inspect the exact borrowed observer pointer without owning or copying it. */
inline bool observer_matches(const sc_dt::sc_fxnum& n, const sc_dt::sc_fxnum_observer* observer)
{
  return (n.*ObserverAccess::getter())() == observer;
}
/** @brief Inspect the exact borrowed fast observer pointer without copying it. */
inline bool observer_matches(const sc_dt::sc_fxnum_fast& n, const sc_dt::sc_fxnum_fast_observer* observer)
{
  return (n.*FastObserverAccess::getter())() == observer;
}
/** @brief Report observer dispatch capability instead of silently skipping it. */
inline bool observers_enabled()
{
#ifdef SC_ENABLE_OBSERVERS
  return true;
#else
  return false;
#endif
}
/** @brief Borrow a static arbitrary-precision value source. */
inline const sc_dt::sc_fxval& fxval_source()
{
  static const sc_dt::sc_fxval value("1.9375");
  return value;
}
/** @brief Borrow a static fast value source. */
inline const sc_dt::sc_fxval_fast& fxval_fast_source()
{
  static const sc_dt::sc_fxval_fast value("2.8125");
  return value;
}
/** @brief Borrow a static configured arbitrary-precision source. */
inline const sc_dt::sc_fxnum& fxnum_source()
{
  static const sc_dt::sc_fixed<20, 8> value("-3.0625");
  return value;
}
/** @brief Borrow a static configured fast source. */
inline const sc_dt::sc_fxnum_fast& fxnum_fast_source()
{
  static const sc_dt::sc_fixed_fast<20, 8> value("4.1875");
  return value;
}
/** @brief Borrow a static signed integer source with a low bit above double precision. */
inline const sc_dt::sc_int_base& int_base_source()
{
  static const sc_dt::sc_int<59> value("9007199254740993");
  return value;
}
/** @brief Borrow a static unsigned integer source above double precision. */
inline const sc_dt::sc_uint_base& uint_base_source()
{
  static const sc_dt::sc_uint<59> value("9007199254740995");
  return value;
}
/** @brief Borrow static signed arbitrary-width numeric storage. */
inline const sc_dt::sc_signed& signed_source()
{
  static const sc_dt::sc_bigint<137> value("-9007199254740993");
  return value;
}
/** @brief Borrow static unsigned arbitrary-width numeric storage. */
inline const sc_dt::sc_unsigned& unsigned_source()
{
  static const sc_dt::sc_biguint<191> value("9007199254740995");
  return value;
}
/** @brief Prove the exact specialization reference/result types for declared operators. */
template <class Number, class SignedMask, class FastMask, class Postfix>
struct Signatures
{
  /** @brief Resolve exact member overloads rather than inherited base returns. */
  static bool valid()
  {
    using Ref = Number&;
    static_assert(std::is_same_v<decltype(++std::declval<Ref>()), Ref>);
    static_assert(std::is_same_v<decltype(--std::declval<Ref>()), Ref>);
    static_assert(std::is_same_v<decltype(std::declval<Ref>()++), const Postfix>);
    static_assert(std::is_same_v<decltype(std::declval<Ref>()--), const Postfix>);
    (void)static_cast<Ref (Number::*)(const Number&)>(&Number::operator=);
    (void)static_cast<Ref (Number::*)(int)>(&Number::operator<<=);
    (void)static_cast<Ref (Number::*)(int)>(&Number::operator>>=);
    (void)static_cast<Ref (Number::*)(const SignedMask&)>(&Number::operator&=);
    (void)static_cast<Ref (Number::*)(const FastMask&)>(&Number::operator&=);
    (void)static_cast<Ref (Number::*)(const SignedMask&)>(&Number::operator|=);
    (void)static_cast<Ref (Number::*)(const FastMask&)>(&Number::operator|=);
    (void)static_cast<Ref (Number::*)(const SignedMask&)>(&Number::operator^=);
    (void)static_cast<Ref (Number::*)(const FastMask&)>(&Number::operator^=);
    return std::is_destructible_v<Number>;
  }
  /** @brief Resolve each exact scalar/source assignment and four arithmetic compounds. */
  template <class Source>
  static bool source()
  {
    using Ref = Number&;
    (void)static_cast<Ref (Number::*)(Source)>(&Number::operator=);
    (void)static_cast<Ref (Number::*)(Source)>(&Number::operator+=);
    (void)static_cast<Ref (Number::*)(Source)>(&Number::operator-=);
    (void)static_cast<Ref (Number::*)(Source)>(&Number::operator*=);
    (void)static_cast<Ref (Number::*)(Source)>(&Number::operator/=);
    return true;
  }
};
}  // namespace fixed_templates_probe

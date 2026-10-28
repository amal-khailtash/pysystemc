/** @file sc_dt_fix_owners_probe.h
 * @brief Exact unconstrained fixed-point owner constructor and member signatures.
 * @details Numeric sources own static storage. Observers are borrowed until each
 * number is destroyed; callers retain their owners. The inherited test helpers
 * inspect protected members through legal pointers-to-members, without casting
 * live numbers to fictitious derived objects. Calls hold the GIL, never wait or
 * retain Python references, and preserve native observer/report build settings.
 */
#pragma once
#include "sc_dt_fixed_templates_probe.h"

namespace fix_owners_probe
{
using Observer = fixed_templates_probe::Observer;
using FastObserver = fixed_templates_probe::FastObserver;
using Float = float;
using UnsignedInt = unsigned int;
using Long = long;
using UnsignedLong = unsigned long;
using fixed_templates_probe::fxnum_fast_source;
using fixed_templates_probe::fxnum_source;
using fixed_templates_probe::fxval_fast_source;
using fixed_templates_probe::fxval_source;
using fixed_templates_probe::int_base_source;
using fixed_templates_probe::observer_matches;
using fixed_templates_probe::signed_source;
using fixed_templates_probe::uint_base_source;
using fixed_templates_probe::unsigned_source;
/** @brief Expose the upstream observer build switch to cppyy.
 * @return True when callbacks are enabled by the native SystemC build.
 * @details This fixture only reports the compile-time capability; callers do
 * not install callbacks. Calls hold the GIL and do not retain Python objects.
 */
inline bool observers_enabled()
{
  return fixed_templates_probe::observers_enabled();
}
/** @brief Prove public owner and protected base/observer constructor boundaries. */
inline bool access_traits()
{
  return !std::is_default_constructible_v<sc_dt::sc_fxnum> && !std::is_default_constructible_v<sc_dt::sc_fxnum_fast> &&
         !std::is_default_constructible_v<sc_dt::sc_fxnum_observer> &&
         !std::is_default_constructible_v<sc_dt::sc_fxnum_fast_observer> &&
         !std::is_destructible_v<sc_dt::sc_fxnum_observer> && !std::is_destructible_v<sc_dt::sc_fxnum_fast_observer>;
}
/** @brief Resolve typed owner mutation/postfix and complement declarations. */
template <class Number, class Mask, class FastMask, class Postfix>
struct Traits : fixed_templates_probe::Signatures<Number, Mask, FastMask, Postfix>
{
  /** @brief Prove owned complement and public owner destruction signatures. */
  static bool owner()
  {
    (void)static_cast<const Number (Number::*)() const>(&Number::operator~);
    static_assert(std::is_same_v<decltype(~std::declval<const Number&>()), const Number>);
    return access_traits() && Traits::valid() && std::is_destructible_v<Number>;
  }
};
/** @brief Prove every native policy/cast/observer constructor shape.
 * @tparam Number One of the four native unconstrained fixed-point owners.
 * @tparam Observer Borrowed matching observer base pointer type.
 * @details Constructor traits do not create callbacks or allocate numeric data.
 */
template <class Number, class Observer>
struct Constructors
{
  /** @brief Check all fourteen source-less native parameter forms and copy. */
  static bool base()
  {
    using Q = sc_dt::sc_q_mode;
    using O = sc_dt::sc_o_mode;
    using Cast = const sc_dt::sc_fxcast_switch&;
    using Params = const sc_dt::sc_fxtype_params&;
    static_assert(std::is_constructible_v<Number, Observer*>);
    static_assert(std::is_constructible_v<Number, int, int, Observer*>);
    static_assert(std::is_constructible_v<Number, Q, O, Observer*>);
    static_assert(std::is_constructible_v<Number, Q, O, int, Observer*>);
    static_assert(std::is_constructible_v<Number, int, int, Q, O, Observer*>);
    static_assert(std::is_constructible_v<Number, int, int, Q, O, int, Observer*>);
    static_assert(std::is_constructible_v<Number, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, int, int, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, Q, O, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, Q, O, int, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, int, int, Q, O, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, int, int, Q, O, int, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, Params, Observer*>);
    static_assert(std::is_constructible_v<Number, Params, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, const Number&>);
    return true;
  }
  /** @brief Check all fourteen source-bearing forms for one exact source type. */
  template <class Source>
  static bool source()
  {
    using Q = sc_dt::sc_q_mode;
    using O = sc_dt::sc_o_mode;
    using Cast = const sc_dt::sc_fxcast_switch&;
    using Params = const sc_dt::sc_fxtype_params&;
    static_assert(std::is_constructible_v<Number, Source, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, int, int, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, Q, O, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, Q, O, int, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, int, int, Q, O, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, int, int, Q, O, int, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, int, int, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, Q, O, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, Q, O, int, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, int, int, Q, O, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, int, int, Q, O, int, Cast, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, Params, Observer*>);
    static_assert(std::is_constructible_v<Number, Source, Params, Cast, Observer*>);
    return true;
  }
};
}  // namespace fix_owners_probe

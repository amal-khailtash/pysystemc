/** @file sc_dt_fix_owners_public.cpp
 * @brief Independent native reference for four unconstrained fixed-point owner classes.
 * @details Numbers own numeric storage and borrow observer pointers. Observers
 * outlive every number retaining them. Sources, copies and postfix values are
 * independent. No Python callbacks, waits or singleton resets occur. Native
 * reports are caught only by the deliberate error matrix; ordinary failures
 * propagate. Python counterparts hold the GIL throughout their native calls.
 */
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "sc_dt_fix_owners_probe.h"

namespace
{
/** @brief Emit one exact native semantic and signature observation. */
template <class... Values>
void emit(const char* operation, const char* family, const char* label, const Values&... values)
{
  std::cout << "FXO\t" << operation << '\t' << family << '\t' << label;
  using swallow = int[];
  (void)swallow{0, ((std::cout << '\t' << values), 0)...};
  std::cout << '\n';
}
/** @brief Render value, configured policies and cast flags from a live number. */
template <class Number>
std::string render(const Number& value)
{
  std::ostringstream out;
  out << value.to_string(sc_dt::SC_DEC) << '\t' << value.wl() << '\t' << value.iwl() << '\t' << int(value.q_mode())
      << '\t' << int(value.o_mode()) << '\t' << value.n_bits() << '\t' << int(value.cast_switch() == sc_dt::SC_ON)
      << '\t' << int(value.quantization_flag()) << '\t' << int(value.overflow_flag());
  return out.str();
}
/** @brief Exercise exact source assignment and four arithmetic compounds. */
template <class Number, class Observer, class Traits, class Source>
void source_case(const char* family, const char* label, const Source& source, Observer& observer)
{
  using Signature = std::conditional_t<std::is_class_v<Source>, const Source&, Source>;
  if (!Traits::template source<Signature>()) throw std::runtime_error("source signature unavailable");
  Number number("37.8125", 19, 7, &observer);
  emit("observer_binding", family, label, int(fix_owners_probe::observer_matches(number, &observer)));
  auto& assigned = (number = source);
  emit("assign", family, label, render(number), int(&assigned == &number),
       int(std::is_same_v<decltype(assigned), Number&>));
  number = "37.8125";
  auto& added = (number += source);
  emit("add", family, label, render(number), int(&added == &number), int(std::is_same_v<decltype(added), Number&>));
  number = "37.8125";
  auto& subtracted = (number -= source);
  emit("subtract", family, label, render(number), int(&subtracted == &number),
       int(std::is_same_v<decltype(subtracted), Number&>));
  number = "37.8125";
  auto& multiplied = (number *= source);
  emit("multiply", family, label, render(number), int(&multiplied == &number),
       int(std::is_same_v<decltype(multiplied), Number&>));
  number = "37.8125";
  auto& divided = (number /= source);
  emit("divide", family, label, render(number), int(&divided == &number),
       int(std::is_same_v<decltype(divided), Number&>));
}
/** @brief Exercise copy, bitwise, shift, increment and observer lifetimes. */
template <class Number, class Observer, class Mask, class FastMask, class Postfix>
void run_family(const char* family)
{
  using Traits = fix_owners_probe::Traits<Number, Mask, FastMask, Postfix>;
  emit("traits", family, "exact", int(Traits::owner()), int(fix_owners_probe::observers_enabled()));
  const int destroyed = Observer::destroyed;
  {
    Observer observer;
    source_case<Number, Observer, Traits>(family, "int", int(-3), observer);
    source_case<Number, Observer, Traits>(family, "unsigned_int", unsigned(0xA5A55A5A), observer);
    source_case<Number, Observer, Traits>(family, "long", long(sizeof(long) >= 8 ? -0x123456789ABCDEFLL : -0x1234567L),
                                          observer);
    source_case<Number, Observer, Traits>(
      family, "unsigned_long",
      static_cast<unsigned long>(sizeof(unsigned long) >= 8 ? 0x123456789ABCDEFULL : 0x12345678UL), observer);
    source_case<Number, Observer, Traits>(family, "float", float(1.375), observer);
    source_case<Number, Observer, Traits>(family, "double", double(2.625), observer);
    source_case<Number, Observer, Traits>(family, "text", static_cast<const char*>("3.1875"), observer);
    source_case<Number, Observer, Traits>(family, "fxval", fix_owners_probe::fxval_source(), observer);
    source_case<Number, Observer, Traits>(family, "fxval_fast", fix_owners_probe::fxval_fast_source(), observer);
    source_case<Number, Observer, Traits>(family, "fxnum", fix_owners_probe::fxnum_source(), observer);
    source_case<Number, Observer, Traits>(family, "fxnum_fast", fix_owners_probe::fxnum_fast_source(), observer);
    source_case<Number, Observer, Traits>(family, "int64", sc_dt::int64(-0x123456789ABCDEFLL), observer);
    source_case<Number, Observer, Traits>(family, "uint64", sc_dt::uint64(0xFEDCBA9876543210ULL), observer);
    source_case<Number, Observer, Traits>(family, "int_base", fix_owners_probe::int_base_source(), observer);
    source_case<Number, Observer, Traits>(family, "uint_base", fix_owners_probe::uint_base_source(), observer);
    source_case<Number, Observer, Traits>(family, "signed", fix_owners_probe::signed_source(), observer);
    source_case<Number, Observer, Traits>(family, "unsigned", fix_owners_probe::unsigned_source(), observer);
    Number number("37.8125");
    Number source("7.875");
    auto& copied = (number = source);
    emit("assign", family, "copy", render(number), int(&copied == &number),
         int(std::is_same_v<decltype(copied), Number&>));
    const Mask mask("21.25", 19, 7);
    const FastMask fast_mask("17.875", 19, 7);
    number = "37.8125";
    auto& and_result = (number &= mask);
    emit("and", family, "slow", render(number), int(&and_result == &number),
         int(std::is_same_v<decltype(and_result), Number&>));
    number = "37.8125";
    auto& and_fast = (number &= fast_mask);
    emit("and", family, "fast", render(number), int(&and_fast == &number),
         int(std::is_same_v<decltype(and_fast), Number&>));
    number = "37.8125";
    auto& or_result = (number |= mask);
    emit("or", family, "slow", render(number), int(&or_result == &number),
         int(std::is_same_v<decltype(or_result), Number&>));
    number = "37.8125";
    auto& or_fast = (number |= fast_mask);
    emit("or", family, "fast", render(number), int(&or_fast == &number),
         int(std::is_same_v<decltype(or_fast), Number&>));
    number = "37.8125";
    auto& xor_result = (number ^= mask);
    emit("xor", family, "slow", render(number), int(&xor_result == &number),
         int(std::is_same_v<decltype(xor_result), Number&>));
    number = "37.8125";
    auto& xor_fast = (number ^= fast_mask);
    emit("xor", family, "fast", render(number), int(&xor_fast == &number),
         int(std::is_same_v<decltype(xor_fast), Number&>));
    number = "37.8125";
    auto& left = (number <<= 2);
    emit("left", family, "int", render(number), int(&left == &number), int(std::is_same_v<decltype(left), Number&>));
    number = "37.8125";
    auto& right = (number >>= 3);
    emit("right", family, "int", render(number), int(&right == &number), int(std::is_same_v<decltype(right), Number&>));
    number = "3.875";
    auto& preup = ++number;
    emit("preinc", family, "none", render(number), int(&preup == &number),
         int(std::is_same_v<decltype(preup), Number&>));
    const auto old_up = number++;
    emit("postinc", family, "int", old_up.to_string(), render(number),
         int(std::is_same_v<decltype(old_up), const Postfix>));
    auto& predown = --number;
    emit("predec", family, "none", render(number), int(&predown == &number),
         int(std::is_same_v<decltype(predown), Number&>));
    const auto old_down = number--;
    emit("postdec", family, "int", old_down.to_string(), render(number),
         int(std::is_same_v<decltype(old_down), const Postfix>));
    const auto complement = ~number;
    emit("complement", family, "owned", render(complement), int(std::is_same_v<decltype(complement), const Number>));
    emit("observer_lifetime", family, "numbers_gone", int(Observer::destroyed == destroyed));
  }
  emit("observer_lifetime", family, "observer_gone", int(Observer::destroyed == destroyed + 1));
  std::unique_ptr<Number> constructor_copy, complement_copy;
  Number copy("0");
  Postfix postfix("0");
  {
    Number source("7.875");
    constructor_copy.reset(new Number(source));
    complement_copy.reset(new Number(~source));
    copy = source;
    postfix = source++;
    source = "0";
  }
  emit("lifetime", family, "copy_postfix", render(copy), postfix.to_string(), render(*constructor_copy),
       render(*complement_copy));
  const auto actions = sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  bool constructor_rejected = false, assignment_rejected = false;
  try
  {
    Number invalid("invalid-number");
  }
  catch (const sc_core::sc_report&)
  {
    constructor_rejected = true;
  }
  try
  {
    copy = "invalid-number";
  }
  catch (const sc_core::sc_report&)
  {
    assignment_rejected = true;
  }
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, actions);
  emit("error", family, "text", int(constructor_rejected), int(assignment_rejected));
}
/** @brief Compare dynamic width, policy application and native fast precision. */
template <class Number>
void policy(const char* family, const char* label, const char* initial, const char* assigned, int width, int integer,
            sc_dt::sc_q_mode q = sc_dt::SC_TRN, sc_dt::sc_o_mode o = sc_dt::SC_WRAP, int n = 0)
{
  Number value(initial, width, integer, q, o, n);
  emit("policy_ctor", family, label, render(value));
  value = assigned;
  emit("policy_assign", family, label, render(value));
}
/** @brief Exercise arbitrary widths, policies and exact large Python integer parity. */
template <class Number>
void policies(const char* family)
{
  policy<Number>(family, "wide137", "9007199254740992.25", "9007199254740993.125", 137, 100);
  policy<Number>(family, "wide191", "18014398509481984.25", "18014398509481985.125", 191, 140);
  policy<Number>(family, "round_saturate", "1.96875", "19.75", 8, 4, sc_dt::SC_RND_CONV, sc_dt::SC_SAT);
  policy<Number>(family, "truncate_wrap", "-9.9375", "19.78125", 8, 4, sc_dt::SC_TRN_ZERO, sc_dt::SC_WRAP, 2);
  policy<Number>(family, "negative_iwl", "0.015625", "-0.00390625", 17, -3);
  policy<Number>(family, "integer_iwl", "1025", "-2049", 17, 25);
  Number exact_integer("1237940039285380274899203449", 137, 100);
  emit("python_integer", family, "exact", render(exact_integer));
}
/** @brief Compare direct native constructors corresponding to exact/public Python calls. */
template <class Number, class Observer, class... Arguments>
void constructor_case(const char* family, const char* label, Observer* expected, const Arguments&... arguments)
{
  Number exact(arguments...);
  emit("ctor", family, label, render(exact), int(fix_owners_probe::observer_matches(exact, expected)));
  Number ordinary(arguments...);
  emit("ctor_public", family, label, render(ordinary), int(fix_owners_probe::observer_matches(ordinary, expected)));
}
/** @brief Exercise all fourteen source-bearing policy/cast/observer constructor forms. */
template <class Number, class Observer, class Source>
void constructor_source(const char* family, const char* source_label, const Source& source, Observer& observer)
{
  using Signature = std::conditional_t<std::is_class_v<Source>, const Source&, Source>;
  if (!fix_owners_probe::Constructors<Number, Observer>::template source<Signature>())
    throw std::runtime_error("constructor source unavailable");
  const sc_dt::sc_fxcast_switch off(sc_dt::SC_OFF);
  const sc_dt::sc_fxtype_params params(137, 100, sc_dt::SC_RND_CONV, sc_dt::SC_SAT, 1);
  const auto label = [source_label](const char* shape) { return std::string(source_label) + "/" + shape; };
#define SOURCE_CTOR(SHAPE, ...) constructor_case<Number>(family, label(SHAPE).c_str(), &observer, source, __VA_ARGS__)
  SOURCE_CTOR("observer", &observer);
  SOURCE_CTOR("width", 19, 7, &observer);
  SOURCE_CTOR("modes", sc_dt::SC_RND_CONV, sc_dt::SC_SAT, &observer);
  SOURCE_CTOR("modes_n", sc_dt::SC_TRN_ZERO, sc_dt::SC_WRAP, 2, &observer);
  SOURCE_CTOR("width_modes", 19, 7, sc_dt::SC_RND_CONV, sc_dt::SC_SAT, &observer);
  SOURCE_CTOR("width_modes_n", 19, 7, sc_dt::SC_TRN_ZERO, sc_dt::SC_WRAP, 2, &observer);
  SOURCE_CTOR("cast", off, &observer);
  SOURCE_CTOR("width_cast", 19, 7, off, &observer);
  SOURCE_CTOR("modes_cast", sc_dt::SC_RND_CONV, sc_dt::SC_SAT, off, &observer);
  SOURCE_CTOR("modes_n_cast", sc_dt::SC_TRN_ZERO, sc_dt::SC_WRAP, 2, off, &observer);
  SOURCE_CTOR("width_modes_cast", 19, 7, sc_dt::SC_RND_CONV, sc_dt::SC_SAT, off, &observer);
  SOURCE_CTOR("width_modes_n_cast", 19, 7, sc_dt::SC_TRN_ZERO, sc_dt::SC_WRAP, 2, off, &observer);
  SOURCE_CTOR("params", params, &observer);
  SOURCE_CTOR("params_cast", params, off, &observer);
#undef SOURCE_CTOR
}
/** @brief Exercise all fourteen source-less forms and the exact copy constructor. */
template <class Number, class Observer>
void constructor_base(const char* family, Observer& observer)
{
  if (!fix_owners_probe::Constructors<Number, Observer>::base())
    throw std::runtime_error("constructor base unavailable");
  const sc_dt::sc_fxcast_switch off(sc_dt::SC_OFF);
  const sc_dt::sc_fxtype_params params(137, 100, sc_dt::SC_RND_CONV, sc_dt::SC_SAT, 1);
#define BASE_CTOR(SHAPE, ...) constructor_case<Number>(family, "none/" SHAPE, &observer, __VA_ARGS__)
  BASE_CTOR("observer", &observer);
  BASE_CTOR("width", 19, 7, &observer);
  BASE_CTOR("modes", sc_dt::SC_RND_CONV, sc_dt::SC_SAT, &observer);
  BASE_CTOR("modes_n", sc_dt::SC_TRN_ZERO, sc_dt::SC_WRAP, 2, &observer);
  BASE_CTOR("width_modes", 19, 7, sc_dt::SC_RND_CONV, sc_dt::SC_SAT, &observer);
  BASE_CTOR("width_modes_n", 19, 7, sc_dt::SC_TRN_ZERO, sc_dt::SC_WRAP, 2, &observer);
  BASE_CTOR("cast", off, &observer);
  BASE_CTOR("width_cast", 19, 7, off, &observer);
  BASE_CTOR("modes_cast", sc_dt::SC_RND_CONV, sc_dt::SC_SAT, off, &observer);
  BASE_CTOR("modes_n_cast", sc_dt::SC_TRN_ZERO, sc_dt::SC_WRAP, 2, off, &observer);
  BASE_CTOR("width_modes_cast", 19, 7, sc_dt::SC_RND_CONV, sc_dt::SC_SAT, off, &observer);
  BASE_CTOR("width_modes_n_cast", 19, 7, sc_dt::SC_TRN_ZERO, sc_dt::SC_WRAP, 2, off, &observer);
  BASE_CTOR("params", params, &observer);
  BASE_CTOR("params_cast", params, off, &observer);
#undef BASE_CTOR
  const Number source("7.875", 19, 7, &observer);
  constructor_case<Number>(family, "copy/copy", static_cast<Observer*>(nullptr), source);
}
/** @brief Execute one deterministic half of all 253 owner constructor forms. */
template <int Group, class Number, class Observer>
void constructors(const char* family)
{
  emit("ctor_traits", family, "exact", int(fix_owners_probe::access_traits()),
       int(fix_owners_probe::observers_enabled()));
  const int before = Observer::destroyed;
  {
    Observer observer;
    if constexpr (Group == 0)
    {
      constructor_base<Number>(family, observer);
      constructor_source<Number>(family, "int", int(-3), observer);
      constructor_source<Number>(family, "unsigned_int", unsigned(0xA5A55A5A), observer);
      constructor_source<Number>(family, "long", long(sizeof(long) >= 8 ? -0x123456789ABCDEFLL : -0x1234567L),
                                 observer);
      constructor_source<Number>(
        family, "unsigned_long",
        static_cast<unsigned long>(sizeof(unsigned long) >= 8 ? 0x123456789ABCDEFULL : 0x12345678UL), observer);
      constructor_source<Number>(family, "float", float(1.375), observer);
      constructor_source<Number>(family, "double", double(2.625), observer);
      constructor_source<Number>(family, "text", static_cast<const char*>("3.1875"), observer);
      constructor_source<Number>(family, "fxval", fix_owners_probe::fxval_source(), observer);
      constructor_source<Number>(family, "fxval_fast", fix_owners_probe::fxval_fast_source(), observer);
    }
    else
    {
      constructor_source<Number>(family, "fxnum", fix_owners_probe::fxnum_source(), observer);
      constructor_source<Number>(family, "fxnum_fast", fix_owners_probe::fxnum_fast_source(), observer);
      constructor_source<Number>(family, "int64", sc_dt::int64(-0x123456789ABCDEFLL), observer);
      constructor_source<Number>(family, "uint64", sc_dt::uint64(0xFEDCBA9876543210ULL), observer);
      constructor_source<Number>(family, "int_base", fix_owners_probe::int_base_source(), observer);
      constructor_source<Number>(family, "uint_base", fix_owners_probe::uint_base_source(), observer);
      constructor_source<Number>(family, "signed", fix_owners_probe::signed_source(), observer);
      constructor_source<Number>(family, "unsigned", fix_owners_probe::unsigned_source(), observer);
    }
    emit("ctor_lifetime", family, "numbers_gone", int(Observer::destroyed == before));
  }
  emit("ctor_lifetime", family, "observer_gone", int(Observer::destroyed == before + 1));
}
/** @brief Select one family and one deterministic constructor/mutation mode. */
template <int Mode, class Number, class Observer, class Mask, class FastMask, class Postfix>
void family(const char* name)
{
  const sc_dt::sc_fxtype_params context_params(Mode == 2 ? 19 : 23, Mode == 2 ? 7 : 9, sc_dt::SC_TRN, sc_dt::SC_WRAP,
                                               0);
  sc_dt::sc_fxtype_context context(context_params);
  if constexpr (Mode < 2)
    constructors<Mode, Number, Observer>(name);
  else
  {
    run_family<Number, Observer, Mask, FastMask, Postfix>(name);
    policies<Number>(name);
  }
}
#ifndef FIX_OWNERS_FAMILY
#define FIX_OWNERS_FAMILY -1
#endif
#ifndef FIX_OWNERS_MODE
#define FIX_OWNERS_MODE -1
#endif
/** @brief Instantiate only the selected constructor or mutation code paths. */
template <class Number, class Observer, class Mask, class FastMask, class Postfix>
void selected(const char* name)
{
#if FIX_OWNERS_MODE < 0 || FIX_OWNERS_MODE == 0
  family<0, Number, Observer, Mask, FastMask, Postfix>(name);
#endif
#if FIX_OWNERS_MODE < 0 || FIX_OWNERS_MODE == 1
  family<1, Number, Observer, Mask, FastMask, Postfix>(name);
#endif
#if FIX_OWNERS_MODE < 0 || FIX_OWNERS_MODE == 2
  family<2, Number, Observer, Mask, FastMask, Postfix>(name);
#endif
}
}  // namespace
/** @brief Run selected independent native family/modes without kernel resets. */
int sc_main(int, char**)
{
#if FIX_OWNERS_FAMILY < 0 || FIX_OWNERS_FAMILY == 0
  selected<sc_dt::sc_fix, fix_owners_probe::Observer, sc_dt::sc_fix, sc_dt::sc_fix_fast, sc_dt::sc_fxval>("sc_fix");
#endif
#if FIX_OWNERS_FAMILY < 0 || FIX_OWNERS_FAMILY == 1
  selected<sc_dt::sc_ufix, fix_owners_probe::Observer, sc_dt::sc_ufix, sc_dt::sc_ufix_fast, sc_dt::sc_fxval>("sc_ufix");
#endif
#if FIX_OWNERS_FAMILY < 0 || FIX_OWNERS_FAMILY == 2
  selected<sc_dt::sc_fix_fast, fix_owners_probe::FastObserver, sc_dt::sc_fix, sc_dt::sc_fix_fast, sc_dt::sc_fxval_fast>(
    "sc_fix_fast");
#endif
#if FIX_OWNERS_FAMILY < 0 || FIX_OWNERS_FAMILY == 3
  selected<sc_dt::sc_ufix_fast, fix_owners_probe::FastObserver, sc_dt::sc_ufix, sc_dt::sc_ufix_fast,
           sc_dt::sc_fxval_fast>("sc_ufix_fast");
#endif
  return 0;
}

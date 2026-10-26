/** @file sc_dt_fixed_templates_public.cpp
 * @brief Independent native reference for four constrained fixed-point templates.
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

#include "sc_dt_fixed_templates_probe.h"

namespace
{
/** @brief Emit one exact native semantic and signature observation. */
template <class... Values>
void emit(const char* operation, const char* family, const char* label, const Values&... values)
{
  std::cout << "FTM\t" << operation << '\t' << family << '\t' << label;
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
/** @brief Exercise all constructor, assignment and arithmetic forms for one exact source. */
template <class Number, class Observer, class Traits, class Source>
void source_case(const char* family, const char* label, const Source& source, Observer& observer)
{
  using Signature = std::conditional_t<std::is_class_v<Source>, const Source&, Source>;
  static_assert(std::is_constructible_v<Number, Signature, Observer*>);
  static_assert(std::is_constructible_v<Number, Signature, const sc_dt::sc_fxcast_switch&, Observer*>);
  if (!Traits::template source<Signature>()) throw std::runtime_error("source signature unavailable");
  const sc_dt::sc_fxcast_switch off(sc_dt::SC_OFF);
  {
    Number number(source, &observer);
    emit("ctor", family, label, render(number), int(fixed_templates_probe::observer_matches(number, &observer)));
  }
  {
    Number number(source, off, &observer);
    emit("ctor_cast", family, label, render(number), int(fixed_templates_probe::observer_matches(number, &observer)));
  }
  {
    Number number(source, &observer);
    emit("ctor_public", family, label, render(number), int(fixed_templates_probe::observer_matches(number, &observer)));
  }
  {
    Number number(source, off, &observer);
    emit("ctor_public_cast", family, label, render(number),
         int(fixed_templates_probe::observer_matches(number, &observer)));
  }
  Number number("37.8125");
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
  using Traits = fixed_templates_probe::Signatures<Number, Mask, FastMask, Postfix>;
  emit("traits", family, "exact", int(Traits::valid()), int(fixed_templates_probe::observers_enabled()));
  const int destroyed = Observer::destroyed;
  {
    Observer observer;
    const sc_dt::sc_fxcast_switch off(sc_dt::SC_OFF);
    {
      Number number(&observer);
      emit("ctor", family, "observer", render(number), int(fixed_templates_probe::observer_matches(number, &observer)));
    }
    {
      Number number(off, &observer);
      emit("ctor", family, "cast_observer", render(number),
           int(fixed_templates_probe::observer_matches(number, &observer)));
    }
    {
      Number source("7.875", &observer);
      Number copy(source);
      emit("ctor", family, "copy", render(copy),
           int(fixed_templates_probe::observer_matches(copy, static_cast<Observer*>(nullptr))));
    }
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
    source_case<Number, Observer, Traits>(family, "fxval", fixed_templates_probe::fxval_source(), observer);
    source_case<Number, Observer, Traits>(family, "fxval_fast", fixed_templates_probe::fxval_fast_source(), observer);
    source_case<Number, Observer, Traits>(family, "fxnum", fixed_templates_probe::fxnum_source(), observer);
    source_case<Number, Observer, Traits>(family, "fxnum_fast", fixed_templates_probe::fxnum_fast_source(), observer);
    source_case<Number, Observer, Traits>(family, "int64", sc_dt::int64(-0x123456789ABCDEFLL), observer);
    source_case<Number, Observer, Traits>(family, "uint64", sc_dt::uint64(0xFEDCBA9876543210ULL), observer);
    source_case<Number, Observer, Traits>(family, "int_base", fixed_templates_probe::int_base_source(), observer);
    source_case<Number, Observer, Traits>(family, "uint_base", fixed_templates_probe::uint_base_source(), observer);
    source_case<Number, Observer, Traits>(family, "signed", fixed_templates_probe::signed_source(), observer);
    source_case<Number, Observer, Traits>(family, "unsigned", fixed_templates_probe::unsigned_source(), observer);
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
    emit("observer_lifetime", family, "numbers_gone", int(Observer::destroyed == destroyed));
  }
  emit("observer_lifetime", family, "observer_gone", int(Observer::destroyed == destroyed + 1));
  std::unique_ptr<Number> constructor_copy;
  Number copy("0");
  Postfix postfix("0");
  {
    Number source("7.875");
    constructor_copy.reset(new Number(source));
    copy = source;
    postfix = source++;
    source = "0";
  }
  emit("lifetime", family, "copy_postfix", render(copy), postfix.to_string(), render(*constructor_copy));
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
/** @brief Compare arbitrary template widths, policy application and native fast precision. */
template <class Number>
void policy(const char* family, const char* label, const char* initial, const char* assigned)
{
  Number value(initial);
  emit("policy_ctor", family, label, render(value));
  value = assigned;
  emit("policy_assign", family, label, render(value));
}
}  // namespace
/** @brief Run all four exact declaration and policy matrices in a fresh native process. */
int sc_main(int, char**)
{
  using namespace sc_dt;
  run_family<sc_fixed<19, 7>, fixed_templates_probe::Observer, sc_fix, sc_fix_fast, sc_fxval>("sc_fixed");
  run_family<sc_ufixed<19, 7>, fixed_templates_probe::Observer, sc_ufix, sc_ufix_fast, sc_fxval>("sc_ufixed");
  run_family<sc_fixed_fast<19, 7>, fixed_templates_probe::FastObserver, sc_fix, sc_fix_fast, sc_fxval_fast>(
    "sc_fixed_fast");
  run_family<sc_ufixed_fast<19, 7>, fixed_templates_probe::FastObserver, sc_ufix, sc_ufix_fast, sc_fxval_fast>(
    "sc_ufixed_fast");
#define POLICY_MATRIX(FAMILY)                                                                     \
  policy<FAMILY<137, 100>>(#FAMILY, "wide137", "9007199254740992.25", "9007199254740993.125");    \
  policy<FAMILY<191, 140>>(#FAMILY, "wide191", "18014398509481984.25", "18014398509481985.125");  \
  policy<FAMILY<8, 4, SC_RND_CONV, SC_SAT>>(#FAMILY, "round_saturate", "1.96875", "19.75");       \
  policy<FAMILY<8, 4, SC_TRN_ZERO, SC_WRAP, 2>>(#FAMILY, "truncate_wrap", "-9.9375", "19.78125"); \
  policy<FAMILY<17, -3>>(#FAMILY, "negative_iwl", "0.015625", "-0.00390625");                     \
  policy<FAMILY<17, 25>>(#FAMILY, "integer_iwl", "1025", "-2049")
  POLICY_MATRIX(sc_fixed);
  POLICY_MATRIX(sc_ufixed);
  POLICY_MATRIX(sc_fixed_fast);
  POLICY_MATRIX(sc_ufixed_fast);
  const char* exact_integer = "1237940039285380274899203449";
#define PYTHON_INTEGER(FAMILY)                               \
  {                                                          \
    FAMILY<137, 100> value(exact_integer);                   \
    emit("python_integer", #FAMILY, "exact", render(value)); \
  }
  PYTHON_INTEGER(sc_fixed);
  PYTHON_INTEGER(sc_ufixed);
  PYTHON_INTEGER(sc_fixed_fast);
  PYTHON_INTEGER(sc_ufixed_fast);
#undef PYTHON_INTEGER
#undef POLICY_MATRIX
  return 0;
}

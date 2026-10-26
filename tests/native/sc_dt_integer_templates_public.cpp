/** @file sc_dt_integer_templates_public.cpp
 * @brief Independent native reference for public SystemC integer templates.
 * @details Every emitted constructor and assignment is called directly in
 *          C++. Template instances own their numeric storage; source views
 *          borrow only sources that remain alive for the whole case. No
 *          callbacks, Python references, streams, or scheduler activity are
 *          retained across a record.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <type_traits>

#include "sc_dt_integer_templates_probe.h"

namespace
{

/** @brief Emit one decimal value with its exact API operation label. */
template <typename Value>
void emit(const char* family, int width, const char* operation, const Value& value)
{
  std::cout << "ITM\t" << family << '\t' << width << '\t' << operation << '\t' << value << '\n';
}

/** @brief Convert a native SystemC integer to its full-width decimal text. */
template <typename Value>
std::string decimal(const Value& value)
{
  return value.to_string(sc_dt::SC_DEC);
}

/** @brief Compare one native constructor and assignment overload directly. */
template <typename Target, typename Source>
void case_source(const char* family, int width, const char* label, const Source& source)
{
  Target constructed(source);
  emit(family, width, (std::string("ctor_") + label).c_str(), decimal(constructed));
  Target assigned;
  Target& result = (assigned = source);
  emit(family, width, (std::string("assign_") + label).c_str(), decimal(assigned));
  emit(family, width, (std::string("assign_") + label + "_self").c_str(), &result == &assigned);
}

/** @brief Exercise an assignment overload when the class has no matching constructor. */
template <typename Target, typename Source>
void case_assignment(const char* family, int width, const char* label, const Source& source)
{
  Target assigned;
  Target& result = (assigned = source);
  emit(family, width, (std::string("assign_") + label).c_str(), decimal(assigned));
  emit(family, width, (std::string("assign_") + label + "_self").c_str(), &result == &assigned);
}

/** @brief Force concat sources through the inventoried generic-base overload.
 * @details sc_concref also exposes conversions to both bit- and logic-vector
 *          bases, so an ordinary C++ overload expression is ambiguous. The
 *          native inventory's generic constructor/assignment is selected
 *          explicitly by binding the same source to its declared CRTP base.
 */
template <typename Target, typename Source>
void case_generic(const char* family, int width, const char* label, const Source& source)
{
  using SourceType = std::remove_cv_t<Source>;
  const sc_dt::sc_generic_base<SourceType>& generic = source;
  Target constructed(generic);
  emit(family, width, (std::string("ctor_") + label).c_str(), decimal(constructed));
  Target assigned;
  Target& result = (assigned = generic);
  emit(family, width, (std::string("assign_") + label).c_str(), decimal(assigned));
  emit(family, width, (std::string("assign_") + label + "_self").c_str(), &result == &assigned);
}

/** @brief Verify copy construction owns an independent value copy. */
template <typename Target>
void case_copy(const char* family, int width, const Target& source)
{
  Target copied(source);
  Target& assign_result = (copied = source);
  emit(family, width, "copy_ctor", decimal(copied));
  emit(family, width, "copy_assign", decimal(assign_result));
  emit(family, width, "copy_assignment_self", &assign_result == &copied);
  Target changed(source);
  changed = 0;
  emit(family, width, "copy_independent", decimal(copied));
  emit(family, width, "copy_lifetime", decimal(copied));
}

/** @brief Check fixed-width compound, prefix, and postfix operators. */
template <typename Target, typename Scalar>
void case_fixed_operators(const char* family, int width)
{
  Target value(48);
  value += static_cast<Scalar>(5);
  emit(family, width, "plus_assign", decimal(value));
  value -= static_cast<Scalar>(3);
  emit(family, width, "minus_assign", decimal(value));
  value *= static_cast<Scalar>(2);
  emit(family, width, "multiply_assign", decimal(value));
  value /= static_cast<Scalar>(4);
  emit(family, width, "divide_assign", decimal(value));
  value %= static_cast<Scalar>(7);
  emit(family, width, "modulo_assign", decimal(value));
  value &= static_cast<Scalar>(6);
  emit(family, width, "and_assign", decimal(value));
  value |= static_cast<Scalar>(8);
  emit(family, width, "or_assign", decimal(value));
  value ^= static_cast<Scalar>(3);
  emit(family, width, "xor_assign", decimal(value));
  value <<= static_cast<Scalar>(2);
  emit(family, width, "left_shift_assign", decimal(value));
  value >>= static_cast<Scalar>(1);
  emit(family, width, "right_shift_assign", decimal(value));
  const Target prefix_up = ++value;
  emit(family, width, "prefix_increment", decimal(prefix_up));
  const Target postfix_up = value++;
  emit(family, width, "postfix_increment_old", decimal(postfix_up));
  emit(family, width, "postfix_increment_new", decimal(value));
  const Target prefix_down = --value;
  emit(family, width, "prefix_decrement", decimal(prefix_down));
  const Target postfix_down = value--;
  emit(family, width, "postfix_decrement_old", decimal(postfix_down));
  emit(family, width, "postfix_decrement_new", decimal(value));
}

/** @brief Run big integer constructor and assignment overload observations. */
template <typename Target>
void exercise_big(const char* family, int width, bool is_signed)
{
  Target initial("12345678901234567890");
  Target default_value;
  emit(family, width, "default_ctor", decimal(default_value));
  case_copy(family, width, initial);

  sc_dt::sc_signed signed_source(80);
  signed_source = "-123456789012";
  sc_dt::sc_unsigned unsigned_source(80);
  unsigned_source = "234567890123";
  auto signed_range = signed_source.range(31, 5);
  auto unsigned_range = unsigned_source.range(31, 5);
  sc_dt::sc_bv<16> bv_source("1010010110100101");
  sc_dt::sc_lv<16> lv_source("0110101011010011");
  const auto& generic_source = (signed_source, unsigned_source);
  sc_dt::sc_int<23> fixed_signed(-345678);
  sc_dt::sc_uint<23> fixed_unsigned(456789);

  case_source<Target>(family, width, "signed", signed_source);
  case_source<Target>(family, width, "signed_subref", signed_range);
  case_generic<Target>(family, width, "generic_concat", generic_source);
  case_source<Target>(family, width, "unsigned", unsigned_source);
  case_source<Target>(family, width, "unsigned_subref", unsigned_range);
  case_source<Target>(family, width, "string", "-765432109876");
  case_source<Target>(family, width, "int64", sc_dt_integer_templates_probe::int64_value());
  case_source<Target>(family, width, "uint64", sc_dt_integer_templates_probe::uint64_value());
  case_source<Target>(family, width, "long", sc_dt_integer_templates_probe::long_value());
  case_source<Target>(family, width, "ulong", sc_dt_integer_templates_probe::ulong_value());
  case_source<Target>(family, width, "int", sc_dt_integer_templates_probe::int_value());
  case_source<Target>(family, width, "uint", sc_dt_integer_templates_probe::uint_value());
  case_source<Target>(family, width, "double", sc_dt_integer_templates_probe::double_value());
  case_source<Target>(family, width, "bv", bv_source);
  case_source<Target>(family, width, "lv", lv_source);
  case_assignment<Target>(family, width, "int_base", fixed_signed);
  case_assignment<Target>(family, width, "uint_base", fixed_unsigned);

#ifdef SC_INCLUDE_FX
  const sc_dt::sc_fxval fx_value(23.75);
  const sc_dt::sc_fxval_fast fx_fast_value(31.5);
  const sc_dt::sc_fixed<20, 12> fx_number(41.75);
  const sc_dt::sc_fixed_fast<20, 12> fx_fast_number(57.25);
  case_source<Target>(family, width, "fxval", fx_value);
  case_source<Target>(family, width, "fxval_fast", fx_fast_value);
  case_source<Target>(family, width, "fxnum", fx_number);
  case_source<Target>(family, width, "fxnum_fast", fx_fast_number);
#endif

  if (is_signed)
  {
    const sc_dt::int64 scalar = 0x123456789abLL;
    Target ordinary_integer(scalar);
    emit(family, width, "python_int_gate_expected", decimal(ordinary_integer));
    emit(family, width, "python_int_gate_ctor", decimal(ordinary_integer));
    Target assigned;
    assigned = scalar;
    emit(family, width, "python_int_gate_assign_expected", decimal(assigned));
    emit(family, width, "python_int_gate_assign", decimal(assigned));
  }
  else
  {
    const sc_dt::uint64 scalar = 0xfedcba9876543210ULL;
    Target ordinary_integer(scalar);
    emit(family, width, "python_int_gate_expected", decimal(ordinary_integer));
    emit(family, width, "python_int_gate_ctor", decimal(ordinary_integer));
    Target assigned;
    assigned = scalar;
    emit(family, width, "python_int_gate_assign_expected", decimal(assigned));
    emit(family, width, "python_int_gate_assign", decimal(assigned));
  }
}

/** @brief Verify native string constructor and assignment error behavior. */
template <typename Target>
void case_invalid_string(const char* family, int width)
{
  const auto old_actions = sc_dt_integer_templates_probe::throw_errors();
  bool constructor_caught = false;
  try
  {
    Target invalid("");
    (void)invalid;
  }
  catch (const sc_core::sc_report&)
  {
    constructor_caught = true;
  }
  emit(family, width, "invalid_string_ctor", constructor_caught);
  Target assigned;
  bool assignment_caught = false;
  try
  {
    assigned = "";
  }
  catch (const sc_core::sc_report&)
  {
    assignment_caught = true;
  }
  emit(family, width, "invalid_string_assign", assignment_caught);
  sc_dt_integer_templates_probe::restore_errors(old_actions);
}

/** @brief Emit a reference value larger than Python's default decimal limit. */
void case_large_python_integer()
{
  constexpr int width = 16384;
  sc_dt::sc_biguint<width> expected;
  expected[16000] = true;
  emit("sc_dt::sc_biguint", width, "python_large_int_ctor_expected", decimal(expected));
  emit("sc_dt::sc_biguint", width, "python_large_int_ctor", decimal(expected));
  emit("sc_dt::sc_biguint", width, "python_large_int_assign_expected", decimal(expected));
  emit("sc_dt::sc_biguint", width, "python_large_int_assign", decimal(expected));
}

/** @brief Exercise width-independent construction, assignment, and copying. */
template <typename Target>
void case_width_probe(const char* family, int width)
{
  Target constructed("123456789");
  emit(family, width, "width_probe_ctor", decimal(constructed));
  Target assigned;
  Target& result = (assigned = constructed);
  emit(family, width, "width_probe_assign", decimal(assigned));
  emit(family, width, "width_probe_assign_self", &result == &assigned);
  Target copied(constructed);
  emit(family, width, "width_probe_copy", decimal(copied));
}

/** @brief Run bounded integer constructors, assignments, and operators. */
template <typename Target, typename SourceFixed, typename Scalar>
void exercise_fixed(const char* family, int width, bool is_signed)
{
  Target initial(0x12345);
  Target default_value;
  emit(family, width, "default_ctor", decimal(default_value));
  case_copy(family, width, initial);

  SourceFixed fixed_source(0x23456);
  const SourceFixed& const_fixed_source = fixed_source;
  auto fixed_range = const_fixed_source.range(15, 3);
  sc_dt::sc_signed signed_source(48);
  signed_source = "-123456789012";
  sc_dt::sc_unsigned unsigned_source(48);
  unsigned_source = "234567890123";
  sc_dt::sc_bv<16> bv_source("1010010110100101");
  sc_dt::sc_lv<16> lv_source("0110101011010011");
  const auto& generic_source = (signed_source, unsigned_source);

  case_source<Target>(family, width, "base", fixed_source);
  case_source<Target>(family, width, "subref", fixed_range);
  case_generic<Target>(family, width, "generic_concat", generic_source);
  case_source<Target>(family, width, "signed", signed_source);
  case_source<Target>(family, width, "unsigned", unsigned_source);
  case_source<Target>(family, width, "string", "-765432109876");
  if (is_signed)
  {
    case_source<Target>(family, width, "int_type", sc_dt_integer_templates_probe::int64_value());
    case_source<Target>(family, width, "uint64", sc_dt_integer_templates_probe::uint64_value());
  }
  else
  {
    case_source<Target>(family, width, "uint_type", sc_dt_integer_templates_probe::uint64_value());
    case_source<Target>(family, width, "int64", sc_dt_integer_templates_probe::int64_value());
  }
  case_source<Target>(family, width, "long", sc_dt_integer_templates_probe::long_value());
  case_source<Target>(family, width, "ulong", sc_dt_integer_templates_probe::ulong_value());
  case_source<Target>(family, width, "int", sc_dt_integer_templates_probe::int_value());
  case_source<Target>(family, width, "uint", sc_dt_integer_templates_probe::uint_value());
  case_source<Target>(family, width, "double", sc_dt_integer_templates_probe::double_value());
  case_source<Target>(family, width, "bv", bv_source);
  case_source<Target>(family, width, "lv", lv_source);

#ifdef SC_INCLUDE_FX
  const sc_dt::sc_fxval fx_value(23.75);
  const sc_dt::sc_fxval_fast fx_fast_value(31.5);
  const sc_dt::sc_fixed<20, 12> fx_number(41.75);
  const sc_dt::sc_fixed_fast<20, 12> fx_fast_number(57.25);
  case_source<Target>(family, width, "fxval", fx_value);
  case_source<Target>(family, width, "fxval_fast", fx_fast_value);
  case_source<Target>(family, width, "fxnum", fx_number);
  case_source<Target>(family, width, "fxnum_fast", fx_fast_number);
#endif

  Target ordinary_integer;
  if (is_signed)
  {
    ordinary_integer = static_cast<sc_dt::int64>(0x123456789abLL);
    emit(family, width, "python_int_gate_expected", decimal(ordinary_integer));
    emit(family, width, "python_int_gate_ctor", decimal(ordinary_integer));
    emit(family, width, "python_int_gate_assign_expected", decimal(ordinary_integer));
    emit(family, width, "python_int_gate_assign", decimal(ordinary_integer));
  }
  else
  {
    ordinary_integer = static_cast<sc_dt::uint64>(0xfedcba9876543210ULL);
    emit(family, width, "python_int_gate_expected", decimal(ordinary_integer));
    emit(family, width, "python_int_gate_ctor", decimal(ordinary_integer));
    emit(family, width, "python_int_gate_assign_expected", decimal(ordinary_integer));
    emit(family, width, "python_int_gate_assign", decimal(ordinary_integer));
  }
  case_fixed_operators<Target, Scalar>(family, width);
}

}  // namespace

/** @brief Emit direct native observations at arbitrary and bounded widths. */
int sc_main(int, char**)
{
  exercise_big<sc_dt::sc_bigint<48>>("sc_dt::sc_bigint", 48, true);
  exercise_big<sc_dt::sc_biguint<48>>("sc_dt::sc_biguint", 48, false);
  case_width_probe<sc_dt::sc_bigint<137>>("sc_dt::sc_bigint", 137);
  case_width_probe<sc_dt::sc_biguint<137>>("sc_dt::sc_biguint", 137);
  case_width_probe<sc_dt::sc_bigint<191>>("sc_dt::sc_bigint", 191);
  case_width_probe<sc_dt::sc_biguint<191>>("sc_dt::sc_biguint", 191);
  case_width_probe<sc_dt::sc_bigint<257>>("sc_dt::sc_bigint", 257);
  case_width_probe<sc_dt::sc_biguint<257>>("sc_dt::sc_biguint", 257);

  case_width_probe<sc_dt::sc_int<19>>("sc_dt::sc_int", 19);
  case_width_probe<sc_dt::sc_uint<19>>("sc_dt::sc_uint", 19);
  exercise_fixed<sc_dt::sc_int<59>, sc_dt::sc_int<17>, sc_dt::int64>("sc_dt::sc_int", 59, true);
  exercise_fixed<sc_dt::sc_uint<59>, sc_dt::sc_uint<17>, sc_dt::uint64>("sc_dt::sc_uint", 59, false);
  case_width_probe<sc_dt::sc_int<63>>("sc_dt::sc_int", 63);
  case_width_probe<sc_dt::sc_uint<63>>("sc_dt::sc_uint", 63);
  case_width_probe<sc_dt::sc_int<64>>("sc_dt::sc_int", 64);
  case_width_probe<sc_dt::sc_uint<64>>("sc_dt::sc_uint", 64);
  case_large_python_integer();
  case_invalid_string<sc_dt::sc_bigint<48>>("sc_dt::sc_bigint", 48);
  case_invalid_string<sc_dt::sc_biguint<48>>("sc_dt::sc_biguint", 48);
  case_invalid_string<sc_dt::sc_int<59>>("sc_dt::sc_int", 59);
  case_invalid_string<sc_dt::sc_uint<59>>("sc_dt::sc_uint", 59);
  return 0;
}

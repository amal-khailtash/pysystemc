/** @file sc_dt_bounded_bases_public.cpp
 * @brief Native reference for bounded integer base declarations.
 * @details The reference directly constructs and calls SystemC values. Native
 *          integers own their storage; ranges, bit references and concatenated
 *          views borrow their owners only within each test case. No callbacks
 *          or Python references are retained.
 */
#include <array>
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>

#include "sc_dt_bounded_bases_probe.h"

namespace
{
using namespace sc_dt_bounded_bases_probe;

template <class Base>
std::string decimal(const Base& value)
{
  return value.to_string(sc_dt::SC_DEC);
}

template <class Base>
void emit_value(const char* family, const char* group, const char* label, const Base& value)
{
  std::cout << "BAS\t" << family << '\t' << group << '\t' << label << '\t' << decimal(value) << '\t' << value.length()
            << '\n';
}

void emit_fields(const char* family, const char* group, const char* label, const std::string& fields)
{
  std::cout << "BAS\t" << family << '\t' << group << '\t' << label << '\t' << fields << '\n';
}

template <class Base, class Source>
void assign_case(const char* family, const char* label, Base& target, const Source& source)
{
  Base& result = (target = source);
  emit_fields(family, "assign", label, decimal(target) + "\t" + std::to_string(&result == &target));
}

template <class Base, class Source>
void constructor_case(const char* family, const char* label, const Source& source)
{
  Base result(source);
  emit_value(family, "construct", label, result);
}

template <class Base, class Scalar>
void exercise_compounds(const char* family, Scalar scalar)
{
  Base value(static_cast<Scalar>(48), 32);
  Base& add_result = (value += static_cast<Scalar>(5));
  emit_fields(family, "compound", "plus_assign", decimal(value) + "\t" + std::to_string(&add_result == &value));
  Base& subtract_result = (value -= static_cast<Scalar>(3));
  emit_fields(family, "compound", "minus_assign", decimal(value) + "\t" + std::to_string(&subtract_result == &value));
  Base& multiply_result = (value *= static_cast<Scalar>(2));
  emit_fields(family, "compound", "multiply_assign",
              decimal(value) + "\t" + std::to_string(&multiply_result == &value));
  Base& divide_result = (value /= static_cast<Scalar>(4));
  emit_fields(family, "compound", "divide_assign", decimal(value) + "\t" + std::to_string(&divide_result == &value));
  Base& modulo_result = (value %= static_cast<Scalar>(7));
  emit_fields(family, "compound", "modulo_assign", decimal(value) + "\t" + std::to_string(&modulo_result == &value));
  Base& and_result = (value &= static_cast<Scalar>(6));
  emit_fields(family, "compound", "and_assign", decimal(value) + "\t" + std::to_string(&and_result == &value));
  Base& or_result = (value |= static_cast<Scalar>(8));
  emit_fields(family, "compound", "or_assign", decimal(value) + "\t" + std::to_string(&or_result == &value));
  Base& xor_result = (value ^= static_cast<Scalar>(3));
  emit_fields(family, "compound", "xor_assign", decimal(value) + "\t" + std::to_string(&xor_result == &value));
  Base& left_result = (value <<= static_cast<Scalar>(2));
  emit_fields(family, "compound", "left_shift_assign", decimal(value) + "\t" + std::to_string(&left_result == &value));
  Base& right_result = (value >>= static_cast<Scalar>(1));
  emit_fields(family, "compound", "right_shift_assign",
              decimal(value) + "\t" + std::to_string(&right_result == &value));

  Base& prefix_up = ++value;
  emit_fields(family, "compound", "prefix_increment", decimal(value) + "\t" + std::to_string(&prefix_up == &value));
  const Base postfix_up = value++;
  emit_fields(family, "compound", "postfix_increment", decimal(postfix_up) + "\t" + decimal(value));
  Base& prefix_down = --value;
  emit_fields(family, "compound", "prefix_decrement", decimal(value) + "\t" + std::to_string(&prefix_down == &value));
  const Base postfix_down = value--;
  emit_fields(family, "compound", "postfix_decrement", decimal(postfix_down) + "\t" + decimal(value));
  (void)scalar;
}

template <class Base, class Scalar>
void exercise_widths(const char* family, Scalar sample)
{
  for (const int width : {1, 19, 59, 63, 64})
  {
    Base value(static_cast<Scalar>(sample), width);
    emit_value(family, "width", std::to_string(width).c_str(), value);
    const Base copy(value);
    value = static_cast<Scalar>(0);
    emit_value(family, "width_copy", std::to_string(width).c_str(), copy);
  }

  const auto previous = throw_errors();
  for (const int width : {0, -1, 65})
  {
    bool caught = false;
    std::string report_type;
    std::string report_message;
    try
    {
      Base invalid(width);
      (void)invalid;
    }
    catch (const sc_core::sc_report& report)
    {
      caught = true;
      report_type = report.get_msg_type();
      report_message = report.get_msg();
    }
    emit_fields(family, "invalid_width", std::to_string(width).c_str(),
                std::to_string(caught) + "\t" + report_type + "\t" + report_message);
  }
  restore_errors(previous);
}

template <class Base, class Scalar>
void exercise_base(const char* family, Scalar scalar, bool signed_value)
{
  using namespace sc_dt;

  Base default_value;
  emit_value(family, "construct", "default", default_value);
  Base zero_value(32);
  emit_value(family, "construct", "width", zero_value);
  Base scalar_value(static_cast<Scalar>(scalar), 32);
  emit_value(family, "construct", "scalar_width", scalar_value);
  const Base copy_value(scalar_value);
  emit_value(family, "construct", "copy", copy_value);

  sc_signed signed_source(48);
  signed_source = "-123456789";
  sc_unsigned unsigned_source(48);
  unsigned_source = "234567890";
  const auto& signed_source_range = static_cast<const sc_signed&>(signed_source).range(31, 4);
  const auto& unsigned_source_range = static_cast<const sc_unsigned&>(unsigned_source).range(31, 4);
  sc_int<24> int_source(-345678);
  sc_uint<24> uint_source(456789);
  const sc_int<24>& const_int_source = int_source;
  const sc_uint<24>& const_uint_source = uint_source;
  const auto& int_source_range = const_int_source.range(19, 3);
  const auto& uint_source_range = const_uint_source.range(19, 3);
  sc_bv<8> bv_source("10100101");
  sc_lv<8> lv_source("01101010");
  sc_signed generic_signed(8);
  generic_signed = 0xab;
  sc_unsigned generic_unsigned(8);
  generic_unsigned = 0x35;
  const auto& generic_value = (generic_signed, generic_unsigned);
  const sc_generic_base<sc_concatref>& generic_source = generic_value;

  constructor_case<Base>(family, "signed", signed_source);
  constructor_case<Base>(family, "unsigned", unsigned_source);
  constructor_case<Base>(family, "signed_subref", signed_source_range);
  constructor_case<Base>(family, "unsigned_subref", unsigned_source_range);
  constructor_case<Base>(family, "int_subref", int_source_range);
  constructor_case<Base>(family, "uint_subref", uint_source_range);
  constructor_case<Base>(family, "bv", bv_source);
  constructor_case<Base>(family, "lv", lv_source);
  Base generic_construct(generic_source);
  emit_value(family, "construct", "generic", generic_construct);

  Base target(32);
  assign_case(family, "base", target, scalar_value);
  assign_case(family, "signed", target, signed_source);
  assign_case(family, "unsigned", target, unsigned_source);
  if constexpr (std::is_same<Base, SignedBase>::value)
    assign_case(family, "int_subref", target, int_source_range);
  else
    assign_case(family, "uint_subref", target, uint_source_range);
  assign_case(family, "bv", target, bv_source);
  assign_case(family, "lv", target, lv_source);
  assign_case(family, "string", target, "-765432109876");
  assign_case(family, "unsigned_long", target, static_cast<unsigned long>(345678901UL));
  assign_case(family, "long", target, static_cast<long>(-123456789L));
  assign_case(family, "unsigned_int", target, static_cast<unsigned int>(2345678U));
  assign_case(family, "int", target, static_cast<int>(-1234567));
  assign_case(family, "double", target, -9876.75);
  assign_case(family, "generic", target, generic_source);
  if constexpr (std::is_same<Base, SignedBase>::value)
  {
    assign_case(family, "scalar_int64", target, static_cast<sc_dt::int64>(scalar));
    assign_case(family, "scalar_uint64", target, static_cast<sc_dt::uint64>(0xfedcba9876543210ULL));
  }
  else
  {
    assign_case(family, "scalar_uint64", target, static_cast<sc_dt::uint64>(scalar));
    assign_case(family, "scalar_int64", target, static_cast<sc_dt::int64>(-0x123456789abLL));
  }

#ifdef SC_INCLUDE_FX
  const sc_fxval fx_value(23.75);
  const sc_fxval_fast fx_fast_value(31.5);
  const sc_fixed<20, 12> fx_number(41.75);
  const sc_fixed_fast<20, 12> fx_fast_number(57.25);
  assign_case(family, "fxval", target, fx_value);
  assign_case(family, "fxval_fast", target, fx_fast_value);
  assign_case(family, "fxnum", target, fx_number);
  assign_case(family, "fxnum_fast", target, fx_fast_number);
#endif

  exercise_compounds<Base>(family, scalar);

  target = static_cast<Scalar>(0x12345);
  emit_fields(family, "observe", "numeric",
              std::to_string(target.value()) + "\t" + std::to_string(target.to_int()) + "\t" +
                std::to_string(target.to_uint()) + "\t" + std::to_string(target.to_long()) + "\t" +
                std::to_string(target.to_ulong()) + "\t" + std::to_string(target.to_int64()) + "\t" +
                std::to_string(target.to_uint64()) + "\t" + std::to_string(target.to_double()) + "\t" +
                std::to_string(target.long_low()) + "\t" + std::to_string(target.long_high()));
  emit_fields(family, "observe", "reductions",
              std::to_string(target.and_reduce()) + "\t" + std::to_string(target.nand_reduce()) + "\t" +
                std::to_string(target.or_reduce()) + "\t" + std::to_string(target.nor_reduce()) + "\t" +
                std::to_string(target.xor_reduce()) + "\t" + std::to_string(target.xnor_reduce()));
  emit_fields(family, "observe", "strings", target.to_string(SC_BIN) + "\t" + target.to_string(SC_HEX, true));
  std::ostringstream printed;
  target.print(printed);
  emit_fields(family, "observe", "print", printed.str());

  target = static_cast<Scalar>(0);
  target.set(3);
  emit_value(family, "bits", "set_one_arg", target);
  target.set(3, false);
  emit_value(family, "bits", "set_bool", target);
  target.set(4, true);
  emit_fields(family, "bits", "test", std::to_string(target.test(4)) + "\t" + std::to_string(target.test(3)));

  target = static_cast<Scalar>(0x5a35);
  bool xz_present = true;
  std::array<sc_digit, 2> ctrl{};
  std::array<sc_digit, 2> data{};
  const int packed_length = target.concat_length(&xz_present);
  const bool ctrl_present = target.concat_get_ctrl(ctrl.data(), 0);
  const bool data_present = target.concat_get_data(data.data(), 0);
  emit_fields(family, "concat", "pack",
              std::to_string(packed_length) + "\t" + std::to_string(xz_present) + "\t" + std::to_string(ctrl_present) +
                "\t" + std::to_string(ctrl[0]) + "\t" + std::to_string(data_present) + "\t" + std::to_string(data[0]) +
                "\t" + std::to_string(target.concat_get_uint64()));
  target.concat_set(static_cast<int64>(5), 3);
  emit_value(family, "concat", "set_int64", target);
  target = static_cast<Scalar>(0);
  target.concat_set(static_cast<uint64>(6), 3);
  emit_value(family, "concat", "set_uint64", target);
  target = static_cast<Scalar>(0);
  target.concat_set(signed_source, 4);
  emit_value(family, "concat", "set_signed", target);
  target = static_cast<Scalar>(0);
  target.concat_set(unsigned_source, 4);
  emit_value(family, "concat", "set_unsigned", target);

  target = static_cast<Scalar>(0x12345);
  const int index = 7;
  auto& mutable_subscript = target[index];
  auto& mutable_bit = target.bit(index);
  const auto& read_subscript =
    signed_value ? static_cast<const Base&>(target)[index] : static_cast<const Base&>(target)[index];
  const auto& read_bit =
    signed_value ? static_cast<const Base&>(target).bit(index) : static_cast<const Base&>(target).bit(index);
  emit_fields(family, "access", "bit",
              std::to_string(mutable_subscript.length()) + "\t" + std::to_string(mutable_bit.value()) + "\t" +
                std::to_string(read_subscript.length()) + "\t" + std::to_string(read_subscript.value()) + "\t" +
                std::to_string(read_bit.value()));
  const int mutable_call_length = target(index, 2).length();
  const int mutable_range_length = target.range(index, 2).length();
  const int read_call_length = static_cast<const Base&>(target)(index, 2).length();
  const int read_range_length = static_cast<const Base&>(target).range(index, 2).length();
  emit_fields(family, "access", "range",
              std::to_string(mutable_call_length) + "\t" + decimal(target) + "\t" +
                std::to_string(mutable_range_length) + "\t" + std::to_string(read_call_length) + "\t" +
                std::to_string(read_range_length));

  target = static_cast<Scalar>(0x5a35);
  std::istringstream input("-27");
  target.scan(input);
  emit_fields(family, "stream", "scan", decimal(target) + "\t" + std::to_string(input.rdstate()));
  const auto old_actions = throw_errors();
  std::istringstream invalid_input("invalid");
  bool scan_caught = false;
  try
  {
    target.scan(invalid_input);
  }
  catch (const sc_core::sc_report&)
  {
    scan_caught = true;
  }
  restore_errors(old_actions);
  emit_fields(family, "stream", "scan_invalid",
              decimal(target) + "\t" + std::to_string(invalid_input.rdstate()) + "\t" + std::to_string(scan_caught));

  exercise_widths<Base>(family, signed_value ? static_cast<Scalar>(-0x12345) : static_cast<Scalar>(0xfedc));

  const Base lifetime_copy = [&]()
  {
    sc_signed lifetime_source(48);
    lifetime_source = "-123456789";
    return Base(lifetime_source);
  }();
  emit_value(family, "lifetime", "source_released", lifetime_copy);
}
}  // namespace

/** @brief Emit native references for the signed and unsigned base families. */
int sc_main(int, char**)
{
  exercise_base<SignedBase>("sc_dt::sc_int_base", signed_scalar(), true);
  exercise_base<UnsignedBase>("sc_dt::sc_uint_base", unsigned_scalar(), false);
  std::cout << "SC_DT_BOUNDED_BASES_PUBLIC_OK\n";
  return 0;
}

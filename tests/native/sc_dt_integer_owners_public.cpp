/** @file sc_dt_integer_owners_public.cpp
 * @brief Native reference for arbitrary-width signed and unsigned integer APIs.
 * @details Each invocation uses independent native values. Range and bit views
 *          borrow their parents and are used only while those parents live.
 */
#include <algorithm>
#include <array>
#include <iostream>
#include <sstream>
#include <string>
#ifndef SC_INCLUDE_FX
#define SC_INCLUDE_FX
#endif
#include <systemc>

#include "sc_dt_integer_owners_probe.h"

namespace
{
using namespace sc_dt_integer_owners_probe;

template <class Base>
std::string decimal(const Base& value)
{
  return value.to_string(sc_dt::SC_DEC);
}

void emit(const char* family, const char* group, const char* label, const std::string& fields)
{
  std::cout << "OWN\t" << family << '\t' << group << '\t' << label << '\t' << fields << '\n';
}

template <class Base>
void emit_value(const char* family, const char* group, const char* label, const Base& value)
{
  emit(family, group, label, decimal(value) + "\t" + std::to_string(value.length()));
}

template <class Base, class Source>
void constructor_case(const char* family, const char* label, const Source& source)
{
  const Base value(source);
  emit_value(family, "construct", label, value);
}

template <class Base, class Source>
void assignment_case(const char* family, const char* label, Base& target, const Source& source)
{
  const Base& result = (target = source);
  emit(family, "assign", label, decimal(target) + "\t" + std::to_string(&result == &target));
}

template <class Base, class Source>
void compound_case(const char* family, const char* operation, const char* type, Base& target, const Source& source)
{
  const Base* result = nullptr;
  const std::string op(operation);
  if (op == "add")
    result = &(target += source);
  else if (op == "subtract")
    result = &(target -= source);
  else if (op == "multiply")
    result = &(target *= source);
  else if (op == "divide")
    result = &(target /= source);
  else if (op == "remainder")
    result = &(target %= source);
  else if (op == "and")
    result = &(target &= source);
  else if (op == "or")
    result = &(target |= source);
  else if (op == "xor")
    result = &(target ^= source);
  else if (op == "left_shift")
    result = &(target <<= source);
  else if (op == "right_shift")
    result = &(target >>= source);
  emit(family, "compound", (op + "_" + type).c_str(), decimal(target) + "\t" + std::to_string(result == &target));
}

template <class Base, class Source>
void compound_matrix(const char* family, const char* type, const Source& source)
{
  for (const char* operation :
       {"add", "subtract", "multiply", "divide", "remainder", "and", "or", "xor", "left_shift", "right_shift"})
  {
    Base target(257);
    target = 80;
    compound_case(family, operation, type, target, source);
  }
}

template <class Base>
void exercise_constructors(const char* family)
{
  Base width(32);
  emit_value(family, "construct", "width", width);
  Base copy(width);
  emit_value(family, "construct", "copy", copy);

  Signed signed_source(257);
  signed_source = "-123456789012345678901234567890123456789";
  Unsigned unsigned_source(257);
  unsigned_source = "987654321098765432109876543210987654321";
  SignedFixed signed_fixed(64);
  signed_fixed = static_cast<SignedScalar>(-0x123456789abLL);
  UnsignedFixed unsigned_fixed(64);
  unsigned_fixed = static_cast<UnsignedScalar>(0xfedcba9876543210ULL);
  const sc_dt::sc_bv<64> bit_vector("1010010110100101101001011010010110100101101001011010010110100101");
  const sc_dt::sc_lv<64> logic_vector("0110101001101010011010100110101001101010011010100110101001101010");
  Signed generic_signed(16);
  generic_signed = 0xa55a;
  Unsigned generic_unsigned(16);
  generic_unsigned = 0x35c3;
  const auto& generic_ref = (generic_signed, generic_unsigned);
  const sc_dt::sc_generic_base<sc_dt::sc_concatref>& generic = generic_ref;
  const auto& signed_range = const_signed_range(signed_source, 127, 64);
  const auto& unsigned_range = const_unsigned_range(unsigned_source, 127, 64);
  const auto& int_range = const_int_range(signed_fixed, 63, 16);
  const auto& uint_range = const_uint_range(unsigned_fixed, 63, 16);

  constructor_case<Base>(family, "signed", signed_source);
  constructor_case<Base>(family, "unsigned", unsigned_source);
  constructor_case<Base>(family, "signed_range", signed_range);
  constructor_case<Base>(family, "unsigned_range", unsigned_range);
  constructor_case<Base>(family, "int_range", int_range);
  constructor_case<Base>(family, "uint_range", uint_range);
  constructor_case<Base>(family, "bv", bit_vector);
  constructor_case<Base>(family, "lv", logic_vector);
  constructor_case<Base>(family, "generic_concat", generic);

  Base target(257);
  assignment_case(family, "signed", target, signed_source);
  assignment_case(family, "unsigned", target, unsigned_source);
  assignment_case(family, "signed_range", target, signed_range);
  assignment_case(family, "unsigned_range", target, unsigned_range);
  assignment_case(family, "sc_int_base", target, signed_fixed);
  assignment_case(family, "sc_uint_base", target, unsigned_fixed);
  assignment_case(family, "bv", target, bit_vector);
  assignment_case(family, "lv", target, logic_vector);
  assignment_case(family, "generic_concat", target, generic);
  assignment_case(family, "string", target, "-765432109876543210987654321");
  assignment_case(family, "int", target, static_cast<int>(-1234567));
  assignment_case(family, "unsigned_int", target, static_cast<unsigned int>(2345678U));
  assignment_case(family, "long", target, static_cast<long>(-123456789L));
  assignment_case(family, "unsigned_long", target, static_cast<unsigned long>(345678901UL));
  assignment_case(family, "int64", target, static_cast<sc_dt::int64>(signed_scalar()));
  assignment_case(family, "uint64", target, static_cast<sc_dt::uint64>(unsigned_scalar()));
  assignment_case(family, "double", target, -9876.75);

#ifdef SC_INCLUDE_FX
  const sc_dt::sc_fxval fx_value(23.75);
  const sc_dt::sc_fxval_fast fx_fast(31.5);
  const sc_dt::sc_fixed<24, 12> fx_number(41.75);
  const sc_dt::sc_fixed_fast<24, 12> fx_fast_number(57.25);
  assignment_case(family, "fxval", target, fx_value);
  assignment_case(family, "fxval_fast", target, fx_fast);
  assignment_case(family, "fxnum", target, fx_number);
  assignment_case(family, "fxnum_fast", target, fx_fast_number);
#endif

  const Base independent(signed_source);
  const std::string retained = decimal(independent);
  emit(family, "lifetime", "copied_owner", retained + "\t" + std::to_string(independent.length()));
}

template <class Base>
void exercise_compounds(const char* family)
{
  compound_matrix<Base>(family, "int", static_cast<int>(3));
  compound_matrix<Base>(family, "unsigned_int", static_cast<unsigned int>(3));
  compound_matrix<Base>(family, "long", static_cast<long>(3));
  compound_matrix<Base>(family, "unsigned_long", static_cast<unsigned long>(3));
  compound_matrix<Base>(family, "int64", static_cast<sc_dt::int64>(3));
  compound_matrix<Base>(family, "uint64", static_cast<sc_dt::uint64>(3));
  SignedFixed mutable_signed(32);
  mutable_signed = 3;
  UnsignedFixed mutable_unsigned(32);
  mutable_unsigned = 3;
  Signed mutable_signed_value(32);
  mutable_signed_value = 3;
  Unsigned mutable_unsigned_value(32);
  mutable_unsigned_value = 3;
  compound_matrix<Base>(family, "sc_int_base", mutable_signed);
  compound_matrix<Base>(family, "sc_uint_base", mutable_unsigned);
  compound_matrix<Base>(family, "sc_signed", mutable_signed_value);
  compound_matrix<Base>(family, "sc_unsigned", mutable_unsigned_value);

  Base value(257);
  value = 80;
  Base& prefix_up = ++value;
  emit(family, "increment", "prefix_up", decimal(value) + "\t" + std::to_string(&prefix_up == &value));
  const Base postfix_up = value++;
  emit(family, "increment", "postfix_up", decimal(postfix_up) + "\t" + decimal(value));
  Base& prefix_down = --value;
  emit(family, "increment", "prefix_down", decimal(value) + "\t" + std::to_string(&prefix_down == &value));
  const Base postfix_down = value--;
  emit(family, "increment", "postfix_down", decimal(postfix_down) + "\t" + decimal(value));
}

template <class Base, bool IsSigned>
void exercise_storage(const char* family)
{
  Base value(257);
  if constexpr (IsSigned)
  {
    value = static_cast<sc_dt::int64>(-0x123456789abLL);
  }
  else
  {
    value = static_cast<sc_dt::uint64>(0xfedcba9876543210ULL);
  }
  emit(family, "observe", "numeric",
       decimal(value) + "\t" + std::to_string(value.length()) + "\t" + std::to_string(value.sign()) + "\t" +
         std::to_string(value.iszero()) + "\t" + std::to_string(value.to_int()) + "\t" +
         std::to_string(value.to_uint()) + "\t" + std::to_string(value.to_long()) + "\t" +
         std::to_string(value.to_ulong()) + "\t" + std::to_string(value.to_int64()) + "\t" +
         std::to_string(value.to_uint64()) + "\t" + std::to_string(value.to_double()) + "\t" +
         std::to_string(value.to_int()) + "\t" + std::to_string(value.to_uint()));
  emit(family, "observe", "reductions",
       std::to_string(value.and_reduce()) + "\t" + std::to_string(value.nand_reduce()) + "\t" +
         std::to_string(value.or_reduce()) + "\t" + std::to_string(value.nor_reduce()) + "\t" +
         std::to_string(value.xor_reduce()) + "\t" + std::to_string(value.xnor_reduce()));
  emit(family, "observe", "text", value.to_string(sc_dt::SC_BIN) + "\t" + value.to_string(sc_dt::SC_HEX, true));
  std::ostringstream printed;
  value.print(printed);
  emit(family, "observe", "print", printed.str());
  std::ostringstream dumped;
  value.dump(dumped);
  emit(family, "observe", "dump", dumped.str());

  const auto digits = digit_count(value.length());
  std::vector<sc_dt::sc_digit> packed(static_cast<std::size_t>(digits));
  value.get_packed_rep(packed.data());
  emit(family, "storage", "packed", digit_text(packed.data(), digits));
  emit(family, "storage", "raw", digit_text(value.get_raw(), digits));
  std::fill(packed.begin(), packed.end(), 0);
  packed.front() = 0x5aU;
  value.set_packed_rep(packed.data());
  emit_value(family, "storage", "set_packed", value);

  value = 0;
  value.set(5);
  value.set(9, true);
  emit_value(family, "bits", "set", value);
  value.clear(5);
  value.invert(7);
  emit(family, "bits", "clear_invert", decimal(value) + "\t" + std::to_string(value.test(7)));
  value.reverse();
  emit_value(family, "bits", "reverse", value);

  value = 0x12345;
  value[7] = true;
  value.bit(8) = false;
  bool read_bit = false;
  if constexpr (IsSigned)
  {
    read_bit = const_signed_bit(value, 7).to_bool();
  }
  else
  {
    read_bit = const_unsigned_bit(value, 7).to_bool();
  }
  emit(family, "view", "bits", std::to_string(value.test(7)) + "\t" + std::to_string(read_bit));
  value.range(15, 8) = 0xa5;
  const int range_length = value.range(15, 8).length();
  emit(family, "view", "mutable_range", decimal(value) + "\t" + std::to_string(range_length));
  value(15, 8) = 0x3c;
  emit(family, "view", "mutable_call", decimal(value) + "\t" + std::to_string(value(15, 8).length()));
  if constexpr (IsSigned)
  {
    const auto& view = const_signed_range(value, 15, 8);
    emit(family, "view", "const_range", decimal(Base(view)) + "\t" + std::to_string(view.length()));
    const auto& call = const_signed_call(value, 15, 8);
    emit(family, "view", "const_call", decimal(Base(call)) + "\t" + std::to_string(call.length()));
  }
  else
  {
    const auto& view = const_unsigned_range(value, 15, 8);
    emit(family, "view", "const_range", decimal(Base(view)) + "\t" + std::to_string(view.length()));
    const auto& call = const_unsigned_call(value, 15, 8);
    emit(family, "view", "const_call", decimal(Base(call)) + "\t" + std::to_string(call.length()));
  }

  bool has_control = false;
  const int concat_bits = value.concat_length(&has_control);
  std::vector<sc_dt::sc_digit> control(static_cast<std::size_t>(digits));
  std::vector<sc_dt::sc_digit> data(static_cast<std::size_t>(digits));
  const bool control_present = value.concat_get_ctrl(control.data(), 0);
  const bool data_present = value.concat_get_data(data.data(), 0);
  emit(family, "concat", "get",
       std::to_string(concat_bits) + "\t" + std::to_string(has_control) + "\t" + std::to_string(control_present) +
         "\t" + digit_text(control.data(), digits) + "\t" + std::to_string(data_present) + "\t" +
         digit_text(data.data(), digits) + "\t" + std::to_string(value.concat_get_uint64()));
  value = 0;
  value.concat_set(static_cast<sc_dt::int64>(-5), 4);
  emit_value(family, "concat", "set_int64", value);
  value = 0;
  value.concat_set(static_cast<sc_dt::uint64>(11), 4);
  emit_value(family, "concat", "set_uint64", value);
  Signed signed_source(16);
  signed_source = -5;
  Unsigned unsigned_source(16);
  unsigned_source = 11;
  value = 0;
  value.concat_set(signed_source, 4);
  emit_value(family, "concat", "set_signed", value);
  value = 0;
  value.concat_set(unsigned_source, 4);
  emit_value(family, "concat", "set_unsigned", value);

  for (const int width : {1, 32, 137, 191, 257})
  {
    Base wide(width);
    if constexpr (IsSigned)
    {
      wide = static_cast<sc_dt::int64>(-0x123456789abLL);
    }
    else
    {
      wide = static_cast<sc_dt::uint64>(0xfedcba9876543210ULL);
    }
    emit_value(family, "width", std::to_string(width).c_str(), wide);
  }

  std::istringstream input("-12345");
  value.scan(input);
  emit(family, "stream", "scan", decimal(value) + "\t" + std::to_string(input.rdstate()));
}

template <class Base>
void exercise_errors(const char* family)
{
  Base value(32);
  value = 7;
  const auto previous = sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  for (const int index : {-1, 32})
  {
    std::string details;
    try
    {
      value.check_index(index);
    }
    catch (const sc_core::sc_report& report)
    {
      details = std::string(report.get_msg_type()) + "\t" + report.get_msg();
    }
    emit(family, "error", ("index_" + std::to_string(index)).c_str(), details);
    details.clear();
    try
    {
      value.invalid_index(index);
    }
    catch (const sc_core::sc_report& report)
    {
      details = std::string(report.get_msg_type()) + "\t" + report.get_msg();
    }
    emit(family, "error", ("invalid_index_" + std::to_string(index)).c_str(), details);
  }
  std::string details;
  try
  {
    value.check_range(32, 0);
  }
  catch (const sc_core::sc_report& report)
  {
    details = std::string(report.get_msg_type()) + "\t" + report.get_msg();
  }
  emit(family, "error", "range_32_0", details);
  details.clear();
  try
  {
    value.invalid_range(32, 0);
  }
  catch (const sc_core::sc_report& report)
  {
    details = std::string(report.get_msg_type()) + "\t" + report.get_msg();
  }
  emit(family, "error", "invalid_range_32_0", details);
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous);
  for (const int width : {0, -1})
  {
    bool caught = false;
    std::string report_type;
    std::string message;
    try
    {
      Base invalid(width);
      (void)invalid;
    }
    catch (const sc_core::sc_report& report)
    {
      caught = true;
      report_type = report.get_msg_type();
      message = report.get_msg();
    }
    emit(family, "error", ("width_" + std::to_string(width)).c_str(),
         std::to_string(caught) + "\t" + report_type + "\t" + message);
  }
}

}  // namespace

int sc_main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cerr << "usage: sc_dt_integer_owners_public.cpp <constructors|compounds|storage|errors>\n";
    return 2;
  }
  const std::string phase(argv[1]);
  if (phase == "constructors")
  {
    exercise_constructors<Signed>("sc_dt::sc_signed");
    exercise_constructors<Unsigned>("sc_dt::sc_unsigned");
  }
  else if (phase == "compounds")
  {
    exercise_compounds<Signed>("sc_dt::sc_signed");
    exercise_compounds<Unsigned>("sc_dt::sc_unsigned");
  }
  else if (phase == "storage")
  {
    exercise_storage<Signed, true>("sc_dt::sc_signed");
    exercise_storage<Unsigned, false>("sc_dt::sc_unsigned");
  }
  else if (phase == "errors")
  {
    exercise_errors<Signed>("sc_dt::sc_signed");
    exercise_errors<Unsigned>("sc_dt::sc_unsigned");
  }
  else
  {
    std::cerr << "unknown phase: " << phase << '\n';
    return 2;
  }
  std::cout << "SC_DT_INTEGER_OWNERS_PUBLIC_OK\n";
  return 0;
}

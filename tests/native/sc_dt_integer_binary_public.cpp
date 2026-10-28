/** @file Native reference expressions for integer and four-state binary operators.
 * @details Each phase constructs independent values. Concatenation views are
 *          used only while both integer parents remain alive; result values are
 *          rendered before source destruction. Calls are synchronous and hold
 *          no Python references or callbacks.
 */
#include <array>
#include <iostream>
#include <string>
#include <type_traits>

#include "sc_dt_integer_binary_probe.h"

namespace
{
template <class Value, class = void>
struct has_length : std::false_type
{
};

template <class Value>
struct has_length<Value, std::void_t<decltype(std::declval<const Value&>().length())>> : std::true_type
{
};

template <class Value>
auto render(const Value& value, int) -> decltype(value.to_string(sc_dt::SC_BIN), std::string())
{
  using Native = std::remove_cv_t<std::remove_reference_t<Value>>;
  const char* kind = "N";
  if constexpr (std::is_same_v<Native, sc_dt::sc_signed>)
    kind = "S";
  else if constexpr (std::is_same_v<Native, sc_dt::sc_unsigned>)
    kind = "U";
  else if constexpr (std::is_base_of_v<sc_dt::sc_bv_base, Native>)
    kind = "B";
  else if constexpr (std::is_base_of_v<sc_dt::sc_lv_base, Native>)
    kind = "V";
  std::string bits;
  if constexpr (std::is_base_of_v<sc_dt::sc_lv_base, Native> || std::is_base_of_v<sc_dt::sc_bv_base, Native>)
    bits = value.to_string();
  else
    bits = value.to_string(sc_dt::SC_BIN);
  std::string result = std::string(kind) + ":" + bits;
  if constexpr (has_length<Value>::value)
    result += ":" + std::to_string(value.length());
  else
    result += ":" + std::to_string(value.wl()) + ":" + std::to_string(value.iwl());
  return result;
}

template <class Value>
auto render(const Value& value, long) -> decltype(value.to_char(), std::string())
{
  return std::string(1, value.to_char());
}

template <class Value>
std::string render(const Value& value, ...)
{
  return std::to_string(value);
}

void emit(const char* label, const std::string& value)
{
  std::cout << "BIN\t" << label << '\t' << value << '\n';
}

template <class Value>
void emit(const char* label, const Value& value)
{
  emit(label, render(value, 0));
}

void integer_phase()
{
  sc_dt::sc_signed signed_left(19);
  signed_left = -12345;
  sc_dt::sc_signed signed_right(13);
  signed_right = 37;
  sc_dt::sc_unsigned unsigned_value(21);
  unsigned_value = 0x1a55a;
  sc_dt::sc_int_base fixed_signed(12);
  fixed_signed = -311;
  sc_dt::sc_uint_base fixed_unsigned(15);
  fixed_unsigned = 0x4a31;

  emit("remainder_signed_signed", signed_left % signed_right);
  emit("remainder_signed_int", signed_left % 29);
  emit("remainder_int_signed", 987 % signed_right);
  emit("remainder_unsigned_fixed", unsigned_value % fixed_unsigned);
  emit("and_signed_unsigned", signed_left & unsigned_value);
  emit("or_unsigned_signed", unsigned_value | signed_left);
  emit("xor_fixed_signed", fixed_signed ^ signed_right);
  emit("and_int_unsigned", 0x735a & unsigned_value);
  emit("or_unsigned_int", unsigned_value | 0x20011);
  emit("xor_int_signed", -901 ^ signed_left);

  sc_dt::sc_signed concat_high(9);
  concat_high = 0x12f;
  sc_dt::sc_unsigned concat_low(7);
  concat_low = 0x53;
  emit("remainder_integer_concat", (concat_high, concat_low) % signed_right);
  emit("and_integer_concat", (concat_high, concat_low) & 0x1ff);
  emit("or_concat_integer", 0x301 | (concat_high, concat_low));
  emit("xor_concat_unsigned", (concat_high, concat_low) ^ unsigned_value);

  sc_dt::sc_signed lifetime_left(11);
  lifetime_left = -77;
  sc_dt::sc_signed lifetime_right(9);
  lifetime_right = 6;
  const sc_dt::sc_signed owned_result(lifetime_left % lifetime_right);
  emit("lifetime_owned_result", owned_result);
}

void vector_phase()
{
  const sc_dt::sc_bv<8> bits("10110010");
  const sc_dt::sc_lv<8> logic("10ZX0101");
  emit("and_bv_lv", bits & logic);
  emit("or_lv_bv", logic | bits);
  emit("xor_bv_int", bits ^ 0x35);
  emit("or_int_bv", 0x0f | bits);
  emit("xor_bitrefs", bits[0] ^ logic[1]);

  const std::array<bool, 8> bool_bits{{true, false, true, true, false, false, true, false}};
  const std::array<char, 9> char_bits{{'1', '0', '1', '1', '0', '0', '1', '0', '\0'}};
  const std::array<sc_dt::sc_logic, 8> logic_bits{{
    sc_dt::sc_logic('1'),
    sc_dt::sc_logic('0'),
    sc_dt::sc_logic('Z'),
    sc_dt::sc_logic('X'),
    sc_dt::sc_logic('0'),
    sc_dt::sc_logic('1'),
    sc_dt::sc_logic('0'),
    sc_dt::sc_logic('1'),
  }};
  emit("and_bool_pointer_left", bool_bits.data() & bits);
  emit("and_bool_pointer_right", bits & bool_bits.data());
  emit("or_char_pointer_left", char_bits.data() | bits);
  emit("or_char_pointer_right", bits | char_bits.data());
  emit("xor_logic_pointer_left", logic_bits.data() ^ bits);
  emit("xor_logic_pointer_right", bits ^ logic_bits.data());

  const sc_dt::sc_lv_base owned_result = []
  {
    const sc_dt::sc_bv<8> source_bits("11001010");
    const sc_dt::sc_lv<8> source_logic("01ZX0110");
    return source_bits & source_logic;
  }();
  emit("lifetime_owned_vector", owned_result);

  const sc_dt::sc_logic logic_value('Z');
  emit("and_logic_bool", logic_value & true);
  emit("or_logic_char", logic_value | '0');
  const sc_dt::sc_bit bit_value(true);
  emit("xor_bit_int", bit_value ^ 0);
}

void matrix_phase(const std::string& operation, const std::string& family)
{
  for (unsigned index = 0; index < sc_dt_integer_binary_probe::native_case_count(family); ++index)
  {
    sc_dt_integer_binary_probe::Sources values;
    const auto result = sc_dt_integer_binary_probe::native_case(family, index, values);
    std::cout << "MATRIX\t" << operation << '\t' << index << '\t' << result << '\n';
  }
}
}  // namespace

int sc_main(int argc, char** argv)
{
  if (argc != 2) return 2;
  const std::string phase(argv[1]);
  if (phase == "integer")
    integer_phase();
  else if (phase == "vector")
    vector_phase();
  else if (phase == "mod")
    matrix_phase(phase, "sc_dt::operator%");
  else if (phase == "and")
    matrix_phase(phase, "sc_dt::operator&");
  else if (phase == "or")
    matrix_phase(phase, "sc_dt::operator|");
  else if (phase == "xor")
    matrix_phase(phase, "sc_dt::operator^");
  else if (phase == "errors")
  {
    const auto previous = sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
    try
    {
      sc_dt::sc_signed numerator(8);
      numerator = 7;
      sc_dt::sc_signed denominator(8);
      denominator = 0;
      const auto result = numerator % denominator;
      (void)result;
      std::cout << "BIN\tremainder_by_zero\tNO_REPORT\n";
    }
    catch (const sc_core::sc_report& report)
    {
      std::cout << "BIN\tremainder_by_zero\tCAUGHT:" << report.get_msg() << '\n';
    }
    sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, previous);
  }
  else
    return 2;
  std::cout << "BIN\tprobe_count\t" << sc_dt_integer_binary_probe::exact_overload_count << '\n';
  std::cout << "SC_DT_INTEGER_BINARY_PUBLIC_OK\n";
  return 0;
}

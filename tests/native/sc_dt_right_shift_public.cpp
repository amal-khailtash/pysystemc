/** @file Independent native reference for datatype shifts and extraction. */
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>
#include <type_traits>

#include "sc_dt_right_shift_probe.h"

namespace
{
using namespace sc_dt_right_shift_probe;

template <class Value, class Count>
void shift(const char* label, const Value& value, const Count& count)
{
  const std::string before = text(value);
  const auto result = value >> count;
  if (before != text(value)) throw std::runtime_error("right shift mutated its source");
  std::cout << "RS\t" << label << '\t' << text(result) << '\t' << length(result) << '\n';
}

template <class Value>
void extract(const char* label, Value& value, const char* token)
{
  const std::string before = text(value);
  std::istringstream stream(std::string(token) + " 99");
  std::istream& returned = stream >> value;
  const int good_state = static_cast<int>(stream.rdstate());
  const bool good = static_cast<bool>(stream);
  const bool same_stream = &returned == &stream;
  const std::string after = text(value);
  if (before == after) throw std::runtime_error("valid extraction did not change its target");
  stream.setstate(std::ios::failbit);
  std::cout << "SE\t" << label << '\t' << before << '\t' << after << '\t' << same_stream << '\t' << good << '\t'
            << good_state << '\t' << static_cast<bool>(stream) << '\t' << static_cast<int>(stream.rdstate()) << '\n';
}

template <class Value, class Parent>
void extract_parent(const char* label, Value& value, Parent& parent, const char* token)
{
  const std::string before = text(parent);
  extract(label, value, token);
  const std::string after = text(parent);
  if (before == after) throw std::runtime_error("proxy extraction did not mutate its parent");
  std::cout << "PAR\t" << label << '\t' << before << '\t' << after << '\n';
}

template <class Value, class Parent1, class Parent2>
void extract_two_parents(const char* label, Value& value, Parent1& left, Parent2& right, const char* token)
{
  const std::string before_left = text(left);
  const std::string before_right = text(right);
  extract(label, value, token);
  const std::string after_left = text(left);
  const std::string after_right = text(right);
  if (before_left == after_left || before_right == after_right)
    throw std::runtime_error("concatenation extraction did not mutate both parents");
  std::cout << "PAR\t" << label << "_left\t" << before_left << '\t' << after_left << '\n';
  std::cout << "PAR\t" << label << "_right\t" << before_right << '\t' << after_right << '\n';
}

template <int Width>
void width_shift()
{
  sc_dt::sc_bigint<Width> signed_value;
  sc_dt::sc_biguint<Width> unsigned_value;
  if constexpr (Width == 137)
  {
    signed_value = "-43556142965880123323311949751266331054023";
    unsigned_value = "43556142965880123323311949751266331089824";
  }
  else if constexpr (Width == 191)
  {
    signed_value = "-784637716923335095479473677900958302012794430558004301767";
    unsigned_value = "784637716923335095479473677900958302012794430558004337568";
  }
  else
  {
    signed_value = "-57896044618658097711785492504343953926634992332820282019728792003956564807623";
    unsigned_value = "57896044618658097711785492504343953926634992332820282019728792003956564843424";
  }
  shift((std::string("bigint_") + std::to_string(Width)).c_str(), signed_value, 3);
  shift((std::string("biguint_") + std::to_string(Width)).c_str(), unsigned_value, 3);
}

void numeric_shifts()
{
  sc_dt::sc_signed signed_value(64);
  sc_dt::sc_unsigned unsigned_value(64);
  signed_value = "-1234567890123";
  unsigned_value = "1234567890123";

  sc_dt::sc_signed signed_count(16);
  sc_dt::sc_unsigned unsigned_count(16);
  sc_dt::sc_int<16> int_count = 2;
  sc_dt::sc_uint<16> uint_count = 2;
  signed_count = 2;
  unsigned_count = 2;
  const int int_value = 2;
  const unsigned int unsigned_int_value = 2;
  const long long_value = 2;
  const unsigned long unsigned_long_value = 2;
  const sc_dt::int64 int64_value = 2;
  const sc_dt::uint64 uint64_value = 2;

  shift("signed_signed", signed_value, signed_count);
  shift("signed_unsigned", signed_value, unsigned_count);
  shift("signed_int64", signed_value, int64_value);
  shift("signed_uint64", signed_value, uint64_value);
  shift("signed_long", signed_value, long_value);
  shift("signed_ulong", signed_value, unsigned_long_value);
  shift("signed_int", signed_value, int_value);
  shift("signed_unsigned_int", signed_value, unsigned_int_value);
  shift("signed_sc_int", signed_value, int_count);
  shift("signed_sc_uint", signed_value, uint_count);
  shift("signed_zero", signed_value, 0);
  shift("signed_negative", signed_value, -1);
  shift("signed_large", signed_value, 257);

  shift("unsigned_signed", unsigned_value, signed_count);
  shift("unsigned_unsigned", unsigned_value, unsigned_count);
  shift("unsigned_int64", unsigned_value, int64_value);
  shift("unsigned_uint64", unsigned_value, uint64_value);
  shift("unsigned_long", unsigned_value, long_value);
  shift("unsigned_ulong", unsigned_value, unsigned_long_value);
  shift("unsigned_int", unsigned_value, int_value);
  shift("unsigned_unsigned_int", unsigned_value, unsigned_int_value);
  shift("unsigned_sc_int", unsigned_value, int_count);
  shift("unsigned_sc_uint", unsigned_value, uint_count);
  shift("unsigned_zero", unsigned_value, 0);
  shift("unsigned_negative", unsigned_value, -1);
  shift("unsigned_large", unsigned_value, 257);

  sc_dt::sc_uint<16> concat_left = 0x1234;
  sc_dt::sc_uint<16> concat_right = 0x5678;
  const sc_dt::sc_concatref& concat_value = sc_dt::concat(concat_left, concat_right);
  shift("concat_int", concat_value, int_value);
  shift("concat_unsigned_int", concat_value, unsigned_int_value);
  shift("concat_long", concat_value, long_value);
  shift("concat_unsigned_long", concat_value, unsigned_long_value);
  shift("concat_int64", concat_value, int64_value);
  shift("concat_uint64", concat_value, uint64_value);
  shift("concat_zero", concat_value, 0);
  shift("concat_large", concat_value, 257);

  sc_dt::sc_fxval fx_value(123.75);
  sc_dt::sc_fxval_fast fx_fast_value(123.75);
  sc_dt::sc_fixed<32, 16> fixed_value(123.75);
  sc_dt::sc_fixed_fast<32, 16> fixed_fast_value(123.75);
  shift("fxval_int", fx_value, int_value);
  shift("fxval_fast_int", fx_fast_value, int_value);
  shift("fxnum_int", fixed_value, int_value);
  shift("fxnum_fast_int", fixed_fast_value, int_value);
  shift("fxval_zero", fx_value, 0);
  shift("fxval_negative", fx_value, -1);
  shift("fxval_large", fx_value, 16);
  shift("fxval_fast_zero", fx_fast_value, 0);
  shift("fxval_fast_negative", fx_fast_value, -1);
  shift("fxval_fast_large", fx_fast_value, 16);
  shift("fxnum_zero", fixed_value, 0);
  shift("fxnum_negative", fixed_value, -1);
  shift("fxnum_large", fixed_value, 16);
  shift("fxnum_fast_zero", fixed_fast_value, 0);
  shift("fxnum_fast_negative", fixed_fast_value, -1);
  shift("fxnum_fast_large", fixed_fast_value, 16);
}

void failed_extractions()
{
  const auto old_actions = sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, sc_core::SC_THROW);
  sc_dt::sc_signed signed_value(16);
  signed_value = 37;
  std::istringstream conversion_input("");
  bool conversion_report = false;
  try
  {
    conversion_input >> signed_value;
  }
  catch (const sc_core::sc_report& report)
  {
    conversion_report = std::string(report.get_msg_type()) == sc_core::SC_ID_CONVERSION_FAILED_;
  }
  std::cout << "FAIL\tsigned_empty_conversion\t" << conversion_report << '\t' << text(signed_value) << '\t'
            << static_cast<int>(conversion_input.rdstate()) << '\t' << sc_core::SC_ID_CONVERSION_FAILED_ << '\n';

  sc_dt::sc_unsigned unsigned_value(16);
  unsigned_value = 37;
  std::istringstream masked_input("");
  masked_input.exceptions(std::ios::failbit);
  bool ios_exception = false;
  std::string ios_diagnostic;
  try
  {
    masked_input >> unsigned_value;
  }
  catch (const std::ios_base::failure& failure)
  {
    ios_exception = true;
    ios_diagnostic = failure.what();
  }
  std::cout << "FAIL\tunsigned_masked_empty\t" << ios_exception << '\t' << text(unsigned_value) << '\t'
            << static_cast<int>(masked_input.rdstate()) << '\t' << ios_diagnostic << '\n';

  sc_dt::sc_string_old old_string("old");
  std::istringstream string_input("");
  std::istream& string_return = string_input >> old_string;
  std::cout << "FAIL\tstring_empty\t" << (&string_return == &string_input) << '\t' << text(old_string) << '\t'
            << static_cast<int>(string_input.rdstate()) << '\t' << static_cast<bool>(string_input) << '\n';
  sc_core::sc_report_handler::set_actions(sc_core::SC_ERROR, old_actions);
}

void independent_lifetime()
{
  const auto result = []
  {
    sc_dt::sc_unsigned source(137);
    source = "123456789012345678901234567890123456789";
    return source >> 17;
  }();
  std::cout << "LIFE\t" << text(result) << '\t' << length(result) << '\n';
}

void stream_extractions()
{
  sc_dt::sc_signed signed_value(16);
  sc_dt::sc_unsigned unsigned_value(16);
  sc_dt::sc_int<16> int_value = 0;
  sc_dt::sc_uint<16> uint_value = 0;
  sc_dt::sc_bv<8> bit_vector("00000000");
  sc_dt::sc_bv<8> bit_vector_right("00000000");
  sc_dt::sc_lv<8> logic_vector("00000000");
  sc_dt::sc_bit bit('0');
  sc_dt::sc_logic logic('0');
  sc_dt::sc_fxval fx_value(0);
  sc_dt::sc_fxval_fast fx_fast_value(0);
  sc_dt::sc_fixed<16, 8> fixed_value(0);
  sc_dt::sc_fixed_fast<16, 8> fixed_fast_value(0);
  sc_dt::sc_string_old old_string("old");

  extract("signed", signed_value, "1");
  extract("unsigned", unsigned_value, "1");
  extract("sc_int_base", int_value, "1");
  extract("sc_uint_base", uint_value, "1");
  extract("sc_bv_proxy", bit_vector, "1");
  extract("sc_lv_proxy", logic_vector, "1");
  extract("sc_bit", bit, "1");
  extract("sc_logic", logic, "1");
  extract("sc_fxval", fx_value, "1");
  extract("sc_fxval_fast", fx_fast_value, "1");
  extract("sc_fxnum", fixed_value, "1");
  extract("sc_fxnum_fast", fixed_fast_value, "1");
  extract("sc_string_old", old_string, "one");

  auto signed_bit = signed_value[0];
  auto unsigned_bit = unsigned_value[0];
  auto int_bit = int_value[0];
  auto uint_bit = uint_value[0];
  auto vector_bit = bit_vector[0];
  auto logic_vector_bit = logic_vector[0];
  auto signed_range = signed_value(3, 0);
  auto unsigned_range = unsigned_value(3, 0);
  auto int_range = int_value(3, 0);
  auto uint_range = uint_value(3, 0);
  auto vector_range = bit_vector(3, 0);
  auto logic_vector_range = logic_vector(3, 0);
  auto fixed_bit = fixed_value.bit(0);
  auto fixed_fast_bit = fixed_fast_value.bit(0);
  auto fixed_range = fixed_value(3, 0);
  auto fixed_fast_range = fixed_fast_value(3, 0);
  extract_parent("sc_signed_bitref", signed_bit, signed_value, "0");
  extract_parent("sc_unsigned_bitref", unsigned_bit, unsigned_value, "0");
  extract_parent("sc_int_bitref", int_bit, int_value, "0");
  extract_parent("sc_uint_bitref", uint_bit, uint_value, "0");
  extract_parent("sc_bitref_template", vector_bit, bit_vector, "0");
  extract_parent("sc_logic_bitref", logic_vector_bit, logic_vector, "0");
  extract_parent("sc_signed_subref", signed_range, signed_value, "3");
  extract_parent("sc_unsigned_subref", unsigned_range, unsigned_value, "3");
  extract_parent("sc_int_subref", int_range, int_value, "3");
  extract_parent("sc_uint_subref", uint_range, uint_value, "3");
  extract_parent("sc_subref_template", vector_range, bit_vector, "0011");
  extract_parent("sc_logic_subref", logic_vector_range, logic_vector, "0011");
  extract_parent("sc_fxnum_bitref", fixed_bit, fixed_value, "1");
  extract_parent("sc_fxnum_fast_bitref", fixed_fast_bit, fixed_fast_value, "1");
  extract_parent("sc_fxnum_subref", fixed_range, fixed_value, "0010");
  extract_parent("sc_fxnum_fast_subref", fixed_fast_range, fixed_fast_value, "0010");

  sc_dt::sc_concref<sc_dt::sc_bv_base, sc_dt::sc_bv_base> concrete(bit_vector, bit_vector_right);
  extract_two_parents("sc_concref", concrete, bit_vector, bit_vector_right, "10101010");
  sc_dt::sc_concatref& concat_value = sc_dt::concat(unsigned_value, uint_value);
  extract_two_parents("sc_concatref", concat_value, unsigned_value, uint_value, "123");
}
}  // namespace

/** @brief Emit reference rows for every declared numeric and stream overload.
 * @return Zero after both independently initialized behavior matrices pass.
 */
int sc_main(int, char**)
{
  numeric_shifts();
  width_shift<137>();
  width_shift<191>();
  width_shift<257>();
  stream_extractions();
  failed_extractions();
  independent_lifetime();
  std::cout << "SC_DT_RIGHT_SHIFT_PUBLIC_OK\n";
  return 0;
}

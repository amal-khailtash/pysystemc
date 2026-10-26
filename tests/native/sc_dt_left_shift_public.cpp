/** @file Independent native reference for public sc_dt left shifts and streams. */
#include <iostream>
#include <systemc>

#include "sc_dt_fixed_bitrefs_probe.h"
#include "sc_dt_fixed_subrefs_probe.h"
#include "sc_dt_integer_bitrefs_probe.h"
#include "sc_dt_integer_subrefs_probe.h"
#include "sc_dt_left_shift_probe.h"

int sc_main(int, char**)
{
  using sc_dt_left_shift_probe::emit_shift;
  using sc_dt_left_shift_probe::emit_stream;
  using sc_dt_left_shift_probe::fixed_value;

  sc_dt::sc_unsigned unsigned_value(257);
  unsigned_value = "0x123456789abcdef001122334455667788";
  sc_dt::sc_signed signed_value(191);
  signed_value = "0x123456789abcdef123456789abcdef";
  signed_value = -signed_value;
  sc_dt::sc_unsigned unsigned_count(137);
  unsigned_count = 3;
  sc_dt::sc_signed signed_count(137);
  signed_count = 2;
  sc_dt::sc_int<64> signed_base_count(5);
  sc_dt::sc_uint<64> unsigned_base_count(6);
  const sc_dt::sc_int_base& int_base_count = signed_base_count;
  const sc_dt::sc_uint_base& uint_base_count = unsigned_base_count;
  const auto int64_count = static_cast<sc_dt::int64>(4);
  const auto uint64_count = static_cast<sc_dt::uint64>(5);
  const auto long_count = static_cast<long>(6);
  const auto ulong_count = static_cast<unsigned long>(7);
  const auto int_count = 2;
  const auto uint_count = static_cast<unsigned int>(3);

  const sc_dt::sc_concatref& concatenated = sc_dt_left_shift_probe::concat(unsigned_value, signed_value);
  emit_shift(1, unsigned_value << int_base_count);
  emit_shift(2, unsigned_value << unsigned_count);
  emit_shift(5, unsigned_value << int64_count);
  emit_shift(9, unsigned_value << ulong_count);
  emit_shift(11, signed_value << ulong_count);
  emit_shift(17, concatenated << ulong_count);
  emit_shift(20, signed_value << uint_count);
  emit_shift(26, signed_value << int64_count);
  emit_shift(29, signed_value << signed_count);
  emit_shift(30, signed_value << unsigned_count);
  emit_shift(31, unsigned_value << signed_count);
  emit_shift(35, signed_value << long_count);
  emit_shift(36, concatenated << long_count);
  emit_shift(37, concatenated << int_count);
  emit_shift(38, concatenated << int64_count);
  emit_shift(39, concatenated << uint64_count);
  emit_shift(41, signed_value << uint64_count);
  emit_shift(43, unsigned_value << signed_count);
  emit_shift(45, signed_value << unsigned_count);
  emit_shift(46, unsigned_value << uint_base_count);
  emit_shift(50, signed_value << int_base_count);
  emit_shift(52, unsigned_value << long_count);
  emit_shift(55, signed_value << int_count);

  sc_dt::sc_fxval fx_value(3.25);
  const auto fixed_shift = fx_value << 3;
  emit_shift(6, fixed_shift);
  emit_shift(56, fixed_value<sc_dt::sc_fix>(-3.25) << 2);
  emit_shift(58, sc_dt::sc_fxval_fast(2.5) << 4);
  emit_shift(59, signed_value << uint_base_count);
  emit_shift(61, unsigned_value << uint64_count);
  emit_shift(63, unsigned_value << uint_count);
  emit_shift(64, fixed_value<sc_dt::sc_fix_fast>(1.75) << 3);
  emit_shift(66, concatenated << uint_count);
  emit_shift(67, unsigned_value << int_count);

  // The remaining rows are exact ostream insertions, including every proxy.
  sc_dt::sc_int<32> int_value(-29);
  sc_dt::sc_uint<32> uint_value(0xabcdefU);
  sc_dt::sc_signed_bitref_r const_signed_bit = sc_dt_integer_bitrefs_probe::const_signed_view(signed_value, 3);
  sc_dt::sc_unsigned_bitref_r const_unsigned_bit = sc_dt_integer_bitrefs_probe::const_unsigned_view(unsigned_value, 4);
  auto fx_owner = fixed_value<sc_dt::sc_fix>(-7.5);
  auto fx_fast_owner = fixed_value<sc_dt::sc_fix_fast>(3.75);
  sc_dt::sc_fxnum_fast_bitref_r const_fast_fixed_bit = sc_dt_left_shift_probe::fixed_fast_bit(fx_fast_owner, 3);
  sc_dt::sc_fxnum_bitref_r const_fixed_bit = sc_dt_left_shift_probe::fixed_bit(fx_owner, 3);
  sc_dt::sc_fxnum_subref const_fixed_range = sc_dt_left_shift_probe::fixed_range(fx_owner, 8, 1);
  sc_dt::sc_fxnum_fast_subref_r const_fast_fixed_range = sc_dt_left_shift_probe::fixed_fast_range(fx_fast_owner, 8, 1);
  sc_dt::sc_int_subref_r const_int_range = sc_dt_integer_subrefs_probe::const_int_range(int_value, 7, 2);
  sc_dt::sc_uint_subref_r const_uint_range = sc_dt_integer_subrefs_probe::const_uint_range(uint_value, 7, 2);
  sc_dt::sc_signed_subref_r const_signed_range = sc_dt_integer_subrefs_probe::const_signed_range(signed_value, 8, 2);
  sc_dt::sc_unsigned_subref_r const_unsigned_range =
    sc_dt_integer_subrefs_probe::const_unsigned_range(unsigned_value, 8, 2);
  sc_dt::sc_length_param length(9);
  sc_dt::sc_fxtype_params fx_params(19, 8, sc_dt::SC_RND, sc_dt::SC_SAT, 0);
  sc_dt::sc_fxcast_switch cast_switch(sc_dt::SC_ON);
  sc_dt::sc_string_old legacy_string("old-string");
  sc_dt::sc_bit bit(true);
  sc_dt::sc_logic logic(sc_dt::SC_LOGIC_Z);
  sc_dt::sc_bv<12> proxy_bits("101010110011");

  emit_stream(3, sc_dt::SC_WRAP);
  emit_stream(4, sc_dt_integer_bitrefs_probe::const_uint_view(uint_value, 4));
  emit_stream(7, static_cast<const sc_dt::sc_uint_base&>(uint_value));
  emit_stream(10, static_cast<const sc_dt::sc_fxnum_fast&>(fx_fast_owner));
  emit_stream(12, const_int_range);
  emit_stream(13, const_signed_bit);
  emit_stream(14, const_fast_fixed_bit);
  emit_stream(15, sc_dt_integer_bitrefs_probe::const_int_view(int_value, 2));
  emit_stream(16, const_uint_range);
  emit_stream(18, const_fixed_bit);
  emit_stream(19, length);
  emit_stream(21, const_unsigned_bit);
  emit_stream(22, fx_value);
  emit_stream(23, sc_dt::SC_DEC);
  emit_stream(24, sc_dt::SC_RND);
  emit_stream(25, bit);
  emit_stream(27, const_fast_fixed_range);
  emit_stream(28, sc_dt::SC_F);
  emit_stream(32, static_cast<const sc_dt::sc_fxnum&>(fx_owner));
  emit_stream(33, sc_dt::SC_TC_);
  emit_stream(34, sc_dt::sc_fxval_fast(6.5));
  emit_stream(40, cast_switch);
  emit_stream(42, const_signed_range);
  emit_stream(44, static_cast<const sc_dt::sc_int_base&>(int_value));
  sc_dt_left_shift_probe::emit_proxy_stream(8, static_cast<const sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(proxy_bits));
  sc_dt_left_shift_probe::emit_proxy_stream(47, static_cast<const sc_dt::sc_proxy<sc_dt::sc_bv_base>&>(proxy_bits));
  emit_stream(48, unsigned_value);
  emit_stream(49, concatenated);
  emit_stream(51, logic);
  emit_stream(53, const_unsigned_range);
  emit_stream(54, fx_params);
  emit_stream(57, legacy_string);
  emit_stream(60, sc_dt::SC_OFF);
  emit_stream(62, signed_value);
  emit_stream(65, const_fixed_range);

  const auto make_logic_pattern = [](std::size_t width, const std::string& digits)
  {
    std::string result;
    result.reserve(width);
    for (std::size_t index = 0; index < width; ++index) result.push_back(digits[index % digits.size()]);
    return result;
  };
  const auto lv137_text = make_logic_pattern(137, "10XZ");
  const auto lv191_text = make_logic_pattern(191, "ZX01");
  sc_dt::sc_lv<137> lv137(lv137_text.c_str());
  sc_dt::sc_lv<191> lv191(lv191_text.c_str());
  emit_stream(910, lv137);
  emit_stream(911, lv191);
  auto nested_parent = lv137.range(120, 80);
  auto nested_proxy = nested_parent.range(30, 5);
  auto vector_range = lv191.range(150, 100);
  emit_stream(912, nested_proxy);
  emit_stream(913, vector_range);

  // SystemC integer shifts preserve values for negative counts and grow by the
  // positive count; these extra observations cover arbitrary runtime widths.
  emit_shift(901, unsigned_value << -3);
  emit_shift(902, unsigned_value << 257);
  emit_shift(903, signed_value << -4);
  emit_shift(904, signed_value << 257);

  sc_dt::sc_unsigned width137_value(137);
  width137_value = "0x123456789abcdef00123456789abcdef";
  emit_shift(905, width137_value << 3);

  sc_dt::sc_unsigned lifetime_source(137);
  lifetime_source = 13;
  auto lifetime_result = lifetime_source << 4;
  lifetime_source = 0;
  std::cout << "LSM\t900\tL\t" << lifetime_result.length() << '\t' << lifetime_result.to_string(sc_dt::SC_DEC) << '\n';

  sc_dt::sc_signed flags_value(32);
  flags_value = 255;
  std::ostringstream flags_output;
  flags_output.setf(std::ios_base::hex, std::ios_base::basefield);
  auto& flags_return = flags_output << flags_value;
  std::cout << "LSM\t906\tF\t" << flags_output.str() << '\t' << (&flags_return == &flags_output) << '\n';

  std::ostringstream chained_output;
  sc_dt::sc_bit chained_bit(true);
  sc_dt::sc_string_old chained_string("tail");
  auto& first_return = chained_output << chained_bit;
  auto& second_return = first_return << chained_string;
  std::cout << "LSM\t907\tC\t" << chained_output.str() << '\t'
            << ((&first_return == &chained_output) && (&second_return == &chained_output)) << '\n';

  std::ostringstream masked_output;
  masked_output.setstate(std::ios_base::badbit);
  bool mask_setup_threw = false;
  try
  {
    masked_output.exceptions(std::ios_base::badbit);
  }
  catch (const std::ios_base::failure&)
  {
    mask_setup_threw = true;
  }
  bool insertion_threw = false;
  try
  {
    masked_output << chained_bit;
  }
  catch (const std::ios_base::failure&)
  {
    insertion_threw = true;
  }
  std::cout << "LSM\t908\tE\t" << mask_setup_threw << '\t' << insertion_threw << '\n';

  auto zero_count_result = unsigned_value << 0;
  std::cout << "LSM\t909\tZ\t" << zero_count_result.length() << '\t' << zero_count_result.to_string(sc_dt::SC_DEC)
            << '\n';

  std::cout << "SC_DT_LEFT_SHIFT_PUBLIC_OK\n";
  return 0;
}

/** @file sc_dt_vector_algorithms_public.cpp
 * @brief Independent native reference for public sc_dt vector algorithms.
 * @details The program owns all vectors and fixed-point values. Proxy and
 * concatenation references are consumed while their parent storage is alive;
 * no callback or scheduler state is retained.
 */
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <systemc>

#include "sc_dt_vector_algorithms_probe.h"

namespace
{

/** @brief Build the deterministic input spelling used by both test models. */
std::string make_pattern(int width, const std::string& alphabet, int offset)
{
  std::string result;
  result.reserve(static_cast<std::size_t>(width));
  for (int index = 0; index < width; ++index)
    result.push_back(alphabet[static_cast<std::size_t>((index * 7 + offset) % static_cast<int>(alphabet.size()))]);
  return result;
}

/** @brief Render a native vector algorithm result without changing its bits. */
template <typename Value>
std::string render_bits(const Value& value)
{
  std::ostringstream stream;
  stream << value;
  return stream.str();
}

/** @brief Emit one native operator result with exact operand labels and width. */
template <typename Value>
void emit_operator_result(const char* left_kind, const char* right_kind, const char* operation, const char* direction,
                          const Value& value)
{
  std::cout << "SCA\toperator\t" << left_kind << '\t' << right_kind << '\t' << operation << '\t' << direction << '\t'
            << value.length() << '\t' << render_bits(value) << '\n';
}

/** @brief Exercise native vector operations in both operand orders. */
template <typename Left, typename Right>
void emit_vector_operator(const char* left_kind, const char* right_kind, const char* operation, const Left& left,
                          const Right& right)
{
  if (std::string(operation) == "and")
  {
    const auto normal = left & right;
    const auto reflected = right & left;
    emit_operator_result(left_kind, right_kind, operation, "normal", normal);
    emit_operator_result(left_kind, right_kind, operation, "reflected", reflected);
  }
  else if (std::string(operation) == "or")
  {
    const auto normal = left | right;
    const auto reflected = right | left;
    emit_operator_result(left_kind, right_kind, operation, "normal", normal);
    emit_operator_result(left_kind, right_kind, operation, "reflected", reflected);
  }
  else
  {
    const auto normal = left ^ right;
    const auto reflected = right ^ left;
    emit_operator_result(left_kind, right_kind, operation, "normal", normal);
    emit_operator_result(left_kind, right_kind, operation, "reflected", reflected);
  }
}

/** @brief Exercise native bool and int operators with vectors on either side. */
template <typename Vector, typename Scalar>
void emit_scalar_operator(const char* vector_kind, const char* scalar_kind, const char* operation, const Vector& vector,
                          Scalar scalar)
{
  if (std::string(operation) == "and")
  {
    const auto normal = vector & scalar;
    const auto reflected = scalar & vector;
    emit_operator_result(vector_kind, scalar_kind, operation, "normal", normal);
    emit_operator_result(scalar_kind, vector_kind, operation, "reflected", reflected);
  }
  else if (std::string(operation) == "or")
  {
    const auto normal = vector | scalar;
    const auto reflected = scalar | vector;
    emit_operator_result(vector_kind, scalar_kind, operation, "normal", normal);
    emit_operator_result(scalar_kind, vector_kind, operation, "reflected", reflected);
  }
  else
  {
    const auto normal = vector ^ scalar;
    const auto reflected = scalar ^ vector;
    emit_operator_result(vector_kind, scalar_kind, operation, "normal", normal);
    emit_operator_result(scalar_kind, vector_kind, operation, "reflected", reflected);
  }
}

/** @brief Exercise native bit-string vector operators with vectors on either side. */
template <typename Vector>
void emit_string_operator(const char* vector_kind, const char* operation, const Vector& vector, const std::string& bits)
{
  if (std::string(operation) == "and")
  {
    const auto normal = vector & bits.c_str();
    const auto reflected = bits.c_str() & vector;
    emit_operator_result(vector_kind, "string", operation, "normal", normal);
    emit_operator_result("string", vector_kind, operation, "reflected", reflected);
  }
  else if (std::string(operation) == "or")
  {
    const auto normal = vector | bits.c_str();
    const auto reflected = bits.c_str() | vector;
    emit_operator_result(vector_kind, "string", operation, "normal", normal);
    emit_operator_result("string", vector_kind, operation, "reflected", reflected);
  }
  else
  {
    const auto normal = vector ^ bits.c_str();
    const auto reflected = bits.c_str() ^ vector;
    emit_operator_result(vector_kind, "string", operation, "normal", normal);
    emit_operator_result("string", vector_kind, operation, "reflected", reflected);
  }
}

/** @brief Emit the arbitrary-width vector type-pair operator matrix. */
void emit_vector_operator_matrix(const sc_dt::sc_bv<137>& bv, const sc_dt::sc_bv<137>& bv_second,
                                 const sc_dt::sc_lv<191>& lv, const sc_dt::sc_lv<191>& lv_second)
{
  for (const char* operation : {"and", "or", "xor"})
  {
    emit_vector_operator("bv137", "bv137", operation, bv, bv_second);
    emit_vector_operator("lv191", "lv191", operation, lv, lv_second);
    emit_vector_operator("bv137", "lv191", operation, bv, lv);
    emit_vector_operator("lv191", "bv137", operation, lv, bv);
  }
}

/** @brief Emit the native bool/int operator matrix for each vector category. */
template <typename Vector>
void emit_scalar_operator_matrix(const char* vector_kind, const Vector& vector, const std::string& string_operand)
{
  for (const char* operation : {"and", "or", "xor"})
  {
    emit_scalar_operator(vector_kind, "bool", operation, vector, true);
    emit_scalar_operator(vector_kind, "int", operation, vector, 5);
    emit_string_operator(vector_kind, operation, vector, string_operand);
  }
}

/** @brief Print a free rotation or reversal result and verify source stability. */
template <typename Proxy>
void emit_vector_algorithms(const char* kind, const Proxy& source)
{
  const std::string original = render_bits(source);
  const int width = source.length();
  const auto left = sc_dt::lrotate(source, 23);
  std::cout << "SCA\trotate\t" << kind << "\tlrotate\t" << render_bits(left) << '\n';
  const auto right = sc_dt::rrotate(source, width + 19);
  std::cout << "SCA\trotate\t" << kind << "\trrotate\t" << render_bits(right) << '\n';
  const auto reversed = sc_dt::reverse(source);
  std::cout << "SCA\trotate\t" << kind << "\treverse\t" << render_bits(reversed) << '\n';
  if (render_bits(source) != original) throw std::runtime_error("free vector algorithm mutated its source");
}

/** @brief Render the bool and four-state results of a reduction overload. */
void emit_reduction_value(const char* kind, const char* operation, bool value)
{
  std::cout << "SCA\treduce\t" << kind << '\t' << operation << '\t' << int(value) << '\n';
}

/** @brief Render the four-state result returned for an sc_lv proxy. */
void emit_reduction_value(const char* kind, const char* operation, sc_dt::sc_logic_value_t value)
{
  std::cout << "SCA\treduce\t" << kind << '\t' << operation << '\t' << sc_dt::sc_logic(value).to_char() << '\n';
}

/** @brief Exercise all six functional reductions on one exact argument type. */
template <typename Value>
void emit_reductions(const char* kind, const Value& value)
{
  emit_reduction_value(kind, "and_reduce", sc_dt::and_reduce(value));
  emit_reduction_value(kind, "nand_reduce", sc_dt::nand_reduce(value));
  emit_reduction_value(kind, "nor_reduce", sc_dt::nor_reduce(value));
  emit_reduction_value(kind, "or_reduce", sc_dt::or_reduce(value));
  emit_reduction_value(kind, "xnor_reduce", sc_dt::xnor_reduce(value));
  emit_reduction_value(kind, "xor_reduce", sc_dt::xor_reduce(value));
}

/** @brief Print a fixed-point result with a stable decimal representation. */
template <typename Value>
std::string render_fixed(const Value& value)
{
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(4) << static_cast<double>(value);
  return stream.str();
}

/** @brief Print the result of one native fixed-point shift destination. */
template <typename Destination>
void emit_shift(const char* kind, const char* operation, const Destination& value)
{
  std::cout << "SCA\tshift\t" << kind << '\t' << operation << '\t' << render_fixed(value) << '\n';
}

/** @brief Exercise both fixed-point shift overloads for one native signature. */
template <typename Destination, typename Source>
void emit_shifts(const char* kind, Destination& destination, const Source& source)
{
  sc_dt::lshift(destination, source, 2);
  emit_shift(kind, "lshift", destination);
  sc_dt::rshift(destination, source, 2);
  emit_shift(kind, "rshift", destination);
}

/** @brief Print the two native optional comparison function results. */
void emit_comparison(const char* operation, const char* left, const char* right, bool value)
{
  std::cout << "SCA\tcompare\t" << operation << '\t' << left << '\t' << right << '\t' << int(value) << '\n';
}

/** @brief Exercise all seven equal and not_equal input signatures. */
void emit_comparisons()
{
  for (int left_value = 0; left_value < 2; ++left_value)
  {
    const sc_dt::sc_bit left(left_value != 0);
    for (int right_value = 0; right_value < 2; ++right_value)
    {
      const sc_dt::sc_bit right(right_value != 0);
      const bool boolean = right_value != 0;
      const char character = right_value == 0 ? '0' : '1';
      const int integer = right_value;
      emit_comparison("equal", "bit", "bit", sc_dt::equal(left, right));
      emit_comparison("equal", "bit", "bool", sc_dt::equal(left, boolean));
      emit_comparison("equal", "bit", "char", sc_dt::equal(left, character));
      emit_comparison("equal", "bit", "int", sc_dt::equal(left, integer));
      emit_comparison("equal", "bool", "bit", sc_dt::equal(boolean, left));
      emit_comparison("equal", "char", "bit", sc_dt::equal(character, left));
      emit_comparison("equal", "int", "bit", sc_dt::equal(integer, left));
      emit_comparison("not_equal", "bit", "bit", sc_dt::not_equal(left, right));
      emit_comparison("not_equal", "bit", "bool", sc_dt::not_equal(left, boolean));
      emit_comparison("not_equal", "bit", "char", sc_dt::not_equal(left, character));
      emit_comparison("not_equal", "bit", "int", sc_dt::not_equal(left, integer));
      emit_comparison("not_equal", "bool", "bit", sc_dt::not_equal(boolean, left));
      emit_comparison("not_equal", "char", "bit", sc_dt::not_equal(character, left));
      emit_comparison("not_equal", "int", "bit", sc_dt::not_equal(integer, left));
    }
  }
}

/** @brief Exercise return-value and output-reference b_not signatures. */
void emit_b_not()
{
  const sc_dt::sc_bit bit_source('0');
  const sc_dt::sc_bit bit_result = sc_dt::b_not(bit_source);
  std::cout << "SCA\tb_not\tbit_return\t" << bit_result.to_char() << '\n';
  sc_dt::sc_bit bit_output('1');
  sc_dt::b_not(bit_output, bit_source);
  std::cout << "SCA\tb_not\tbit_output\t" << bit_output.to_char() << '\n';

  const sc_dt::sc_fix fix_source(3.25, 8, 4);
  sc_dt::sc_fix fix_output(0, 8, 4);
  sc_dt::b_not(fix_output, fix_source);
  std::cout << "SCA\tb_not\tfix\t" << render_fixed(fix_output) << '\n';

  const sc_dt::sc_ufix ufix_source(3.25, 8, 4);
  sc_dt::sc_ufix ufix_output(0, 8, 4);
  sc_dt::b_not(ufix_output, ufix_source);
  std::cout << "SCA\tb_not\tufix\t" << render_fixed(ufix_output) << '\n';

  const sc_dt::sc_fix_fast fix_fast_source(3.25, 8, 4);
  sc_dt::sc_fix_fast fix_fast_output(0, 8, 4);
  sc_dt::b_not(fix_fast_output, fix_fast_source);
  std::cout << "SCA\tb_not\tfix_fast\t" << render_fixed(fix_fast_output) << '\n';

  const sc_dt::sc_ufix_fast ufix_fast_source(3.25, 8, 4);
  sc_dt::sc_ufix_fast ufix_fast_output(0, 8, 4);
  sc_dt::b_not(ufix_fast_output, ufix_fast_source);
  std::cout << "SCA\tb_not\tufix_fast\t" << render_fixed(ufix_fast_output) << '\n';
}

/** @brief Print deterministic behavior for every assigned public family. */
int run()
{
  const std::string bv_pattern = make_pattern(137, "01", 3);
  const std::string lv_pattern = make_pattern(191, "01ZX", 1);
  const std::string lv_second_pattern = make_pattern(191, "01ZX", 2);
  const sc_dt::sc_bv<137> bv(bv_pattern.c_str());
  const sc_dt::sc_bv<137> bv_second(make_pattern(137, "01", 4).c_str());
  const sc_dt::sc_lv<191> lv(lv_pattern.c_str());
  const sc_dt::sc_lv<191> lv_second(lv_second_pattern.c_str());
  emit_vector_algorithms("bv137", bv);
  emit_vector_algorithms("lv191", lv);
  const auto lv_subref = lv.range(155, 31);
  emit_vector_algorithms("lv_subref", lv_subref);
  const sc_dt::sc_lv_base lv_expression = lv ^ lv_second;
  std::cout << "SCA\texpression\tlv_xor\t" << render_bits(lv_expression) << '\n';
  emit_vector_algorithms("lv_expression", lv_expression);

  emit_reductions("bv137", bv);
  emit_reductions("lv191", lv);
  const sc_dt::sc_uint<37> uint_value(0x5a35U);
  const sc_dt::sc_int<41> int_value(-0x13579);
  emit_reductions("uint37", uint_value);
  emit_reductions("int41", int_value);
  const sc_dt::sc_uint<64> uint64(std::stoull(make_pattern(64, "01", 2), nullptr, 2));
  const sc_dt::sc_int<64> int64(std::stoll(make_pattern(63, "01", 1), nullptr, 2));
  const auto uint_read_ref = sc_dt_vector_algorithms_probe::uint_subref(uint64, 61, 17);
  const auto int_read_ref = sc_dt_vector_algorithms_probe::int_subref(int64, 60, 12);
  emit_reductions("uint_subref", uint_read_ref);
  emit_reductions("int_subref", int_read_ref);
  const auto& concat = sc_dt_vector_algorithms_probe::concat(uint_value, int_value);
  emit_reductions("concat", concat);

  emit_b_not();

  const sc_dt::sc_fxval fx_value(3.25);
  sc_dt::sc_fxval fx_out(0);
  emit_shifts("fxval", fx_out, fx_value);
  const sc_dt::sc_fxval_fast fx_fast_value(3.25);
  sc_dt::sc_fxval_fast fx_fast_out(0);
  emit_shifts("fxval_fast", fx_fast_out, fx_fast_value);
  const sc_dt::sc_fix fix_value(3.25, 8, 4);
  sc_dt::sc_fxval fix_val_out(0);
  emit_shifts("fxval_from_fxnum", fix_val_out, fix_value);
  sc_dt::sc_fix fix_out(0, 8, 4);
  emit_shifts("fxnum", fix_out, fix_value);
  const sc_dt::sc_fix_fast fix_fast_value(3.25, 8, 4);
  sc_dt::sc_fxval_fast fix_fast_val_out(0);
  emit_shifts("fxval_fast_from_fxnum_fast", fix_fast_val_out, fix_fast_value);
  sc_dt::sc_fix_fast fix_fast_out(0, 8, 4);
  emit_shifts("fxnum_fast", fix_fast_out, fix_fast_value);

  emit_comparisons();
  emit_vector_operator_matrix(bv, bv_second, lv, lv_second);
  emit_scalar_operator_matrix("bv137", bv, make_pattern(137, "01", 4));
  emit_scalar_operator_matrix("lv191", lv, make_pattern(191, "01ZX", 3));
  return 0;
}

}  // namespace

/** @brief Run the independent SystemC reference without kernel simulation. */
int sc_main(int, char**)
{
  return run();
}

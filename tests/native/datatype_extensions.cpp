/**
 * @file
 * @brief Native reference for writable concatenation and dynamic/fixed arithmetic.
 * @details Stack-owned values survive all assertions; no callbacks or Python/GIL state is used.
 */
#include <systemc>

int sc_main(int, char**)
{
  using namespace sc_dt;
  sc_uint<4> high = 10;
  sc_uint<4> low = 3;
  sc_assert(concat(high, low).to_uint64() == 0xA3);
  concat(high, low) = 0x5C;
  sc_assert(high == 5 && low == 12);
  for (int width : {7, 73, 193})
  {
    sc_unsigned value(width);
    value = ("0b" + std::string(width, '1')).c_str();
    sc_assert(value.length() == width && value.range(6, 0).to_uint() == 127);
    sc_signed signed_value(width);
    signed_value = -17;
    sc_assert(signed_value.to_int64() == -17);
  }
  sc_fixed<16, 8> left = 1.5;
  sc_fixed<16, 8> right = 2.25;
  sc_assert((left + right).to_double() == 3.75);
  sc_assert((left * right).to_double() == 3.375);
  sc_fixed_fast<16, 8> fast_left = 1.5;
  sc_fixed_fast<16, 8> fast_right = 2.25;
  sc_assert((fast_left + fast_right).to_double() == 3.75);
  sc_assert((fast_left * fast_right).to_double() == 3.375);
  std::cout << "DATATYPE_EXTENSIONS_OK\n";
  return 0;
}

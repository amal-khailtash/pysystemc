/** @file sc_core_trace_printf_matrix.cpp
 * @brief Independent native printf comment reference for all supported conversions.
 * @details Trace files borrow the signal until closure. Formatting is synchronous,
 * owns no callbacks, and does not interact with the Python GIL.
 */
#include <clocale>
#include <cstdint>
#include <cwchar>
#include <iostream>
#include <systemc>

/** @brief Emit typed native comments into both formats and close the files. */
int sc_main(int, char**)
{
  using namespace sc_core;
  std::setlocale(LC_ALL, "C");
  sc_signal<bool> signal("sample");
  sc_trace_file* files[] = {sc_create_vcd_trace_file("printf_vcd"), sc_create_wif_trace_file("printf_wif")};
  for (auto* trace : files)
  {
    trace->set_time_unit(1, SC_NS);
    sc_trace(trace, signal, "sample");
    tprintf(trace, "PRINTF grouping_flag %'d", 1234567);
    tprintf(trace, "PRINTF signed_default_d %d", static_cast<int>(-17));
    tprintf(trace, "PRINTF signed_default_i %i", static_cast<int>(-17));
    tprintf(trace, "PRINTF signed_hh_d %hhd", static_cast<int>(-17));
    tprintf(trace, "PRINTF signed_hh_i %hhi", static_cast<int>(-17));
    tprintf(trace, "PRINTF signed_h_d %hd", static_cast<int>(-17));
    tprintf(trace, "PRINTF signed_h_i %hi", static_cast<int>(-17));
    tprintf(trace, "PRINTF signed_l_d %ld", static_cast<long>(-17));
    tprintf(trace, "PRINTF signed_l_i %li", static_cast<long>(-17));
    tprintf(trace, "PRINTF signed_ll_d %lld", static_cast<long long>(-17));
    tprintf(trace, "PRINTF signed_ll_i %lli", static_cast<long long>(-17));
    tprintf(trace, "PRINTF signed_j_d %jd", static_cast<std::intmax_t>(-17));
    tprintf(trace, "PRINTF signed_j_i %ji", static_cast<std::intmax_t>(-17));
    tprintf(trace, "PRINTF signed_z_d %zd", static_cast<std::ptrdiff_t>(-17));
    tprintf(trace, "PRINTF signed_z_i %zi", static_cast<std::ptrdiff_t>(-17));
    tprintf(trace, "PRINTF signed_t_d %td", static_cast<std::ptrdiff_t>(-17));
    tprintf(trace, "PRINTF signed_t_i %ti", static_cast<std::ptrdiff_t>(-17));
    tprintf(trace, "PRINTF unsigned_default_u %#u", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_default_o %#o", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_default_x %#x", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_default_X %#X", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_hh_u %#hhu", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_hh_o %#hho", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_hh_x %#hhx", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_hh_X %#hhX", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_h_u %#hu", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_h_o %#ho", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_h_x %#hx", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_h_X %#hX", static_cast<unsigned int>(255));
    tprintf(trace, "PRINTF unsigned_l_u %#lu", static_cast<unsigned long>(255));
    tprintf(trace, "PRINTF unsigned_l_o %#lo", static_cast<unsigned long>(255));
    tprintf(trace, "PRINTF unsigned_l_x %#lx", static_cast<unsigned long>(255));
    tprintf(trace, "PRINTF unsigned_l_X %#lX", static_cast<unsigned long>(255));
    tprintf(trace, "PRINTF unsigned_ll_u %#llu", static_cast<unsigned long long>(255));
    tprintf(trace, "PRINTF unsigned_ll_o %#llo", static_cast<unsigned long long>(255));
    tprintf(trace, "PRINTF unsigned_ll_x %#llx", static_cast<unsigned long long>(255));
    tprintf(trace, "PRINTF unsigned_ll_X %#llX", static_cast<unsigned long long>(255));
    tprintf(trace, "PRINTF unsigned_j_u %#ju", static_cast<std::uintmax_t>(255));
    tprintf(trace, "PRINTF unsigned_j_o %#jo", static_cast<std::uintmax_t>(255));
    tprintf(trace, "PRINTF unsigned_j_x %#jx", static_cast<std::uintmax_t>(255));
    tprintf(trace, "PRINTF unsigned_j_X %#jX", static_cast<std::uintmax_t>(255));
    tprintf(trace, "PRINTF unsigned_z_u %#zu", static_cast<std::size_t>(255));
    tprintf(trace, "PRINTF unsigned_z_o %#zo", static_cast<std::size_t>(255));
    tprintf(trace, "PRINTF unsigned_z_x %#zx", static_cast<std::size_t>(255));
    tprintf(trace, "PRINTF unsigned_z_X %#zX", static_cast<std::size_t>(255));
    tprintf(trace, "PRINTF unsigned_t_u %#tu", static_cast<std::size_t>(255));
    tprintf(trace, "PRINTF unsigned_t_o %#to", static_cast<std::size_t>(255));
    tprintf(trace, "PRINTF unsigned_t_x %#tx", static_cast<std::size_t>(255));
    tprintf(trace, "PRINTF unsigned_t_X %#tX", static_cast<std::size_t>(255));
    tprintf(trace, "PRINTF float_default_f %+.4f", -1.25);
    tprintf(trace, "PRINTF float_default_F %+.4F", -1.25);
    tprintf(trace, "PRINTF float_default_e %+.4e", -1.25);
    tprintf(trace, "PRINTF float_default_E %+.4E", -1.25);
    tprintf(trace, "PRINTF float_default_g %+.4g", -1.25);
    tprintf(trace, "PRINTF float_default_G %+.4G", -1.25);
    tprintf(trace, "PRINTF float_default_a %+.4a", -1.25);
    tprintf(trace, "PRINTF float_default_A %+.4A", -1.25);
    tprintf(trace, "PRINTF float_l_f %+.4lf", -1.25);
    tprintf(trace, "PRINTF float_l_F %+.4lF", -1.25);
    tprintf(trace, "PRINTF float_l_e %+.4le", -1.25);
    tprintf(trace, "PRINTF float_l_E %+.4lE", -1.25);
    tprintf(trace, "PRINTF float_l_g %+.4lg", -1.25);
    tprintf(trace, "PRINTF float_l_G %+.4lG", -1.25);
    tprintf(trace, "PRINTF float_l_a %+.4la", -1.25);
    tprintf(trace, "PRINTF float_l_A %+.4lA", -1.25);
    tprintf(trace, "PRINTF float_L_f %+.4Lf", -1.25L);
    tprintf(trace, "PRINTF float_L_F %+.4LF", -1.25L);
    tprintf(trace, "PRINTF float_L_e %+.4Le", -1.25L);
    tprintf(trace, "PRINTF float_L_E %+.4LE", -1.25L);
    tprintf(trace, "PRINTF float_L_g %+.4Lg", -1.25L);
    tprintf(trace, "PRINTF float_L_G %+.4LG", -1.25L);
    tprintf(trace, "PRINTF float_L_a %+.4La", -1.25L);
    tprintf(trace, "PRINTF float_L_A %+.4LA", -1.25L);
    tprintf(trace, "PRINTF star %*.*f", -12, 4, 1.25);
    tprintf(trace, "PRINTF star_negative_precision %.*g", -1, 1.25);
    tprintf(trace, "PRINTF precision_zero %.0d %.0o %.0x", 0, 0u, 0u);
    tprintf(trace, "PRINTF integer_flags %+08d %#08X %-8u % d", -17, 255u, 29u, 17);
    tprintf(trace, "PRINTF strings %.3s %8s %-8ls", "alphabet", "word", L"wide");
    tprintf(trace, "PRINTF characters %c %lc", 'Q', std::wint_t('W'));
    tprintf(trace, "PRINTF positional_reuse %2$i %1$s %2$08i", "word", 17);
    tprintf(trace, "PRINTF positional_star %3$*1$.*2$G", 10, 4, 1.25);
    tprintf(trace, "PRINTF null_pointer %p", static_cast<void*>(nullptr));
    tprintf(trace, "PRINTF percent %%");
    int count0 = -1;
    tprintf(trace, "PRINTF count_default abc%n", &count0);
    sc_assert(count0 == 24);
    tprintf(trace, "PRINTF count_default_value %d", static_cast<int>(count0));
    signed char count1 = -1;
    tprintf(trace, "PRINTF count_hh abc%hhn", &count1);
    sc_assert(count1 == 19);
    tprintf(trace, "PRINTF count_hh_value %d", static_cast<int>(count1));
    short count2 = -1;
    tprintf(trace, "PRINTF count_h abc%hn", &count2);
    sc_assert(count2 == 18);
    tprintf(trace, "PRINTF count_h_value %d", static_cast<int>(count2));
    long count3 = -1;
    tprintf(trace, "PRINTF count_l abc%ln", &count3);
    sc_assert(count3 == 18);
    tprintf(trace, "PRINTF count_l_value %d", static_cast<int>(count3));
    long long count4 = -1;
    tprintf(trace, "PRINTF count_ll abc%lln", &count4);
    sc_assert(count4 == 19);
    tprintf(trace, "PRINTF count_ll_value %d", static_cast<int>(count4));
    std::intmax_t count5 = -1;
    tprintf(trace, "PRINTF count_j abc%jn", &count5);
    sc_assert(count5 == 18);
    tprintf(trace, "PRINTF count_j_value %d", static_cast<int>(count5));
    std::ptrdiff_t count6 = -1;
    tprintf(trace, "PRINTF count_z abc%zn", &count6);
    sc_assert(count6 == 18);
    tprintf(trace, "PRINTF count_z_value %d", static_cast<int>(count6));
    std::ptrdiff_t count7 = -1;
    tprintf(trace, "PRINTF count_t abc%tn", &count7);
    sc_assert(count7 == 18);
    tprintf(trace, "PRINTF count_t_value %d", static_cast<int>(count7));
  }
  sc_start(1, SC_NS);
  sc_close_vcd_trace_file(files[0]);
  sc_close_wif_trace_file(files[1]);
  return 0;
}

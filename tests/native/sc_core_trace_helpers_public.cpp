/** @file sc_core_trace_helpers_public.cpp
 * @brief Native reference for trace comments, delta transitions and closure.
 */
#include <cstdint>
#include <cwchar>
#include <iostream>
#include <string>
#include <systemc>

/** @brief Run a finite trace simulation; signals outlive both open files. */
int sc_main(int, char**)
{
  using namespace sc_core;
  sc_signal<bool> signal("sample");
  sc_trace_file* files[] = {sc_create_vcd_trace_file("helpers"), sc_create_wif_trace_file("helpers")};
  for (auto* trace : files)
  {
    trace->set_time_unit(1, SC_PS);
    sc_trace(trace, signal, "sample");
    sc_trace_delta_cycles(trace);
    sc_trace_delta_cycles(trace, false);
    sc_trace_delta_cycles(trace, true);
    sc_write_comment(trace, "plain comment");
    tprintf(trace, "formatted %s %d %u %.2f %%", "word", -17, 29u, 1.25);
    tprintf(trace, "lengths %lld %zu %08x %c", 1LL << 40, std::size_t(137), 255u, 'Q');
    tprintf(trace, "dynamic %*.*f", 8, 3, 1.25);
    tprintf(trace, "hexfloat %a", 1.25);
    tprintf(trace, "types %hhd %hd %ld %jd %td %.2Lf %ls %lc %p", -3, -7, 23L, std::intmax_t(1LL << 40),
            std::ptrdiff_t(17), 1.25L, L"wide", std::wint_t('W'), static_cast<void*>(nullptr));
    tprintf(trace, "no arguments %%");
    tprintf(trace, "position %2$d %1$s", "word", 17);
    tprintf(trace, "position dynamic %3$*1$.*2$f", 8, 3, 1.25);
    int count = 0;
    tprintf(trace, "counted%n", &count);
    sc_assert(count == 7);
    tprintf(trace, "count value %d", count);
    // Native tprintf's fixed buffer cannot hold the long comment; equivalent text avoids overflow.
    sc_write_comment(trace, "long " + std::string(5000, 'a'));
  }
  sc_close_vcd_trace_file(nullptr);
  sc_close_wif_trace_file(nullptr);
  sc_trace_delta_cycles(nullptr);
  sc_write_comment(nullptr, "ignored");
  tprintf(nullptr, "%s", "ignored");
  int null_count = 0;
  tprintf(nullptr, "null counted%n", &null_count);
  sc_assert(null_count == 12);
  signal.write(false);
  sc_start(1, SC_NS);
  signal.write(true);
  sc_start(SC_ZERO_TIME);
  signal.write(false);
  sc_start(1, SC_NS);
  signal.write(true);
  sc_start(1, SC_NS);
  sc_close_vcd_trace_file(files[0]);
  sc_close_wif_trace_file(files[1]);
  sc_assert(sc_time_stamp() == sc_time(3, SC_NS));
  std::cout << "TRACE_HELPERS_REFERENCE " << sc_time_stamp() << '\n';
  return 0;
}

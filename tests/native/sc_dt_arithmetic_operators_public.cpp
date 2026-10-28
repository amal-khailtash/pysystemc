/** @file sc_dt_arithmetic_operators_public.cpp
 * @brief Execute one compile-time selected native arithmetic signature chunk.
 * @details Each selected native result comes from an exact function pointer;
 * source views remain alive during invocation, and result serialization copies
 * exact fast/arbitrary precision state before teardown. No callbacks, waits or
 * kernel state are retained.
 */
#include <iostream>
#include <string>
#include <utility>

#include "sc_dt_arithmetic_operators_probe.h"
#if !defined(SC_DT_ARITHMETIC_FAMILY) || !defined(SC_DT_ARITHMETIC_START) || !defined(SC_DT_ARITHMETIC_END)
#error "selector source must define one arithmetic family chunk"
#endif
namespace
{
using namespace sc_dt_arithmetic_operators_probe;
template <std::size_t... Offset>
void run_chunk(Sources& sources, std::index_sequence<Offset...>)
{
  ((std::cout << "NATIVE\t" << SC_DT_ARITHMETIC_FAMILY << '\t' << (SC_DT_ARITHMETIC_START + Offset) << '\t'
              << NativeCase<SC_DT_ARITHMETIC_FAMILY, SC_DT_ARITHMETIC_START + Offset>::call(sources) << '\n'),
   ...);
}
}  // namespace
int sc_main(int, char**)
{
  Sources sources;
  run_chunk(sources, std::make_index_sequence<SC_DT_ARITHMETIC_END - SC_DT_ARITHMETIC_START>{});
  std::cout << "SC_DT_ARITHMETIC_OPERATORS_PUBLIC_OK " << SC_DT_ARITHMETIC_FAMILY << ' ' << SC_DT_ARITHMETIC_START
            << ' ' << SC_DT_ARITHMETIC_END << '\n';
  return 0;
}

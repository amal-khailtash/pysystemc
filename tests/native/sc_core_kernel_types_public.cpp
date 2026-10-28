/** @file sc_core_kernel_types_public.cpp
 * @brief Native pointer/type declaration reference without kernel reset mutations.
 */
#include <iostream>
#include <systemc>

#include "sc_core_kernel_types_probe.h"

/** @brief Verify stable borrowed reset identity and exact native handler transport. */
int sc_main(int, char**)
{
  sc_core::sc_signal<bool> source("reset_view_source");
  small_types_probe::ResetView view(source);
  const sc_core::sc_reset* pointer = view.borrowed();
  if (!pointer || pointer != &view || view.borrowed() != pointer) return 2;
  if (!small_types_probe::handler_roundtrip()) return 3;
  std::cout << "SMALL_TYPES_OK 1 1\n";
  return 0;
}

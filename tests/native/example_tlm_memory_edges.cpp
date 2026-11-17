/** @file example_tlm_memory_edges.cpp
 * @brief Run native references for upstream TLM memory and DMI edge behavior.
 */
#define REPORT_DEFINE_GLOBALS
#include "example_tlm_memory_edges.h"

#include "reporting.h"

/** @brief Run memory synchronously, then probe DMI before and during simulation. */
int sc_main(int, char**)
{
  REPORT_ENABLE_ALL_REPORTING();
  example_tlm_memory_edges::MemoryProbe memory_probe;
  memory_probe.run();
  example_tlm_memory_edges::DmiProbe dmi_probe("dmi_probe");
  dmi_probe.check_enabled();
  sc_core::sc_start();
  return 0;
}

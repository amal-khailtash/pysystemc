/** @file tlm_ports_analysis_types.h
 *  @brief Custom payload shared by Python and native TLM1 coverage.
 */
#ifndef PYSYSTEMC_TLM_PORTS_ANALYSIS_TYPES_H
#define PYSYSTEMC_TLM_PORTS_ANALYSIS_TYPES_H

#include <systemc>

namespace tlm_ports_analysis_probe
{
/** @brief Nontrivial user payload with an arbitrary 137-bit value. */
struct Sample
{
  sc_dt::sc_biguint<137> bits;
  unsigned sequence;

  /** @brief Construct a tagged sample with the high bit and selected low bits. */
  Sample(unsigned seq = 0, unsigned low = 0) : bits((sc_dt::sc_biguint<137>(1) << 136) | low), sequence(seq)
  {
  }
};
}  // namespace tlm_ports_analysis_probe

#endif  // PYSYSTEMC_TLM_PORTS_ANALYSIS_TYPES_H

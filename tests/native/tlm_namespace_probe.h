/** @file tlm_namespace_probe.h
 * @brief Native extension registry used to observe max_num_extensions.
 * @details Registration is process-global and monotonic. This probe registers
 * a real extension type without constructing payloads or retaining callbacks.
 * Calls hold the GIL, do not suspend and propagate native allocation errors.
 */
#ifndef PYSYSTEMC_TLM_NAMESPACE_PROBE_H
#define PYSYSTEMC_TLM_NAMESPACE_PROBE_H
#include <tlm_core/tlm_2/tlm_generic_payload/tlm_array.h>

#include <systemc>
#include <tlm>
namespace tlm_namespace_probe
{
/** @brief Register an extension without relying on template static initialization. */
struct Registry : tlm::tlm_extension_base
{
  /** @brief Copy owned native extension storage. */
  tlm::tlm_extension_base* clone() const override
  {
    return new Registry;
  }
  /** @brief No value state needs copying in this registry-only extension. */
  void copy_from(const tlm::tlm_extension_base&) override
  {
  }
  /** @brief Return the stable native ID of this exact extension type. */
  static unsigned register_type()
  {
    return register_extension(typeid(Registry));
  }
};
/** @brief Count null borrowed pointer slots without accessing their pointees. */
inline unsigned null_slots(const tlm::tlm_array<tlm::tlm_extension_base*>& array)
{
  unsigned count = 0;
  for (unsigned index = 0; index < array.size(); ++index)
    if (!array[index]) ++count;
  return count;
}
}  // namespace tlm_namespace_probe
#endif

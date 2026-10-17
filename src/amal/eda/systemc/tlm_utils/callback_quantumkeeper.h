/** @file Python-compatible custom local quantum callback adapter. */
#pragma once
#include <tlm_utils/tlm_quantumkeeper.h>

#include <stdexcept>

namespace pysystemc_interfaces
{
/** @brief Preserve the native protected hook through a pointer-return trampoline.
 * @details Python owns the dispatcher and returned time slot. The hook borrows
 * the slot only for the copy below; exceptions propagate. cppyy acquires the GIL
 * on callback entry. Blocking synchronization must enter through guarded helpers
 * with the GIL released. Retain this keeper throughout native use and suspension.
 */
class CallbackQuantumKeeper : public tlm_utils::tlm_quantumkeeper
{
 public:
  /** @brief Return borrowed live time storage; Python retains it across this call. */
  virtual const sc_core::sc_time* local_quantum_pointer() = 0;

 protected:
  /** @brief Copy callback time into native synchronization-point computation. */
  sc_core::sc_time compute_local_quantum() override
  {
    const auto* value = local_quantum_pointer();
    if (!value) throw std::runtime_error("local quantum callback returned null");
    return *value;
  }
};
}  // namespace pysystemc_interfaces

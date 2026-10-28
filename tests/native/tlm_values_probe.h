/** @file tlm_values_probe.h
 * @brief Native adapters for phase registration and stream inspection.
 * @details The extended phase is a process-lifetime static registry entry. The
 * returned phase reference is borrowed and remains valid for the process. The
 * stream helper performs synchronous native formatting and does not retain
 * Python objects or call Python callbacks.
 */
#ifndef PYSYSTEMC_TEST_TLM_VALUES_PROBE_H
#define PYSYSTEMC_TEST_TLM_VALUES_PROBE_H

#include <tlm_utils/tlm_quantumkeeper.h>

#include <sstream>
#include <string>
#include <systemc>
#include <tlm>

namespace tlm_values_probe
{

TLM_DECLARE_EXTENDED_PHASE(PYTHON_AUDIT_PHASE);

/** @brief Expose the protected global-quantum constructor for audit use.
 * @details This independently owned derived instance does not replace the
 * process-wide singleton. Its public inherited value methods follow the native
 * SystemC time configuration and it has ordinary stack/Python proxy lifetime.
 */
class GlobalQuantumProbe : public tlm::tlm_global_quantum
{
 public:
  /** @brief Construct through the native protected base constructor. */
  GlobalQuantumProbe() : tlm::tlm_global_quantum()
  {
  }
};

/** @brief Expose the protected base quantumkeeper calculation for audit use.
 * @details The helper calls the base virtual implementation synchronously and
 * borrows the process-wide global quantum singleton. No callbacks are retained.
 */
class QuantumkeeperProbe : public tlm_utils::tlm_quantumkeeper
{
 public:
  /** @brief Count destruction after native keeper teardown. */
  ~QuantumkeeperProbe() override
  {
    ++destruction_count();
  }

  /** @brief Call the inherited protected local-quantum computation. */
  sc_core::sc_time compute_local_quantum_probe()
  {
    return tlm_utils::tlm_quantumkeeper::compute_local_quantum();
  }

  /** @brief Return the process-local destruction count for this probe type. */
  static unsigned destructions()
  {
    return destruction_count();
  }

 private:
  /** @brief Own the audit counter without external storage or callbacks. */
  static unsigned& destruction_count()
  {
    static unsigned count = 0;
    return count;
  }
};

/** @brief Return the process-lifetime registered custom phase by reference.
 * @return Borrowed extended phase owned by the static registration object.
 */
inline const tlm::tlm_phase& custom_phase()
{
  return PYTHON_AUDIT_PHASE.get_phase();
}

/** @brief Format a phase through the native free stream insertion operator.
 * @param phase Borrowed phase value used synchronously during formatting.
 * @return Owned string containing the registered phase name.
 */
inline std::string stream_phase(const tlm::tlm_phase& phase)
{
  std::ostringstream stream;
  stream << phase;
  return stream.str();
}

}  // namespace tlm_values_probe

#endif  // PYSYSTEMC_TEST_TLM_VALUES_PROBE_H

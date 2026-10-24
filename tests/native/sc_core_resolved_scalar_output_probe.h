/** @file sc_core_resolved_scalar_output_probe.h
 * @brief Native lifecycle probes for resolved scalar inout and output ports.
 * @details Probes retain no Python objects or callbacks. SystemC calls their
 *          lifecycle hooks on the kernel thread; the test reads counters from
 *          its owning process. The virtual deletion check uses unbound ports
 *          with no consumers while a module hierarchy is active.
 */
#ifndef PYSYSTEMC_SC_CORE_RESOLVED_SCALAR_OUTPUT_PROBE_H
#define PYSYSTEMC_SC_CORE_RESOLVED_SCALAR_OUTPUT_PROBE_H

#include <array>
#include <cstddef>
#include <memory>
#include <systemc>

namespace pysystemc_resolved_scalar_output
{
/** @brief Store elaboration-hook and virtual-destruction counts by family. */
inline std::array<int, 4>& counters()
{
  static std::array<int, 4> values{};
  return values;
}

/** @brief Reset test-only lifecycle counters before a fresh kernel run. */
inline void reset_counters()
{
  counters().fill(0);
}

/** @brief Return the native end-of-elaboration count for one family. */
inline int elaboration_count(int family)
{
  return counters()[static_cast<std::size_t>(family)];
}

/** @brief Return the virtual-destruction count for one family. */
inline int destruction_count(int family)
{
  return counters()[static_cast<std::size_t>(family + 2)];
}

/** @brief Probe sc_inout_resolved's native virtual lifecycle. */
class InoutResolvedProbe : public sc_core::sc_inout_resolved
{
  using Base = sc_core::sc_inout_resolved;

 public:
  /** @brief Construct a named port that borrows its eventual interface. */
  explicit InoutResolvedProbe(const char* name) : Base(name)
  {
  }

  /** @brief Preserve base validation and record successful elaboration. */
  void end_of_elaboration() override
  {
    Base::end_of_elaboration();
    ++counters()[0];
  }

  /** @brief Count deletion through the public polymorphic port base. */
  ~InoutResolvedProbe() override
  {
    ++counters()[2];
  }
};

/** @brief Probe sc_out_resolved's native virtual lifecycle. */
class OutResolvedProbe : public sc_core::sc_out_resolved
{
  using Base = sc_core::sc_out_resolved;

 public:
  /** @brief Construct a named port that borrows its eventual interface. */
  explicit OutResolvedProbe(const char* name) : Base(name)
  {
  }

  /** @brief Preserve base validation and record successful elaboration. */
  void end_of_elaboration() override
  {
    Base::end_of_elaboration();
    ++counters()[1];
  }

  /** @brief Count deletion through the public polymorphic port base. */
  ~OutResolvedProbe() override
  {
    ++counters()[3];
  }
};

/** @brief Verify both public port bases delete derived probes exactly once.
 * @details The probes are unbound and have no consumers. They are deleted
 *          through their respective public native port base while the test
 *          module hierarchy is active and before simulation elaboration.
 */
inline bool verify_virtual_destruction()
{
  {
    std::unique_ptr<sc_core::sc_inout_resolved> port(new InoutResolvedProbe("destroy_inout_resolved"));
  }
  {
    std::unique_ptr<sc_core::sc_out_resolved> port(new OutResolvedProbe("destroy_out_resolved"));
  }
  return destruction_count(0) == 1 && destruction_count(1) == 1;
}
}  // namespace pysystemc_resolved_scalar_output

#endif

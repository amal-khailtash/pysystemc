/** @file sc_core_resolved_ports_probe.h
 * @brief Native probes for resolved scalar and vector input ports.
 * @details The probes retain no Python values or callbacks. Their lifecycle
 *          hooks execute on the SystemC kernel thread; counters are read by
 *          the owning test process after simulation or native destruction.
 */
#ifndef PYSYSTEMC_SC_CORE_RESOLVED_PORTS_PROBE_H
#define PYSYSTEMC_SC_CORE_RESOLVED_PORTS_PROBE_H

#include <array>
#include <cstddef>
#include <memory>
#include <systemc>

namespace pysystemc_resolved_ports
{
/** @brief Store hook and virtual-destruction counts for the two input types. */
inline std::array<int, 4>& counters()
{
  static std::array<int, 4> values{};
  return values;
}

/** @brief Reset test-only counters before a fresh kernel simulation. */
inline void reset_counters()
{
  counters().fill(0);
}

/** @brief Read a native end-of-elaboration hook count. */
inline int elaboration_count(int family)
{
  return counters()[static_cast<std::size_t>(family)];
}

/** @brief Read a virtual-destruction count for one family. */
inline int destruction_count(int family)
{
  return counters()[static_cast<std::size_t>(family + 2)];
}

/** @brief Probe the scalar resolved input port's virtual lifecycle. */
class InResolvedProbe : public sc_core::sc_in_resolved
{
  using Base = sc_core::sc_in_resolved;

 public:
  /** @brief Construct a named port that borrows its eventual channel. */
  explicit InResolvedProbe(const char* name) : Base(name)
  {
  }

  /** @brief Run native validation before recording successful elaboration. */
  void end_of_elaboration() override
  {
    Base::end_of_elaboration();
    ++counters()[0];
  }

  /** @brief Record deletion through the polymorphic SystemC port base. */
  ~InResolvedProbe() override
  {
    ++counters()[2];
  }
};

/** @brief Probe a resolved vector input port at any positive width. */
template <int Width>
class InRvProbe : public sc_core::sc_in_rv<Width>
{
  using Base = sc_core::sc_in_rv<Width>;

 public:
  /** @brief Construct a named port that borrows its eventual channel. */
  explicit InRvProbe(const char* name) : Base(name)
  {
  }

  /** @brief Run native validation before recording successful elaboration. */
  void end_of_elaboration() override
  {
    Base::end_of_elaboration();
    ++counters()[1];
  }

  /** @brief Record deletion through the polymorphic SystemC port base. */
  ~InRvProbe() override
  {
    ++counters()[3];
  }
};

/** @brief Verify scalar and vector input ports have virtual destructors.
 * @details Each derived probe is deleted through its public native port base
 *          before simulation while a parent SystemC module hierarchy is
 *          active. No Python callbacks, GIL state, or channel ownership is
 *          involved.
 */
inline bool verify_virtual_destruction()
{
  {
    std::unique_ptr<sc_core::sc_in_resolved> port(new InResolvedProbe("destroy_in_resolved"));
  }
  {
    std::unique_ptr<sc_core::sc_in_rv<137>> port(new InRvProbe<137>("destroy_in_rv"));
  }
  return destruction_count(0) == 1 && destruction_count(1) == 1;
}
}  // namespace pysystemc_resolved_ports

#endif

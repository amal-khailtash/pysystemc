/** @file sc_core_resolved_vector_output_probe.h
 * @brief Native lifecycle probes for resolved vector output ports.
 * @details These probes retain no Python values or callbacks. Their hooks run
 *          on the SystemC kernel thread, and the destruction check deletes
 *          each derived probe through its public polymorphic port base.
 */
#ifndef PYSYSTEMC_SC_CORE_RESOLVED_VECTOR_OUTPUT_PROBE_H
#define PYSYSTEMC_SC_CORE_RESOLVED_VECTOR_OUTPUT_PROBE_H

#include <array>
#include <cstddef>
#include <memory>
#include <systemc>

namespace pysystemc_resolved_vector_output
{
/** @brief Hold hook counts followed by virtual-destructor counts. */
inline std::array<int, 8>& counters()
{
  static std::array<int, 8> values{};
  return values;
}

/** @brief Reset test-only counters before a fresh kernel simulation. */
inline void reset_counters()
{
  counters().fill(0);
}

/** @brief Return one end-of-elaboration count. */
inline int elaboration_count(int family)
{
  return counters()[static_cast<std::size_t>(family)];
}

/** @brief Return one virtual-destruction count. */
inline int destruction_count(int family)
{
  return counters()[static_cast<std::size_t>(family + 4)];
}

/** @brief Derive from sc_inout_rv and count native lifecycle calls. */
template <int Width>
class InoutRvProbe : public sc_core::sc_inout_rv<Width>
{
  using Base = sc_core::sc_inout_rv<Width>;
  static constexpr int family = Width == 137 ? 0 : 1;

 public:
  /** @brief Construct a named port that borrows its eventual channel. */
  explicit InoutRvProbe(const char* name) : Base(name)
  {
  }

  /** @brief Run native validation before recording elaboration. */
  void end_of_elaboration() override
  {
    Base::end_of_elaboration();
    ++counters()[static_cast<std::size_t>(family)];
  }

  /** @brief Record deletion through the public inout port base. */
  ~InoutRvProbe() override
  {
    ++counters()[static_cast<std::size_t>(family + 4)];
  }
};

/** @brief Derive from sc_out_rv and count native lifecycle calls. */
template <int Width>
class OutRvProbe : public sc_core::sc_out_rv<Width>
{
  using Base = sc_core::sc_out_rv<Width>;
  static constexpr int family = Width == 137 ? 2 : 3;

 public:
  /** @brief Construct a named port that borrows its eventual channel. */
  explicit OutRvProbe(const char* name) : Base(name)
  {
  }

  /** @brief Run native validation before recording elaboration. */
  void end_of_elaboration() override
  {
    Base::end_of_elaboration();
    ++counters()[static_cast<std::size_t>(family)];
  }

  /** @brief Record deletion through the public output port base. */
  ~OutRvProbe() override
  {
    ++counters()[static_cast<std::size_t>(family + 4)];
  }
};

/** @brief Verify both port families at both widths have virtual destructors.
 * @details The derived instances are deleted through their public native port
 *          bases while a parent module hierarchy is active. They have no
 *          consumers and use no Python callbacks, GIL state, or channels.
 */
inline bool verify_virtual_destruction()
{
  {
    std::unique_ptr<sc_core::sc_inout_rv<137>> port(new InoutRvProbe<137>("destroy_inout_rv137"));
  }
  {
    std::unique_ptr<sc_core::sc_inout_rv<5>> port(new InoutRvProbe<5>("destroy_inout_rv5"));
  }
  {
    std::unique_ptr<sc_core::sc_out_rv<137>> port(new OutRvProbe<137>("destroy_out_rv137"));
  }
  {
    std::unique_ptr<sc_core::sc_out_rv<5>> port(new OutRvProbe<5>("destroy_out_rv5"));
  }
  return destruction_count(0) == 1 && destruction_count(1) == 1 && destruction_count(2) == 1 &&
         destruction_count(3) == 1;
}
}  // namespace pysystemc_resolved_vector_output

#endif

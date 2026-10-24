/** @file sc_core_input_ports_probe.h
 *  @brief Native lifecycle probes for the sc_in declaration family.
 *  @details Probes borrow their interface and trace-file objects. Ports own
 *  cached event finders and deferred trace records; this header retains no
 *  Python callbacks or references and invokes no Python code from native
 *  threads. All helpers are synchronous and require the caller to keep the
 *  SystemC hierarchy and borrowed objects alive through each call.
 */
#ifndef PYSYSTEMC_SC_CORE_INPUT_PORTS_PROBE_H
#define PYSYSTEMC_SC_CORE_INPUT_PORTS_PROBE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <systemc>

namespace pysystemc_input_ports
{

/** @brief Store native elaboration and destruction counts for three input types. */
inline std::array<int, 6>& lifecycle_counts()
{
  static std::array<int, 6> values{};
  return values;
}

/** @brief Clear test lifecycle counters before creating any probe ports. */
inline void reset_lifecycle_counts()
{
  lifecycle_counts().fill(0);
}

/** @brief Return the end-of-elaboration count for one data family. */
inline int elaboration_count(int family)
{
  return lifecycle_counts()[static_cast<std::size_t>(family)];
}

/** @brief Return the virtual-destructor count for one data family. */
inline int destruction_count(int family)
{
  return lifecycle_counts()[static_cast<std::size_t>(family + 3)];
}

/** @brief Derive from sc_in to inspect deferred trace ownership and lifecycle. */
template <class T, int Family>
class InputProbe : public sc_core::sc_in<T>
{
  using Base = sc_core::sc_in<T>;

 public:
  /** @brief Construct a named input port in the active SystemC hierarchy. */
  explicit InputProbe(const char* name) : Base(name)
  {
  }

  /** @brief Return the number of trace parameter records owned by this input. */
  std::size_t trace_record_count() const
  {
    return this->m_traces == nullptr ? 0 : this->m_traces->size();
  }

  /** @brief Release pending trace parameters using the native protected helper. */
  void discard_trace_records() const
  {
    this->remove_traces();
  }

  /** @brief Run native trace cleanup and record end-of-elaboration dispatch. */
  void end_of_elaboration() override
  {
    Base::end_of_elaboration();
    ++lifecycle_counts()[static_cast<std::size_t>(Family)];
  }

  /** @brief Record destruction through the public polymorphic sc_in base. */
  ~InputProbe() override
  {
    ++lifecycle_counts()[static_cast<std::size_t>(Family + 3)];
  }
};

using InputWideProbe = InputProbe<sc_dt::sc_bv<137>, 0>;
using InputBoolProbe = InputProbe<bool, 1>;
using InputLogicProbe = InputProbe<sc_dt::sc_logic, 2>;

static_assert(std::is_same<sc_core::sc_in<sc_dt::sc_bv<137>>::data_type, sc_dt::sc_bv<137>>::value,
              "generic input data alias");
static_assert(std::is_same<sc_core::sc_in<bool>::data_type, bool>::value, "bool input data alias");
static_assert(std::is_same<sc_core::sc_in<sc_dt::sc_logic>::data_type, sc_dt::sc_logic>::value,
              "logic input data alias");

/** @brief Return a borrowed interface with const qualification for overload tests.
 *  @tparam IF Native input interface type.
 *  @tparam OBJECT Concrete signal type implementing the interface.
 *  @param object Live channel that owns the interface.
 *  @return Const interface reference; ownership remains with the channel.
 */
template <class IF, class OBJECT>
inline const IF& const_interface_ref(const OBJECT& object)
{
  return dynamic_cast<const IF&>(object);
}

/** @brief Return the address exposed by sc_in::read().
 *  @tparam Port Native sc_in specialization.
 *  @param port Bound port whose borrowed channel value is inspected.
 *  @return Address of the value reference owned by the bound channel.
 */
template <class Port>
inline std::uintptr_t read_value_address(const Port& port)
{
  return reinterpret_cast<std::uintptr_t>(&port.read());
}

/** @brief Return the address exposed by sc_in's const-value conversion.
 *  @tparam Port Native sc_in specialization.
 *  @param port Bound port whose conversion operator is inspected.
 *  @return Address of the borrowed channel value.
 */
template <class Port>
inline std::uintptr_t conversion_value_address(const Port& port)
{
  using value_type = typename Port::data_type;
  const value_type& value = static_cast<const value_type&>(port);
  return reinterpret_cast<std::uintptr_t>(&value);
}

/** @brief Verify virtual destruction for the generic, bool and logic inputs.
 *  @details Temporary probes are unbound and own no callbacks. Their names are
 *  registered under the caller's active module hierarchy and removed before
 *  this function returns.
 *  @return True if each derived destructor ran through its base pointer.
 */
inline bool verify_virtual_destruction()
{
  {
    std::unique_ptr<sc_core::sc_in<sc_dt::sc_bv<137>>> port(new InputWideProbe("destroy_input_bv137"));
  }
  {
    std::unique_ptr<sc_core::sc_in<bool>> port(new InputBoolProbe("destroy_input_bool"));
  }
  {
    std::unique_ptr<sc_core::sc_in<sc_dt::sc_logic>> port(new InputLogicProbe("destroy_input_logic"));
  }
  return destruction_count(0) == 1 && destruction_count(1) == 1 && destruction_count(2) == 1;
}

}  // namespace pysystemc_input_ports

#endif

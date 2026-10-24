/** @file sc_core_inout_ports_probe.h
 * @brief Native lifecycle probes for generic, bool and logic inout/output ports.
 * @details Probe instances own no Python references. SystemC invokes lifecycle
 * hooks on its kernel thread; borrowed interfaces and trace files must outlive
 * their ports. Destruction checks delete native-derived objects through the
 * public polymorphic port base while a module hierarchy is active.
 */
#ifndef PYSYSTEMC_SC_CORE_INOUT_PORTS_PROBE_H
#define PYSYSTEMC_SC_CORE_INOUT_PORTS_PROBE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <systemc>
#include <type_traits>

namespace pysystemc_inout_ports
{

/** @brief Keep hook and destructor counts for three port data types. */
inline std::array<int, 12>& lifecycle_counts()
{
  static std::array<int, 12> values{};
  return values;
}

/** @brief Clear test-only lifecycle counters before constructing the model. */
inline void reset_lifecycle_counts()
{
  lifecycle_counts().fill(0);
}

/** @brief Read one inout end-of-elaboration count. */
inline int inout_elaboration_count(int family)
{
  return lifecycle_counts()[static_cast<std::size_t>(family)];
}

/** @brief Read one inout virtual-destructor count. */
inline int inout_destruction_count(int family)
{
  return lifecycle_counts()[static_cast<std::size_t>(family + 3)];
}

/** @brief Read one output end-of-elaboration count. */
inline int out_elaboration_count(int family)
{
  return lifecycle_counts()[static_cast<std::size_t>(family + 6)];
}

/** @brief Read one output virtual-destructor count. */
inline int out_destruction_count(int family)
{
  return lifecycle_counts()[static_cast<std::size_t>(family + 9)];
}

/** @brief Derive from sc_inout and expose its deferred lifecycle storage.
 * @tparam T Native SystemC signal value type.
 * @tparam Family Counter slot: generic, bool or sc_logic.
 */
template <class T, int Family>
class InoutProbe : public sc_core::sc_inout<T>
{
  using Base = sc_core::sc_inout<T>;

 public:
  /** @brief Construct a named native port inside its owning module scope. */
  explicit InoutProbe(const char* name) : Base(name)
  {
  }

  /** @brief Report whether initialize() is waiting for elaboration. */
  bool has_pending_initial_value() const
  {
    return this->m_init_val != nullptr;
  }

  /** @brief Return the number of deferred trace registrations owned by the port. */
  std::size_t trace_record_count() const
  {
    return this->m_traces == nullptr ? 0 : this->m_traces->size();
  }

  /** @brief Release trace parameter records through the protected base helper. */
  void discard_trace_records() const
  {
    this->remove_traces();
  }

  /** @brief Run native initialization/trace work and count elaboration. */
  void end_of_elaboration() override
  {
    Base::end_of_elaboration();
    ++lifecycle_counts()[static_cast<std::size_t>(Family)];
  }

  /** @brief Record polymorphic destruction through sc_inout<T>. */
  ~InoutProbe() override
  {
    ++lifecycle_counts()[static_cast<std::size_t>(Family + 3)];
  }
};

/** @brief Derive from sc_out and count inherited lifecycle hooks/destruction.
 * @tparam T Native SystemC signal value type.
 * @tparam Family Counter slot: generic, bool or sc_logic.
 */
template <class T, int Family>
class OutProbe : public sc_core::sc_out<T>
{
  using Base = sc_core::sc_out<T>;

 public:
  /** @brief Construct a named native output port inside its module scope. */
  explicit OutProbe(const char* name) : Base(name)
  {
  }

  /** @brief Run inherited inout initialization/trace work and count elaboration. */
  void end_of_elaboration() override
  {
    Base::end_of_elaboration();
    ++lifecycle_counts()[static_cast<std::size_t>(Family + 6)];
  }

  /** @brief Record polymorphic destruction through sc_out<T>. */
  ~OutProbe() override
  {
    ++lifecycle_counts()[static_cast<std::size_t>(Family + 9)];
  }
};

using InoutWideProbe = InoutProbe<sc_dt::sc_bv<137>, 0>;
using InoutBoolProbe = InoutProbe<bool, 1>;
using InoutLogicProbe = InoutProbe<sc_dt::sc_logic, 2>;
using OutWideProbe = OutProbe<sc_dt::sc_bv<137>, 0>;
using OutBoolProbe = OutProbe<bool, 1>;
using OutLogicProbe = OutProbe<sc_dt::sc_logic, 2>;

static_assert(std::is_same_v<sc_core::sc_inout<bool>::data_type, bool>);
static_assert(std::is_same_v<sc_core::sc_inout<sc_dt::sc_logic>::data_type, sc_dt::sc_logic>);
static_assert(std::is_same_v<sc_core::sc_inout<sc_dt::sc_bv<137>>::data_type, sc_dt::sc_bv<137>>);
static_assert(std::is_same_v<sc_core::sc_out<bool>::data_type, bool>);
static_assert(std::is_same_v<sc_core::sc_out<sc_dt::sc_logic>::data_type, sc_dt::sc_logic>);
static_assert(std::is_same_v<sc_core::sc_out<sc_dt::sc_bv<137>>::data_type, sc_dt::sc_bv<137>>);

/** @brief Return the address reached through a port's const-value conversion.
 * @tparam Port Native inout or output port type.
 * @param port Bound port whose public conversion operator is exercised.
 * @return Address of the channel value returned by the conversion operator.
 */
template <class Port>
inline std::uintptr_t conversion_value_address(const Port& port)
{
  using value_type = typename Port::data_type;
  const value_type& value = static_cast<const value_type&>(port);
  return reinterpret_cast<std::uintptr_t>(&value);
}

/** @brief Return the address reached through the public read() method.
 * @tparam Port Native inout or output port type.
 * @param port Bound port whose read reference is inspected.
 * @return Address of the channel value returned by read().
 */
template <class Port>
inline std::uintptr_t read_value_address(const Port& port)
{
  return reinterpret_cast<std::uintptr_t>(&port.read());
}

/** @brief Verify virtual destruction for all six public port bases.
 * @details The parent module hierarchy must already be active. All temporary
 * ports are unbound and have no consumers or callbacks.
 * @return True if each derived destructor ran exactly once.
 */
inline bool verify_virtual_destruction()
{
  {
    std::unique_ptr<sc_core::sc_inout<sc_dt::sc_bv<137>>> port(new InoutWideProbe("destroy_inout_bv137"));
  }
  {
    std::unique_ptr<sc_core::sc_inout<bool>> port(new InoutBoolProbe("destroy_inout_bool"));
  }
  {
    std::unique_ptr<sc_core::sc_inout<sc_dt::sc_logic>> port(new InoutLogicProbe("destroy_inout_logic"));
  }
  {
    std::unique_ptr<sc_core::sc_out<sc_dt::sc_bv<137>>> port(new OutWideProbe("destroy_out_bv137"));
  }
  {
    std::unique_ptr<sc_core::sc_out<bool>> port(new OutBoolProbe("destroy_out_bool"));
  }
  {
    std::unique_ptr<sc_core::sc_out<sc_dt::sc_logic>> port(new OutLogicProbe("destroy_out_logic"));
  }
  return inout_destruction_count(0) == 1 && inout_destruction_count(1) == 1 && inout_destruction_count(2) == 1 &&
         out_destruction_count(0) == 1 && out_destruction_count(1) == 1 && out_destruction_count(2) == 1;
}

}  // namespace pysystemc_inout_ports

#endif

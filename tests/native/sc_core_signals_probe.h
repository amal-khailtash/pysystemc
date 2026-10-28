/** @file sc_core_signals_probe.h
 * @brief Test-only access to protected generic SystemC signal operations.
 * @details The probe derives from sc_signal_t because its constructor and
 * destructor are protected by design. It adds no storage and owns no Python
 * callbacks. Instances are ordinary native primitive channels and must be
 * destroyed before the singleton simulation context is torn down. Calls into
 * these helpers require the SystemC kernel's normal thread context; no GIL
 * state is acquired or retained here.
 */
#ifndef PYSYSTEMC_TEST_SC_CORE_SIGNALS_PROBE_H
#define PYSYSTEMC_TEST_SC_CORE_SIGNALS_PROBE_H

#include <cstdint>
#include <systemc>

namespace signal_probe
{

/** @brief Expose sc_signal_t<T,POL>'s protected construction to tests.
 * @tparam T Native signal value type.
 * @tparam POL Native SystemC writer policy.
 * @details The base class owns the channel and value state; this derived
 * fixture adds no resources. The name and initial value are copied by the
 * native base constructor. It carries no callback or GIL requirements.
 */
template <class T, sc_core::sc_writer_policy POL>
class signal_t_access : public sc_core::sc_signal_t<T, POL>
{
  using base_type = sc_core::sc_signal_t<T, POL>;

 public:
  using base_type::operator=;

  /** @brief Construct the protected generic signal base as a native channel. */
  signal_t_access(const char* name, const T& initial) : base_type(name, initial)
  {
  }
};

/** @brief Complete bool signal interfaces using SystemC's native specialization.
 * @details sc_signal_t<bool,POL> is abstract because bool input interfaces add
 * edge methods. The public sc_signal specialization provides their native
 * implementations while retaining the signal_t base API under test.
 */
template <sc_core::sc_writer_policy POL>
class signal_t_access<bool, POL> : public sc_core::sc_signal<bool, POL>
{
  using base_type = sc_core::sc_signal<bool, POL>;

 public:
  using base_type::operator=;

  /** @brief Construct the native bool specialization through its signal_t base. */
  signal_t_access(const char* name, const bool& initial) : base_type(name, initial)
  {
  }
};

/** @brief Complete sc_logic signal interfaces through SystemC's specialization.
 * @details sc_signal_t<sc_logic,POL> is abstract because the logic input
 * interface adds edge methods. This uses the native sc_signal specialization.
 */
template <sc_core::sc_writer_policy POL>
class signal_t_access<sc_dt::sc_logic, POL> : public sc_core::sc_signal<sc_dt::sc_logic, POL>
{
  using base_type = sc_core::sc_signal<sc_dt::sc_logic, POL>;

 public:
  using base_type::operator=;

  /** @brief Construct the native logic specialization through its signal_t base. */
  signal_t_access(const char* name, const sc_dt::sc_logic& initial) : base_type(name, initial)
  {
  }
};

/** @brief Return the input-interface view needed to select a signal assignment overload.
 * @tparam T Native signal value type.
 * @param input An existing signal input interface.
 * @return A borrowed interface reference; the source channel must outlive it.
 * @details No ownership transfer occurs and no callbacks or GIL state are held.
 */
template <class T>
const sc_core::sc_signal_in_if<T>& input_view(const sc_core::sc_signal_in_if<T>& input)
{
  return input;
}

/** @brief Return sc_signal_t's native conversion-to-value reference.
 * @tparam T Native signal value type.
 * @tparam POL Native SystemC writer policy.
 * @param signal A live generic signal base.
 * @return A borrowed reference to the signal-owned current value. It is valid
 * only while the signal lives and can observe subsequent updates. Callers must
 * not retain it beyond the signal lifetime. No Python object or GIL state is
 * retained by this helper.
 */
template <class T, sc_core::sc_writer_policy POL>
const T& value_conversion(const sc_core::sc_signal_t<T, POL>& signal)
{
  return static_cast<const T&>(signal);
}

/** @brief Return the native address produced by sc_signal_t's value conversion.
 * @tparam T Native signal value type.
 * @tparam POL Native SystemC writer policy.
 * @param signal A live generic signal base.
 * @return Address within the signal-owned current value storage.
 * @details The address is invalidated by signal destruction and may identify
 * scalar storage even when cppyy returns a Python scalar copy.
 */
template <class T, sc_core::sc_writer_policy POL>
std::uintptr_t value_conversion_address(const sc_core::sc_signal_t<T, POL>& signal)
{
  return reinterpret_cast<std::uintptr_t>(&value_conversion<T, POL>(signal));
}

/** @brief Return the address from the deprecated tracing data-reference API.
 * @tparam T Native signal value type.
 * @tparam POL Native SystemC writer policy.
 * @param signal A live generic signal base.
 * @return Address within the signal-owned current value storage.
 */
template <class T, sc_core::sc_writer_policy POL>
std::uintptr_t data_ref_address(const sc_core::sc_signal_t<T, POL>& signal)
{
  return reinterpret_cast<std::uintptr_t>(&signal.get_data_ref());
}

}  // namespace signal_probe

#endif  // PYSYSTEMC_TEST_SC_CORE_SIGNALS_PROBE_H

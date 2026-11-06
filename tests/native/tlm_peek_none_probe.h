/** @file tlm_peek_none_probe.h
 *  @brief Concrete native reference for blocking combined get-peek interfaces.
 *  @details Probe instances own their sample and counters. Python interface
 *  views borrow those instances and must not outlive them. The probes retain no
 *  callbacks or Python references, acquire no GIL state, and never wait or
 *  advance the SystemC kernel.
 */
#ifndef PYSYSTEMC_TLM_PEEK_NONE_PROBE_H
#define PYSYSTEMC_TLM_PEEK_NONE_PROBE_H

#include <cstdint>
#include <systemc>
#include <tlm>

namespace tlm_peek_none
{

/** @brief Implement blocking get and peek for one arbitrary native value type.
 *  @tparam T Native TLM value type, including arbitrary SystemC widths.
 *  @details The object owns its stored sample and call counters. Interface
 *  pointers returned by this header borrow the object; retain it for the full
 *  lifetime of every view. Calls are synchronous, hold no Python callback and
 *  do not release or acquire the GIL.
 */
template <class T>
class PeekNoneProbe : public tlm::tlm_blocking_get_peek_if<T>
{
 public:
  using GetBase = tlm::tlm_blocking_get_if<T>;
  using PeekBase = tlm::tlm_blocking_peek_if<T>;
  using GetPeekBase = tlm::tlm_blocking_get_peek_if<T>;
  using GetBase::get;
  using PeekBase::peek;

  /** @brief Construct the probe with its owned sample. */
  explicit PeekNoneProbe(const T& value) : value_(value)
  {
  }

  /** @brief Return the sample and count a native blocking get call. */
  T get(tlm::tlm_tag<T>* = nullptr) override
  {
    ++get_calls_;
    return value_;
  }

  /** @brief Return the sample without consuming it and count the call. */
  T peek(tlm::tlm_tag<T>* = nullptr) const override
  {
    ++peek_calls_;
    return value_;
  }

  /** @brief Borrow the stored sample for native parity checks. */
  const T& value() const
  {
    return value_;
  }

  /** @brief Return the number of blocking peek calls. */
  int peek_calls() const
  {
    return peek_calls_;
  }

  /** @brief Return the number of blocking get calls. */
  int get_calls() const
  {
    return get_calls_;
  }

 private:
  T value_;
  mutable int peek_calls_ = 0;
  int get_calls_ = 0;
};

/** @brief Return the pointer-adjusted borrowed combined-interface address.
 *  @param probe Live owner retained by the caller.
 *  @return Address of its ``tlm_blocking_get_peek_if<T>`` virtual base.
 *  @details The address is borrowed, ownership is unchanged, and the function
 *  invokes no Python code or scheduler operation.
 */
template <class T>
std::uintptr_t combined_interface_address(PeekNoneProbe<T>& probe)
{
  return reinterpret_cast<std::uintptr_t>(static_cast<tlm::tlm_blocking_get_peek_if<T>*>(&probe));
}

/** @brief Call native peek with an explicitly typed null tag pointer.
 *  @tparam T Native TLM value type.
 *  @param interface Borrowed blocking peek interface whose owner remains live.
 *  @return A copied value returned by the native interface.
 *  @details The call is synchronous, retains no storage and does not invoke
 *  Python callbacks, acquire the GIL, wait, or advance simulation.
 */
template <class T>
T typed_nullptr_peek(tlm::tlm_blocking_peek_if<T>& interface)
{
  return interface.peek(static_cast<tlm::tlm_tag<T>*>(nullptr));
}

}  // namespace tlm_peek_none

#endif  // PYSYSTEMC_TLM_PEEK_NONE_PROBE_H

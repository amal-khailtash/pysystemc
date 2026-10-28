/** @file Guard native TLM FIFO suspension across Python process frames. */
#pragma once

#include <utility>

namespace pysystemc_interfaces
{
/** @brief Invoke exact public blocking interfaces for arbitrary FIFO value types.
 * @tparam T Native default-constructible, copyable FIFO value type.
 * @details Channels, tags and input/output storage remain borrowed throughout
 * suspension. Callers release the GIL on entry and retain their arguments.
 * Native allocation, conversion and callback errors propagate. False completion
 * requires Python frame cleanup before the shared scheduler adapter rethrows
 * the original reset/kill exception. No kernel state is reset or duplicated.
 */
template <class T>
struct guarded_tlm_fifo
{
  /** @brief Consume a native value, owning its result until Python receives it. */
  static std::pair<T, bool> get_value(tlm::tlm_blocking_get_if<T>& channel, tlm::tlm_tag<T>* tag = nullptr)
  {
    T value{};
    const bool complete = sc_core::python_guarded_operation([&] { value = channel.get(tag); });
    return {value, complete};
  }

  /** @brief Consume into borrowed caller storage through the exact output form. */
  static bool get_output(tlm::tlm_blocking_get_if<T>& channel, T& output)
  {
    return sc_core::python_guarded_operation([&] { channel.get(output); });
  }

  /** @brief Copy a native peek value without consuming the queued transaction. */
  static std::pair<T, bool> peek_value(const tlm::tlm_blocking_peek_if<T>& channel, tlm::tlm_tag<T>* tag = nullptr)
  {
    T value{};
    const bool complete = sc_core::python_guarded_operation([&] { value = channel.peek(tag); });
    return {value, complete};
  }

  /** @brief Peek into borrowed caller storage without consuming a transaction. */
  static bool peek_output(const tlm::tlm_blocking_peek_if<T>& channel, T& output)
  {
    return sc_core::python_guarded_operation([&] { channel.peek(output); });
  }

  /** @brief Copy a borrowed input into the native FIFO after possible suspension. */
  static bool put(tlm::tlm_blocking_put_if<T>& channel, const T& input)
  {
    return sc_core::python_guarded_operation([&] { channel.put(input); });
  }
};
}  // namespace pysystemc_interfaces

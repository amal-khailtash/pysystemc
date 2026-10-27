/** @file sc_core_blocking_completion.h
 * @brief Build C++-only operations for guarded channel-internal wait coverage.
 * @details Factories borrow native channels and output buffers, and copy input samples.
 * Python must retain each referenced object and the returned operation until
 * native_blocking_call completes. The operator never calls back into Python.
 */
#pragma once

#include <functional>
#include <systemc>
#include <tlm>
#include <utility>

namespace pysystemc_blocking_completion
{
/** @brief Own a C++ call closure while borrowing its channel and arguments. */
struct NativeOperation
{
  std::function<void()> call;

  /** @brief Copy a C++ closure that already captured native references. */
  explicit NativeOperation(std::function<void()> operation) : call(std::move(operation))
  {
  }

  /** @brief Invoke the stored C++ channel operation without Python callbacks. */
  void operator()() const
  {
    call();
  }
};

/** @brief Capture native FIFO value read and discard its result after completion. */
template <class T>
NativeOperation core_fifo_read_value(sc_core::sc_fifo<T>& channel)
{
  return NativeOperation([&channel] { (void)channel.read(); });
}

/** @brief Capture native FIFO output read into borrowed caller storage. */
template <class T>
NativeOperation core_fifo_read_output(sc_core::sc_fifo<T>& channel, T& output)
{
  return NativeOperation([&channel, &output] { channel.read(output); });
}

/** @brief Copy a native FIFO input into the closure, borrowing only the channel. */
template <class T>
NativeOperation core_fifo_write(sc_core::sc_fifo<T>& channel, const T& input)
{
  return NativeOperation([&channel, input] { channel.write(input); });
}

/** @brief Capture a native TLM FIFO value get with a borrowed optional tag. */
template <class T>
NativeOperation tlm_fifo_get_value(tlm::tlm_fifo<T>& channel, tlm::tlm_tag<T>* tag)
{
  return NativeOperation([&channel, tag] { (void)channel.get(tag); });
}

/** @brief Capture native TLM FIFO get through its exact output interface. */
template <class T>
NativeOperation tlm_fifo_get_output(tlm::tlm_fifo<T>& channel, T& output)
{
  return NativeOperation([&channel, &output] { static_cast<tlm::tlm_blocking_get_if<T>&>(channel).get(output); });
}

/** @brief Capture a native TLM FIFO value peek with a borrowed optional tag. */
template <class T>
NativeOperation tlm_fifo_peek_value(tlm::tlm_fifo<T>& channel, tlm::tlm_tag<T>* tag)
{
  return NativeOperation([&channel, tag] { (void)channel.peek(tag); });
}

/** @brief Capture native TLM FIFO peek through its exact output interface. */
template <class T>
NativeOperation tlm_fifo_peek_output(tlm::tlm_fifo<T>& channel, T& output)
{
  return NativeOperation([&channel, &output] { static_cast<tlm::tlm_blocking_peek_if<T>&>(channel).peek(output); });
}

/** @brief Capture output peek through a borrowed generic blocking interface. */
template <class T>
NativeOperation tlm_interface_peek_output(tlm::tlm_blocking_peek_if<T>& channel, T& output)
{
  return NativeOperation([&channel, &output] { channel.peek(output); });
}

/** @brief Copy a blocking TLM FIFO input into the closure, borrowing only the channel. */
template <class T>
NativeOperation tlm_fifo_put(tlm::tlm_fifo<T>& channel, const T& input)
{
  return NativeOperation([&channel, input] { channel.put(input); });
}

/** @brief Capture scoped-lock construction on a borrowed mutex interface. */
inline NativeOperation scoped_lock(sc_core::sc_mutex_if& mutex)
{
  return NativeOperation([&mutex] { sc_core::sc_scoped_lock lock(mutex); });
}
}  // namespace pysystemc_blocking_completion

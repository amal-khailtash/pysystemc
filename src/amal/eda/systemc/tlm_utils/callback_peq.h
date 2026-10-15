/** @file Native member-pointer bridge for Python payload queue callbacks. */
#include <tlm_utils/peq_with_cb_and_phase.h>

#include <functional>

namespace pysystemc_interfaces
{
/** @brief Own native PEQ and a callback for arbitrary protocol types.
 * @details Python retains this object and the callable for the kernel process
 * lifetime. Payloads are borrowed until delivery/cancellation; phases are copied
 * by the native queue. cppyy acquires GIL on callback entry. Callback errors
 * propagate through the kernel; method callbacks must not wait.
 */
template <class Types = tlm::tlm_base_protocol_types>
class CallbackPeq
{
 public:
  using Payload = typename Types::tlm_payload_type;
  using Phase = typename Types::tlm_phase_type;
  using Callback = std::function<void(Payload&, const Phase&)>;
  /** @brief Retain callback and register native queue process before simulation. */
  CallbackPeq(const char* name, Callback callback)
    : callback(std::move(callback)), queue(name, this, &CallbackPeq::deliver)
  {
  }
  /** @brief Borrow payload and copy phase for timed native delivery. */
  void notify(Payload& payload, const Phase& phase, const sc_core::sc_time& delay)
  {
    queue.notify(payload, phase, delay);
  }
  /** @brief Borrow payload and copy phase for immediate native notification. */
  void notify(Payload& payload, const Phase& phase) { queue.notify(payload, phase); }
  /** @brief Cancel pending deliveries without destroying caller-owned payloads. */
  void cancel_all() { queue.cancel_all(); }

 private:
  /** @brief Invoke retained callback with borrowed payload and native phase. */
  void deliver(Payload& payload, const Phase& phase) { callback(payload, phase); }
  Callback callback;
  tlm_utils::peq_with_cb_and_phase<CallbackPeq, Types> queue;
};
}  // namespace pysystemc_interfaces

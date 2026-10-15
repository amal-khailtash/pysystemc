/** @file Member-pointer adapters for native convenience socket registration. */
#include <tlm_utils/simple_target_socket.h>

#include <functional>
#include <limits>
#include <stdexcept>

namespace pysystemc_interfaces
{
/** @brief Retain callable transport callbacks independently of native MODULE types.
 * @details Python retains this bridge and callbacks for the socket/kernel lifetime.
 * Payloads and delays are borrowed. cppyy acquires GIL on callback entry; blocking
 * native callers release GIL. Callback errors propagate and guarded waits preserve unwind.
 */
template <class Types>
struct SocketCallbacks
{
  using Payload = typename Types::tlm_payload_type;
  using Phase = typename Types::tlm_phase_type;
  std::function<void(Payload&, sc_core::sc_time&)> blocking;
  std::function<tlm::tlm_sync_enum(Payload&, Phase&, sc_core::sc_time&)> nonblocking;
  std::function<long(Payload&)> debug;
  std::function<bool(Payload&, tlm::tlm_dmi&)> dmi;
  /** @brief Invoke borrowed payload/delay callback through a native member pointer. */
  void b_transport(Payload& payload, sc_core::sc_time& delay) { blocking(payload, delay); }
  /** @brief Forward mutable phase/delay references without suspension. */
  tlm::tlm_sync_enum nb_transport_fw(Payload& payload, Phase& phase, sc_core::sc_time& delay)
  {
    return nonblocking(payload, phase, delay);
  }
  /** @brief Validate Python debug counts before native unsigned conversion. */
  unsigned int transport_dbg(Payload& payload)
  {
    auto count = debug(payload);
    if (count < 0 || static_cast<unsigned long>(count) > std::numeric_limits<unsigned int>::max())
      throw std::out_of_range("transport_dbg result outside unsigned int range");
    return static_cast<unsigned int>(count);
  }
  /** @brief Forward borrowed DMI output; caller retains granted memory. */
  bool get_direct_mem_ptr(Payload& payload, tlm::tlm_dmi& result) { return dmi(payload, result); }
};
/** @brief Socket retains native behavior while accepting a callable blocking registration.
 * @details Width, protocol and port policy remain arbitrary native template parameters.
 * Python must retain callback owner; socket borrows bridge until shutdown.
 */
template <unsigned int Width = 32, class Types = tlm::tlm_base_protocol_types,
          sc_core::sc_port_policy Policy = sc_core::SC_ONE_OR_MORE_BOUND>
class CallbackTargetSocket : public tlm_utils::simple_target_socket_b<SocketCallbacks<Types>, Width, Types, Policy>
{
  SocketCallbacks<Types> callbacks;

 public:
  using Base = tlm_utils::simple_target_socket_b<SocketCallbacks<Types>, Width, Types, Policy>;
  /** @brief Construct native socket during elaboration. */
  explicit CallbackTargetSocket(const char* name) : Base(name) {}
  /** @brief Register callable using upstream member-pointer registration checks. */
  void register_b_transport(std::function<void(typename Types::tlm_payload_type&, sc_core::sc_time&)> callback)
  {
    Base::register_b_transport(&callbacks, &SocketCallbacks<Types>::b_transport);
    callbacks.blocking = std::move(callback);
  }
  /** @brief Register nonblocking forward callback through upstream checks. */
  void register_nb_transport_fw(std::function<tlm::tlm_sync_enum(typename Types::tlm_payload_type&,
                                                                 typename Types::tlm_phase_type&, sc_core::sc_time&)>
                                  callback)
  {
    Base::register_nb_transport_fw(&callbacks, &SocketCallbacks<Types>::nb_transport_fw);
    callbacks.nonblocking = std::move(callback);
  }
  /** @brief Register range-checked debug callback; it must not wait. */
  void register_transport_dbg(std::function<long(typename Types::tlm_payload_type&)> callback)
  {
    Base::register_transport_dbg(&callbacks, &SocketCallbacks<Types>::transport_dbg);
    callbacks.debug = std::move(callback);
  }
  /** @brief Register DMI callback with borrowed transaction/output references. */
  void register_get_direct_mem_ptr(std::function<bool(typename Types::tlm_payload_type&, tlm::tlm_dmi&)> callback)
  {
    Base::register_get_direct_mem_ptr(&callbacks, &SocketCallbacks<Types>::get_direct_mem_ptr);
    callbacks.dmi = std::move(callback);
  }
};
}  // namespace pysystemc_interfaces

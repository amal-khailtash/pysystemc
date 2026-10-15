/** @file Member-pointer adapters for native convenience socket registration. */
#include <tlm_utils/simple_target_socket.h>

#include <functional>

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
  std::function<void(Payload&, sc_core::sc_time&)> blocking;
  /** @brief Invoke borrowed payload/delay callback through a native member pointer. */
  void b_transport(Payload& payload, sc_core::sc_time& delay) { blocking(payload, delay); }
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
};
}  // namespace pysystemc_interfaces

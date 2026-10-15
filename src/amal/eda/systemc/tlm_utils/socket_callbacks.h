/** @file Member-pointer adapters for native convenience socket registration. */
#include <tlm_utils/simple_initiator_socket.h>
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
  std::function<void(sc_dt::uint64, sc_dt::uint64)> invalidate;

  /** @brief Invoke borrowed payload/delay callback through a native member pointer. */
  void b_transport(Payload& payload, sc_core::sc_time& delay)
  {
    blocking(payload, delay);
  }

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
  bool get_direct_mem_ptr(Payload& payload, tlm::tlm_dmi& result)
  {
    return dmi(payload, result);
  }

  /** @brief Forward backward response with mutable native phase/delay. */
  tlm::tlm_sync_enum nb_transport_bw(Payload& payload, Phase& phase, sc_core::sc_time& delay)
  {
    return nonblocking(payload, phase, delay);
  }

  /** @brief Forward full-width invalidation range without requesting DMI. */
  void invalidate_direct_mem_ptr(sc_dt::uint64 first, sc_dt::uint64 last)
  {
    invalidate(first, last);
  }
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
  explicit CallbackTargetSocket(const char* name) : Base(name)
  {
  }

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

/** @brief Native tagged target forwarding a registered ID to blocking callbacks.
 * @details Python retains callback/socket through kernel lifetime. Payload and
 * delay are borrowed; caller releases GIL, cppyy acquires it for callback entry.
 * Exceptions and guarded reset unwind propagate. Width/types/policy stay native.
 */
template <unsigned int Width = 32, class Types = tlm::tlm_base_protocol_types,
          sc_core::sc_port_policy Policy = sc_core::SC_ONE_OR_MORE_BOUND>
class CallbackTaggedTargetSocket
  : public tlm_utils::simple_target_socket_tagged_b<CallbackTaggedTargetSocket<Width, Types, Policy>, Width, Types,
                                                    Policy>
{
  using Payload = typename Types::tlm_payload_type;
  std::function<void(int, Payload&, sc_core::sc_time&)> callback;
  std::function<long(int, Payload&)> debug;
  std::function<bool(int, Payload&, tlm::tlm_dmi&)> dmi;

 public:
  using Base = tlm_utils::simple_target_socket_tagged_b<CallbackTaggedTargetSocket, Width, Types, Policy>;

  /** @brief Construct native tagged socket before simulation. */
  explicit CallbackTaggedTargetSocket(const char* name) : Base(name)
  {
  }

  /** @brief Register callable and user ID through upstream registration checks. */
  void register_b_transport(std::function<void(int, Payload&, sc_core::sc_time&)> function, int id)
  {
    Base::register_b_transport(this, &CallbackTaggedTargetSocket::deliver, id);
    callback = std::move(function);
  }

  /** @brief Forward tagged borrowed transport arguments without copying payload. */
  void deliver(int id, Payload& payload, sc_core::sc_time& delay)
  {
    callback(id, payload, delay);
  }

  /** @brief Register debug callable with independently selected native user ID. */
  void register_transport_dbg(std::function<long(int, Payload&)> function, int id)
  {
    Base::register_transport_dbg(this, &CallbackTaggedTargetSocket::debug_transport, id);
    debug = std::move(function);
  }

  /** @brief Forward tagged debug calls and reject counts outside unsigned range. */
  unsigned int debug_transport(int id, Payload& payload)
  {
    auto count = debug(id, payload);
    if (count < 0 || static_cast<unsigned long>(count) > std::numeric_limits<unsigned int>::max())
      throw std::out_of_range("transport_dbg result outside unsigned int range");
    return static_cast<unsigned int>(count);
  }

  /** @brief Register DMI callable and independent native user ID. */
  void register_get_direct_mem_ptr(std::function<bool(int, Payload&, tlm::tlm_dmi&)> function, int id)
  {
    Base::register_get_direct_mem_ptr(this, &CallbackTaggedTargetSocket::direct_memory, id);
    dmi = std::move(function);
  }

  /** @brief Forward borrowed DMI output; callback owner retains any granted memory. */
  bool direct_memory(int id, Payload& payload, tlm::tlm_dmi& result)
  {
    return dmi(id, payload, result);
  }
};

/** @brief Native simple initiator socket accepting retained Python callbacks.
 * @details Python retains callback owners until shutdown. Protocol, width and
 * policy remain native templates; backward callbacks must not wait. GIL is
 * acquired by cppyy on callback entry; errors propagate to the native caller.
 */
template <unsigned int Width = 32, class Types = tlm::tlm_base_protocol_types,
          sc_core::sc_port_policy Policy = sc_core::SC_ONE_OR_MORE_BOUND>
class CallbackInitiatorSocket
  : public tlm_utils::simple_initiator_socket_b<SocketCallbacks<Types>, Width, Types, Policy>
{
  SocketCallbacks<Types> callbacks;

 public:
  using Base = tlm_utils::simple_initiator_socket_b<SocketCallbacks<Types>, Width, Types, Policy>;

  /** @brief Construct during native elaboration. */
  explicit CallbackInitiatorSocket(const char* name) : Base(name)
  {
  }

  /** @brief Register response callback through native member-pointer checks. */
  void register_nb_transport_bw(std::function<tlm::tlm_sync_enum(typename Types::tlm_payload_type&,
                                                                 typename Types::tlm_phase_type&, sc_core::sc_time&)>
                                  callback)
  {
    Base::register_nb_transport_bw(&callbacks, &SocketCallbacks<Types>::nb_transport_bw);
    callbacks.nonblocking = std::move(callback);
  }

  /** @brief Register range invalidation callback without borrowing memory buffers. */
  void register_invalidate_direct_mem_ptr(std::function<void(sc_dt::uint64, sc_dt::uint64)> callback)
  {
    Base::register_invalidate_direct_mem_ptr(&callbacks, &SocketCallbacks<Types>::invalidate_direct_mem_ptr);
    callbacks.invalidate = std::move(callback);
  }
};
}  // namespace pysystemc_interfaces

/** @file tlm_covariant_socket_probe.h
 * @brief Exact opt-in abstract socket interface probes.
 * @details All returned interfaces borrow their module owner; no Python
 * callback or GIL transition occurs. Compile every translation unit and the
 * linked SystemC library with SC_ENABLE_COVARIANT_VIRTUAL_BASE enabled.
 */
#ifndef PYSYSTEMC_TLM_COVARIANT_SOCKET_PROBE_H
#define PYSYSTEMC_TLM_COVARIANT_SOCKET_PROBE_H
#ifndef SC_ENABLE_COVARIANT_VIRTUAL_BASE
#error "This probe requires a consistently configured covariant SystemC build"
#endif
#include "tlm_native_sockets_probe.h"

using TlmCovariantMutableGetter = sc_core::sc_interface& (tlm::tlm_base_socket_if::*)();
using TlmCovariantConstGetter = const sc_core::sc_interface& (tlm::tlm_base_socket_if::*)() const;
static_assert(
  std::is_same<decltype(static_cast<TlmCovariantMutableGetter>(&tlm::tlm_base_socket_if::get_base_interface)),
               TlmCovariantMutableGetter>::value,
  "exact mutable abstract getter");
static_assert(std::is_same<decltype(static_cast<TlmCovariantConstGetter>(&tlm::tlm_base_socket_if::get_base_interface)),
                           TlmCovariantConstGetter>::value,
              "exact const abstract getter");

/** @brief Invoke the exact mutable virtual member through the abstract base.
 * @return Borrowed interface, valid only while its bound endpoint is alive.
 */
inline sc_core::sc_interface& tlm_covariant_mutable(tlm::tlm_base_socket_if& socket)
{
  auto getter = static_cast<TlmCovariantMutableGetter>(&tlm::tlm_base_socket_if::get_base_interface);
  return (socket.*getter)();
}
/** @brief Invoke the exact const virtual member through the abstract base.
 * @return Borrowed const interface; no ownership transfer or allocation.
 */
inline const sc_core::sc_interface& tlm_covariant_const(const tlm::tlm_base_socket_if& socket)
{
  auto getter = static_cast<TlmCovariantConstGetter>(&tlm::tlm_base_socket_if::get_base_interface);
  return (socket.*getter)();
}

/** @brief Return the borrowed abstract socket subobject for an exact Python proxy. */
inline std::uintptr_t tlm_covariant_base_address(tlm::tlm_base_socket_if& socket)
{
  return reinterpret_cast<std::uintptr_t>(&socket);
}

/** @brief Return a borrowed interface subobject address after native conversion. */
inline std::uintptr_t tlm_covariant_interface_address(const sc_core::sc_interface& interface)
{
  return reinterpret_cast<std::uintptr_t>(&interface);
}

/** @brief Default-protocol endpoint whose lifetime belongs to its module. */
struct TlmCovariantDefaultEndpoint : sc_core::sc_channel, tlm::tlm_fw_transport_if<>, tlm::tlm_bw_transport_if<>
{
  /** @brief Construct a module-owned endpoint without retaining callbacks. */
  explicit TlmCovariantDefaultEndpoint(sc_core::sc_module_name name) : sc_core::sc_channel(name)
  {
  }
  /** @brief Complete a native forward request without retaining arguments. */
  tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload&, tlm::tlm_phase&, sc_core::sc_time&) override
  {
    return tlm::TLM_COMPLETED;
  }
  /** @brief Accept a native blocking request without retaining arguments. */
  void b_transport(tlm::tlm_generic_payload&, sc_core::sc_time&) override
  {
  }
  /** @brief Deny direct memory access. */
  bool get_direct_mem_ptr(tlm::tlm_generic_payload&, tlm::tlm_dmi&) override
  {
    return false;
  }
  /** @brief Return a zero-byte debug response. */
  unsigned int transport_dbg(tlm::tlm_generic_payload&) override
  {
    return 0;
  }
  /** @brief Complete a native backward request. */
  tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload&, tlm::tlm_phase&, sc_core::sc_time&) override
  {
    return tlm::TLM_COMPLETED;
  }
  /** @brief Accept an invalidation without retaining the range. */
  void invalidate_direct_mem_ptr(sc_dt::uint64, sc_dt::uint64) override
  {
  }
};

/** @brief Own default and arbitrary custom protocol sockets through simulation. */
struct TlmCovariantOwner : sc_core::sc_module
{
  TlmCovariantDefaultEndpoint endpoint;
  tlm::tlm_initiator_socket<32, tlm::tlm_base_protocol_types, 1, sc_core::SC_ZERO_OR_MORE_BOUND> initiator;
  tlm::tlm_target_socket<32, tlm::tlm_base_protocol_types, 1, sc_core::SC_ZERO_OR_MORE_BOUND> target;
  TlmNativeSocketsEndpoint custom_endpoint;
  TlmNativeInitiator custom_initiator;
  TlmNativeTarget custom_target;
  static inline int live_count = 0;
  /** @brief Bind endpoints once before elaboration; retain them as members. */
  explicit TlmCovariantOwner(sc_core::sc_module_name name)
    : sc_module(name)
    , endpoint("endpoint")
    , initiator("initiator")
    , target("target")
    , custom_endpoint("custom_endpoint")
    , custom_initiator("custom_initiator")
    , custom_target("custom_target")
  {
    initiator.bind(static_cast<tlm::tlm_bw_transport_if<>&>(endpoint));
    target.bind(static_cast<tlm::tlm_fw_transport_if<>&>(endpoint));
    custom_initiator.bind(static_cast<TlmNativeSocketsBackward&>(custom_endpoint));
    custom_target.bind(static_cast<TlmNativeSocketsForward&>(custom_endpoint));
    ++live_count;
  }
  /** @brief Account for explicit owner destruction after simulation stops. */
  ~TlmCovariantOwner() override
  {
    --live_count;
  }
  /** @brief Check cv-qualified reference identity without copying interfaces. */
  template <typename Socket>
  static bool identity(Socket& socket)
  {
    return &tlm_covariant_mutable(socket) == &socket.get_base_interface() &&
           &tlm_covariant_const(socket) == &static_cast<const Socket&>(socket).get_base_interface();
  }
  /** @brief Verify all four socket/transport orientations through the base. */
  bool identities()
  {
    return identity(initiator) && identity(target) && identity(custom_initiator) && identity(custom_target) &&
           &tlm_covariant_mutable(initiator) == static_cast<sc_core::sc_interface*>(&endpoint) &&
           &tlm_covariant_mutable(target) == static_cast<sc_core::sc_interface*>(&endpoint) &&
           &tlm_covariant_mutable(custom_initiator) == static_cast<sc_core::sc_interface*>(&custom_endpoint) &&
           &tlm_covariant_mutable(custom_target) == static_cast<sc_core::sc_interface*>(&custom_endpoint);
  }
};
#endif

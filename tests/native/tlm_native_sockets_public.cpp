/** @file tlm_native_sockets_public.cpp
 *  @brief Native reference for Python's public TLM socket binding matrix.
 *  @details Socket ports and exports belong to their modules. Test transactions
 *  remain caller-owned, and native callback invocation stays on the kernel
 *  thread without entering Python.
 */
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include "tlm_native_sockets_probe.h"

/** @brief Throw a native test failure with a focused message. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Emit portable socket metadata shared with the Python model. */
template <typename Socket>
void emit_socket(const char* kind, const Socket& socket, tlm::tlm_socket_category category)
{
  const auto width = socket.get_bus_width();
  const auto observed_category = socket.get_socket_category();
  const auto count = socket.get_base_port().size();
  verify(width == 73, "custom socket bus width");
  verify(observed_category == category, "socket category");
  verify(socket.get_protocol_types() == TlmNativeSocketsOwner::protocol_index(), "socket protocol type");
  verify(socket.kind() == std::string(kind), "socket kind");
  std::cout << "SOCKET_NATIVE_REC|" << kind << '|' << width << '|' << static_cast<int>(observed_category) << '|'
            << count << '|' << socket.kind() << '\n';
}

/** @brief Compare cv-qualified abstract getters with their concrete entries. */
template <typename Socket>
void verify_socket_if(const Socket& socket)
{
  verify(TlmNativeSocketsOwner::interface_port(const_cast<Socket&>(socket)) ==
           TlmNativeSocketsOwner::const_interface_port(socket),
         "abstract port getter cv dispatch");
  verify(TlmNativeSocketsOwner::interface_export(const_cast<Socket&>(socket)) ==
           TlmNativeSocketsOwner::const_interface_export(socket),
         "abstract export getter cv dispatch");
}

/** @brief Construct sockets, execute every overload and compare interface traffic. */
int sc_main(int, char**)
{
  {
    TlmNativeSocketsOwner owner("top");
    for (auto* socket : {&owner.base_initiator_default, &owner.base_initiator_named}) socket->bind(owner.endpoint);
    for (auto* socket : {&owner.base_target_default, &owner.base_target_named}) socket->bind(owner.endpoint);
    for (auto* socket : {&owner.initiator_default, &owner.initiator_named}) socket->bind(owner.endpoint);
    for (auto* socket : {&owner.target_default, &owner.target_named}) socket->bind(owner.endpoint);

    verify(TlmNativeSocketsOwner::live_count == 1, "owner constructed");
    verify_socket_if(owner.base_initiator_default);
    verify_socket_if(owner.base_target_default);
    verify_socket_if(owner.initiator_default);
    verify_socket_if(owner.target_default);
    verify(TlmNativeSocketsOwner::const_initiator_port(owner.base_initiator_default) ==
             TlmNativeSocketsOwner::interface_port(owner.base_initiator_default),
           "initiator port cv overload");
    verify(TlmNativeSocketsOwner::const_initiator_export(owner.base_initiator_default) ==
             TlmNativeSocketsOwner::interface_export(owner.base_initiator_default),
           "initiator export cv overload");
    verify(TlmNativeSocketsOwner::const_initiator_interface(owner.base_initiator_default) ==
             TlmNativeSocketsOwner::initiator_interface(owner.base_initiator_default),
           "initiator interface cv overload");
    verify(TlmNativeSocketsOwner::const_target_port(owner.base_target_default) ==
             TlmNativeSocketsOwner::interface_port(owner.base_target_default),
           "target port cv overload");
    verify(TlmNativeSocketsOwner::const_target_export(owner.base_target_default) ==
             TlmNativeSocketsOwner::interface_export(owner.base_target_default),
           "target export cv overload");
    verify(TlmNativeSocketsOwner::const_target_interface(owner.base_target_default) ==
             TlmNativeSocketsOwner::target_interface(owner.base_target_default),
           "target interface cv overload");

    emit_socket("tlm_base_initiator_socket", owner.base_initiator_default, tlm::TLM_INITIATOR_SOCKET);
    emit_socket("tlm_base_target_socket", owner.base_target_default, tlm::TLM_TARGET_SOCKET);
    emit_socket("tlm_initiator_socket", owner.initiator_default, tlm::TLM_INITIATOR_SOCKET);
    emit_socket("tlm_target_socket", owner.target_default, tlm::TLM_TARGET_SOCKET);
    emit_socket("tlm_base_initiator_socket", owner.base_initiator_named, tlm::TLM_INITIATOR_SOCKET);
    emit_socket("tlm_base_target_socket", owner.base_target_named, tlm::TLM_TARGET_SOCKET);
    emit_socket("tlm_initiator_socket", owner.initiator_named, tlm::TLM_INITIATOR_SOCKET);
    emit_socket("tlm_target_socket", owner.target_named, tlm::TLM_TARGET_SOCKET);
    verify(std::string(owner.base_initiator_named.basename()) == "base_initiator_named", "named base initiator");
    verify(std::string(owner.base_target_named.basename()) == "base_target_named", "named base target");
    verify(std::string(owner.initiator_named.basename()) == "initiator_named", "named initiator");
    verify(std::string(owner.target_named.basename()) == "target_named", "named target");

    auto& bindings = owner.bindings;
    bindings.bind_all();
    verify(bindings.target_socket_bind.size() == 1, "target bind size");
    verify(TlmNativeSocketsOwner::target_arrow(bindings.target_socket_bind) ==
             TlmNativeSocketsOwner::target_index(bindings.target_socket_bind, 0),
           "target arrow/index dispatch");

    TlmNativeSocketsPayload payload;
    payload.set_address(37);
    payload.value = (sc_dt::sc_biguint<137>(1) << 136) | 0xA5;
    verify(payload.value[136] && payload.value.to_uint64() == 0xA5, "137-bit payload value");
    auto* forward_pointer =
      dynamic_cast<TlmNativeSocketsForward*>(bindings.initiator_target_bind.get_base_port().get_interface());
    verify(forward_pointer != nullptr, "forward interface binding");
    auto& forward = *forward_pointer;
    auto& backward = *bindings.target_socket_bind.operator[](0);
    tlm::tlm_phase phase = tlm::BEGIN_REQ;
    sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
    verify(forward.nb_transport_fw(payload, phase, delay) == tlm::TLM_UPDATED, "forward transport status");
    verify(phase == tlm::BEGIN_RESP, "forward response phase");
    forward.b_transport(payload, delay);
    tlm::tlm_dmi dmi;
    verify(!forward.get_direct_mem_ptr(payload, dmi), "DMI denial callback");
    verify(forward.transport_dbg(payload) == 0, "debug callback");
    phase = tlm::BEGIN_RESP;
    verify(backward.nb_transport_bw(payload, phase, delay) == tlm::TLM_COMPLETED, "backward transport status");
    verify(phase == tlm::END_RESP, "backward completion phase");
    backward.invalidate_direct_mem_ptr(0x1000000000000000ULL, 0xF000000000000000ULL);
    verify(bindings.endpoint.calls == 6, "all custom protocol methods invoked");
    std::cout << "SOCKET_BIND_REC|" << bindings.target_socket_bind.size() << '|' << static_cast<int>(phase) << '|'
              << bindings.endpoint.calls << '\n';

    sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_NS));
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(1, sc_core::SC_NS), "simulation time");
    sc_core::sc_stop();
  }
  verify(TlmNativeSocketsOwner::live_count == 0, "owner destruction");
  std::cout << "TLM_NATIVE_SOCKETS_NATIVE_OK\n";
  return 0;
}

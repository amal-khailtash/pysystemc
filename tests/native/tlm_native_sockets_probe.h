/** @file tlm_native_sockets_probe.h
 *  @brief Native probes for the public SystemC TLM socket declaration matrix.
 *  @details Every interface and socket borrows the probe module and its
 *  endpoint for the complete simulation. No callback enters Python; the
 *  kernel invokes endpoint methods on its own thread.
 */
#ifndef PYSYSTEMC_TEST_TLM_NATIVE_SOCKETS_PROBE_H
#define PYSYSTEMC_TEST_TLM_NATIVE_SOCKETS_PROBE_H

#include <cstdint>
#include <systemc>
#include <tlm>
#include <type_traits>
#include <utility>

/** @brief Payload that exercises a value wider than a native machine word. */
struct TlmNativeSocketsPayload : tlm::tlm_generic_payload
{
  sc_dt::sc_biguint<137> value = 0;
};

/** @brief Non-default protocol carried by every test socket specialization. */
struct TlmNativeSocketsProtocol
{
  using tlm_payload_type = TlmNativeSocketsPayload;
  using tlm_phase_type = tlm::tlm_phase;
};

using TlmNativeSocketsForward = tlm::tlm_fw_transport_if<TlmNativeSocketsProtocol>;
using TlmNativeSocketsBackward = tlm::tlm_bw_transport_if<TlmNativeSocketsProtocol>;

using TlmNativeBaseInitiatorB = tlm::tlm_base_initiator_socket_b<73, TlmNativeSocketsForward, TlmNativeSocketsBackward>;
using TlmNativeBaseTargetB = tlm::tlm_base_target_socket_b<73, TlmNativeSocketsForward, TlmNativeSocketsBackward>;
using TlmNativeBaseInitiator = tlm::tlm_base_initiator_socket<73, TlmNativeSocketsForward, TlmNativeSocketsBackward, 2,
                                                              sc_core::SC_ZERO_OR_MORE_BOUND>;
using TlmNativeBaseTarget =
  tlm::tlm_base_target_socket<73, TlmNativeSocketsForward, TlmNativeSocketsBackward, 2, sc_core::SC_ZERO_OR_MORE_BOUND>;
using TlmNativeInitiator = tlm::tlm_initiator_socket<73, TlmNativeSocketsProtocol, 2, sc_core::SC_ZERO_OR_MORE_BOUND>;
using TlmNativeTarget = tlm::tlm_target_socket<73, TlmNativeSocketsProtocol, 2, sc_core::SC_ZERO_OR_MORE_BOUND>;

static_assert(std::is_abstract<tlm::tlm_base_socket_if>::value, "socket interface remains abstract");
static_assert(std::has_virtual_destructor<tlm::tlm_base_socket_if>::value, "socket interface virtual destructor");
static_assert(std::is_abstract<TlmNativeBaseInitiatorB>::value, "base initiator interface remains abstract");
static_assert(std::has_virtual_destructor<TlmNativeBaseInitiatorB>::value, "base initiator virtual destructor");
static_assert(std::is_abstract<TlmNativeBaseTargetB>::value, "base target interface remains abstract");
static_assert(std::has_virtual_destructor<TlmNativeBaseTargetB>::value, "base target virtual destructor");
static_assert(std::is_base_of<TlmNativeBaseInitiator, TlmNativeInitiator>::value,
              "convenience initiator inherits arbitrary base specialization");
static_assert(std::is_base_of<TlmNativeBaseTarget, TlmNativeTarget>::value,
              "convenience target inherits arbitrary base specialization");

static_assert(std::is_same<decltype(std::declval<TlmNativeBaseInitiator&>().get_base_port()),
                           sc_core::sc_port_b<TlmNativeSocketsForward>&>::value,
              "non-const initiator port getter");
static_assert(std::is_same<decltype(std::declval<const TlmNativeBaseInitiator&>().get_base_port()),
                           const sc_core::sc_port_b<TlmNativeSocketsForward>&>::value,
              "const initiator port getter");
static_assert(std::is_same<decltype(std::declval<TlmNativeBaseInitiator&>().get_base_export()),
                           sc_core::sc_export<TlmNativeSocketsBackward>&>::value,
              "non-const initiator export getter");
static_assert(std::is_same<decltype(std::declval<const TlmNativeBaseInitiator&>().get_base_export()),
                           const sc_core::sc_export<TlmNativeSocketsBackward>&>::value,
              "const initiator export getter");
static_assert(std::is_same<decltype(std::declval<TlmNativeBaseInitiator&>().get_base_interface()),
                           TlmNativeSocketsBackward&>::value,
              "non-const initiator interface getter");
static_assert(std::is_same<decltype(std::declval<const TlmNativeBaseInitiator&>().get_base_interface()),
                           const TlmNativeSocketsBackward&>::value,
              "const initiator interface getter");
static_assert(std::is_same<decltype(std::declval<TlmNativeBaseTarget&>().get_base_port()),
                           sc_core::sc_port_b<TlmNativeSocketsBackward>&>::value,
              "non-const target port getter");
static_assert(std::is_same<decltype(std::declval<const TlmNativeBaseTarget&>().get_base_port()),
                           const sc_core::sc_port_b<TlmNativeSocketsBackward>&>::value,
              "const target port getter");
static_assert(std::is_same<decltype(std::declval<TlmNativeBaseTarget&>().get_base_export()),
                           sc_core::sc_export<TlmNativeSocketsForward>&>::value,
              "non-const target export getter");
static_assert(std::is_same<decltype(std::declval<const TlmNativeBaseTarget&>().get_base_export()),
                           const sc_core::sc_export<TlmNativeSocketsForward>&>::value,
              "const target export getter");
static_assert(
  std::is_same<decltype(std::declval<TlmNativeBaseTarget&>().get_base_interface()), TlmNativeSocketsForward&>::value,
  "non-const target interface getter");
static_assert(std::is_same<decltype(std::declval<const TlmNativeBaseTarget&>().get_base_interface()),
                           const TlmNativeSocketsForward&>::value,
              "const target interface getter");
static_assert(
  std::is_same<decltype(std::declval<TlmNativeBaseTarget&>().operator->()), TlmNativeSocketsBackward*>::value,
  "target arrow endpoint");
static_assert(
  std::is_same<decltype(std::declval<TlmNativeBaseTarget&>().operator[](0)), TlmNativeSocketsBackward*>::value,
  "target indexed endpoint");

static_assert(std::is_same<decltype(static_cast<void (TlmNativeBaseInitiator::*)(TlmNativeBaseTargetB&)>(
                             &TlmNativeBaseInitiator::bind)),
                           void (TlmNativeBaseInitiator::*)(TlmNativeBaseTargetB&)>::value,
              "initiator bind target overload");
static_assert(std::is_same<decltype(static_cast<void (TlmNativeBaseInitiator::*)(TlmNativeBaseInitiatorB&)>(
                             &TlmNativeBaseInitiator::bind)),
                           void (TlmNativeBaseInitiator::*)(TlmNativeBaseInitiatorB&)>::value,
              "initiator bind hierarchy overload");
static_assert(std::is_same<decltype(static_cast<void (TlmNativeBaseInitiator::*)(TlmNativeSocketsBackward&)>(
                             &TlmNativeBaseInitiator::bind)),
                           void (TlmNativeBaseInitiator::*)(TlmNativeSocketsBackward&)>::value,
              "initiator bind interface overload");
static_assert(std::is_same<decltype(static_cast<void (TlmNativeBaseInitiator::*)(TlmNativeBaseTargetB&)>(
                             &TlmNativeBaseInitiator::operator())),
                           void (TlmNativeBaseInitiator::*)(TlmNativeBaseTargetB&)>::value,
              "initiator call target overload");
static_assert(std::is_same<decltype(static_cast<void (TlmNativeBaseInitiator::*)(TlmNativeBaseInitiatorB&)>(
                             &TlmNativeBaseInitiator::operator())),
                           void (TlmNativeBaseInitiator::*)(TlmNativeBaseInitiatorB&)>::value,
              "initiator call hierarchy overload");
static_assert(std::is_same<decltype(static_cast<void (TlmNativeBaseInitiator::*)(TlmNativeSocketsBackward&)>(
                             &TlmNativeBaseInitiator::operator())),
                           void (TlmNativeBaseInitiator::*)(TlmNativeSocketsBackward&)>::value,
              "initiator call interface overload");
static_assert(std::is_same<decltype(static_cast<void (TlmNativeBaseTarget::*)(TlmNativeBaseInitiatorB&)>(
                             &TlmNativeBaseTarget::bind)),
                           void (TlmNativeBaseTarget::*)(TlmNativeBaseInitiatorB&)>::value,
              "target bind initiator overload");
static_assert(
  std::is_same<decltype(static_cast<void (TlmNativeBaseTarget::*)(TlmNativeBaseTargetB&)>(&TlmNativeBaseTarget::bind)),
               void (TlmNativeBaseTarget::*)(TlmNativeBaseTargetB&)>::value,
  "target bind hierarchy overload");
static_assert(std::is_same<decltype(static_cast<void (TlmNativeBaseTarget::*)(TlmNativeSocketsForward&)>(
                             &TlmNativeBaseTarget::bind)),
                           void (TlmNativeBaseTarget::*)(TlmNativeSocketsForward&)>::value,
              "target bind interface overload");
static_assert(std::is_same<decltype(static_cast<void (TlmNativeBaseTarget::*)(TlmNativeBaseInitiatorB&)>(
                             &TlmNativeBaseTarget::operator())),
                           void (TlmNativeBaseTarget::*)(TlmNativeBaseInitiatorB&)>::value,
              "target call initiator overload");
static_assert(std::is_same<decltype(static_cast<void (TlmNativeBaseTarget::*)(TlmNativeBaseTargetB&)>(
                             &TlmNativeBaseTarget::operator())),
                           void (TlmNativeBaseTarget::*)(TlmNativeBaseTargetB&)>::value,
              "target call hierarchy overload");
static_assert(std::is_same<decltype(static_cast<void (TlmNativeBaseTarget::*)(TlmNativeSocketsForward&)>(
                             &TlmNativeBaseTarget::operator())),
                           void (TlmNativeBaseTarget::*)(TlmNativeSocketsForward&)>::value,
              "target call interface overload");

static_assert(std::is_same<TlmNativeBaseInitiator::fw_interface_type, TlmNativeSocketsForward>::value,
              "base initiator fw alias");
static_assert(std::is_same<TlmNativeBaseInitiator::bw_interface_type, TlmNativeSocketsBackward>::value,
              "base initiator bw alias");
static_assert(std::is_same<TlmNativeBaseInitiator::port_type,
                           sc_core::sc_port<TlmNativeSocketsForward, 2, sc_core::SC_ZERO_OR_MORE_BOUND>>::value,
              "base initiator port alias");
static_assert(std::is_same<TlmNativeBaseInitiator::export_type, sc_core::sc_export<TlmNativeSocketsBackward>>::value,
              "base initiator export alias");
static_assert(std::is_same<TlmNativeBaseInitiator::base_target_socket_type, TlmNativeBaseTargetB>::value,
              "base initiator target alias");
static_assert(std::is_same<TlmNativeBaseInitiator::base_type, TlmNativeBaseInitiatorB>::value,
              "base initiator base alias");
static_assert(std::is_same<TlmNativeBaseTarget::fw_interface_type, TlmNativeSocketsForward>::value,
              "base target fw alias");
static_assert(std::is_same<TlmNativeBaseTarget::bw_interface_type, TlmNativeSocketsBackward>::value,
              "base target bw alias");
static_assert(std::is_same<TlmNativeBaseTarget::port_type,
                           sc_core::sc_port<TlmNativeSocketsBackward, 2, sc_core::SC_ZERO_OR_MORE_BOUND>>::value,
              "base target port alias");
static_assert(std::is_same<TlmNativeBaseTarget::export_type, sc_core::sc_export<TlmNativeSocketsForward>>::value,
              "base target export alias");
static_assert(std::is_same<TlmNativeBaseTarget::base_initiator_socket_type, TlmNativeBaseInitiatorB>::value,
              "base target initiator alias");
static_assert(std::is_same<TlmNativeBaseTarget::base_type, TlmNativeBaseTargetB>::value, "base target base alias");

/** @brief Native channel implementing both sides of the custom protocol.
 *  @details Exports may share this endpoint in independent test bindings. The
 *  channel owns no payloads or DMI storage, and all calls remain on the kernel
 *  thread.
 */
struct TlmNativeSocketsEndpoint : sc_core::sc_channel, TlmNativeSocketsForward, TlmNativeSocketsBackward
{
  int calls = 0;

  /** @brief Construct the native channel used as a socket endpoint.
   *  @param name Unique SystemC object name retained through simulation.
   */
  explicit TlmNativeSocketsEndpoint(const char* name) : sc_core::sc_channel(sc_core::sc_module_name(name))
  {
  }

  /** @brief Complete a custom-protocol forward nonblocking request. */
  tlm::tlm_sync_enum nb_transport_fw(TlmNativeSocketsPayload&, tlm::tlm_phase& phase, sc_core::sc_time&) override
  {
    ++calls;
    phase = tlm::BEGIN_RESP;
    return tlm::TLM_UPDATED;
  }

  /** @brief Complete a custom-protocol forward blocking request. */
  void b_transport(TlmNativeSocketsPayload&, sc_core::sc_time&) override
  {
    ++calls;
  }

  /** @brief Grant no direct-memory region while recording the call. */
  bool get_direct_mem_ptr(TlmNativeSocketsPayload&, tlm::tlm_dmi&) override
  {
    ++calls;
    return false;
  }

  /** @brief Report a zero-byte debug transfer. */
  unsigned int transport_dbg(TlmNativeSocketsPayload&) override
  {
    ++calls;
    return 0;
  }

  /** @brief Complete a custom-protocol backward nonblocking response. */
  tlm::tlm_sync_enum nb_transport_bw(TlmNativeSocketsPayload&, tlm::tlm_phase& phase, sc_core::sc_time&) override
  {
    ++calls;
    phase = tlm::END_RESP;
    return tlm::TLM_COMPLETED;
  }

  /** @brief Record an invalidated range without retaining borrowed arguments. */
  void invalidate_direct_mem_ptr(sc_dt::uint64, sc_dt::uint64) override
  {
    ++calls;
  }
};

/** @brief Hold independent sockets for every bind and call overload.
 *  @details All sockets use zero-or-more-bound policy. Binding operations are
 *  performed once before elaboration by the Python model or native reference.
 */
struct TlmNativeSocketsBindings : sc_core::sc_module
{
  TlmNativeSocketsEndpoint endpoint;
  TlmNativeBaseInitiator initiator_interface_bind;
  TlmNativeBaseInitiator initiator_interface_call;
  TlmNativeBaseTarget target_interface_bind;
  TlmNativeBaseTarget target_interface_call;
  TlmNativeBaseInitiator initiator_target_bind;
  TlmNativeBaseInitiator initiator_target_call;
  TlmNativeBaseTarget target_socket_bind;
  TlmNativeBaseTarget target_socket_call;
  TlmNativeBaseInitiator initiator_hierarchy_bind_parent;
  TlmNativeBaseInitiator initiator_hierarchy_bind_child;
  TlmNativeBaseTarget initiator_hierarchy_bind_target;
  TlmNativeBaseInitiator initiator_hierarchy_call_parent;
  TlmNativeBaseInitiator initiator_hierarchy_call_child;
  TlmNativeBaseTarget initiator_hierarchy_call_target;
  TlmNativeBaseInitiator target_initiator_bind;
  TlmNativeBaseInitiator target_initiator_call;
  TlmNativeBaseTarget target_initiator_bind_socket;
  TlmNativeBaseTarget target_initiator_call_socket;
  TlmNativeBaseInitiator target_hierarchy_bind_initiator;
  TlmNativeBaseTarget target_hierarchy_bind_parent;
  TlmNativeBaseTarget target_hierarchy_bind_child;
  TlmNativeBaseInitiator target_hierarchy_call_initiator;
  TlmNativeBaseTarget target_hierarchy_call_parent;
  TlmNativeBaseTarget target_hierarchy_call_child;

  /** @brief Construct a module whose sockets remain owned through simulation.
   *  @param name Unique SystemC name for this isolated binding matrix.
   */
  explicit TlmNativeSocketsBindings(sc_core::sc_module_name name) : sc_module(name), endpoint("endpoint")
  {
  }

  /** @brief Invoke all six overloads for each direction and socket hierarchy. */
  void bind_all()
  {
    initiator_interface_bind.bind(endpoint);
    initiator_interface_call(endpoint);
    target_interface_bind.bind(endpoint);
    target_interface_call(endpoint);

    target_socket_bind.bind(endpoint);
    initiator_target_bind.bind(endpoint);
    initiator_target_bind.bind(target_socket_bind);
    target_socket_call.bind(endpoint);
    initiator_target_call.bind(endpoint);
    initiator_target_call(target_socket_call);

    initiator_hierarchy_bind_target.bind(endpoint);
    initiator_hierarchy_bind_parent.bind(endpoint);
    initiator_hierarchy_bind_parent.bind(initiator_hierarchy_bind_child);
    initiator_hierarchy_bind_child.bind(initiator_hierarchy_bind_target);
    initiator_hierarchy_call_target.bind(endpoint);
    initiator_hierarchy_call_parent.bind(endpoint);
    initiator_hierarchy_call_parent(initiator_hierarchy_call_child);
    initiator_hierarchy_call_child(initiator_hierarchy_call_target);

    target_initiator_bind_socket.bind(endpoint);
    target_initiator_bind.bind(endpoint);
    target_initiator_bind_socket.bind(target_initiator_bind);
    target_initiator_call_socket.bind(endpoint);
    target_initiator_call.bind(endpoint);
    target_initiator_call_socket(target_initiator_call);

    target_hierarchy_bind_initiator.bind(endpoint);
    target_hierarchy_bind_child.bind(endpoint);
    target_hierarchy_bind_parent.bind(target_hierarchy_bind_child);
    target_hierarchy_bind_initiator.bind(target_hierarchy_bind_parent);
    target_hierarchy_call_initiator.bind(endpoint);
    target_hierarchy_call_child.bind(endpoint);
    target_hierarchy_call_parent(target_hierarchy_call_child);
    target_hierarchy_call_initiator(target_hierarchy_call_parent);
  }
};

/** @brief Own default- and named-constructed sockets for Python reflection.
 *  @details Sockets and endpoint are children of this module. All use a custom
 *  137-bit payload protocol, a 73-bit bus and two zero-or-more-bound ports.
 */
struct TlmNativeSocketsOwner : sc_core::sc_module
{
  TlmNativeSocketsEndpoint endpoint;
  TlmNativeBaseInitiator base_initiator_default;
  TlmNativeBaseInitiator base_initiator_named;
  TlmNativeBaseTarget base_target_default;
  TlmNativeBaseTarget base_target_named;
  TlmNativeInitiator initiator_default;
  TlmNativeInitiator initiator_named;
  TlmNativeTarget target_default;
  TlmNativeTarget target_named;
  TlmNativeSocketsBindings bindings;
  static inline int live_count = 0;

  /** @brief Return the native type index for the custom socket protocol. */
  static sc_core::sc_type_index protocol_index()
  {
    return typeid(TlmNativeSocketsProtocol);
  }

  /** @brief Construct one owner and both constructor forms of every socket.
   *  @param name Module name retained by SystemC for the socket lifetime.
   */
  explicit TlmNativeSocketsOwner(sc_core::sc_module_name name)
    : sc_module(name)
    , endpoint("endpoint")
    , base_initiator_named("base_initiator_named")
    , base_target_named("base_target_named")
    , initiator_named("initiator_named")
    , target_named("target_named")
    , bindings("bindings")
  {
    ++live_count;
  }

  /** @brief Release owner accounting after SystemC has stopped. */
  ~TlmNativeSocketsOwner() override
  {
    --live_count;
  }

  /** @brief Return non-const base-interface addresses through the abstract API. */
  static std::uintptr_t interface_port(tlm::tlm_base_socket_if& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<void*>(&socket.get_base_port()));
  }

  /** @brief Return const base-interface addresses through the abstract API. */
  static std::uintptr_t const_interface_port(const tlm::tlm_base_socket_if& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<const void*>(&socket.get_base_port()));
  }

  /** @brief Return non-const base-export addresses through the abstract API. */
  static std::uintptr_t interface_export(tlm::tlm_base_socket_if& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<void*>(&socket.get_base_export()));
  }

  /** @brief Return const base-export addresses through the abstract API. */
  static std::uintptr_t const_interface_export(const tlm::tlm_base_socket_if& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<const void*>(&socket.get_base_export()));
  }

  /** @brief Return const initiator base-port address for overload validation. */
  static std::uintptr_t const_initiator_port(const TlmNativeBaseInitiator& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<const void*>(&socket.get_base_port()));
  }

  /** @brief Return const target base-port address for overload validation. */
  static std::uintptr_t const_target_port(const TlmNativeBaseTarget& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<const void*>(&socket.get_base_port()));
  }

  /** @brief Return const initiator base-export address for overload validation. */
  static std::uintptr_t const_initiator_export(const TlmNativeBaseInitiator& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<const void*>(&socket.get_base_export()));
  }

  /** @brief Return const target base-export address for overload validation. */
  static std::uintptr_t const_target_export(const TlmNativeBaseTarget& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<const void*>(&socket.get_base_export()));
  }

  /** @brief Return const initiator interface address for overload validation. */
  static std::uintptr_t const_initiator_interface(const TlmNativeBaseInitiator& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<const void*>(&socket.get_base_interface()));
  }

  /** @brief Return the non-const initiator interface address for comparison. */
  static std::uintptr_t initiator_interface(TlmNativeBaseInitiator& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<void*>(&socket.get_base_interface()));
  }

  /** @brief Return const target interface address for overload validation. */
  static std::uintptr_t const_target_interface(const TlmNativeBaseTarget& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<const void*>(&socket.get_base_interface()));
  }

  /** @brief Return the non-const target interface address for comparison. */
  static std::uintptr_t target_interface(TlmNativeBaseTarget& socket)
  {
    return reinterpret_cast<std::uintptr_t>(dynamic_cast<void*>(&socket.get_base_interface()));
  }

  /** @brief Return the target's pointer selected by native operator arrow. */
  static std::uintptr_t target_arrow(TlmNativeBaseTarget& socket)
  {
    return reinterpret_cast<std::uintptr_t>(socket.operator->());
  }

  /** @brief Return the target's indexed backward interface pointer. */
  static std::uintptr_t target_index(TlmNativeBaseTarget& socket, int index)
  {
    return reinterpret_cast<std::uintptr_t>(socket.operator[](index));
  }
};

#endif  // PYSYSTEMC_TEST_TLM_NATIVE_SOCKETS_PROBE_H

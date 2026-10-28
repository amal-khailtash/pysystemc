/** @file tlm_socket_wrappers_probe.h
 *  @brief Native owner for public tlm_utils socket constructor probes.
 *  @details The owner outlives all sockets and transactions. Socket modules own
 *  their native helpers; transactions and callbacks borrow owner state. All
 *  callbacks execute on the SystemC kernel thread without Python or GIL access.
 */
#ifndef PYSYSTEMC_TEST_TLM_SOCKET_WRAPPERS_PROBE_H
#define PYSYSTEMC_TEST_TLM_SOCKET_WRAPPERS_PROBE_H

#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/passthrough_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <array>
#include <systemc>
#include <tlm>
#include <vector>

/** @brief Custom payload with arbitrary-width storage for wrapper dispatch. */
struct TlmSocketWrapperPayload : tlm::tlm_generic_payload
{
  sc_dt::sc_biguint<137> value = 0;
};

/** @brief Native custom TLM protocol used by each wrapper specialization. */
struct TlmSocketWrapperProtocol
{
  using tlm_payload_type = TlmSocketWrapperPayload;
  using tlm_phase_type = tlm::tlm_phase;
};

/** @brief Record one callback's payload, phase and kernel timing. */
struct TlmSocketWrapperRecord
{
  std::uint64_t address;
  std::uint64_t low_value;
  bool high_bit;
  int phase;
  sc_core::sc_time time;
};

/** @brief Own default/named public socket templates and run one callback path.
 *  @details Socket members are native children of this module. Callback
 *  registrations borrow this object; payload storage remains owned until the
 *  simulation completes. Methods execute on the kernel thread and do not enter
 *  the Python GIL.
 */
struct TlmSocketWrapperProbe : sc_core::sc_module
{
  using Module = TlmSocketWrapperProbe;
  using Payload = TlmSocketWrapperPayload;
  using Protocol = TlmSocketWrapperProtocol;
  using SimpleInitiator = tlm_utils::simple_initiator_socket<Module, 73, Protocol>;
  using SimpleInitiatorOptional = tlm_utils::simple_initiator_socket_optional<Module, 73, Protocol>;
  using SimpleInitiatorTagged = tlm_utils::simple_initiator_socket_tagged<Module, 73, Protocol>;
  using SimpleInitiatorTaggedOptional = tlm_utils::simple_initiator_socket_tagged_optional<Module, 73, Protocol>;
  using SimpleTarget = tlm_utils::simple_target_socket<Module, 73, Protocol>;
  using SimpleTargetOptional = tlm_utils::simple_target_socket_optional<Module, 73, Protocol>;
  using SimpleTargetTagged = tlm_utils::simple_target_socket_tagged<Module, 73, Protocol>;
  using SimpleTargetTaggedOptional = tlm_utils::simple_target_socket_tagged_optional<Module, 73, Protocol>;
  using PassthroughTarget = tlm_utils::passthrough_target_socket<Module, 73, Protocol>;
  using PassthroughTargetOptional = tlm_utils::passthrough_target_socket_optional<Module, 73, Protocol>;
  using PassthroughTargetTagged = tlm_utils::passthrough_target_socket_tagged<Module, 73, Protocol>;
  using PassthroughTargetTaggedOptional = tlm_utils::passthrough_target_socket_tagged_optional<Module, 73, Protocol>;
  using MultiInitiator = tlm_utils::multi_passthrough_initiator_socket_optional<Module, 73, Protocol>;
  using MultiTarget = tlm_utils::multi_passthrough_target_socket_optional<Module, 73, Protocol>;

  SimpleInitiator simple_initiator_default;
  SimpleInitiator simple_initiator_named;
  SimpleInitiatorOptional simple_initiator_optional_default;
  SimpleInitiatorOptional simple_initiator_optional_named;
  SimpleInitiatorTagged simple_initiator_tagged_default;
  SimpleInitiatorTagged simple_initiator_tagged_named;
  SimpleInitiatorTaggedOptional simple_initiator_tagged_optional_default;
  SimpleInitiatorTaggedOptional simple_initiator_tagged_optional_named;

  SimpleTarget simple_target_default;
  SimpleTarget simple_target_named;
  SimpleTargetOptional simple_target_optional_default;
  SimpleTargetOptional simple_target_optional_named;
  SimpleTargetTagged simple_target_tagged_default;
  SimpleTargetTagged simple_target_tagged_named;
  SimpleTargetTaggedOptional simple_target_tagged_optional_default;
  SimpleTargetTaggedOptional simple_target_tagged_optional_named;

  PassthroughTarget passthrough_target_default;
  PassthroughTarget passthrough_target_named;
  PassthroughTargetOptional passthrough_target_optional_default;
  PassthroughTargetOptional passthrough_target_optional_named;
  PassthroughTargetTagged passthrough_target_tagged_default;
  PassthroughTargetTagged passthrough_target_tagged_named;
  PassthroughTargetTaggedOptional passthrough_target_tagged_optional_default;
  PassthroughTargetTaggedOptional passthrough_target_tagged_optional_named;

  MultiInitiator multi_initiator_default;
  MultiInitiator multi_initiator_named;
  MultiTarget multi_target_default;
  MultiTarget multi_target_named;

  std::array<SimpleInitiator, 4> passthrough_binders;
  Payload payload;
  std::vector<TlmSocketWrapperRecord> records;
  bool transport_complete = false;
  SC_HAS_PROCESS(TlmSocketWrapperProbe);

  /** @brief Construct named and default sockets, bind required ports and register callbacks.
   *  @param name Native module name retained for this module's lifecycle.
   *  @throws sc_core::sc_report if native socket construction or binding fails.
   */
  explicit TlmSocketWrapperProbe(sc_core::sc_module_name name)
    : sc_module(name)
    , simple_initiator_named("simple_initiator_named")
    , simple_initiator_optional_named("simple_initiator_optional_named")
    , simple_initiator_tagged_named("simple_initiator_tagged_named")
    , simple_initiator_tagged_optional_named("simple_initiator_tagged_optional_named")
    , simple_target_named("simple_target_named")
    , simple_target_optional_named("simple_target_optional_named")
    , simple_target_tagged_named("simple_target_tagged_named")
    , simple_target_tagged_optional_named("simple_target_tagged_optional_named")
    , passthrough_target_named("passthrough_target_named")
    , passthrough_target_optional_named("passthrough_target_optional_named")
    , passthrough_target_tagged_named("passthrough_target_tagged_named")
    , passthrough_target_tagged_optional_named("passthrough_target_tagged_optional_named")
    , multi_initiator_named("multi_initiator_named")
    , multi_target_named("multi_target_named")
  {
    simple_target_default.register_b_transport(this, &Module::transport);
    simple_target_default.register_nb_transport_fw(this, &Module::forward);
    simple_target_named.register_b_transport(this, &Module::transport);
    simple_target_named.register_nb_transport_fw(this, &Module::forward);
    multi_target_default.register_b_transport(this, &Module::multi_transport);
    multi_target_named.register_b_transport(this, &Module::multi_transport);
    simple_initiator_default.bind(simple_target_default);
    simple_initiator_named.bind(simple_target_named);
    passthrough_binders[0].bind(passthrough_target_default);
    passthrough_binders[1].bind(passthrough_target_named);
    passthrough_binders[2].bind(passthrough_target_tagged_default);
    passthrough_binders[3].bind(passthrough_target_tagged_named);
    simple_initiator_tagged_default.bind(simple_target_tagged_default);
    simple_initiator_tagged_named.bind(simple_target_tagged_named);
    simple_initiator_optional_default.bind(simple_target_optional_default);
    simple_initiator_optional_named.bind(simple_target_optional_named);
    simple_initiator_tagged_optional_default.bind(simple_target_tagged_optional_default);
    simple_initiator_tagged_optional_named.bind(simple_target_tagged_optional_named);
    SC_THREAD(run);
  }

  /** @brief Complete a blocking request after its annotated delay. */
  void transport(Payload& trans, sc_core::sc_time& delay)
  {
    if (&trans != &payload || !trans.value[136] || trans.value.to_uint64() != 17) SC_REPORT_FATAL("WRAPPER", "payload");
    wait(delay);
    trans.set_address(trans.get_address() + 4);
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
    trans.value = 23;
    delay = sc_core::SC_ZERO_TIME;
    records.push_back(
      {trans.get_address(), trans.value.to_uint64(), trans.value[136].to_bool(), -1, sc_core::sc_time_stamp()});
  }

  /** @brief Update the request phase and annotated forward latency. */
  tlm::tlm_sync_enum forward(Payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    if (&trans != &payload || trans.value[136] || trans.value.to_uint64() != 23)
      SC_REPORT_FATAL("WRAPPER", "phase payload");
    phase = tlm::BEGIN_RESP;
    delay = sc_core::sc_time(3, sc_core::SC_NS);
    records.push_back({trans.get_address(), trans.value.to_uint64(), trans.value[136].to_bool(),
                       static_cast<int>(phase), sc_core::sc_time_stamp()});
    return tlm::TLM_UPDATED;
  }

  /** @brief Provide the required export callback for optional multi targets. */
  void multi_transport(int, Payload&, sc_core::sc_time&)
  {
  }

  /** @brief Drive blocking and nonblocking transport and retain result state. */
  void run()
  {
    payload.set_address(17);
    payload.value = (sc_dt::sc_biguint<137>(1) << 136) | 17;
    sc_core::sc_time delay(2, sc_core::SC_NS);
    simple_initiator_default->b_transport(payload, delay);
    payload.set_address(17);
    payload.value = (sc_dt::sc_biguint<137>(1) << 136) | 17;
    delay = sc_core::sc_time(1, sc_core::SC_NS);
    simple_initiator_named->b_transport(payload, delay);
    tlm::tlm_phase phase = tlm::BEGIN_REQ;
    delay = sc_core::SC_ZERO_TIME;
    const auto status = simple_initiator_default->nb_transport_fw(payload, phase, delay);
    if (status != tlm::TLM_UPDATED || phase != tlm::BEGIN_RESP || delay != sc_core::sc_time(3, sc_core::SC_NS))
      SC_REPORT_FATAL("WRAPPER", "nonblocking dispatch");
    transport_complete = records.size() == 3 && records[0].time == sc_core::sc_time(2, sc_core::SC_NS) &&
                         records[1].time == sc_core::sc_time(3, sc_core::SC_NS) &&
                         records[2].time == sc_core::sc_time(3, sc_core::SC_NS);
  }
};

#endif  // PYSYSTEMC_TEST_TLM_SOCKET_WRAPPERS_PROBE_H

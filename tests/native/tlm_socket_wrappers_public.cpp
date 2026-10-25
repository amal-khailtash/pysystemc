/** @file tlm_socket_wrappers_public.cpp
 *  @brief Independent native reference for custom-protocol socket dispatch.
 *  @details The reference owns its sockets and transaction through simulation.
 *  Callbacks borrow the model and payload. They execute on the SystemC kernel
 *  thread and do not enter Python or acquire the GIL.
 */
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <iostream>
#include <stdexcept>
#include <systemc>
#include <tlm>
#include <vector>

/** @brief Custom transaction wider than native machine integers. */
struct ReferencePayload : tlm::tlm_generic_payload
{
  sc_dt::sc_biguint<137> value = 0;
};

/** @brief Select a custom payload with the standard mutable TLM phase. */
struct ReferenceProtocol
{
  using tlm_payload_type = ReferencePayload;
  using tlm_phase_type = tlm::tlm_phase;
};

/** @brief Store observable callback outputs without owning the transaction. */
struct ReferenceRecord
{
  std::uint64_t address;
  std::uint64_t value;
  bool high_bit;
  int phase;
  sc_core::sc_time time;
};

/** @brief Native model for blocking and nonblocking forward dispatch. */
struct Reference : sc_core::sc_module
{
  using Module = Reference;
  using Initiator = tlm_utils::simple_initiator_socket<Module, 73, ReferenceProtocol>;
  using Target = tlm_utils::simple_target_socket<Module, 73, ReferenceProtocol>;

  Initiator initiator;
  Initiator initiator_named;
  Target target;
  Target target_named;
  ReferencePayload payload;
  std::vector<ReferenceRecord> records;
  bool complete = false;
  SC_HAS_PROCESS(Reference);

  /** @brief Bind native sockets and register borrowed member callbacks.
   *  @param name Native module name controlling child socket lifetime.
   */
  explicit Reference(sc_core::sc_module_name name)
    : sc_module(name)
    , initiator("initiator")
    , initiator_named("initiator_named")
    , target("target")
    , target_named("target_named")
  {
    target.register_b_transport(this, &Module::transport);
    target.register_nb_transport_fw(this, &Module::forward);
    target_named.register_b_transport(this, &Module::transport);
    target_named.register_nb_transport_fw(this, &Module::forward);
    initiator.bind(target);
    initiator_named.bind(target_named);
    SC_THREAD(run);
  }

  /** @brief Mutate the borrowed request after its annotated delay. */
  void transport(ReferencePayload& trans, sc_core::sc_time& delay)
  {
    if (&trans != &payload || !trans.value[136] || trans.value.to_uint64() != 17)
      SC_REPORT_FATAL("REFERENCE", "blocking payload mismatch");
    wait(delay);
    trans.set_address(trans.get_address() + 4);
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
    trans.value = 23;
    delay = sc_core::SC_ZERO_TIME;
    records.push_back(
      {trans.get_address(), trans.value.to_uint64(), trans.value[136].to_bool(), -1, sc_core::sc_time_stamp()});
  }

  /** @brief Return updated response phase with an annotated delay. */
  tlm::tlm_sync_enum forward(ReferencePayload& trans, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    if (&trans != &payload || trans.value[136] || trans.value.to_uint64() != 23)
      SC_REPORT_FATAL("REFERENCE", "phase payload mismatch");
    phase = tlm::BEGIN_RESP;
    delay = sc_core::sc_time(3, sc_core::SC_NS);
    records.push_back({trans.get_address(), trans.value.to_uint64(), trans.value[136].to_bool(),
                       static_cast<int>(phase), sc_core::sc_time_stamp()});
    return tlm::TLM_UPDATED;
  }

  /** @brief Drive both forward interfaces and check phase, value and timing. */
  void run()
  {
    payload.set_address(17);
    payload.value = (sc_dt::sc_biguint<137>(1) << 136) | 17;
    sc_core::sc_time delay(2, sc_core::SC_NS);
    initiator->b_transport(payload, delay);
    payload.set_address(17);
    payload.value = (sc_dt::sc_biguint<137>(1) << 136) | 17;
    delay = sc_core::sc_time(1, sc_core::SC_NS);
    initiator_named->b_transport(payload, delay);
    tlm::tlm_phase phase = tlm::BEGIN_REQ;
    delay = sc_core::SC_ZERO_TIME;
    const auto status = initiator->nb_transport_fw(payload, phase, delay);
    if (status != tlm::TLM_UPDATED || phase != tlm::BEGIN_RESP || delay != sc_core::sc_time(3, sc_core::SC_NS))
      SC_REPORT_FATAL("REFERENCE", "nonblocking result mismatch");
    complete = records.size() == 3 && records[0].time == sc_core::sc_time(2, sc_core::SC_NS) &&
               records[1].time == sc_core::sc_time(3, sc_core::SC_NS) &&
               records[2].time == sc_core::sc_time(3, sc_core::SC_NS);
  }
};

/** @brief Run a standalone native kernel and print deterministic records. */
int sc_main(int, char**)
{
  Reference model("top");
  sc_core::sc_start(4, sc_core::SC_NS);
  if (!model.complete || model.records.size() != 3) throw std::runtime_error("reference callback count");
  const auto& blocking = model.records[0];
  const auto& named = model.records[1];
  const auto& nonblocking = model.records[2];
  std::cout << "SOCKET_WRAPPER_BLOCK_DEFAULT " << blocking.address << ' ' << blocking.value << ' ' << blocking.high_bit
            << ' ' << blocking.time << '\n';
  std::cout << "SOCKET_WRAPPER_BLOCK_NAMED " << named.address << ' ' << named.value << ' ' << named.high_bit << ' '
            << named.time << '\n';
  std::cout << "SOCKET_WRAPPER_PHASE " << nonblocking.phase << ' ' << nonblocking.time << '\n';
  std::cout << "SOCKET_WRAPPERS_NATIVE_OK\n";
  sc_core::sc_stop();
  return 0;
}

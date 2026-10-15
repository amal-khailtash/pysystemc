/** @file Independent custom protocol reference using upstream passthrough target sockets. */
#include <tlm_utils/passthrough_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>

#include <iostream>
#include <limits>
#include <stdexcept>
#include <systemc>
#include <tlm>

/** @brief Own a wide field alongside ordinary transaction state. */
struct Payload : tlm::tlm_generic_payload
{
  sc_dt::sc_biguint<137> value;
};

/** @brief Select custom payload with the native mutable phase protocol. */
struct Protocol
{
  using tlm_payload_type = Payload;
  using tlm_phase_type = tlm::tlm_phase;
};

/** @brief Check results in release builds without side effects in sc_assert. */
void verify(bool condition)
{
  if (!condition) throw std::runtime_error("custom socket reference assertion failed");
}

/** @brief Own sockets and borrowed transaction through the entire native simulation.
 * @details Native callbacks execute on the kernel thread without Python or GIL.
 * Assertion exceptions terminate the reference; no resources escape the module.
 */
struct Reference : sc_core::sc_module
{
  tlm_utils::passthrough_target_socket<Reference, 73, Protocol> target;
  tlm_utils::simple_initiator_socket<Reference, 73, Protocol> initiator;
  Payload payload;
  unsigned int count = 0;
  unsigned int callbacks = 0;
  bool invalidated = false;
  bool completed = false;
  SC_HAS_PROCESS(Reference);

  /** @brief Register member callbacks and retain all state until kernel completion. */
  explicit Reference(sc_core::sc_module_name name) : sc_module(name), target("target"), initiator("initiator")
  {
    payload.value = "0x10000000000000000000000000000000011";
    payload.set_address(17);
    target.register_b_transport(this, &Reference::transport);
    target.register_nb_transport_fw(this, &Reference::forward);
    target.register_transport_dbg(this, &Reference::debug);
    target.register_get_direct_mem_ptr(this, &Reference::dmi);
    initiator.register_nb_transport_bw(this, &Reference::backward);
    initiator.register_invalidate_direct_mem_ptr(this, &Reference::invalidate);
    initiator.bind(target);
    SC_THREAD(run);
  }

  /** @brief Verify borrowed identity and high bit before every payload callback. */
  void check(Payload& trans)
  {
    verify(&trans == &payload && trans.value[136].to_bool() && trans.value.to_uint64() == 17);
    ++callbacks;
  }

  /** @brief Mutate native phase and delay without suspension. */
  tlm::tlm_sync_enum forward(Payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    check(trans);
    phase = tlm::BEGIN_RESP;
    delay = sc_core::sc_time(1, sc_core::SC_NS);
    return tlm::TLM_UPDATED;
  }

  /** @brief Return the complete native unsigned debug count range. */
  unsigned int debug(Payload& trans)
  {
    check(trans);
    return count;
  }

  /** @brief Reject borrowed DMI output with explicit range and permissions. */
  bool dmi(Payload& trans, tlm::tlm_dmi& result)
  {
    check(trans);
    result.set_start_address(17);
    result.set_end_address(31);
    result.allow_none();
    return false;
  }

  /** @brief Preserve backward payload identity and complete the response phase. */
  tlm::tlm_sync_enum backward(Payload& trans, tlm::tlm_phase& phase, sc_core::sc_time&)
  {
    check(trans);
    phase = tlm::END_RESP;
    return tlm::TLM_COMPLETED;
  }

  /** @brief Verify the full native invalidation address range. */
  void invalidate(sc_dt::uint64 first, sc_dt::uint64 last)
  {
    verify(first == 0 && last == std::numeric_limits<sc_dt::uint64>::max());
    invalidated = true;
  }

  /** @brief Wait in native transport then mutate the original transaction. */
  void transport(Payload& trans, sc_core::sc_time& delay)
  {
    check(trans);
    wait(delay);
    delay = sc_core::SC_ZERO_TIME;
    trans.set_address(trans.get_address() + 4);
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
    trans.value = 23;
  }

  /** @brief Exercise both socket directions and assert state after the timed call. */
  void run()
  {
    sc_core::sc_time delay(2, sc_core::SC_NS);
    tlm::tlm_phase phase = tlm::BEGIN_REQ;
    verify(initiator->nb_transport_fw(payload, phase, delay) == tlm::TLM_UPDATED);
    verify(phase == tlm::BEGIN_RESP && delay == sc_core::sc_time(1, sc_core::SC_NS));
    tlm::tlm_dmi result;
    verify(!initiator->get_direct_mem_ptr(payload, result));
    verify(result.get_start_address() == 17 && result.get_end_address() == 31);
    verify(result.is_none_allowed());
    verify(initiator->transport_dbg(payload) == 0);
    count = std::numeric_limits<unsigned int>::max();
    verify(initiator->transport_dbg(payload) == count);
    verify(target->nb_transport_bw(payload, phase, delay) == tlm::TLM_COMPLETED);
    verify(phase == tlm::END_RESP);
    target->invalidate_direct_mem_ptr(0, std::numeric_limits<sc_dt::uint64>::max());
    verify(invalidated);
    delay = sc_core::sc_time(2, sc_core::SC_NS);
    initiator->b_transport(payload, delay);
    verify(payload.get_address() == 21 && payload.is_response_ok());
    verify(delay == sc_core::SC_ZERO_TIME && payload.value.to_uint64() == 23 && !payload.value[136].to_bool());
    verify(callbacks == 6 && sc_core::sc_time_stamp() == sc_core::sc_time(2, sc_core::SC_NS));
    completed = true;
    std::cout << "CUSTOM_SOCKET 21 23 0 2 ns\n";
  }
};

/** @brief Run one independent kernel with model ownership intact. */
int sc_main(int, char**)
{
  Reference model("top");
  sc_core::sc_start(3, sc_core::SC_NS);
  verify(model.completed);
  return 0;
}

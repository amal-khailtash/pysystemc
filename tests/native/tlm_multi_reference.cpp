/** @file Independent upstream multi-to-multi socket timing/state reference. */
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>

#include <iostream>
#include <limits>
#include <stdexcept>
#include <systemc>
#include <tlm>

/** @brief Enforce assertions regardless of NDEBUG; no Python/GIL involved. */
void verify(bool value)
{
  if (!value) throw std::runtime_error("multi reference assertion failed");
}
/** @brief Own all sockets, callbacks and transactions across native suspension. */
struct Model : sc_core::sc_module
{
  tlm_utils::multi_passthrough_initiator_socket<Model, 73> initiator;
  tlm_utils::multi_passthrough_target_socket<Model, 73> first, second;
  tlm::tlm_generic_payload payloads[2];
  SC_HAS_PROCESS(Model);
  /** @brief Bind two independent target connections and register native callbacks. */
  explicit Model(sc_core::sc_module_name name)
    : sc_module(name), initiator("initiator"), first("first"), second("second")
  {
    for (auto* target : {&first, &second})
    {
      target->register_b_transport(this, &Model::blocking);
      target->register_nb_transport_fw(this, &Model::forward);
      target->register_transport_dbg(this, &Model::debug);
      target->register_get_direct_mem_ptr(this, &Model::dmi);
      initiator.bind(*target);
    }
    initiator.register_nb_transport_bw(this, &Model::backward);
    initiator.register_invalidate_direct_mem_ptr(this, &Model::invalidate);
    SC_THREAD(run_first);
    SC_THREAD(run_second);
  }
  /** @brief Resolve transaction identity independently of target connection index. */
  int index(tlm::tlm_generic_payload& payload)
  {
    verify(&payload == &payloads[0] || &payload == &payloads[1]);
    return &payload == &payloads[0] ? 0 : 1;
  }
  /** @brief Wait on annotation and update borrowed native state. */
  void blocking(int connection, tlm::tlm_generic_payload& payload, sc_core::sc_time& delay)
  {
    verify(connection == 0);
    auto i = index(payload);
    wait(delay);
    payload.set_address(21 + i);
    delay = sc_core::SC_ZERO_TIME;
    std::cout << "MULTI_REF blocking " << i << ' ' << payload.get_address() << ' ' << delay << ' '
              << sc_core::sc_time_stamp() << '\n';
  }
  /** @brief Update forward phase/delay through native indexed dispatch. */
  tlm::tlm_sync_enum forward(int connection, tlm::tlm_generic_payload& payload, tlm::tlm_phase& phase,
                             sc_core::sc_time& delay)
  {
    verify(connection == 0);
    phase = tlm::BEGIN_RESP;
    delay = sc_core::sc_time(index(payload) + 1, sc_core::SC_NS);
    return tlm::TLM_UPDATED;
  }
  /** @brief Return connection-specific native debug counts. */
  unsigned int debug(int connection, tlm::tlm_generic_payload& payload)
  {
    verify(connection == 0);
    return index(payload) + 7;
  }
  /** @brief Reject DMI with distinct borrowed output ranges. */
  bool dmi(int connection, tlm::tlm_generic_payload& payload, tlm::tlm_dmi& result)
  {
    verify(connection == 0);
    auto i = index(payload);
    result.set_start_address(17 + i);
    result.set_end_address(31 + i);
    result.allow_none();
    return false;
  }
  /** @brief Preserve backward connection identity and mutable phase/delay. */
  tlm::tlm_sync_enum backward(int i, tlm::tlm_generic_payload& payload, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    verify(index(payload) == i);
    phase = tlm::END_RESP;
    delay = sc_core::sc_time(i + 1, sc_core::SC_NS);
    std::cout << "MULTI_REF backward " << i << ' ' << phase << ' ' << delay << '\n';
    return tlm::TLM_COMPLETED;
  }
  /** @brief Validate full-width invalidation and native backward index. */
  void invalidate(int i, sc_dt::uint64 begin, sc_dt::uint64 end)
  {
    verify(begin == 0 && end == std::numeric_limits<sc_dt::uint64>::max());
    std::cout << "MULTI_REF invalidate " << i << ' ' << begin << ' ' << end << '\n';
  }
  /** @brief Compare synchronous forward results and blocking completion time. */
  void run(int i)
  {
    tlm::tlm_phase phase = tlm::BEGIN_REQ;
    sc_core::sc_time delay;
    verify(initiator[i]->nb_transport_fw(payloads[i], phase, delay) == tlm::TLM_UPDATED);
    verify(phase == tlm::BEGIN_RESP && delay == sc_core::sc_time(i + 1, sc_core::SC_NS));
    std::cout << "MULTI_REF forward " << i << ' ' << phase << ' ' << delay << '\n';
    auto count = initiator[i]->transport_dbg(payloads[i]);
    verify(count == static_cast<unsigned int>(i + 7));
    std::cout << "MULTI_REF debug " << i << ' ' << count << '\n';
    tlm::tlm_dmi result;
    verify(!initiator[i]->get_direct_mem_ptr(payloads[i], result));
    verify(result.get_start_address() == static_cast<unsigned int>(17 + i) &&
           result.get_end_address() == static_cast<unsigned int>(31 + i) && result.is_none_allowed());
    std::cout << "MULTI_REF dmi " << i << ' ' << result.get_start_address() << ' ' << result.get_end_address() << '\n';
    delay = sc_core::sc_time(i + 2, sc_core::SC_NS);
    initiator[i]->b_transport(payloads[i], delay);
    verify(payloads[i].get_address() == static_cast<unsigned int>(21 + i) && delay == sc_core::SC_ZERO_TIME);
  }
  /** @brief Run first independently scheduled connection. */
  void run_first()
  {
    run(0);
  }
  /** @brief Run second independently scheduled connection. */
  void run_second()
  {
    run(1);
  }
};
/** @brief Run bounded native simulation and check backward paths before processes. */
int sc_main(int, char**)
{
  Model model("top");
  sc_core::sc_start(sc_core::SC_ZERO_TIME);
  // Processes ran their forward callbacks at zero time; backward comparisons are
  // sorted by record identity, so their relative placement is immaterial.
  for (int i = 0; i < 2; ++i)
  {
    tlm::tlm_phase phase = tlm::BEGIN_RESP;
    sc_core::sc_time delay;
    auto* backward = i == 0 ? model.first[0] : model.second[0];
    verify(backward->nb_transport_bw(model.payloads[i], phase, delay) == tlm::TLM_COMPLETED);
    verify(phase == tlm::END_RESP && delay == sc_core::sc_time(i + 1, sc_core::SC_NS));
    backward->invalidate_direct_mem_ptr(0, std::numeric_limits<sc_dt::uint64>::max());
  }
  sc_core::sc_start(4, sc_core::SC_NS);
  return 0;
}

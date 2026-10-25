/** @file tlm_utils_passthrough_b_public.cpp
 *  @brief Independent native reference for both passthrough target B APIs.
 *  @details The module owns native member-pointer callbacks and custom-width
 *  payload storage. Blocking transport runs on the native kernel thread.
 */
#include <iostream>
#include <limits>

#include "tlm_utils_passthrough_b_probe.h"

int sc_main(int, char**)
{
  using namespace tlm_utils_passthrough_b_probe;
  {
    Module model("top");
    model.plain_initiator.bind(model);
    model.tagged_initiator.bind(model);
    model.plain_target.bind(model.plain_initiator);
    model.tagged_target.bind(model.tagged_initiator);

    auto& plain = model.plain_target.get_base_interface();
    auto& tagged = model.tagged_target.get_base_interface();
    tlm::tlm_phase phase = tlm::BEGIN_REQ;
    sc_core::sc_time delay(0, sc_core::SC_NS);
    verify(plain.nb_transport_fw(model.payload, phase, delay) == tlm::TLM_UPDATED, "plain forward result mismatch");
    verify(phase == tlm::BEGIN_RESP && delay == sc_core::sc_time(1, sc_core::SC_NS), "plain phase/delay mismatch");
    phase = tlm::BEGIN_REQ;
    delay = sc_core::SC_ZERO_TIME;
    verify(tagged.nb_transport_fw(model.payload, phase, delay) == tlm::TLM_UPDATED, "tagged forward result mismatch");
    verify(phase == tlm::END_REQ && delay == sc_core::sc_time(2, sc_core::SC_NS), "tagged phase/delay mismatch");

    tlm::tlm_dmi plain_dmi;
    verify(plain.get_direct_mem_ptr(model.payload, plain_dmi), "plain DMI result mismatch");
    verify(plain_dmi.get_start_address() == 0x1000 && plain_dmi.get_end_address() == 0x10ff &&
             plain_dmi.is_read_allowed() && plain_dmi.is_write_allowed() &&
             plain_dmi.get_read_latency() == sc_core::sc_time(3, sc_core::SC_NS) &&
             plain_dmi.get_write_latency() == sc_core::sc_time(4, sc_core::SC_NS),
           "plain DMI fields mismatch");
    tlm::tlm_dmi tagged_dmi;
    verify(tagged.get_direct_mem_ptr(model.payload, tagged_dmi), "tagged DMI result mismatch");
    verify(tagged_dmi.get_start_address() == 0x2000 && tagged_dmi.get_end_address() == 0x20ff &&
             tagged_dmi.is_read_allowed() && !tagged_dmi.is_write_allowed() &&
             tagged_dmi.get_read_latency() == sc_core::sc_time(6, sc_core::SC_NS),
           "tagged DMI fields mismatch");
    verify(plain.transport_dbg(model.payload) == 0xf0000011U, "plain debug result mismatch");
    verify(tagged.transport_dbg(model.payload) == 0xf0000025U, "tagged debug result mismatch");

    model.payload.marker = 3;
    phase = tlm::BEGIN_RESP;
    delay = sc_core::sc_time(2, sc_core::SC_NS);
    verify(model.plain_target[0]->nb_transport_bw(model.payload, phase, delay) == tlm::TLM_COMPLETED,
           "plain backward result mismatch");
    verify(phase == tlm::END_RESP && delay == sc_core::sc_time(7, sc_core::SC_NS),
           "plain backward phase/delay mismatch");
    model.payload.marker = 4;
    phase = tlm::END_REQ;
    delay = sc_core::sc_time(3, sc_core::SC_NS);
    verify(model.tagged_target[0]->nb_transport_bw(model.payload, phase, delay) == tlm::TLM_COMPLETED,
           "tagged backward result mismatch");
    verify(phase == tlm::END_RESP && delay == sc_core::sc_time(8, sc_core::SC_NS),
           "tagged backward phase/delay mismatch");
    model.payload.marker = 5;
    model.plain_target[0]->invalidate_direct_mem_ptr(0, std::numeric_limits<sc_dt::uint64>::max());
    model.payload.marker = 6;
    model.tagged_target[0]->invalidate_direct_mem_ptr(0x100000000ULL, std::numeric_limits<sc_dt::uint64>::max());

    sc_core::sc_start(6, sc_core::SC_NS);
    verify(model.blocking_done, "blocking callbacks did not finish");
    verify(model.callbacks == 10, "passthrough callback count mismatch");
    for (const auto& event : model.events) std::cout << "B_SOCKET_EVENT " << event << '\n';
  }
  verify(Module::destroyed == 1, "native socket owner was not destroyed");
  std::cout << "B_SOCKET_LIFETIME 1\n";
  std::cout << "TLM_UTILS_PASSTHROUGH_B_PUBLIC_OK\n";
  sc_core::sc_stop();
  return 0;
}

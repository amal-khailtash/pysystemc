/** @file Native reference for both multi-socket hierarchy binding forms. */
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "tlm_utils_multi_probe.h"

/** @brief Check native callbacks independently of disabled SystemC assertions. */
static void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

int sc_main(int argc, char** argv)
{
  using namespace pysystemc_tlm_utils_multi;
  const bool call_binding = argc > 1 && std::string(argv[1]) == "call";
  static_assert(!std::is_convertible_v<decltype(&Owner::target_b), InitiatorBase::nb_cb>);
  SocketLifecycle::reset();
  {
    Owner owner("top");
    owner.register_initiator_callbacks();
    owner.register_target_callbacks();
    if (call_binding)
    {
      owner.outer_initiator(owner.inner_initiator);
      owner.outer_target(owner.inner_target);
    }
    else
    {
      owner.outer_initiator.bind(owner.inner_initiator);
      owner.outer_target.bind(owner.inner_target);
    }
    owner.inner_initiator.bind(owner.forward_target_0);
    owner.inner_initiator(owner.forward_target_1);
    owner.backward_initiator_0.bind(owner.outer_target);
    owner.backward_initiator_1.bind(owner.outer_target);
    sc_core::sc_start(sc_core::SC_ZERO_TIME);
    verify(owner.outer_initiator.size() == 2 && owner.inner_initiator.size() == 2 && owner.outer_target.size() == 2 &&
             owner.inner_target.size() == 2,
           "hierarchy size");
    verify(SocketLifecycle::initiator_before_end()[0] == 2 && SocketLifecycle::target_end()[0] == 2, "lifecycle hooks");
    verify(sc_core::sc_time_stamp() == sc_core::SC_ZERO_TIME, "elaboration time");
    (void)owner.outer_initiator.get_base_interface();
    (void)owner.outer_target.get_base_interface();
    (void)owner.inner_initiator.get_base_export();
    (void)owner.inner_target.get_base_export();
    verify(
      const_initiator_interface_rejected(owner.outer_initiator) && const_target_interface_rejected(owner.outer_target),
      "native const interface rejection");
    verify(const_initiator_export(owner.outer_initiator) && const_target_export(owner.outer_target), "const exports");

    for (int index = 0; index < 2; ++index)
    {
      Payload payload;
      payload.set_address(0xA000 + index);
      payload.value = wide(index);
      sc_core::sc_time delay;
      owner.outer_initiator[index]->b_transport(payload, delay);
      verify(payload.get_address() == 0xA100 + index && payload.value == wide(0x10 + index) &&
               delay == sc_core::sc_time(10 + index, sc_core::SC_NS),
             "initiator blocking");
      std::cout << "MULTI_SOCKET_EVENT INIT_B " << index << ' ' << payload.get_address() << ' ' << payload.value << ' '
                << delay << '\n';
      payload.set_address(0xA000 + index);
      tlm::tlm_phase phase = tlm::BEGIN_REQ;
      delay = sc_core::SC_ZERO_TIME;
      verify(owner.outer_initiator[index]->nb_transport_fw(payload, phase, delay) == tlm::TLM_UPDATED &&
               phase == tlm::BEGIN_RESP && delay == sc_core::sc_time(20 + index, sc_core::SC_NS) &&
               owner.initiator_nb_calls[index] == 1 &&
               owner.last_backward_phase[index] == static_cast<int>(tlm::END_RESP),
             "initiator nonblocking");
      std::cout << "MULTI_SOCKET_EVENT INIT_NB " << index << ' ' << phase << ' ' << delay << ' '
                << owner.initiator_nb_calls[index] << '\n';
      payload.set_address(0xA000 + index);
      const auto debug = owner.outer_initiator[index]->transport_dbg(payload);
      verify(debug == 30 + index, "initiator debug");
      std::cout << "MULTI_SOCKET_EVENT INIT_DBG " << index << ' ' << debug << '\n';
      payload.set_address(0xA000 + index);
      tlm::tlm_dmi dmi;
      verify(owner.outer_initiator[index]->get_direct_mem_ptr(payload, dmi) &&
               dmi.get_start_address() == 0xE000 + index && dmi.get_end_address() == 0xE0FF + index &&
               dmi.is_read_allowed() && dmi.is_write_allowed() &&
               dmi.get_read_latency() == sc_core::sc_time(1 + index, sc_core::SC_NS) &&
               dmi.get_write_latency() == sc_core::sc_time(2 + index, sc_core::SC_NS),
             "initiator DMI");
      std::cout << "MULTI_SOCKET_EVENT INIT_DMI " << index << ' ' << dmi.get_start_address() << ' '
                << dmi.get_end_address() << ' ' << dmi.get_read_latency() << ' ' << dmi.get_write_latency() << '\n';
      (index == 0 ? owner.forward_target_0 : owner.forward_target_1)
        ->invalidate_direct_mem_ptr(0xC000 + index, 0xC0FF + index);
      verify(owner.initiator_dmi_calls[index] == 1 && owner.last_invalidation[index] == index,
             "initiator invalidation");
      std::cout << "MULTI_SOCKET_EVENT INIT_INVALIDATE " << index << " 1\n";

      Payload target_payload;
      target_payload.set_address(0xB000 + index);
      delay = sc_core::SC_ZERO_TIME;
      owner.issue_target_b(index, target_payload, delay);
      verify(target_payload.get_address() == 0xB100 + index && target_payload.value == wide(0x40 + index) &&
               delay == sc_core::sc_time(40 + index, sc_core::SC_NS),
             "target blocking");
      std::cout << "MULTI_SOCKET_EVENT TARGET_B " << index << ' ' << target_payload.get_address() << ' '
                << target_payload.value << ' ' << delay << '\n';
      target_payload.set_address(0xB000 + index);
      phase = tlm::BEGIN_REQ;
      delay = sc_core::SC_ZERO_TIME;
      verify(owner.issue_target_nb(index, target_payload, phase, delay) == tlm::TLM_UPDATED &&
               phase == tlm::BEGIN_RESP && delay == sc_core::sc_time(50 + index, sc_core::SC_NS) &&
               owner.backward_nb_calls[index] == 1,
             "target nonblocking");
      std::cout << "MULTI_SOCKET_EVENT TARGET_NB " << index << ' ' << phase << ' ' << delay << ' '
                << owner.backward_nb_calls[index] << '\n';
      target_payload.set_address(0xB000 + index);
      verify(owner.issue_target_dbg(index, target_payload) == 70 + index, "target debug");
      std::cout << "MULTI_SOCKET_EVENT TARGET_DBG " << index << ' ' << 70 + index << '\n';
      target_payload.set_address(0xB000 + index);
      verify(!owner.issue_target_dmi(index, target_payload, dmi) && dmi.get_start_address() == 0xD000 + index &&
               dmi.get_end_address() == 0xD0FF + index && dmi.is_none_allowed(),
             "target DMI");
      std::cout << "MULTI_SOCKET_EVENT TARGET_DMI " << index << ' ' << dmi.get_start_address() << ' '
                << dmi.get_end_address() << '\n';
      target_payload.set_address(0xB000 + index);
      phase = tlm::BEGIN_RESP;
      delay = sc_core::SC_ZERO_TIME;
      verify(owner.inner_target[index]->nb_transport_bw(target_payload, phase, delay) == tlm::TLM_COMPLETED &&
               phase == tlm::END_RESP && delay == sc_core::sc_time(80 + index, sc_core::SC_NS),
             "target backward");
      owner.inner_target[index]->invalidate_direct_mem_ptr(0xC000 + index, 0xC0FF + index);
      verify(owner.backward_dmi_calls[index] == 1 && owner.call_total(index) == 13, "backward callback counts");
      std::cout << "MULTI_SOCKET_EVENT TARGET_BW " << index << ' ' << phase << ' ' << delay << " 1\n";
    }
    verify(sc_core::sc_time_stamp() == sc_core::SC_ZERO_TIME, "annotations never wait");
    sc_core::sc_start(3, sc_core::SC_NS);
    verify(sc_core::sc_time_stamp() == sc_core::sc_time(3, sc_core::SC_NS), "simulation time");
    std::cout << "MULTI_SOCKET_TIME " << sc_core::sc_time_stamp() << '\n';
  }
  verify(SocketLifecycle::initiator_destroyed()[0] == 2 && SocketLifecycle::target_destroyed()[0] == 2,
         "socket destruction");
  std::cout << "MULTI_SOCKET_LIFETIME 2 2\nTLM_UTILS_MULTI_PUBLIC_OK\n";
  sc_core::sc_stop();
  return 0;
}

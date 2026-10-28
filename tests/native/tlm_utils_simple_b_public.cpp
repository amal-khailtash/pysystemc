/** @file tlm_utils_simple_b_public.cpp
 *  @brief Native reference for simple `_b` socket callback and port behavior.
 *  @details This process calls native fw/bw interfaces independently of Python.
 *  The module owns socket and DMI storage; test payloads remain caller-owned.
 */
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "tlm_utils_simple_b_probe.h"

namespace
{

/** @brief Enforce reference checks independently of debug assertions. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Compare public aliases against arbitrary payload and 73-bit socket types. */
static_assert(std::is_same<SimpleBNativeOwner::PlainInitiator::transaction_type, SimpleBPayload>::value,
              "plain initiator transaction_type");
static_assert(std::is_same<SimpleBNativeOwner::TaggedInitiator::phase_type, tlm::tlm_phase>::value,
              "tagged initiator phase_type");
static_assert(
  std::is_same<SimpleBNativeOwner::PlainTarget::fw_interface_type, tlm::tlm_fw_transport_if<SimpleBProtocol>>::value,
  "plain target fw_interface_type");
static_assert(std::is_same<SimpleBNativeOwner::TaggedTarget::base_type,
                           tlm::tlm_target_socket<73, SimpleBProtocol, 1, sc_core::SC_ONE_OR_MORE_BOUND>>::value,
              "tagged target base_type");

/** @brief Call all nonblocking/DMI/debug directions from a native client process. */
void exercise_immediate(SimpleBNativeOwner& owner, int token, SimpleBPayload& payload)
{
  auto& forward = owner.forward_interface(token);
  auto& backward = owner.backward_interface(token);
  tlm::tlm_phase phase = tlm::BEGIN_REQ;
  sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
  verify(forward.nb_transport_fw(payload, phase, delay) == tlm::TLM_UPDATED, "forward sync result");
  verify(phase == tlm::BEGIN_RESP && delay == sc_core::sc_time(1, sc_core::SC_NS), "forward phase/delay");
  verify(forward.transport_dbg(payload) == static_cast<unsigned int>(32 + token), "debug count");
  tlm::tlm_dmi dmi;
  verify(forward.get_direct_mem_ptr(payload, dmi), "DMI grant");
  verify(dmi.get_start_address() == static_cast<sc_dt::uint64>(16 + token * 16) &&
           dmi.get_end_address() == static_cast<sc_dt::uint64>(31 + token * 16),
         "DMI address range");
  verify(dmi.is_read_allowed() && dmi.is_write_allowed(), "DMI permissions");
  verify(dmi.get_read_latency() == sc_core::sc_time(1, sc_core::SC_NS) &&
           dmi.get_write_latency() == sc_core::sc_time(2, sc_core::SC_NS),
         "DMI latency");
  dmi.get_dmi_ptr()[3] = static_cast<unsigned char>(80 + token);
  verify(owner.memory[token * 16 + 3] == 80 + token, "borrowed DMI storage");
  phase = tlm::BEGIN_RESP;
  verify(backward.nb_transport_bw(payload, phase, delay) == tlm::TLM_COMPLETED, "backward sync result");
  verify(phase == tlm::END_RESP, "backward phase");
  backward.invalidate_direct_mem_ptr(0x1000000000000000ULL + token, 0xF000000000000000ULL + token);
}

/** @brief Print portable output while retaining a native payload identity check. */
void print_records(SimpleBNativeOwner& owner, const std::array<SimpleBPayload, 4>& payloads)
{
  std::vector<const SimpleBRecord*> ordered;
  for (const auto& record : owner.records) ordered.push_back(&record);
  std::sort(ordered.begin(), ordered.end(),
            [](auto* lhs, auto* rhs)
            { return lhs->token == rhs->token ? lhs->kind < rhs->kind : lhs->token < rhs->token; });
  for (const auto* record : ordered)
  {
    const bool identity = record->kind != SimpleBNativeOwner::invalidate;
    if (identity)
      verify(record->payload_address == reinterpret_cast<std::uintptr_t>(&payloads[record->token]),
             "callback borrowed transaction identity");
    const int expected_id =
      record->kind == SimpleBNativeOwner::backward_nb || record->kind == SimpleBNativeOwner::invalidate
        ? 200 + record->token
        : 100 + record->token;
    verify(record->callback_id == expected_id, "callback registered identifier");
    std::cout << "SIMPLE_B_REC|" << record->token << '|' << record->callback_id << '|' << record->kind << '|'
              << identity << '|' << record->address << '|' << record->low_value << '|' << record->high_bit << '|'
              << record->phase << '|' << record->time << '\n';
  }
}

}  // namespace

/** @brief Run native interface calls in one isolated SystemC kernel. */
int sc_main(int, char**)
{
  SimpleBNativeOwner owner("top");
  std::array<SimpleBPayload, 4> payloads{};
  for (int token = 0; token < 4; ++token)
  {
    auto& payload = payloads[token];
    payload.token = token;
    payload.value = (sc_dt::sc_biguint<137>(1) << 136) | (17 + token);
    payload.set_address(17 + token);
    exercise_immediate(owner, token, payload);
  }

  std::vector<sc_core::sc_process_handle> handles;
  for (int token = 0; token < 4; ++token)
  {
    auto* payload = &payloads[token];
    auto* forward = &owner.forward_interface(token);
    handles.push_back(sc_core::sc_spawn(
      [payload, forward, token]()
      {
        sc_core::sc_time delay(2, sc_core::SC_NS);
        forward->b_transport(*payload, delay);
        if (payload->get_address() != static_cast<sc_dt::uint64>(21 + token) || !payload->is_response_ok() ||
            payload->value.to_uint64() != static_cast<unsigned>(23 + token) || payload->value[136] ||
            delay != sc_core::SC_ZERO_TIME || sc_core::sc_time_stamp() != sc_core::sc_time(2, sc_core::SC_NS))
          SC_REPORT_FATAL("SIMPLE_B_NATIVE", "blocking result");
      },
      (std::string("native_block_") + std::to_string(token)).c_str()));
  }
  sc_core::sc_start(3, sc_core::SC_NS);
  for (const auto& handle : handles) verify(handle.terminated(), "blocking process lifecycle");
  verify(SimpleBTarget::start_count == 2 && SimpleBTaggedTarget::start_count == 2,
         "protected target start_of_simulation hook");
  verify(owner.records.size() == 24, "native callback count");
  print_records(owner, payloads);
  std::cout << "SIMPLE_B_NATIVE_REFERENCE_OK\n";
  sc_core::sc_stop();
  return 0;
}

/** @file tlm_peq_public.cpp
 * @brief Native reference for PEQ constructors, ordering, cancellation and callbacks.
 * @details Native queues borrow payload and owner pointers. The model retains
 * them through the complete simulation; callback queues are destroyed only
 * after the kernel stops and no future dispatch can use their spawned methods.
 */
#include <tlm_utils/peq_with_cb_and_phase.h>
#include <tlm_utils/peq_with_get.h>

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

#include "tlm_peq_probe.h"

namespace
{

using GetQueue = tlm_utils::peq_with_get<PeqPayload>;
using CallbackQueue = tlm_utils::peq_with_cb_and_phase<PeqOwner, PeqProtocol>;
using DefaultCallbackQueue = tlm_utils::peq_with_cb_and_phase<PeqDefaultOwner>;

/** @brief Stop the reference on the first mismatch, independent of NDEBUG. */
void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Check public native aliases against their declared template types. */
static_assert(std::is_same<GetQueue::transaction_type, PeqPayload>::value, "transaction_type alias");
static_assert(std::is_same<GetQueue::pair_type, std::pair<const sc_core::sc_time, PeqPayload*>>::value,
              "pair_type alias");

}  // namespace

/** @brief Own get queues, callback owners and payloads for one isolated kernel. */
struct PeqModel : sc_core::sc_module
{
  GetQueue queue;
  GetQueue cancelled_queue;
  PeqOwner owner;
  PeqOwner named_owner;
  CallbackQueue callback_queue;
  CallbackQueue named_callback_queue;
  CallbackQueue cancelled_callback_queue;
  PeqDefaultOwner default_owner;
  DefaultCallbackQueue default_callback_queue;
  tlm::tlm_generic_payload standard_payload;
  std::array<PeqPayload, 6> payloads{};
  SC_HAS_PROCESS(PeqModel);

  /** @brief Construct named/default callback queues and register native processes. */
  explicit PeqModel(sc_core::sc_module_name name)
    : sc_module(name)
    , queue("get_queue")
    , cancelled_queue("cancelled_queue")
    , callback_queue(&owner, &PeqOwner::deliver)
    , named_callback_queue("named_callback", &named_owner, &PeqOwner::deliver)
    , cancelled_callback_queue("cancelled_callback", &named_owner, &PeqOwner::deliver)
    , default_callback_queue(&default_owner, &PeqDefaultOwner::deliver)
  {
    for (int index = 0; index < static_cast<int>(payloads.size()); ++index)
    {
      payloads[index].id = index;
      payloads[index].value = 0;
      payloads[index].value[136] = true;
      payloads[index].value = (sc_dt::sc_biguint<137>(1) << 136) | (index + 17);
    }
    SC_THREAD(produce);
    SC_THREAD(consume);
  }

  /** @brief Schedule immediate, delta, equal-time and cancelled notifications. */
  void produce()
  {
    queue.notify(payloads[0]);
    queue.notify(payloads[1]);
    queue.notify(payloads[2], sc_core::SC_ZERO_TIME);
    queue.notify(payloads[3], sc_core::sc_time(2, sc_core::SC_NS));
    queue.notify(payloads[4], sc_core::sc_time(2, sc_core::SC_NS));
    cancelled_queue.notify(payloads[5], sc_core::sc_time(5, sc_core::SC_NS));
    cancelled_queue.cancel_all();

    callback_queue.notify(payloads[0], 41);
    callback_queue.notify(payloads[1], 42);
    callback_queue.notify(payloads[2], 43, sc_core::SC_ZERO_TIME);
    named_callback_queue.notify(payloads[3], 44, sc_core::sc_time(2, sc_core::SC_NS));
    named_callback_queue.notify(payloads[4], 45, sc_core::sc_time(2, sc_core::SC_NS));
    cancelled_callback_queue.notify(payloads[5], 46, sc_core::sc_time(5, sc_core::SC_NS));
    cancelled_callback_queue.cancel_all();

    standard_payload.set_address(601);
    tlm::tlm_phase phase = tlm::BEGIN_REQ;
    default_callback_queue.notify(standard_payload, phase, sc_core::sc_time(1, sc_core::SC_NS));
    phase = tlm::END_RESP;
    wait(6, sc_core::SC_NS);
    verify(cancelled_queue.get_next_transaction() == nullptr, "get cancellation");
    std::cout << "PEQ_CANCEL_OK " << sc_core::sc_time_stamp() << '\n';
  }

  /** @brief Drain borrowed payload pointers and check queue event identity. */
  void consume()
  {
    sc_core::sc_event* stable_event = &queue.get_event();
    const std::array<int, 5> expected{0, 1, 2, 3, 4};
    int count = 0;
    while (count < static_cast<int>(expected.size()))
    {
      wait(queue.get_event());
      verify(stable_event == &queue.get_event(), "borrowed event address");
      for (auto* payload = queue.get_next_transaction(); payload != nullptr; payload = queue.get_next_transaction())
      {
        verify(count < static_cast<int>(expected.size()), "extra queue payload");
        verify(payload == &payloads[expected[count]], "borrowed payload address and stable equal-time order");
        verify(payload->value[136] && payload->value.to_uint64() == static_cast<unsigned>(payload->id + 17),
               "get wide payload");
        std::cout << "PEQ_GET " << payload->id << ' ' << sc_core::sc_time_stamp() << ' ' << sc_core::sc_delta_count()
                  << '\n';
        ++count;
      }
    }
  }
};

/** @brief Run native reference, stop dispatch, then destroy callback queues safely. */
int sc_main(int, char**)
{
  auto* model = new PeqModel("top");
  sc_core::sc_start(7, sc_core::SC_NS);
  verify(sc_core::sc_time_stamp() == sc_core::sc_time(7, sc_core::SC_NS), "bounded native duration");
  verify(model->owner.records.size() == 3 && model->named_owner.records.size() == 2, "native callback counts");
  for (const auto& record : model->owner.records)
  {
    verify(record.payload_address == reinterpret_cast<std::uintptr_t>(&model->payloads[record.id]),
           "native immediate callback borrowed address");
    verify(model->payloads[record.id].value[136] &&
             model->payloads[record.id].value.to_uint64() == static_cast<unsigned>(record.id + 17),
           "native callback custom wide payload");
    std::cout << "PEQ_CB " << record.id << ' ' << record.phase << ' ' << record.time << ' ' << record.delta << '\n';
  }
  for (const auto& record : model->named_owner.records)
  {
    verify(record.payload_address == reinterpret_cast<std::uintptr_t>(&model->payloads[record.id]),
           "native timed callback borrowed address");
    std::cout << "PEQ_CB " << record.id << ' ' << record.phase << ' ' << record.time << ' ' << record.delta << '\n';
  }
  verify(model->default_owner.records.size() == 1 && model->default_owner.records[0].address == 601 &&
           model->default_owner.records[0].begin_request,
         "native default-protocol phase copy");
  verify(model->default_owner.records[0].payload_address == reinterpret_cast<std::uintptr_t>(&model->standard_payload),
         "native default-protocol borrowed address");
  const auto& standard_record = model->default_owner.records[0];
  std::cout << "PEQ_DEFAULT " << standard_record.address << " BEGIN_REQ " << standard_record.time << ' '
            << standard_record.delta << '\n';
  std::cout << "PEQ_NATIVE_OK\n";
  sc_core::sc_stop();
  delete model;
  return 0;
}

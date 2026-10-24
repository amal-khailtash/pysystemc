/** @file tlm_peq_probe.h
 * @brief Custom native payload and callback owner for PEQ references.
 * @details The queue owns neither entries nor callback owners. Callers keep
 * each payload and owner alive until all queued callbacks or retrievals finish.
 */
#ifndef PYSYSTEMC_TEST_TLM_PEQ_PROBE_H
#define PYSYSTEMC_TEST_TLM_PEQ_PROBE_H

#include <tlm_utils/peq_with_cb_and_phase.h>

#include <cstdint>
#include <systemc>
#include <tlm>
#include <vector>

/** @brief Payload exercising arbitrary custom native data and 137-bit storage. */
struct PeqPayload
{
  int id = 0;
  sc_dt::sc_biguint<137> value = 0;
};

/** @brief Protocol using a custom payload and integral phase. */
struct PeqProtocol
{
  using tlm_payload_type = PeqPayload;
  using tlm_phase_type = unsigned int;
};

/** @brief One native callback observation, including borrowed address and kernel position. */
struct PeqCallbackSample
{
  int id;
  unsigned int phase;
  std::uintptr_t payload_address;
  sc_core::sc_time time;
  sc_dt::uint64 delta;
};

/** @brief Native OWNER for the member-pointer PEQ callback constructors. */
struct PeqOwner
{
  std::vector<PeqCallbackSample> records;

  /** @brief Retain borrowed payload address, copied phase and exact kernel position. */
  void deliver(PeqPayload& payload, const unsigned int& phase)
  {
    records.push_back({payload.id, phase, reinterpret_cast<std::uintptr_t>(&payload), sc_core::sc_time_stamp(),
                       sc_core::sc_delta_count()});
  }
};

/** @brief Instantiate and own both public custom-protocol member-pointer constructors. */
struct PeqNativeCallbackProbe
{
  using Queue = tlm_utils::peq_with_cb_and_phase<PeqOwner, PeqProtocol>;
  PeqOwner unnamed_owner;
  PeqOwner named_owner;
  Queue unnamed_queue;
  Queue named_queue;

  /** @brief Construct one auto-named and one explicitly named callback queue. */
  PeqNativeCallbackProbe()
    : unnamed_queue(&unnamed_owner, &PeqOwner::deliver)
    , named_queue("python_named_callback", &named_owner, &PeqOwner::deliver)
  {
  }
};

/** @brief One default-protocol callback observation for a generic payload. */
struct PeqDefaultSample
{
  std::uint64_t address;
  bool begin_request;
  std::uintptr_t payload_address;
  sc_core::sc_time time;
  sc_dt::uint64 delta;
};

/** @brief Native OWNER for the callback queue's default TLM protocol template. */
struct PeqDefaultOwner
{
  std::vector<PeqDefaultSample> records;

  /** @brief Record copied phase and payload address using the default protocol. */
  void deliver(tlm::tlm_generic_payload& payload, const tlm::tlm_phase& phase)
  {
    records.push_back({payload.get_address(), phase == tlm::BEGIN_REQ, reinterpret_cast<std::uintptr_t>(&payload),
                       sc_core::sc_time_stamp(), sc_core::sc_delta_count()});
  }
};

/** @brief Instantiate default-protocol member-pointer constructors for Python probing. */
struct PeqNativeDefaultCallbackProbe
{
  using Queue = tlm_utils::peq_with_cb_and_phase<PeqDefaultOwner>;
  PeqDefaultOwner unnamed_owner;
  PeqDefaultOwner named_owner;
  Queue unnamed_queue;
  Queue named_queue;

  /** @brief Construct auto-named and named queues with the native default TYPES. */
  PeqNativeDefaultCallbackProbe()
    : unnamed_queue(&unnamed_owner, &PeqDefaultOwner::deliver)
    , named_queue("python_named_default_callback", &named_owner, &PeqDefaultOwner::deliver)
  {
  }
};

#endif  // PYSYSTEMC_TEST_TLM_PEQ_PROBE_H

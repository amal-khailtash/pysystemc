/** @file tlm_utils_simple_b_probe.h
 *  @brief Native owner and callback records for simple socket `_b` probes.
 *  @details The owner outlives its sockets and callbacks. Native member-pointer
 *  registrations borrow this module; transactions and DMI storage are owned by
 *  callers/owner as documented. Callback methods execute on the kernel thread,
 *  do not enter Python, and may wait only on the blocking path.
 */
#ifndef PYSYSTEMC_TEST_TLM_UTILS_SIMPLE_B_PROBE_H
#define PYSYSTEMC_TEST_TLM_UTILS_SIMPLE_B_PROBE_H

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <array>
#include <cstdint>
#include <systemc>
#include <tlm>
#include <vector>

/** @brief Custom payload with a bit above native machine word width. */
struct SimpleBPayload : tlm::tlm_generic_payload
{
  int token = 0;
  sc_dt::sc_biguint<137> value = 0;
};

/** @brief Custom protocol preserving the wide payload and mutable TLM phase. */
struct SimpleBProtocol
{
  using tlm_payload_type = SimpleBPayload;
  using tlm_phase_type = tlm::tlm_phase;
};

/** @brief Capture exact callback identity, result state and kernel position. */
struct SimpleBRecord
{
  int token;
  int callback_id;
  int kind;
  std::uintptr_t payload_address;
  std::uint64_t address;
  std::uint64_t low_value;
  bool high_bit;
  int phase;
  sc_core::sc_time time;
  sc_dt::uint64 delta;
};

/** @brief Observe the target base's protected lifecycle hook while forwarding it. */
struct SimpleBTarget : tlm_utils::simple_target_socket_b<struct SimpleBNativeOwner, 73, SimpleBProtocol>
{
  using Base = tlm_utils::simple_target_socket_b<SimpleBNativeOwner, 73, SimpleBProtocol>;
  static inline int start_count = 0;

  /** @brief Construct the audited target with a native default or explicit name. */
  explicit SimpleBTarget(const char* name = Base::default_name()) : Base(name)
  {
  }

  /** @brief Invoke the protected native hook and count its lifecycle dispatch. */
  void start_of_simulation() override
  {
    Base::start_of_simulation();
    ++start_count;
  }
};

/** @brief Observe the tagged target base's protected lifecycle hook. */
struct SimpleBTaggedTarget : tlm_utils::simple_target_socket_tagged_b<struct SimpleBNativeOwner, 73, SimpleBProtocol>
{
  using Base = tlm_utils::simple_target_socket_tagged_b<SimpleBNativeOwner, 73, SimpleBProtocol>;
  static inline int start_count = 0;

  /** @brief Construct the audited tagged target with its native name overload. */
  explicit SimpleBTaggedTarget(const char* name = Base::default_name()) : Base(name)
  {
  }

  /** @brief Invoke the protected native hook and count its lifecycle dispatch. */
  void start_of_simulation() override
  {
    Base::start_of_simulation();
    ++start_count;
  }
};

/** @brief Own four native `_b` socket variants and register all native callbacks.
 *  @details Named and default-argument constructors are both used. All required
 *  sockets are bound before elaboration. Callback member pointers borrow this
 *  object; payload objects are supplied by Python/native callers. Destruction
 *  must follow simulation completion so no process retains this owner.
 */
struct SimpleBNativeOwner : sc_core::sc_module
{
  using Module = SimpleBNativeOwner;
  using Payload = SimpleBPayload;
  using Protocol = SimpleBProtocol;
  using PlainInitiator = tlm_utils::simple_initiator_socket_b<Module, 73, Protocol>;
  using TaggedInitiator = tlm_utils::simple_initiator_socket_tagged_b<Module, 73, Protocol>;
  using PlainTarget = tlm_utils::simple_target_socket_b<Module, 73, Protocol>;
  using TaggedTarget = tlm_utils::simple_target_socket_tagged_b<Module, 73, Protocol>;
  enum Kind
  {
    forward_nb = 1,
    forward_debug = 2,
    forward_dmi = 3,
    forward_block = 4,
    backward_nb = 5,
    invalidate = 6
  };

  PlainInitiator initiator_default;
  PlainInitiator initiator_named;
  TaggedInitiator tagged_initiator_default;
  TaggedInitiator tagged_initiator_named;
  SimpleBTarget target_default;
  SimpleBTarget target_named;
  SimpleBTaggedTarget tagged_target_default;
  SimpleBTaggedTarget tagged_target_named;
  std::array<unsigned char, 64> memory{};
  std::vector<SimpleBRecord> records;
  static inline int live_count = 0;

  SC_HAS_PROCESS(SimpleBNativeOwner);

  /** @brief Construct named/default sockets and install each callback overload.
   *  @param name Native module name retained for the complete socket lifecycle.
   *  @throws sc_core::sc_report if socket binding or registration fails.
   */
  explicit SimpleBNativeOwner(sc_core::sc_module_name name)
    : sc_module(name)
    , initiator_named("initiator_named")
    , tagged_initiator_named("tagged_initiator_named")
    , target_named("target_named")
    , tagged_target_named("tagged_target_named")
  {
    ++live_count;
    initiator_default.register_nb_transport_bw(this, &Module::backward_plain);
    initiator_default.register_invalidate_direct_mem_ptr(this, &Module::invalidate_plain);
    initiator_named.register_nb_transport_bw(this, &Module::backward_plain);
    initiator_named.register_invalidate_direct_mem_ptr(this, &Module::invalidate_plain_named);
    tagged_initiator_default.register_nb_transport_bw(this, &Module::backward_tagged, 202);
    tagged_initiator_default.register_invalidate_direct_mem_ptr(this, &Module::invalidate_tagged, 202);
    tagged_initiator_named.register_nb_transport_bw(this, &Module::backward_tagged, 203);
    tagged_initiator_named.register_invalidate_direct_mem_ptr(this, &Module::invalidate_tagged, 203);

    target_default.register_b_transport(this, &Module::blocking_plain);
    target_default.register_nb_transport_fw(this, &Module::forward_plain);
    target_default.register_transport_dbg(this, &Module::debug_plain);
    target_default.register_get_direct_mem_ptr(this, &Module::dmi_plain);
    target_named.register_b_transport(this, &Module::blocking_plain);
    target_named.register_nb_transport_fw(this, &Module::forward_plain);
    target_named.register_transport_dbg(this, &Module::debug_plain);
    target_named.register_get_direct_mem_ptr(this, &Module::dmi_plain);
    tagged_target_default.register_b_transport(this, &Module::blocking_tagged, 102);
    tagged_target_default.register_nb_transport_fw(this, &Module::forward_tagged, 102);
    tagged_target_default.register_transport_dbg(this, &Module::debug_tagged, 102);
    tagged_target_default.register_get_direct_mem_ptr(this, &Module::dmi_tagged, 102);
    tagged_target_named.register_b_transport(this, &Module::blocking_tagged, 103);
    tagged_target_named.register_nb_transport_fw(this, &Module::forward_tagged, 103);
    tagged_target_named.register_transport_dbg(this, &Module::debug_tagged, 103);
    tagged_target_named.register_get_direct_mem_ptr(this, &Module::dmi_tagged, 103);

    initiator_default.bind(target_default);
    initiator_named.bind(target_named);
    tagged_initiator_default.bind(tagged_target_default);
    tagged_initiator_named.bind(tagged_target_named);
  }

  /** @brief Release socket/module ownership after no callback can be dispatched. */
  ~SimpleBNativeOwner() override
  {
    --live_count;
  }

  /** @brief Return the initiator forward interface selected by token 0..3. */
  tlm::tlm_fw_transport_if<Protocol>& forward_interface(int token)
  {
    if (token == 0) return *initiator_default.operator->();
    if (token == 1) return *initiator_named.operator->();
    if (token == 2) return *tagged_initiator_default.operator->();
    return *tagged_initiator_named.operator->();
  }

  /** @brief Return the target's owned backward adapter for token 0..3. */
  tlm::tlm_bw_transport_if<Protocol>& backward_interface(int token)
  {
    if (token == 0) return *target_default.operator->();
    if (token == 1) return *target_named.operator->();
    if (token == 2) return *tagged_target_default.operator->();
    return *tagged_target_named.operator->();
  }

  /** @brief Return the address produced by the target's native operator->. */
  std::uintptr_t backward_operator_address(int token)
  {
    return reinterpret_cast<std::uintptr_t>(&backward_interface(token));
  }

  /** @brief Expose the exact ordinary base specialization for Python reflection. */
  PlainTarget& plain_target_base(int token)
  {
    if (token == 0) return target_default;
    return target_named;
  }

  /** @brief Expose the exact tagged base specialization for Python reflection. */
  TaggedTarget& tagged_target_base(int token)
  {
    if (token == 0) return tagged_target_default;
    return tagged_target_named;
  }

  /** @brief Append a borrowed transaction observation with exact kernel state. */
  void record(int token, int callback_id, int kind, Payload& payload, int phase)
  {
    records.push_back({token, callback_id, kind, reinterpret_cast<std::uintptr_t>(&payload), payload.get_address(),
                       payload.value.to_uint64(), payload.value[136].to_bool(), phase, sc_core::sc_time_stamp(),
                       sc_core::sc_delta_count()});
  }

  /** @brief Handle a blocking request and preserve the original borrowed payload. */
  void blocking_plain(Payload& payload, sc_core::sc_time& delay)
  {
    blocking(payload.token, 100 + payload.token, payload, delay);
  }
  /** @brief Handle a tagged blocking request with the registered target ID. */
  void blocking_tagged(int id, Payload& payload, sc_core::sc_time& delay)
  {
    if (id != 100 + payload.token) SC_REPORT_FATAL("SIMPLE_B", "tagged blocking ID");
    blocking(payload.token, id, payload, delay);
  }

  /** @brief Complete common blocking callback after annotated native wait. */
  void blocking(int token, int callback_id, Payload& payload, sc_core::sc_time& delay)
  {
    check(token, payload);
    wait(delay);
    payload.set_address(payload.get_address() + 4);
    payload.set_response_status(tlm::TLM_OK_RESPONSE);
    payload.value = 23 + token;
    delay = sc_core::SC_ZERO_TIME;
    record(token, callback_id, forward_block, payload, -1);
  }

  /** @brief Update an ordinary target's nonblocking forward phase. */
  tlm::tlm_sync_enum forward_plain(Payload& payload, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    check(payload.token, payload);
    phase = tlm::BEGIN_RESP;
    delay = sc_core::sc_time(1, sc_core::SC_NS);
    record(payload.token, 100 + payload.token, forward_nb, payload, static_cast<int>(phase));
    return tlm::TLM_UPDATED;
  }
  /** @brief Update a tagged target's nonblocking phase with its registered ID. */
  tlm::tlm_sync_enum forward_tagged(int id, Payload& payload, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    if (id != 100 + payload.token) SC_REPORT_FATAL("SIMPLE_B", "tagged forward ID");
    check(payload.token, payload);
    phase = tlm::BEGIN_RESP;
    delay = sc_core::sc_time(1, sc_core::SC_NS);
    record(payload.token, id, forward_nb, payload, static_cast<int>(phase));
    return tlm::TLM_UPDATED;
  }

  /** @brief Return a deterministic ordinary debug byte count. */
  unsigned int debug_plain(Payload& payload)
  {
    check(payload.token, payload);
    record(payload.token, 100 + payload.token, forward_debug, payload, -1);
    return 32 + payload.token;
  }
  /** @brief Return a deterministic tagged debug byte count. */
  unsigned int debug_tagged(int id, Payload& payload)
  {
    if (id != 100 + payload.token) SC_REPORT_FATAL("SIMPLE_B", "tagged debug ID");
    check(payload.token, payload);
    record(payload.token, id, forward_debug, payload, -1);
    return 32 + payload.token;
  }

  /** @brief Grant ordinary DMI into owner memory retained through simulation. */
  bool dmi_plain(Payload& payload, tlm::tlm_dmi& result)
  {
    return grant_dmi(payload.token, 100 + payload.token, payload, result);
  }
  /** @brief Grant tagged DMI using its registration ID. */
  bool dmi_tagged(int id, Payload& payload, tlm::tlm_dmi& result)
  {
    if (id != 100 + payload.token) SC_REPORT_FATAL("SIMPLE_B", "tagged DMI ID");
    return grant_dmi(payload.token, id, payload, result);
  }
  /** @brief Populate DMI range, storage pointer and permission/latency state. */
  bool grant_dmi(int token, int callback_id, Payload& payload, tlm::tlm_dmi& result)
  {
    check(token, payload);
    result.set_start_address(16 + token * 16);
    result.set_end_address(31 + token * 16);
    result.set_dmi_ptr(memory.data() + token * 16);
    result.set_read_latency(sc_core::sc_time(1, sc_core::SC_NS));
    result.set_write_latency(sc_core::sc_time(2, sc_core::SC_NS));
    result.allow_read_write();
    record(token, callback_id, forward_dmi, payload, -1);
    return true;
  }

  /** @brief Complete ordinary backward response with native completion status. */
  tlm::tlm_sync_enum backward_plain(Payload& payload, tlm::tlm_phase& phase, sc_core::sc_time&)
  {
    phase = tlm::END_RESP;
    record(payload.token, 200 + payload.token, backward_nb, payload, static_cast<int>(phase));
    return tlm::TLM_COMPLETED;
  }
  /** @brief Complete tagged backward response after checking registered ID. */
  tlm::tlm_sync_enum backward_tagged(int id, Payload& payload, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    if (id != 200 + payload.token) SC_REPORT_FATAL("SIMPLE_B", "tagged backward ID");
    return backward_plain(payload, phase, delay);
  }
  /** @brief Record ordinary full-width invalidation range. */
  void invalidate_plain(sc_dt::uint64 first, sc_dt::uint64 last)
  {
    invalidate_common(0, first, last);
  }
  /** @brief Record the named ordinary initiator's invalidation callback. */
  void invalidate_plain_named(sc_dt::uint64 first, sc_dt::uint64 last)
  {
    invalidate_common(1, first, last);
  }
  /** @brief Record tagged full-width invalidation after checking user ID. */
  void invalidate_tagged(int id, sc_dt::uint64 first, sc_dt::uint64 last)
  {
    if (id != 202 && id != 203) SC_REPORT_FATAL("SIMPLE_B", "tagged invalidation ID");
    invalidate_common(id - 200, first, last);
  }
  /** @brief Retain invalidation boundaries and exact 64-bit callback values. */
  void invalidate_common(int token, sc_dt::uint64 first, sc_dt::uint64 last)
  {
    SimpleBRecord sample{
      token, 200 + token, invalidate, 0, first, last, false, -1, sc_core::sc_time_stamp(), sc_core::sc_delta_count()};
    records.push_back(sample);
  }

  /** @brief Validate identity, address and wide data before forward callbacks. */
  void check(int token, Payload& payload)
  {
    if (token < 0 || token > 3) SC_REPORT_FATAL("SIMPLE_B", "payload token range");
    if (payload.token != token) SC_REPORT_FATAL("SIMPLE_B", "payload token mismatch");
    if (payload.get_address() != static_cast<sc_dt::uint64>(17 + token)) SC_REPORT_FATAL("SIMPLE_B", "payload address");
    if (!payload.value[136] || payload.value.to_uint64() != static_cast<unsigned>(17 + token))
      SC_REPORT_FATAL("SIMPLE_B", "payload wide value");
  }
};

#endif  // PYSYSTEMC_TEST_TLM_UTILS_SIMPLE_B_PROBE_H

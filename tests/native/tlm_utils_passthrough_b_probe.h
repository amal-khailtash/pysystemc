/** @file tlm_utils_passthrough_b_probe.h
 *  @brief Native owners for the non-wrapper passthrough target socket APIs.
 *  @details The module owns its sockets, callbacks, and payload. Socket
 *  interfaces borrow that storage; callers must keep the module alive through
 *  all interface calls and simulation. Native callbacks run synchronously on
 *  the SystemC thread, and the blocking callbacks wait only from the owner's
 *  registered SC_THREAD. No Python callbacks or GIL transitions are involved.
 */
#ifndef PYSYSTEMC_TLM_UTILS_PASSTHROUGH_B_PROBE_H
#define PYSYSTEMC_TLM_UTILS_PASSTHROUGH_B_PROBE_H

#include <tlm_utils/passthrough_target_socket.h>

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <tlm>
#include <vector>

namespace tlm_utils_passthrough_b_probe
{
/** @brief Throw on a native reference mismatch, including release builds. */
inline void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief A transaction with a payload field wider than 128 bits. */
struct Payload137 : tlm::tlm_generic_payload
{
  sc_dt::sc_biguint<137> wide;
  int marker = 0;
};

/** @brief A protocol preserving custom payload and mutable native phase types. */
struct Protocol137
{
  using tlm_payload_type = Payload137;
  using tlm_phase_type = tlm::tlm_phase;
};

/** @brief Owner registering native callbacks on both passthrough B families.
 *  @details The target ports and initiator exports are bound by the Python
 *  model or native reference before elaboration. Registered member pointers
 *  borrow this module. Socket and payload proxies are also borrowed from this
 *  owner and must not outlive it.
 */
struct Module : sc_core::sc_module, tlm::tlm_bw_transport_if<Protocol137>
{
  using plain_socket_type = tlm_utils::passthrough_target_socket_b<Module, 73, Protocol137>;
  using tagged_socket_type = tlm_utils::passthrough_target_socket_tagged_b<Module, 73, Protocol137>;
  using initiator_socket_type = tlm::tlm_initiator_socket<73, Protocol137>;

  plain_socket_type plain_target;
  tagged_socket_type tagged_target;
  initiator_socket_type plain_initiator;
  initiator_socket_type tagged_initiator;
  Payload137 payload;
  std::vector<std::string> events;
  unsigned plain_start_ns = 0;
  unsigned plain_done_ns = 0;
  unsigned tagged_start_ns = 0;
  unsigned tagged_done_ns = 0;
  unsigned callbacks = 0;
  bool blocking_done = false;
  inline static unsigned destroyed = 0;

  SC_HAS_PROCESS(Module);

  /** @brief Construct sockets and register process-lifetime native callbacks. */
  explicit Module(sc_core::sc_module_name name)
    : sc_core::sc_module(name)
    , tagged_target("tagged_target")
    , plain_initiator("plain_initiator")
    , tagged_initiator("tagged_initiator")
  {
    payload.set_address(17);
    payload.wide = (sc_dt::sc_biguint<137>(1) << 136) | 17;
    plain_target.register_nb_transport_fw(this, &Module::plain_forward);
    plain_target.register_b_transport(this, &Module::plain_blocking);
    plain_target.register_transport_dbg(this, &Module::plain_debug);
    plain_target.register_get_direct_mem_ptr(this, &Module::plain_dmi);
    tagged_target.register_nb_transport_fw(this, &Module::tagged_forward, 37);
    tagged_target.register_b_transport(this, &Module::tagged_blocking, 37);
    tagged_target.register_transport_dbg(this, &Module::tagged_debug, 37);
    tagged_target.register_get_direct_mem_ptr(this, &Module::tagged_dmi, 37);
    SC_THREAD(run_blocking_calls);
  }

  /** @brief Count destruction after all borrowed callbacks and interfaces end. */
  ~Module() override
  {
    ++destroyed;
  }

  /** @brief Handle the backward interface reached through target operator[]. */
  tlm::tlm_sync_enum nb_transport_bw(Payload137& trans, tlm::tlm_phase& phase, sc_core::sc_time& delay) override
  {
    check_payload(trans);
    const int old_phase = static_cast<int>(phase);
    const auto old_delay = delay.value();
    phase = tlm::END_RESP;
    delay += sc_core::sc_time(5, sc_core::SC_NS);
    std::ostringstream record;
    record << "backward|" << trans.marker << '|' << old_phase << '|' << static_cast<int>(phase) << '|' << old_delay
           << '|' << delay.value();
    events.push_back(record.str());
    return tlm::TLM_COMPLETED;
  }

  /** @brief Record a DMI invalidation received through target operator[]. */
  void invalidate_direct_mem_ptr(sc_dt::uint64 first, sc_dt::uint64 last) override
  {
    std::ostringstream record;
    record << "invalidate|" << payload.marker << '|' << first << '|' << last;
    events.push_back(record.str());
  }

  /** @brief Record a native callback and assert payload identity and width. */
  void check_payload(Payload137& trans)
  {
    verify(&trans == &payload, "passthrough callback lost payload identity");
    verify(trans.wide[136].to_bool(), "passthrough callback lost bit 136");
    ++callbacks;
  }

  /** @brief Append a forward-callback observation for native/Python parity. */
  void record_forward(const char* label, int id, Payload137& trans, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    std::ostringstream record;
    record << label << '|' << id << '|' << trans.get_address() << '|' << trans.wide.to_uint64() << '|'
           << trans.wide[136].to_bool() << '|' << static_cast<int>(phase) << '|' << delay.value();
    events.push_back(record.str());
  }

  /** @brief Mutate forward phase and annotation in the untagged callback. */
  tlm::tlm_sync_enum plain_forward(Payload137& trans, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    check_payload(trans);
    phase = tlm::BEGIN_RESP;
    delay = sc_core::sc_time(1, sc_core::SC_NS);
    record_forward("forward_plain", -1, trans, phase, delay);
    return tlm::TLM_UPDATED;
  }

  /** @brief Mutate forward phase, annotation, and registered tagged ID. */
  tlm::tlm_sync_enum tagged_forward(int id, Payload137& trans, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    check_payload(trans);
    verify(id == 37, "tagged forward callback ID mismatch");
    phase = tlm::END_REQ;
    delay = sc_core::sc_time(2, sc_core::SC_NS);
    record_forward("forward_tagged", id, trans, phase, delay);
    return tlm::TLM_UPDATED;
  }

  /** @brief Wait for the plain annotation then mutate its borrowed payload. */
  void plain_blocking(Payload137& trans, sc_core::sc_time& delay)
  {
    check_payload(trans);
    plain_start_ns = now_ns();
    const auto annotated = delay.value();
    wait(delay);
    plain_done_ns = now_ns();
    delay = sc_core::SC_ZERO_TIME;
    trans.set_address(trans.get_address() + 4);
    trans.wide = (sc_dt::sc_biguint<137>(1) << 136) | 23;
    append_blocking("blocking_plain", -1, annotated, delay, trans, plain_start_ns, plain_done_ns);
  }

  /** @brief Wait for the tagged annotation and record its native callback ID. */
  void tagged_blocking(int id, Payload137& trans, sc_core::sc_time& delay)
  {
    check_payload(trans);
    verify(id == 37, "tagged blocking callback ID mismatch");
    tagged_start_ns = now_ns();
    const auto annotated = delay.value();
    wait(delay);
    tagged_done_ns = now_ns();
    delay = sc_core::SC_ZERO_TIME;
    trans.set_address(trans.get_address() + 8);
    trans.wide = (sc_dt::sc_biguint<137>(1) << 136) | 29;
    append_blocking("blocking_tagged", id, annotated, delay, trans, tagged_start_ns, tagged_done_ns);
  }

  /** @brief Return the plain callback's full-width unsigned debug result. */
  unsigned int plain_debug(Payload137& trans)
  {
    check_payload(trans);
    events.emplace_back("debug_plain|4026531857");
    return 0xf0000011U;
  }

  /** @brief Return a tagged debug result that includes the registered ID. */
  unsigned int tagged_debug(int id, Payload137& trans)
  {
    check_payload(trans);
    verify(id == 37, "tagged debug callback ID mismatch");
    events.emplace_back("debug_tagged|37|4026531877");
    return 0xf0000025U;
  }

  /** @brief Fill all read/write DMI metadata in the plain callback. */
  bool plain_dmi(Payload137& trans, tlm::tlm_dmi& dmi)
  {
    check_payload(trans);
    dmi.set_start_address(0x1000);
    dmi.set_end_address(0x10ff);
    dmi.allow_read_write();
    dmi.set_read_latency(sc_core::sc_time(3, sc_core::SC_NS));
    dmi.set_write_latency(sc_core::sc_time(4, sc_core::SC_NS));
    events.emplace_back("dmi_plain|4096|4351|3|4");
    return true;
  }

  /** @brief Fill DMI metadata for the registered tagged callback. */
  bool tagged_dmi(int id, Payload137& trans, tlm::tlm_dmi& dmi)
  {
    check_payload(trans);
    verify(id == 37, "tagged DMI callback ID mismatch");
    dmi.set_start_address(0x2000);
    dmi.set_end_address(0x20ff);
    dmi.allow_read();
    dmi.set_read_latency(sc_core::sc_time(6, sc_core::SC_NS));
    events.emplace_back("dmi_tagged|37|8192|8447|6");
    return true;
  }

  /** @brief Return nanoseconds as an exact integer at default time resolution. */
  static unsigned now_ns()
  {
    return static_cast<unsigned>(sc_core::sc_time_stamp().value() / sc_core::sc_time(1, sc_core::SC_NS).value());
  }

  /** @brief Append callback time, mutated payload, and returned delay. */
  void append_blocking(const char* label, int id, std::uint64_t annotated, const sc_core::sc_time& delay,
                       const Payload137& trans, unsigned start, unsigned done)
  {
    std::ostringstream record;
    record << label << '|' << id << '|' << start << '|' << done << '|' << annotated << '|' << delay.value() << '|'
           << trans.get_address() << '|' << trans.wide.to_uint64() << '|' << trans.wide[136].to_bool();
    events.push_back(record.str());
  }

  /** @brief Drive blocking calls from an SC_THREAD so wait() has native context. */
  void run_blocking_calls()
  {
    sc_core::sc_time plain_delay(2, sc_core::SC_NS);
    plain_target.get_base_interface().b_transport(payload, plain_delay);
    verify(plain_start_ns == 0 && plain_done_ns == 2, "plain blocking wait/time mismatch");
    verify(plain_delay == sc_core::SC_ZERO_TIME, "plain blocking callback did not clear delay");

    payload.set_address(17);
    payload.wide = (sc_dt::sc_biguint<137>(1) << 136) | 17;
    sc_core::sc_time tagged_delay(3, sc_core::SC_NS);
    tagged_target.get_base_interface().b_transport(payload, tagged_delay);
    verify(tagged_start_ns == 2 && tagged_done_ns == 5, "tagged blocking wait/time mismatch");
    verify(tagged_delay == sc_core::SC_ZERO_TIME, "tagged blocking callback did not clear delay");
    blocking_done = true;
  }
};
}  // namespace tlm_utils_passthrough_b_probe

#endif  // PYSYSTEMC_TLM_UTILS_PASSTHROUGH_B_PROBE_H

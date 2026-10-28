/** @file tlm_utils_multi_probe.h
 *  @brief Native owner and protocol for multi-passthrough socket probes.
 *  @details Socket members own native binders. Callback member functions are
 *  registered against their exact upstream typedefs; payloads, phases and
 *  delays remain caller-owned. The callbacks run on the SystemC kernel thread
 *  and do not acquire the Python GIL.
 */
#ifndef PYSYSTEMC_TLM_UTILS_MULTI_PROBE_H
#define PYSYSTEMC_TLM_UTILS_MULTI_PROBE_H

#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <systemc>
#include <tlm>
#include <type_traits>

namespace pysystemc_tlm_utils_multi
{

using Wide = sc_dt::sc_biguint<137>;

/** @brief Return a 137-bit value with the high bit and low word set. */
inline Wide wide(unsigned int low)
{
  Wide value = low;
  value[136] = true;
  return value;
}

/** @brief Custom payload carries an arbitrary-width application value. */
struct Payload : tlm::tlm_generic_payload
{
  Wide value = 0;
};

/** @brief Native protocol with the custom payload and standard phase type. */
struct Protocol
{
  using tlm_payload_type = Payload;
  using tlm_phase_type = tlm::tlm_phase;
};

struct Owner;

/** @brief Count socket lifecycle hooks and destruction in the standalone probes. */
struct SocketLifecycle
{
  static std::array<int, 2>& initiator_before_end();
  static std::array<int, 2>& target_end();
  static std::array<int, 2>& initiator_destroyed();
  static std::array<int, 2>& target_destroyed();
  static void reset();
};

using InitiatorBase =
  tlm_utils::multi_passthrough_initiator_socket<Owner, 73, Protocol, 0, sc_core::SC_ZERO_OR_MORE_BOUND>;
using TargetBase = tlm_utils::multi_passthrough_target_socket<Owner, 73, Protocol, 0, sc_core::SC_ZERO_OR_MORE_BOUND>;

/** @brief Instrument the native initiator hook and destructor. */
struct InitiatorSocket : InitiatorBase
{
  using InitiatorBase::InitiatorBase;
  explicit InitiatorSocket(const char* name = InitiatorBase::default_name()) : InitiatorBase(name)
  {
  }
  ~InitiatorSocket() override;
  void before_end_of_elaboration() override;
};

/** @brief Instrument the native target hook and destructor. */
struct TargetSocket : TargetBase
{
  using TargetBase::TargetBase;
  explicit TargetSocket(const char* name = TargetBase::default_name()) : TargetBase(name)
  {
  }
  ~TargetSocket() override;
  void end_of_elaboration() override;
};

/** @brief Own two hierarchical socket paths and native callback registrations.
 *  @details The outer initiator and target sockets use native default names;
 *  their inner sockets use explicit names. Python may bind the public members
 *  directly. The owner outlives all sockets and callbacks.
 */
struct Owner : sc_core::sc_module
{
  using Initiator = InitiatorSocket;
  using Target = TargetSocket;
  using SimpleTarget = tlm_utils::simple_target_socket<Owner, 73, Protocol>;
  using SimpleInitiator = tlm_utils::simple_initiator_socket<Owner, 73, Protocol>;

  Initiator outer_initiator;
  Initiator inner_initiator;
  Target outer_target;
  Target inner_target;
  SimpleTarget forward_target_0;
  SimpleTarget forward_target_1;
  SimpleInitiator backward_initiator_0;
  SimpleInitiator backward_initiator_1;

  std::array<int, 2> initiator_nb_calls{};
  std::array<int, 2> initiator_dmi_calls{};
  std::array<int, 2> target_b_calls{};
  std::array<int, 2> target_nb_calls{};
  std::array<int, 2> target_dbg_calls{};
  std::array<int, 2> target_dmi_calls{};
  std::array<int, 2> forward_b_calls{};
  std::array<int, 2> forward_nb_calls{};
  std::array<int, 2> forward_dbg_calls{};
  std::array<int, 2> forward_dmi_calls{};
  std::array<int, 2> backward_nb_calls{};
  std::array<int, 2> backward_dmi_calls{};
  std::array<int, 2> last_backward_phase{};
  std::array<std::uint64_t, 2> last_backward_delay{};
  std::array<int, 2> last_invalidation{};

  SC_HAS_PROCESS(Owner);

  /** @brief Construct native sockets and register simple endpoint callbacks. */
  explicit Owner(sc_core::sc_module_name name)
    : sc_module(name)
    , inner_initiator("inner_initiator")
    , inner_target("inner_target")
    , forward_target_0("forward_target_0")
    , forward_target_1("forward_target_1")
    , backward_initiator_0("backward_initiator_0")
    , backward_initiator_1("backward_initiator_1")
  {
    forward_target_0.register_b_transport(this, &Owner::forward_b);
    forward_target_0.register_nb_transport_fw(this, &Owner::forward_nb);
    forward_target_0.register_transport_dbg(this, &Owner::forward_dbg);
    forward_target_0.register_get_direct_mem_ptr(this, &Owner::forward_dmi);
    forward_target_1.register_b_transport(this, &Owner::forward_b);
    forward_target_1.register_nb_transport_fw(this, &Owner::forward_nb);
    forward_target_1.register_transport_dbg(this, &Owner::forward_dbg);
    forward_target_1.register_get_direct_mem_ptr(this, &Owner::forward_dmi);
    backward_initiator_0.register_nb_transport_bw(this, &Owner::backward_nb);
    backward_initiator_0.register_invalidate_direct_mem_ptr(this, &Owner::backward_dmi);
    backward_initiator_1.register_nb_transport_bw(this, &Owner::backward_nb);
    backward_initiator_1.register_invalidate_direct_mem_ptr(this, &Owner::backward_dmi);
  }

  /** @brief Register initiator callbacks with exact native member-pointer aliases. */
  void register_initiator_callbacks()
  {
    using NbCallback = InitiatorBase::nb_cb;
    using DmiCallback = InitiatorBase::dmi_cb;
    static_assert(
      std::is_same_v<NbCallback, tlm::tlm_sync_enum (Owner::*)(int, Payload&, tlm::tlm_phase&, sc_core::sc_time&)>);
    static_assert(std::is_same_v<DmiCallback, void (Owner::*)(int, sc_dt::uint64, sc_dt::uint64)>);
    const NbCallback nb = &Owner::initiator_nb;
    const DmiCallback dmi = &Owner::initiator_invalidate;
    outer_initiator.register_nb_transport_bw(this, nb);
    outer_initiator.register_invalidate_direct_mem_ptr(this, dmi);
  }

  /** @brief Register target callbacks with exact native member-pointer aliases. */
  void register_target_callbacks()
  {
    using NbCallback = TargetBase::nb_cb;
    using BCallback = TargetBase::b_cb;
    using DebugCallback = TargetBase::dbg_cb;
    using DmiCallback = TargetBase::dmi_cb;
    static_assert(
      std::is_same_v<NbCallback, tlm::tlm_sync_enum (Owner::*)(int, Payload&, tlm::tlm_phase&, sc_core::sc_time&)>);
    static_assert(std::is_same_v<BCallback, void (Owner::*)(int, Payload&, sc_core::sc_time&)>);
    static_assert(std::is_same_v<DebugCallback, unsigned int (Owner::*)(int, Payload&)>);
    static_assert(std::is_same_v<DmiCallback, bool (Owner::*)(int, Payload&, tlm::tlm_dmi&)>);
    const NbCallback nb = &Owner::target_nb;
    const BCallback blocking = &Owner::target_b;
    const DebugCallback debug = &Owner::target_dbg;
    const DmiCallback dmi = &Owner::target_dmi;
    inner_target.register_nb_transport_fw(this, nb);
    inner_target.register_b_transport(this, blocking);
    inner_target.register_transport_dbg(this, debug);
    inner_target.register_get_direct_mem_ptr(this, dmi);
  }

  /** @brief Bind one hierarchical path per socket family and two connections.
   *  @details Uses native base casts because cppyy cannot always convert a
   *  derived multi target socket to its dependent native base alias.
   */
  void bind_paths()
  {
    outer_initiator.bind(static_cast<InitiatorBase&>(inner_initiator));
    inner_initiator.bind(forward_target_0);
    inner_initiator(forward_target_1);
    outer_target.bind(static_cast<TargetBase&>(inner_target));
    backward_initiator_0.bind(outer_target);
    backward_initiator_1.bind(outer_target);
  }

  /** @brief Send one forward call from an indexed native initiator socket.
   *  @details These helpers let Python reach the simple initiator's forward
   *  port through cppyy while preserving the native callback dispatch.
   */
  tlm::tlm_sync_enum issue_target_nb(int index, Payload& payload, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    return (index == 0 ? backward_initiator_0 : backward_initiator_1)->nb_transport_fw(payload, phase, delay);
  }
  void issue_target_b(int index, Payload& payload, sc_core::sc_time& delay)
  {
    (index == 0 ? backward_initiator_0 : backward_initiator_1)->b_transport(payload, delay);
  }
  unsigned int issue_target_dbg(int index, Payload& payload)
  {
    return (index == 0 ? backward_initiator_0 : backward_initiator_1)->transport_dbg(payload);
  }
  bool issue_target_dmi(int index, Payload& payload, tlm::tlm_dmi& dmi)
  {
    return (index == 0 ? backward_initiator_0 : backward_initiator_1)->get_direct_mem_ptr(payload, dmi);
  }

  /** @brief Return the connection index encoded in a request address. */
  static int request_id(const Payload& payload, std::uint64_t base)
  {
    const auto address = payload.get_address();
    if (address < base || address >= base + 2) throw std::runtime_error("unexpected multi-socket payload address");
    return static_cast<int>(address - base);
  }

  /** @brief Update the backward phase and delay from a multi initiator callback. */
  tlm::tlm_sync_enum initiator_nb(int index, Payload& payload, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    if (index < 0 || index > 1 || request_id(payload, 0xA000) != index)
      throw std::runtime_error("multi initiator callback index mismatch");
    ++initiator_nb_calls[index];
    phase = tlm::END_RESP;
    delay = sc_core::sc_time(60 + index, sc_core::SC_NS);
    last_backward_phase[index] = static_cast<int>(phase);
    last_backward_delay[index] = delay.value();
    return tlm::TLM_COMPLETED;
  }

  /** @brief Record a backward invalidation with its bound-connection index. */
  void initiator_invalidate(int index, sc_dt::uint64 first, sc_dt::uint64 last)
  {
    if (index < 0 || index > 1 || first != 0xC000 + index || last != 0xC0FF + index)
      throw std::runtime_error("multi initiator invalidation mismatch");
    ++initiator_dmi_calls[index];
    last_invalidation[index] = index;
  }

  /** @brief Update a multi target request and its annotated blocking delay. */
  void target_b(int index, Payload& payload, sc_core::sc_time& delay)
  {
    if (index < 0 || index > 1 || request_id(payload, 0xB000) != index)
      throw std::runtime_error("multi target blocking index mismatch");
    ++target_b_calls[index];
    payload.set_address(0xB100 + index);
    payload.value = wide(0x40 + index);
    delay = sc_core::sc_time(40 + index, sc_core::SC_NS);
  }

  /** @brief Update a multi target request phase and delay, then return upstream. */
  tlm::tlm_sync_enum target_nb(int index, Payload& payload, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    if (index < 0 || index > 1 || request_id(payload, 0xB000) != index)
      throw std::runtime_error("multi target nonblocking index mismatch");
    ++target_nb_calls[index];
    tlm::tlm_phase backward_phase = tlm::BEGIN_RESP;
    sc_core::sc_time backward_delay = sc_core::SC_ZERO_TIME;
    const auto result = inner_target[index]->nb_transport_bw(payload, backward_phase, backward_delay);
    if (result != tlm::TLM_COMPLETED || backward_phase != tlm::END_RESP ||
        backward_delay != sc_core::sc_time(80 + index, sc_core::SC_NS))
      throw std::runtime_error("multi target backward callback mismatch");
    phase = tlm::BEGIN_RESP;
    delay = sc_core::sc_time(50 + index, sc_core::SC_NS);
    return tlm::TLM_UPDATED;
  }

  /** @brief Return a distinct debug byte count for each target connection. */
  unsigned int target_dbg(int index, Payload& payload)
  {
    if (index < 0 || index > 1 || request_id(payload, 0xB000) != index)
      throw std::runtime_error("multi target debug index mismatch");
    ++target_dbg_calls[index];
    return 70 + static_cast<unsigned int>(index);
  }

  /** @brief Fill a denied DMI result with connection-specific output bounds. */
  bool target_dmi(int index, Payload& payload, tlm::tlm_dmi& dmi)
  {
    if (index < 0 || index > 1 || request_id(payload, 0xB000) != index)
      throw std::runtime_error("multi target DMI index mismatch");
    ++target_dmi_calls[index];
    dmi.set_start_address(0xD000 + index);
    dmi.set_end_address(0xD0FF + index);
    dmi.allow_none();
    return false;
  }

  /** @brief Complete blocking traffic through a multi initiator connection. */
  void forward_b(Payload& payload, sc_core::sc_time& delay)
  {
    const int index = request_id(payload, 0xA000);
    ++forward_b_calls[index];
    payload.set_address(0xA100 + index);
    payload.value = wide(0x10 + index);
    delay = sc_core::sc_time(10 + index, sc_core::SC_NS);
  }

  /** @brief Complete nonblocking traffic through a multi initiator connection. */
  tlm::tlm_sync_enum forward_nb(Payload& payload, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    const int index = request_id(payload, 0xA000);
    ++forward_nb_calls[index];
    tlm::tlm_phase backward_phase = tlm::BEGIN_REQ;
    sc_core::sc_time backward_delay = sc_core::SC_ZERO_TIME;
    const auto result =
      (index == 0 ? forward_target_0 : forward_target_1)->nb_transport_bw(payload, backward_phase, backward_delay);
    if (result != tlm::TLM_COMPLETED || backward_phase != tlm::END_RESP ||
        backward_delay != sc_core::sc_time(60 + index, sc_core::SC_NS))
      throw std::runtime_error("multi initiator backward callback mismatch");
    phase = tlm::BEGIN_RESP;
    delay = sc_core::sc_time(20 + index, sc_core::SC_NS);
    return tlm::TLM_UPDATED;
  }

  /** @brief Return a distinct debug count from each simple target. */
  unsigned int forward_dbg(Payload& payload)
  {
    const int index = request_id(payload, 0xA000);
    ++forward_dbg_calls[index];
    return 30 + static_cast<unsigned int>(index);
  }

  /** @brief Grant per-target DMI over the custom payload value storage. */
  bool forward_dmi(Payload& payload, tlm::tlm_dmi& dmi)
  {
    const int index = request_id(payload, 0xA000);
    ++forward_dmi_calls[index];
    dmi.set_start_address(0xE000 + index);
    dmi.set_end_address(0xE0FF + index);
    dmi.set_dmi_ptr(reinterpret_cast<unsigned char*>(&payload.value));
    dmi.set_read_latency(sc_core::sc_time(1 + index, sc_core::SC_NS));
    dmi.set_write_latency(sc_core::sc_time(2 + index, sc_core::SC_NS));
    dmi.allow_read_write();
    return true;
  }

  /** @brief Complete reverse traffic from a multi target's upstream connection. */
  tlm::tlm_sync_enum backward_nb(Payload& payload, tlm::tlm_phase& phase, sc_core::sc_time& delay)
  {
    const int index = request_id(payload, 0xB000);
    ++backward_nb_calls[index];
    phase = tlm::END_RESP;
    delay = sc_core::sc_time(80 + index, sc_core::SC_NS);
    return tlm::TLM_COMPLETED;
  }

  /** @brief Record reverse DMI invalidation without retaining its arguments. */
  void backward_dmi(sc_dt::uint64 first, sc_dt::uint64 last)
  {
    int index = -1;
    for (int candidate = 0; candidate != 2; ++candidate)
      if (first == 0xC000 + candidate && last == 0xC0FF + candidate) index = candidate;
    if (index < 0) throw std::runtime_error("multi target invalidation mismatch");
    ++backward_dmi_calls[index];
    last_invalidation[index] = index;
  }

  /** @brief Return zero-width-independent callback summary for one connection. */
  int call_total(int index) const
  {
    return initiator_nb_calls[index] + initiator_dmi_calls[index] + target_b_calls[index] + target_nb_calls[index] +
           target_dbg_calls[index] + target_dmi_calls[index] + forward_b_calls[index] + forward_nb_calls[index] +
           forward_dbg_calls[index] + forward_dmi_calls[index] + backward_nb_calls[index] + backward_dmi_calls[index];
  }
};

inline std::array<int, 2>& SocketLifecycle::initiator_before_end()
{
  static std::array<int, 2> values{};
  return values;
}
inline std::array<int, 2>& SocketLifecycle::target_end()
{
  static std::array<int, 2> values{};
  return values;
}
inline std::array<int, 2>& SocketLifecycle::initiator_destroyed()
{
  static std::array<int, 2> values{};
  return values;
}
inline std::array<int, 2>& SocketLifecycle::target_destroyed()
{
  static std::array<int, 2> values{};
  return values;
}
inline void SocketLifecycle::reset()
{
  initiator_before_end().fill(0);
  target_end().fill(0);
  initiator_destroyed().fill(0);
  target_destroyed().fill(0);
}
inline InitiatorSocket::~InitiatorSocket()
{
  ++SocketLifecycle::initiator_destroyed()[0];
}
inline void InitiatorSocket::before_end_of_elaboration()
{
  ++SocketLifecycle::initiator_before_end()[0];
  InitiatorBase::before_end_of_elaboration();
}
inline TargetSocket::~TargetSocket()
{
  ++SocketLifecycle::target_destroyed()[0];
}
inline void TargetSocket::end_of_elaboration()
{
  ++SocketLifecycle::target_end()[0];
  TargetBase::end_of_elaboration();
}

/** @brief Dispatch a const native interface lookup and report its diagnostic. */
inline bool const_initiator_interface_rejected(const InitiatorSocket& socket)
{
  try
  {
    (void)socket.get_base_interface();
  }
  catch (const sc_core::sc_report&)
  {
    return true;
  }
  return false;
}

/** @brief Dispatch a const native interface lookup and report its diagnostic. */
inline bool const_target_interface_rejected(const TargetSocket& socket)
{
  try
  {
    (void)socket.get_base_interface();
  }
  catch (const sc_core::sc_report&)
  {
    return true;
  }
  return false;
}

/** @brief Return borrowed export addresses through the native const overloads. */
inline std::uintptr_t const_initiator_export(const InitiatorSocket& socket)
{
  return reinterpret_cast<std::uintptr_t>(&socket.get_base_export());
}
inline std::uintptr_t const_target_export(const TargetSocket& socket)
{
  return reinterpret_cast<std::uintptr_t>(&socket.get_base_export());
}

}  // namespace pysystemc_tlm_utils_multi

#endif  // PYSYSTEMC_TLM_UTILS_MULTI_PROBE_H

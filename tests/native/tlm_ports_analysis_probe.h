/** @file tlm_ports_analysis_probe.h
 *  @brief Custom TLM1 port, finder, analysis, and FIFO probes.
 *  @details Ports borrow bound interfaces, event finders borrow their port,
 *  and analysis ports borrow subscriber interfaces. Keep the owner and all
 *  subscribers alive through simulation. The methods are synchronous; the
 *  owner processes wait only in their native SystemC process contexts.
 */
#ifndef PYSYSTEMC_TLM_PORTS_ANALYSIS_PROBE_H
#define PYSYSTEMC_TLM_PORTS_ANALYSIS_PROBE_H

#include <tlm_core/tlm_1/tlm_analysis/tlm_analysis_fifo.h>
#include <tlm_core/tlm_1/tlm_analysis/tlm_analysis_port.h>
#include <tlm_core/tlm_1/tlm_analysis/tlm_analysis_triple.h>
#include <tlm_core/tlm_1/tlm_req_rsp/tlm_ports/tlm_event_finder.h>
#include <tlm_core/tlm_1/tlm_req_rsp/tlm_ports/tlm_nonblocking_port.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <tlm>
#include <vector>

#include "tlm_ports_analysis_types.h"

namespace tlm_ports_analysis_probe
{
/** @brief Throw on a failed native semantic check in release builds. */
inline void verify(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

/** @brief Get-interface implementation with an independently notified event. */
struct GetStore : tlm::tlm_nonblocking_get_if<Sample>
{
  bool available = false;
  Sample value;
  sc_core::sc_event ready;

  /** @brief Read one available sample and clear readiness. */
  bool nb_get(Sample& out) override
  {
    if (!available) return false;
    out = value;
    available = false;
    return true;
  }

  /** @brief Report whether a sample can be read. */
  bool nb_can_get(tlm::tlm_tag<Sample>* = nullptr) const override
  {
    return available;
  }

  /** @brief Return the borrowed readiness event. */
  const sc_core::sc_event& ok_to_get(tlm::tlm_tag<Sample>* = nullptr) const override
  {
    return ready;
  }

  /** @brief Publish one sample and schedule readiness in the next delta. */
  void publish(const Sample& sample)
  {
    value = sample;
    available = true;
    ready.notify(sc_core::SC_ZERO_TIME);
  }
};

/** @brief Peek-interface implementation preserving its sample after reads. */
struct PeekStore : tlm::tlm_nonblocking_peek_if<Sample>
{
  bool available = false;
  Sample value;
  sc_core::sc_event ready;

  /** @brief Copy the available value without consuming it. */
  bool nb_peek(Sample& out) const override
  {
    if (!available) return false;
    out = value;
    return true;
  }

  /** @brief Report whether a value can be peeked. */
  bool nb_can_peek(tlm::tlm_tag<Sample>* = nullptr) const override
  {
    return available;
  }

  /** @brief Return the borrowed peek readiness event. */
  const sc_core::sc_event& ok_to_peek(tlm::tlm_tag<Sample>* = nullptr) const override
  {
    return ready;
  }

  /** @brief Publish one value and schedule readiness in the next delta. */
  void publish(const Sample& sample)
  {
    value = sample;
    available = true;
    ready.notify(sc_core::SC_ZERO_TIME);
  }
};

/** @brief Put-interface implementation with externally enabled capacity. */
struct PutStore : tlm::tlm_nonblocking_put_if<Sample>
{
  bool available = false;
  Sample value;
  sc_core::sc_event ready;

  /** @brief Accept one value only after capacity is signalled. */
  bool nb_put(const Sample& sample) override
  {
    if (!available) return false;
    value = sample;
    available = false;
    return true;
  }

  /** @brief Report whether the channel currently accepts a value. */
  bool nb_can_put(tlm::tlm_tag<Sample>* = nullptr) const override
  {
    return available;
  }

  /** @brief Return the borrowed capacity event. */
  const sc_core::sc_event& ok_to_put(tlm::tlm_tag<Sample>* = nullptr) const override
  {
    return ready;
  }

  /** @brief Enable one put and notify registered port sensitivities. */
  void enable()
  {
    available = true;
    ready.notify(sc_core::SC_ZERO_TIME);
  }
};

/** @brief Analysis subscriber retaining value copies in delivery order. */
struct Subscriber : tlm::tlm_analysis_if<Sample>
{
  std::vector<Sample> received;

  /** @brief Retain one borrowed analysis value by value. */
  void write(const Sample& sample) override
  {
    received.push_back(sample);
  }
};

/** @brief Owner for all 137-bit nonblocking, event finder, and analysis tests.
 *  @details The ports and analysis interfaces borrow the member stores and
 *  subscribers. They remain valid while this module lives. The native event
 *  finder instances installed into process sensitivity are kernel-owned for
 *  the process lifetime; separately constructed finders are caller-owned.
 */
struct Top : sc_core::sc_module
{
  using sample_type = Sample;
  using get_port_type = tlm::tlm_nonblocking_get_port<Sample>;
  using peek_port_type = tlm::tlm_nonblocking_peek_port<Sample>;
  using put_port_type = tlm::tlm_nonblocking_put_port<Sample>;
  using get_interface_type = tlm::tlm_nonblocking_get_if<Sample>;
  using event_finder_type = tlm::tlm_event_finder_t<get_interface_type, Sample>;
  using triple_type = tlm::tlm_analysis_triple<Sample>;

  GetStore get_store;
  PeekStore peek_store;
  PutStore put_store;
  get_port_type get_port;
  peek_port_type peek_port;
  put_port_type put_port;
  tlm::tlm_analysis_port<Sample> analysis_default;
  tlm::tlm_analysis_port<Sample> analysis_named;
  tlm::tlm_analysis_fifo<Sample> fifo_default;
  tlm::tlm_analysis_fifo<Sample> fifo_named;
  Subscriber sink_a;
  Subscriber sink_b;
  Subscriber sink_c;
  std::vector<std::string> port_events;
  bool get_seen = false;
  bool peek_seen = false;
  bool put_seen = false;
  inline static unsigned destroyed = 0;

  SC_HAS_PROCESS(Top);

  /** @brief Construct named ports, default/named analysis objects, and sensitivities. */
  explicit Top(sc_core::sc_module_name name)
    : sc_core::sc_module(name)
    , get_port("get_port")
    , peek_port("peek_port")
    , put_port("put_port")
    , analysis_named("analysis_named")
    , fifo_named("fifo_named")
  {
    SC_METHOD(on_get_ready);
    sensitive << get_port.ok_to_get();
    dont_initialize();
    SC_METHOD(on_peek_ready);
    sensitive << peek_port.ok_to_peek();
    dont_initialize();
    SC_METHOD(on_put_ready);
    sensitive << put_port.ok_to_put();
    dont_initialize();
    SC_THREAD(publish_events);
  }

  /** @brief Count owner destruction after ports and analysis subscribers expire. */
  ~Top() override
  {
    ++destroyed;
  }

  /** @brief Resolve the get finder sensitivity and consume its notified value. */
  void on_get_ready()
  {
    Sample value;
    auto* interface = dynamic_cast<get_interface_type*>(get_port.get_interface());
    verify(interface != nullptr, "get port resolved an incompatible interface");
    get_seen = interface->nb_get(value);
    verify(get_seen && value.sequence == 10 && value.bits[136].to_bool(), "get finder sensitivity or payload mismatch");
    std::ostringstream record;
    record << "get|" << now_ns() << '|' << value.sequence << '|' << value.bits.to_uint64() << '|'
           << value.bits[136].to_bool();
    port_events.push_back(record.str());
  }

  /** @brief Resolve the peek finder sensitivity without consuming its value. */
  void on_peek_ready()
  {
    Sample value;
    auto* interface = dynamic_cast<tlm::tlm_nonblocking_peek_if<Sample>*>(peek_port.get_interface());
    verify(interface != nullptr, "peek port resolved an incompatible interface");
    peek_seen = interface->nb_peek(value);
    verify(peek_seen && value.sequence == 20 && value.bits[136].to_bool(),
           "peek finder sensitivity or payload mismatch");
    std::ostringstream record;
    record << "peek|" << now_ns() << '|' << value.sequence << '|' << value.bits.to_uint64() << '|'
           << value.bits[136].to_bool();
    port_events.push_back(record.str());
  }

  /** @brief Resolve the put finder sensitivity and submit a custom payload. */
  void on_put_ready()
  {
    const Sample value(30, 303);
    auto* interface = dynamic_cast<tlm::tlm_nonblocking_put_if<Sample>*>(put_port.get_interface());
    verify(interface != nullptr, "put port resolved an incompatible interface");
    put_seen = interface->nb_put(value);
    verify(put_seen && put_store.value.sequence == 30 && put_store.value.bits[136].to_bool(),
           "put finder sensitivity or payload mismatch");
    std::ostringstream record;
    record << "put|" << now_ns() << '|' << value.sequence << '|' << value.bits.to_uint64() << '|'
           << value.bits[136].to_bool();
    port_events.push_back(record.str());
  }

  /** @brief Notify get/peek at one nanosecond and put capacity at three. */
  void publish_events()
  {
    wait(sc_core::sc_time(1, sc_core::SC_NS));
    get_store.publish(Sample(10, 101));
    peek_store.publish(Sample(20, 202));
    wait(sc_core::sc_time(2, sc_core::SC_NS));
    put_store.enable();
  }

  /** @brief Return exact integer nanoseconds at the configured time resolution. */
  static unsigned now_ns()
  {
    return static_cast<unsigned>(sc_core::sc_time_stamp().value() / sc_core::sc_time(1, sc_core::SC_NS).value());
  }
};

/** @brief Allocate a caller-owned get finder using the exact native member type. */
inline Top::event_finder_type* make_get_finder(sc_core::sc_port_base& port)
{
  return new Top::event_finder_type(port, &Top::get_interface_type::ok_to_get);
}

/** @brief Exercise the mutable value conversion operator on an analysis triple. */
inline Sample triple_to_value(Top::triple_type& triple)
{
  return static_cast<Sample>(triple);
}

/** @brief Exercise the const-reference conversion operator on an analysis triple. */
inline const Sample& triple_to_const_reference(const Top::triple_type& triple)
{
  return static_cast<const Sample&>(triple);
}
}  // namespace tlm_ports_analysis_probe

#endif  // PYSYSTEMC_TLM_PORTS_ANALYSIS_PROBE_H

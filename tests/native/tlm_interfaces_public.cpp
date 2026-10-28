/** @file tlm_interfaces_public.cpp
 *  @brief Native runtime reference for TLM interface family probes.
 *  @details Each concrete adapter owns its sample and events. TLM-2 payload,
 *  DMI and timing values remain caller-owned. This standalone kernel retains
 *  no Python callbacks and performs no GIL operations.
 */
#include <iostream>
#include <stdexcept>

#include "tlm_interfaces_probe.h"

using namespace sc_core;
using namespace pysystemc_tlm_interfaces;

/** @brief Exercise data interfaces using an arbitrary-width unsigned value. */
static void check_wide()
{
  sc_dt::sc_biguint<137> sample = 17;
  sample[136] = true;
  WideValueAdapter adapter(sample);
  tlm::tlm_get_peek_if<sc_dt::sc_biguint<137>>& combined = adapter;
  tlm::tlm_blocking_get_if<sc_dt::sc_biguint<137>>& blocking_get = combined;
  tlm::tlm_nonblocking_get_if<sc_dt::sc_biguint<137>>& nonblocking_get = combined;
  tlm::tlm_blocking_peek_if<sc_dt::sc_biguint<137>>& blocking_peek = combined;
  tlm::tlm_nonblocking_peek_if<sc_dt::sc_biguint<137>>& nonblocking_peek = combined;
  tlm::tlm_tag<sc_dt::sc_biguint<137>> tag;
  const bool can_get_before = nonblocking_get.nb_can_get(&tag);
  const bool can_peek_before = nonblocking_peek.nb_can_peek(&tag);
  const auto* get_event = &nonblocking_get.ok_to_get(&tag);
  const auto* peek_event = &nonblocking_peek.ok_to_peek(&tag);
  if (get_event != &nonblocking_get.ok_to_get() || peek_event != &nonblocking_peek.ok_to_peek())
    throw std::runtime_error("explicit tag event identity mismatch");

  sc_dt::sc_biguint<137> nonblocking_peek_output = 0;
  if (!nonblocking_peek.nb_peek(nonblocking_peek_output) || nonblocking_peek_output.to_uint64() != 17 ||
      !nonblocking_peek_output[136])
    throw std::runtime_error("wide nonblocking peek mismatch");
  const auto peeked = blocking_peek.peek(&tag);
  sc_dt::sc_biguint<137> blocking_peek_output = 0;
  blocking_peek.peek(blocking_peek_output);
  const bool can_get_after_output_peek = nonblocking_get.nb_can_get(&tag);
  if (!can_get_after_output_peek) throw std::runtime_error("blocking peek output consumed its sample");
  const auto consumed = blocking_get.get(&tag);

  reset_wide_value_adapter(adapter, sample);
  sc_dt::sc_biguint<137> blocking_get_output = 0;
  blocking_get.get(blocking_get_output);

  reset_wide_value_adapter(adapter, sample);
  sc_dt::sc_biguint<137> nonblocking_get_output = 0;
  if (!nonblocking_get.nb_get(nonblocking_get_output)) throw std::runtime_error("wide nb_get failed");
  sc_dt::sc_biguint<137> failed_get_output = 91;
  failed_get_output[136] = true;
  const sc_dt::sc_biguint<137> get_sentinel = failed_get_output;
  sc_dt::sc_biguint<137> failed_peek_output = 93;
  failed_peek_output[136] = true;
  const sc_dt::sc_biguint<137> peek_sentinel = failed_peek_output;
  if (nonblocking_get.nb_get(failed_get_output) || failed_get_output != get_sentinel)
    throw std::runtime_error("failed wide nb_get changed output");
  if (nonblocking_peek.nb_peek(failed_peek_output) || failed_peek_output != peek_sentinel)
    throw std::runtime_error("failed wide nb_peek changed output");
  const bool can_get_after = nonblocking_get.nb_can_get(&tag);
  const bool can_peek_after = nonblocking_peek.nb_can_peek(&tag);
  if (!can_get_before || !can_peek_before || can_get_after || can_peek_after)
    throw std::runtime_error("wide readiness mismatch");

  std::cout << "TLM_IFACE_WIDE " << nonblocking_peek_output.to_uint64() << ' ' << nonblocking_peek_output[136] << ' '
            << peeked.to_uint64() << ' ' << peeked[136] << ' ' << blocking_peek_output.to_uint64() << ' '
            << blocking_peek_output[136] << ' ' << consumed.to_uint64() << ' ' << consumed[136] << ' '
            << blocking_get_output.to_uint64() << ' ' << blocking_get_output[136] << ' '
            << nonblocking_get_output.to_uint64() << ' ' << nonblocking_get_output[136] << ' '
            << failed_get_output.to_uint64() << ' ' << failed_get_output[136] << ' ' << failed_peek_output.to_uint64()
            << ' ' << failed_peek_output[136] << ' ' << can_get_before << ' ' << can_peek_before << ' '
            << can_get_after_output_peek << ' ' << can_get_after << ' ' << can_peek_after << ' '
            << (get_event == &nonblocking_get.ok_to_get()) << ' ' << (peek_event == &nonblocking_peek.ok_to_peek())
            << ' ' << adapter.get_calls() << ' ' << adapter.peek_calls() << '\n';
}

/** @brief Exercise analysis and delayed writes with a user-defined value. */
static void check_custom_write()
{
  CustomWriteAdapter adapter;
  tlm::tlm_analysis_if<Record>& analysis = adapter;
  tlm::tlm_delayed_analysis_if<Record>& delayed = adapter;
  const Record immediate{37};
  const Record later{42};
  analysis.write(immediate);
  delayed.write(later, sc_time(3, SC_NS));
  if (adapter.immediate().code != 37 || adapter.delayed().code != 42 || adapter.delay() != sc_time(3, SC_NS))
    throw std::runtime_error("custom write interface mismatch");
  std::cout << "TLM_IFACE_WRITE " << adapter.immediate().code << ' ' << adapter.delayed().code << ' '
            << adapter.immediate_calls() << ' ' << adapter.delayed_calls() << ' ' << adapter.delay().value() << '\n';
}

/** @brief Exercise FIFO debug overloads and configuration defaults. */
static void check_fifo_control()
{
  sc_dt::sc_biguint<137> newest_sample = 17;
  sc_dt::sc_biguint<137> oldest_sample = 23;
  newest_sample[136] = true;
  oldest_sample[136] = true;
  WideFifoAdapter adapter(newest_sample, oldest_sample);
  tlm::tlm_fifo_debug_if<sc_dt::sc_biguint<137>>& debug = adapter;
  tlm::tlm_fifo_config_size_if& config = adapter;
  sc_dt::sc_biguint<137> newest = 0;
  sc_dt::sc_biguint<137> oldest = 0;
  if (!debug.nb_peek(newest, 0) || !debug.nb_peek(oldest, 1)) throw std::runtime_error("FIFO debug peek failed");
  sc_dt::sc_biguint<137> replacement = 31;
  replacement[136] = true;
  const bool poked = debug.nb_poke(replacement);
  sc_dt::sc_biguint<137> changed = 0;
  if (!debug.nb_peek(changed, 0)) throw std::runtime_error("FIFO debug re-read failed");
  const bool out_of_range = debug.nb_poke(sc_dt::sc_biguint<137>(99), 2);
  debug.debug();
  config.nb_expand();
  const int after_expand = debug.size();
  config.nb_unbound();
  const int after_unbound = debug.size();
  const bool reduced = config.nb_reduce();
  const int after_reduce = debug.size();
  const bool small_bound = config.nb_bound(1);
  const bool valid_bound = config.nb_bound(2);
  if (!poked || out_of_range || newest.to_uint64() != 17 || oldest.to_uint64() != 23 || changed.to_uint64() != 31 ||
      !reduced || small_bound || !valid_bound)
    throw std::runtime_error("FIFO debug/config interface mismatch");
  std::cout << "TLM_IFACE_FIFO " << debug.used() << ' ' << after_expand << ' ' << after_unbound << ' ' << after_reduce
            << ' ' << valid_bound << ' ' << adapter.debug_calls() << ' ' << newest.to_uint64() << ' ' << newest[136]
            << ' ' << oldest.to_uint64() << ' ' << oldest[136] << ' ' << changed.to_uint64() << ' ' << changed[136]
            << '\n';
}

/** @brief Exercise the standard combined TLM-2 forward/backward contracts. */
static void check_base_protocol()
{
  BaseProtocolAdapter adapter;
  tlm::tlm_fw_transport_if<tlm::tlm_base_protocol_types>& fw = adapter;
  tlm::tlm_bw_transport_if<tlm::tlm_base_protocol_types>& bw = adapter;
  tlm::tlm_generic_payload payload;
  tlm::tlm_phase phase = tlm::BEGIN_REQ;
  sc_time delay(2, SC_NS);
  fw.b_transport(payload, delay);
  const auto forward = fw.nb_transport_fw(payload, phase, delay);
  const auto backward = bw.nb_transport_bw(payload, phase, delay);
  tlm::tlm_dmi dmi;
  const bool granted = fw.get_direct_mem_ptr(payload, dmi);
  bw.invalidate_direct_mem_ptr(0x20, 0x2f);
  const auto transferred = fw.transport_dbg(payload);
  if (delay != sc_time(3, SC_NS) || forward != tlm::TLM_UPDATED || backward != tlm::TLM_COMPLETED || !granted ||
      dmi.get_start_address() != 0x10 || dmi.get_end_address() != 0x1f || transferred != 4 ||
      adapter.invalidated_start() != 0x20 || adapter.invalidated_end() != 0x2f)
    throw std::runtime_error("base protocol interface mismatch");
  std::cout << "TLM_IFACE_PROTOCOL " << delay.value() << ' ' << forward << ' ' << backward << ' ' << granted << ' '
            << dmi.get_start_address() << ' ' << dmi.get_end_address() << ' ' << transferred << ' '
            << adapter.fw_calls() << ' ' << adapter.bw_calls() << ' ' << adapter.blocking_calls() << ' '
            << adapter.dmi_calls() << ' ' << adapter.invalidation_calls() << ' ' << adapter.debug_transport_calls()
            << '\n';
}

/** @brief Run native calls for every tested TLM interface family. */
int sc_main(int, char**)
{
  check_wide();
  check_custom_write();
  check_fifo_control();
  check_base_protocol();
  if (destruction_counts() != std::array<int, 4>{1, 1, 1, 1})
    throw std::runtime_error("native interface destruction mismatch");
  std::cout << "TLM_IFACE_DESTRUCTION 1 1 1 1\n";
  return 0;
}

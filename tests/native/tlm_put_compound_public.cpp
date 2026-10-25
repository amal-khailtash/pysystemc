/** @file tlm_put_compound_public.cpp
 *  @brief Native reference for small TLM put, compound and FIFO interfaces.
 *  @details This process uses a fresh SystemC kernel. All returned interface
 *  and event references borrow live adapter-owned storage; no GIL is involved.
 */
#include <iostream>
#include <stdexcept>

#include "tlm_put_compound_probe.h"

using namespace pysystemc_tlm_put_compound;

/** @brief Check combined put inheritance, overload tags and event identity. */
static void check_put()
{
  PutAdapter adapter;
  tlm::tlm_put_if<Wide>& combined = adapter;
  tlm::tlm_blocking_put_if<Wide>& blocking = combined;
  tlm::tlm_nonblocking_put_if<Wide>& nonblocking = combined;
  tlm::tlm_tag<Wide> tag;
  blocking.put(wide(17));
  if (!nonblocking.nb_put(wide(19))) throw std::runtime_error("native nonblocking put failed");
  const bool can_default = nonblocking.nb_can_put();
  const bool can_tagged = nonblocking.nb_can_put(&tag);
  const auto* event_default = &nonblocking.ok_to_put();
  const auto* event_tagged = &nonblocking.ok_to_put(&tag);
  combined.put(wide(23));
  if (!combined.nb_put(wide(29))) throw std::runtime_error("native combined put failed");
  if (!combined.nb_can_put() || !combined.nb_can_put(&tag))
    throw std::runtime_error("native combined readiness failed");
  if (!can_default || !can_tagged || event_default != event_tagged || adapter.sample_count() != 4)
    throw std::runtime_error("native put-family mismatch");
  std::cout << "TLM_PUT " << adapter.sample_count() << ' ' << adapter.put_calls << ' ' << adapter.nb_put_calls << ' '
            << adapter.can_put_calls << ' ' << adapter.event_calls << ' ' << (event_default == event_tagged) << '\n';
}

/** @brief Check blocking and nonblocking overloads on a master/slave type. */
template <class Combined, class Blocking, class Nonblocking>
static void check_compound(const char* marker)
{
  CompoundAdapter<Combined> adapter;
  Combined& combined = adapter;
  Blocking& blocking = adapter;
  Nonblocking& nonblocking = adapter;
  tlm::tlm_tag<Wide> tag;

  adapter.set_responses({wide(7), wide(9), wide(11)});
  blocking.put(wide(17));
  if (blocking.get().to_uint64() != 7 || blocking.get(&tag).to_uint64() != 9)
    throw std::runtime_error("native compound tagged get mismatch");
  Wide output = wide(99);
  tlm::tlm_blocking_get_if<Wide>& get_base = blocking;
  get_base.get(output);
  if (output.to_uint64() != 11 || !output[136]) throw std::runtime_error("native get output forwarding mismatch");
  adapter.set_responses({wide(13)});
  const auto peek_default = blocking.peek();
  const auto peek_tagged = blocking.peek(&tag);
  tlm::tlm_blocking_peek_if<Wide>& peek_base = blocking;
  output = wide(99);
  peek_base.peek(output);
  if (peek_default.to_uint64() != 13 || peek_tagged.to_uint64() != 13 || output.to_uint64() != 13 || !output[136])
    throw std::runtime_error("native peek output forwarding mismatch");

  if (!nonblocking.nb_put(wide(19)) || !nonblocking.nb_can_put() || !nonblocking.nb_can_put(&tag))
    throw std::runtime_error("native compound nonblocking put mismatch");
  const auto* put_event = &nonblocking.ok_to_put();
  if (put_event != &nonblocking.ok_to_put(&tag)) throw std::runtime_error("native put event mismatch");
  adapter.set_responses({wide(31), wide(37)});
  const bool can_get_default = nonblocking.nb_can_get();
  const bool can_get_tagged = nonblocking.nb_can_get(&tag);
  const auto* get_event = &nonblocking.ok_to_get();
  if (get_event != &nonblocking.ok_to_get(&tag)) throw std::runtime_error("native get event mismatch");
  output = wide(0);
  if (!nonblocking.nb_get(output) || output.to_uint64() != 31 || !output[136])
    throw std::runtime_error("native compound nb_get first value mismatch");
  if (!nonblocking.nb_get(output) || output.to_uint64() != 37 || !output[136])
    throw std::runtime_error("native compound nb_get second value mismatch");
  output = wide(91);
  if (nonblocking.nb_get(output) || output.to_uint64() != 91 || !output[136])
    throw std::runtime_error("native failed nb_get changed output");
  adapter.set_responses({wide(41)});
  const bool can_peek_default = nonblocking.nb_can_peek();
  const bool can_peek_tagged = nonblocking.nb_can_peek(&tag);
  const auto* peek_event = &nonblocking.ok_to_peek();
  if (peek_event != &nonblocking.ok_to_peek(&tag)) throw std::runtime_error("native peek event mismatch");
  output = wide(0);
  if (!nonblocking.nb_peek(output) || output.to_uint64() != 41 || !output[136])
    throw std::runtime_error("native compound nb_peek mismatch");
  adapter.set_responses({});
  output = wide(93);
  if (nonblocking.nb_peek(output) || output.to_uint64() != 93 || !output[136])
    throw std::runtime_error("native failed nb_peek changed output");

  combined.put(wide(23));
  if (!combined.nb_put(wide(29))) throw std::runtime_error("native combined request failed");
  adapter.set_responses({wide(43), wide(47)});
  const auto combined_get = combined.get();
  output = wide(0);
  if (!combined.nb_get(output)) throw std::runtime_error("native combined response failed");
  if (combined_get.to_uint64() != 43 || output.to_uint64() != 47 || !output[136])
    throw std::runtime_error("native combined inherited methods mismatch");

  if (!can_get_default || !can_get_tagged || !can_peek_default || !can_peek_tagged || put_event == get_event ||
      get_event == peek_event)
    throw std::runtime_error("native compound readiness mismatch");
  std::cout << marker << ' ' << adapter.sent_count() << ' ' << adapter.put_calls << ' ' << adapter.get_calls << ' '
            << adapter.peek_calls << ' ' << adapter.nb_put_calls << ' ' << adapter.can_put_calls << ' '
            << adapter.nb_get_calls << ' ' << adapter.can_get_calls << ' ' << adapter.nb_peek_calls << ' '
            << adapter.can_peek_calls << ' ' << output.to_uint64() << ' ' << output[136] << '\n';
}

/** @brief Check FIFO compound inheritance and both debug peek overloads. */
static void check_fifo()
{
  FifoAdapter adapter;
  tlm::tlm_fifo_get_if<Wide>& get = adapter;
  tlm::tlm_fifo_put_if<Wide>& put = adapter;
  tlm::tlm_fifo_debug_if<Wide>& debug = adapter;
  tlm::tlm_tag<Wide> tag;
  if (debug.used() != 2 || debug.size() != 4) throw std::runtime_error("native FIFO dimensions mismatch");
  debug.debug();
  Wide newest = wide(0), oldest = wide(0);
  if (!debug.nb_peek(newest, 0) || !debug.nb_peek(oldest, 1)) throw std::runtime_error("native indexed peek failed");
  if (newest.to_uint64() != 17 || oldest.to_uint64() != 23 || !newest[136] || !oldest[136])
    throw std::runtime_error("native FIFO ordering mismatch");
  Wide out_of_range = wide(95);
  if (debug.nb_peek(out_of_range, 8) || out_of_range.to_uint64() != 95 || !out_of_range[136])
    throw std::runtime_error("native failed indexed peek changed output");
  if (!debug.nb_poke(wide(31)) || !debug.nb_poke(wide(37), 1) || debug.nb_poke(wide(99), 8))
    throw std::runtime_error("native FIFO poke contract mismatch");
  if (!put.nb_can_put() || !put.nb_can_put(&tag)) throw std::runtime_error("native FIFO put readiness mismatch");
  const auto* put_event = &put.ok_to_put();
  if (put_event != &put.ok_to_put(&tag)) throw std::runtime_error("native FIFO put event mismatch");
  put.put(wide(41));
  if (!put.nb_put(wide(43))) throw std::runtime_error("native FIFO nonblocking put mismatch");
  if (get.peek().to_uint64() != 37 || get.peek(&tag).to_uint64() != 37)
    throw std::runtime_error("native FIFO tagged peek mismatch");
  tlm::tlm_blocking_peek_if<Wide>& blocking_peek = get;
  Wide output = wide(0);
  blocking_peek.peek(output);
  if (output.to_uint64() != 37 || !output[136]) throw std::runtime_error("native FIFO peek output mismatch");
  const bool can_get_default = get.nb_can_get();
  const bool can_get_tagged = get.nb_can_get(&tag);
  const bool can_peek_default = get.nb_can_peek();
  const bool can_peek_tagged = get.nb_can_peek(&tag);
  const auto* get_event = &get.ok_to_get();
  const auto* peek_event = &get.ok_to_peek();
  if (get_event != &get.ok_to_get(&tag) || peek_event != &get.ok_to_peek(&tag))
    throw std::runtime_error("native FIFO get/peek event mismatch");
  if (!can_get_default || !can_get_tagged || !can_peek_default || !can_peek_tagged || put_event == get_event ||
      get_event == peek_event)
    throw std::runtime_error("native FIFO initial readiness mismatch");
  tlm::tlm_nonblocking_peek_if<Wide>& nonblocking_peek = get;
  if (!nonblocking_peek.nb_peek(output) || output.to_uint64() != 37 || !output[136])
    throw std::runtime_error("native FIFO nonblocking peek mismatch");
  if (get.get().to_uint64() != 37 || get.get(&tag).to_uint64() != 31)
    throw std::runtime_error("native FIFO tagged get mismatch");
  tlm::tlm_blocking_get_if<Wide>& blocking_get = get;
  blocking_get.get(output);
  if (output.to_uint64() != 41 || !output[136]) throw std::runtime_error("native FIFO get output mismatch");
  output = wide(0);
  if (!get.nb_get(output) || output.to_uint64() != 43 || !output[136])
    throw std::runtime_error("native FIFO nb_get mismatch");
  output = wide(91);
  if (get.nb_get(output) || output.to_uint64() != 91 || !output[136])
    throw std::runtime_error("native FIFO failed nb_get changed output");
  output = wide(93);
  if (nonblocking_peek.nb_peek(output) || output.to_uint64() != 93 || !output[136])
    throw std::runtime_error("native failed FIFO nb_peek changed output");
  output = wide(91);
  if (get.nb_can_get() || get.nb_can_get(&tag) || get.nb_can_peek() || get.nb_can_peek(&tag))
    throw std::runtime_error("native FIFO empty readiness mismatch");
  if (debug.used() != 0 || adapter.put_calls != 1 || adapter.nb_put_calls != 1 || adapter.get_calls != 3 ||
      adapter.peek_calls != 3 || adapter.nb_peek_calls != 2 || adapter.nb_get_calls != 2 ||
      adapter.can_get_calls != 4 || adapter.can_peek_calls != 4 || adapter.indexed_peek_calls != 3 ||
      adapter.poke_calls != 3)
    throw std::runtime_error("native FIFO exact dispatch mismatch");
  std::cout << "TLM_FIFO " << debug.used() << ' ' << newest.to_uint64() << ' ' << newest[136] << ' '
            << oldest.to_uint64() << ' ' << oldest[136] << ' ' << output.to_uint64() << ' ' << output[136] << ' '
            << adapter.put_calls << ' ' << adapter.nb_put_calls << ' ' << adapter.can_put_calls << ' '
            << adapter.get_calls << ' ' << adapter.peek_calls << ' ' << adapter.nb_peek_calls << ' '
            << adapter.nb_get_calls << ' ' << adapter.can_get_calls << ' ' << adapter.can_peek_calls << ' '
            << adapter.indexed_peek_calls << ' ' << adapter.poke_calls << ' ' << adapter.debug_calls << '\n';
}

/** @brief Exercise both transport overloads through base interface references. */
static void check_transport()
{
  TransportAdapter adapter;
  tlm::tlm_transport_if<Record, Record>& transport = adapter;
  const Record request{37};
  const auto value_result = transport.transport(request);
  Record output{-1};
  transport.transport(request, output);
  if (value_result.code != 46 || output.code != 46 || adapter.calls != 2)
    throw std::runtime_error("native transport output forwarding mismatch");
  std::cout << "TLM_TRANSPORT " << value_result.code << ' ' << output.code << ' ' << adapter.calls << '\n';
}

/** @brief Run independent native references for all assigned interface groups. */
int sc_main(int, char**)
{
  check_put();
  check_compound<tlm::tlm_master_if<Wide, Wide>, tlm::tlm_blocking_master_if<Wide, Wide>,
                 tlm::tlm_nonblocking_master_if<Wide, Wide>>("TLM_MASTER");
  check_compound<tlm::tlm_slave_if<Wide, Wide>, tlm::tlm_blocking_slave_if<Wide, Wide>,
                 tlm::tlm_nonblocking_slave_if<Wide, Wide>>("TLM_SLAVE");
  check_fifo();
  check_transport();
  return 0;
}

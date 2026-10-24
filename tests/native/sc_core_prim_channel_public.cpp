/** @file sc_core_prim_channel_public.cpp
 * @brief Native reference for sc_prim_channel declarations and scheduling.
 * @details A native derived channel invokes every protected wait and
 * next_trigger overload from valid SystemC process contexts. The host worker
 * calls only async_request_update and never enters the Python C API.
 */
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "sc_core_prim_channel_probe.h"

namespace
{
using prim_channel_public_probe::AsyncChannel;
using prim_channel_public_probe::Channel;

/** @brief Own the native channel, events, and processes for one comparison. */
struct Model : sc_core::sc_module
{
  sc_core::sc_clock clock;
  sc_core::sc_in<bool> clock_port;
  sc_core::sc_event event_a{"event_a"};
  sc_core::sc_event event_b{"event_b"};
  sc_core::sc_event kick{"kick"};
  sc_core::sc_event async_ready{"async_ready"};
  sc_core::sc_event_or_list or_list;
  sc_core::sc_event_and_list and_list;
  std::vector<std::string> lifecycle;
  std::vector<std::string> waits;
  std::vector<std::string> triggers;
  Channel channel;
  AsyncChannel async_channel;
  unsigned trigger_index = 0;
  unsigned wait_count = 0;
  unsigned trigger_count = 0;
  bool async_started = false;
  bool async_observed = false;

  SC_HAS_PROCESS(Model);
  explicit Model(sc_core::sc_module_name name)
    : sc_module(name)
    , clock("prim_clock", sc_core::sc_time(2, sc_core::SC_NS))
    , clock_port("clock_port")
    , or_list(event_a)
    , and_list(event_a)
    , channel(lifecycle)
    , async_channel(async_ready, lifecycle)
  {
    clock_port(clock);
    or_list |= event_b;
    and_list &= event_b;

    SC_THREAD(wait_0);
    sensitive << kick;
    SC_THREAD(wait_1);
    SC_THREAD(wait_2);
    SC_THREAD(wait_3);
    SC_THREAD(wait_4);
    SC_THREAD(wait_5);
    SC_THREAD(wait_6);
    SC_THREAD(wait_7);
    SC_THREAD(wait_8);
    SC_THREAD(wait_9);
    SC_THREAD(wait_10);
    SC_THREAD(wait_11);
    SC_CTHREAD(wait_12, clock_port.pos());

    SC_METHOD(trigger);
    sensitive << kick;
    dont_initialize();
    SC_THREAD(drive);
    SC_METHOD(async_complete);
    sensitive << async_ready;
    dont_initialize();
  }

  /** @brief Record a process resumption with simulation time, delta and timeout. */
  void record(std::vector<std::string>& destination, const std::string& label, bool timeout)
  {
    destination.push_back(label + "@" + std::to_string(sc_core::sc_time_stamp().value()) + "/" +
                          std::to_string(sc_core::sc_delta_count()) + "/" + (timeout ? "1" : "0"));
  }

  /** @brief Run one native protected wait overload and record its resumption. */
  void run_wait(unsigned form)
  {
    channel.wait_form(form, event_a, or_list, and_list);
    record(waits, std::to_string(form), channel.protected_timed_out());
    ++wait_count;
    maybe_start_async_request();
  }

#define DEFINE_WAIT(ID) \
  void wait_##ID()      \
  {                     \
    run_wait(ID);       \
  }
  DEFINE_WAIT(0)
  DEFINE_WAIT(1)
  DEFINE_WAIT(2)
  DEFINE_WAIT(3)
  DEFINE_WAIT(4)
  DEFINE_WAIT(5)
  DEFINE_WAIT(6)
  DEFINE_WAIT(7)
  DEFINE_WAIT(8)
  DEFINE_WAIT(9)
  DEFINE_WAIT(10)
  DEFINE_WAIT(11)
  DEFINE_WAIT(12)
#undef DEFINE_WAIT

  /** @brief Exercise each protected next_trigger form over successive callbacks. */
  void trigger()
  {
    if (trigger_index != 0)
    {
      record(triggers, std::to_string(trigger_index - 1), channel.protected_timed_out());
      ++trigger_count;
    }
    if (trigger_index == 12)
    {
      maybe_start_async_request();
      return;
    }
    channel.trigger_form(trigger_index, event_a, or_list, and_list);
    ++trigger_index;
  }

  /** @brief Start the host request only after all scheduler forms complete. */
  void maybe_start_async_request()
  {
    if (!async_started && wait_count == 13 && trigger_count == 12)
    {
      async_started = true;
      async_channel.start_request();
    }
  }

  /** @brief Drive recurring events and the static method sensitivity. */
  void drive()
  {
    channel.request_update();
    channel.request_update();
    channel.request_update();
    for (unsigned i = 1; i <= 16; ++i)
    {
      sc_core::wait(1, sc_core::SC_NS);
      kick.notify(sc_core::SC_ZERO_TIME);
      if (i % 2 == 0) event_a.notify(sc_core::SC_ZERO_TIME);
      if (i % 3 == 0) event_b.notify(sc_core::SC_ZERO_TIME);
    }
  }

  /** @brief Detach the async channel after its native host-thread update. */
  void async_complete()
  {
    async_observed = true;
    async_channel.detach();
    sc_core::sc_stop();
  }
};
}  // namespace

/** @brief Compare all overload callbacks and report destruction counts. */
int sc_main(int, char**)
{
  {
    Channel temporary;
    std::cout << "PRIM_DEFAULT\t" << temporary.kind() << '\n';
  }
  std::cout << "PRIM_EXPLICIT_DESTRUCTION\t" << Channel::destroyed << '\n';

  std::vector<std::string> lifecycle;
  {
    Model model("prim_channel_model");
    sc_core::sc_start();

    std::sort(model.waits.begin(), model.waits.end());
    std::sort(model.triggers.begin(), model.triggers.end());
    for (const auto& item : model.waits) std::cout << "PRIM_WAIT\t" << item << '\n';
    for (const auto& item : model.triggers) std::cout << "PRIM_TRIGGER\t" << item << '\n';
    std::cout << "PRIM_UPDATE\t" << model.channel.updates << '\t' << model.channel.update_requested_during_update
              << '\t' << model.channel.update_requested() << '\n';
    std::cout << "PRIM_ASYNC\t" << model.async_channel.attached << '\t' << model.async_observed << '\t'
              << model.async_channel.detached << '\t' << model.async_channel.updates << '\n';
    std::cout << "PRIM_LIFECYCLE\t";
    for (const auto& item : model.lifecycle) std::cout << item;
    std::cout << '\n';
    lifecycle = model.lifecycle;
  }
  std::cout << "PRIM_DESTRUCTION\t" << Channel::destroyed << '\t' << AsyncChannel::destroyed << '\n';
  return 0;
}

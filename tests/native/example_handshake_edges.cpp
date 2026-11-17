/**
 * @file example_handshake_edges.cpp
 * @brief Native reference probes for bounded ready-valid and toggle-toggle cases.
 *
 * The probes use the upstream sc_rvd/sc_ttd ports and their reset methods.
 * Every scenario runs in a fresh runner process. Reset is the same active-low
 * CTHREAD reset used by the upstream examples; no channel state is modified
 * outside the native endpoint reset methods.
 */

#include <systemc.h>

#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

#include "sc_rvd.h"
#include "sc_ttd.h"

using namespace sc_core;
using namespace sc_dt;

namespace
{

/** Print one comparable role/value/simulation-time record. */
void record(const char* tag, int value = -1)
{
  const auto ns = static_cast<long long>(std::llround(sc_time_stamp().to_seconds() * 1.0e9));
  std::cout << "EDGE " << tag << " value=" << value << " ns=" << ns << std::endl;
}

/** Drive one value sequence through the native upstream output port. */
template <typename Channel>
struct Source : sc_module
{
  using output_type = typename Channel::out;

  sc_in<bool> clock{"clock"};
  sc_in<bool> reset{"reset"};
  output_type output{"output"};
  std::string scenario;
  bool toggle_protocol;
  int epochs = 0;

  SC_HAS_PROCESS(Source);

  Source(sc_module_name name, std::string selected, bool toggle)
    : sc_module(name), scenario(std::move(selected)), toggle_protocol(toggle)
  {
    SC_CTHREAD(run, clock.pos());
    reset_signal_is(reset, false);
  }

  void transfer(int value)
  {
    record("SEND_BEGIN", value);
    output.write(sc_uint<8>(value));
    record("SEND_DONE", value);
  }

  void run()
  {
    output.reset();
    wait();
    if (scenario == "stalled_consumer")
    {
      transfer(41);
    }
    else if (scenario == "delayed_ack")
    {
      transfer(11);
      transfer(22);
    }
    else if (scenario == "reset_pending")
    {
      if (epochs == 0)
      {
        epochs = 1;
        transfer(41);
        // The second operation remains pending until reset or a read.
        transfer(42);
      }
      else
      {
        epochs = 2;
        transfer(toggle_protocol ? 43 : 42);
      }
    }
    while (true)
    {
      wait();
    }
  }
};

/** Delay reads, then consume and record native upstream input-port values. */
template <typename Channel>
struct Sink : sc_module
{
  using input_type = typename Channel::in;

  sc_in<bool> clock{"clock"};
  sc_in<bool> reset{"reset"};
  input_type input{"input"};
  std::string scenario;

  SC_HAS_PROCESS(Sink);

  Sink(sc_module_name name, std::string selected) : sc_module(name), scenario(std::move(selected))
  {
    SC_CTHREAD(run, clock.pos());
    reset_signal_is(reset, false);
  }

  void receive()
  {
    const auto value = input.read();
    record("RECV", static_cast<int>(value));
  }

  void run()
  {
    input.reset();
    wait();
    if (scenario == "stalled_consumer")
    {
      wait(3);
      receive();
    }
    else if (scenario == "delayed_ack")
    {
      wait(3);
      receive();
      wait(3);
      receive();
    }
    else if (scenario == "reset_pending")
    {
      wait(8);
      receive();
    }
    sc_stop();
  }
};

/** Assert and release the native active-low endpoint reset during a transfer. */
struct ResetDriver : sc_module
{
  sc_out<bool> reset{"reset"};
  bool enabled;

  SC_HAS_PROCESS(ResetDriver);

  ResetDriver(sc_module_name name, bool active) : sc_module(name), enabled(active)
  {
    SC_THREAD(run);
  }

  void run()
  {
    if (!enabled)
    {
      return;
    }
    wait(25, SC_NS);
    reset.write(false);
    record("RESET_ASSERT");
    wait(20, SC_NS);
    reset.write(true);
    record("RESET_RELEASE");
  }
};

/** Instantiate one protocol/scenario, run it, and retain native lifetimes. */
template <typename Channel>
int simulate(const std::string& scenario, bool toggle_protocol)
{
  sc_clock clock("clock", 10, SC_NS, 0.5, 0, SC_NS);
  Channel channel;
  sc_signal<bool> reset("reset");
  Source<Channel> source("source", scenario, toggle_protocol);
  Sink<Channel> sink("sink", scenario);
  source.clock(clock);
  source.reset(reset);
  source.output(channel);
  sink.clock(clock);
  sink.reset(reset);
  sink.input(channel);
  ResetDriver reset_driver("reset_driver", scenario == "reset_pending");
  reset_driver.reset(reset);

  reset.write(false);
  sc_start(1, SC_NS);
  reset.write(true);
  sc_start();
  if (!sc_end_of_simulation_invoked())
  {
    return 2;
  }
  return 0;
}

}  // namespace

/** Select one isolated ready-valid or toggle-toggle reference scenario. */
int sc_main(int argc, char* argv[])
{
  if (argc != 3)
  {
    std::cerr << "usage: example_handshake_edges <rvd|ttd> <scenario>\n";
    return 2;
  }
  const std::string protocol(argv[1]);
  const std::string scenario(argv[2]);
  if (scenario != "stalled_consumer" && scenario != "delayed_ack" && scenario != "reset_pending")
  {
    std::cerr << "unsupported scenario: " << scenario << '\n';
    return 2;
  }
  if (protocol == "rvd")
  {
    return simulate<sc_rvd<sc_uint<8>>>(scenario, false);
  }
  if (protocol == "ttd")
  {
    return simulate<sc_ttd<sc_uint<8>>>(scenario, true);
  }
  std::cerr << "unsupported protocol: " << protocol << '\n';
  return 2;
}

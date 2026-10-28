/** @file sc_core_spawn_options_public.cpp
 * @brief Independent native reference for all public dynamic process options.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <systemc>
#include <tuple>
#include <vector>

#include "sc_core_spawn_options_probe.h"

namespace
{
using spawn_options_probe::Options;
using callback_record = std::pair<std::string, sc_dt::uint64>;
using reset_record = std::tuple<std::string, std::string, sc_dt::uint64>;

std::vector<callback_record> callback_records;
std::vector<reset_record> reset_records;

/** @brief Record reset cleanup when SystemC unwinds a native callback. */
struct Cleanup
{
  const char* label;
  ~Cleanup()
  {
    reset_records.emplace_back(label, "cleanup", sc_core::sc_time_stamp().value());
  }
};

/** @brief Model live source, port, export, and reset ownership. */
struct Fixture : sc_core::sc_module
{
  sc_core::sc_clock clock;
  sc_core::sc_event event;
  sc_core::sc_signal<bool> port_signal;
  sc_core::sc_in<bool> port;
  sc_core::sc_signal<bool> interface_signal;
  sc_core::sc_signal<bool> export_signal;
  sc_core::sc_export<sc_core::sc_signal_in_if<bool>> export_port;
  sc_core::sc_signal<bool> finder_signal;
  sc_core::sc_in<bool> finder_port;
  sc_core::sc_signal<bool> reset_signal;
  sc_core::sc_signal<bool> reset_in_signal;
  sc_core::sc_signal<bool> reset_inout_signal;
  sc_core::sc_signal<bool> reset_out_signal;
  sc_core::sc_in<bool> reset_in;
  sc_core::sc_inout<bool> reset_inout;
  sc_core::sc_out<bool> reset_out;
  std::vector<std::unique_ptr<Options>> options;

  /** @brief Construct and bind all borrowed sensitivity and reset sources. */
  explicit Fixture(sc_core::sc_module_name name)
    : sc_module(name)
    , clock("options_clock", sc_core::sc_time(2, sc_core::SC_NS))
    , event("options_event")
    , port_signal("options_port_signal")
    , port("options_port")
    , interface_signal("options_interface_signal")
    , export_signal("options_export_signal")
    , export_port("options_export")
    , finder_signal("options_finder_signal")
    , finder_port("options_finder_port")
    , reset_signal("options_reset_signal")
    , reset_in_signal("options_reset_in_signal")
    , reset_inout_signal("options_reset_inout_signal")
    , reset_out_signal("options_reset_out_signal")
    , reset_in("options_reset_in")
    , reset_inout("options_reset_inout")
    , reset_out("options_reset_out")
  {
    port.bind(port_signal);
    export_port.bind(export_signal);
    finder_port.bind(finder_signal);
    reset_in.bind(reset_in_signal);
    reset_inout.bind(reset_inout_signal);
    reset_out.bind(reset_out_signal);
  }

  /** @brief Allocate options whose borrowed sources outlive the simulation. */
  Options& make_options()
  {
    options.emplace_back(std::make_unique<Options>());
    return *options.back();
  }

  /** @brief Record one native method callback at its active simulation time. */
  void record_callback(const char* label)
  {
    callback_records.emplace_back(label, sc_core::sc_time_stamp().value());
  }

  /** @brief Wait for reset and preserve cleanup evidence during coroutine unwind. */
  void reset_worker(const char* label, bool asynchronous)
  {
    reset_records.emplace_back(label, "start", sc_core::sc_time_stamp().value());
    Cleanup guard{label};
    while (true)
    {
      if (asynchronous)
        sc_core::wait(100, sc_core::SC_NS);
      else
        sc_core::wait();
    }
  }

  /** @brief Trigger sensitivities, assert all reset sources and stop the run. */
  void drive()
  {
    sc_core::wait(1, sc_core::SC_NS);
    port_signal.write(true);
    interface_signal.write(true);
    export_signal.write(true);
    finder_signal.write(true);
    event.notify();
    sc_core::wait(500, sc_core::SC_PS);
    reset_signal.write(true);
    reset_in_signal.write(true);
    reset_inout.write(true);
    reset_out.write(true);
    sc_core::wait(2, sc_core::SC_NS);
    reset_signal.write(false);
    reset_in_signal.write(false);
    reset_inout.write(false);
    reset_out.write(false);
    sc_core::wait(1500, sc_core::SC_PS);
    sc_core::sc_stop();
  }
};

using option_setter = void (*)(Options&, Fixture&);

/** @brief Append an event pointer through the public sensitivity overload. */
void set_event_sensitivity(Options& options, Fixture& fixture)
{
  options.set_sensitivity(&fixture.event);
}

/** @brief Append a port base pointer through the public sensitivity overload. */
void set_port_sensitivity(Options& options, Fixture& fixture)
{
  options.set_sensitivity(&fixture.port);
}

/** @brief Append an interface pointer through the public sensitivity overload. */
void set_interface_sensitivity(Options& options, Fixture& fixture)
{
  options.set_sensitivity(static_cast<sc_core::sc_interface*>(&fixture.interface_signal));
}

/** @brief Append an export base pointer through the public sensitivity overload. */
void set_export_sensitivity(Options& options, Fixture& fixture)
{
  options.set_sensitivity(&fixture.export_port);
}

/** @brief Append an event finder through the public sensitivity overload. */
void set_finder_sensitivity(Options& options, Fixture& fixture)
{
  options.set_sensitivity(&fixture.port.pos());
}

/** @brief Configure one sensitivity option, spawn its callback, and check stored flags. */
void register_method(Fixture& fixture, const char* label, const char* name, option_setter configure,
                     std::size_t expected_events, std::size_t expected_finders, std::size_t expected_interfaces,
                     std::size_t expected_ports)
{
  auto& option = fixture.make_options();
  option.spawn_method();
  option.dont_initialize();
  option.set_stack_size(256 * 1024);
  configure(option, fixture);
  if (!option.is_method() || !option.no_initialization() || option.stack_size() != 256 * 1024 ||
      option.event_count() != expected_events || option.finder_count() != expected_finders ||
      option.interface_count() != expected_interfaces || option.port_count() != expected_ports)
    throw std::runtime_error("spawn method option state mismatch");
  sc_core::sc_spawn(std::function<void()>(std::bind(&Fixture::record_callback, &fixture, label)), name, &option);
}

using reset_setter = void (*)(Options&, Fixture&);

/** @brief Configure asynchronous reset through an input bool port. */
void set_async_reset_in(Options& options, Fixture& fixture)
{
  options.async_reset_signal_is(fixture.reset_in, true);
}

/** @brief Configure asynchronous reset through an inout bool port. */
void set_async_reset_inout(Options& options, Fixture& fixture)
{
  options.async_reset_signal_is(fixture.reset_inout, true);
}

/** @brief Configure asynchronous reset through an output bool port. */
void set_async_reset_out(Options& options, Fixture& fixture)
{
  options.async_reset_signal_is(fixture.reset_out, true);
}

/** @brief Configure asynchronous reset through the bool input interface. */
void set_async_reset_interface(Options& options, Fixture& fixture)
{
  options.async_reset_signal_is(fixture.reset_signal, true);
}

/** @brief Configure synchronous reset through an input bool port. */
void set_sync_reset_in(Options& options, Fixture& fixture)
{
  options.reset_signal_is(fixture.reset_in, true);
}

/** @brief Configure synchronous reset through an inout bool port. */
void set_sync_reset_inout(Options& options, Fixture& fixture)
{
  options.reset_signal_is(fixture.reset_inout, true);
}

/** @brief Configure synchronous reset through an output bool port. */
void set_sync_reset_out(Options& options, Fixture& fixture)
{
  options.reset_signal_is(fixture.reset_out, true);
}

/** @brief Configure synchronous reset through the bool input interface. */
void set_sync_reset_interface(Options& options, Fixture& fixture)
{
  options.reset_signal_is(fixture.reset_signal, true);
}

/** @brief Configure one reset overload and spawn its wait/unwind callback. */
void register_reset(Fixture& fixture, const char* label, bool asynchronous, reset_setter configure)
{
  auto& option = fixture.make_options();
  if (!asynchronous) option.set_sensitivity(&fixture.clock.posedge_event());
  configure(option, fixture);
  if (option.reset_count() != 1) throw std::runtime_error("spawn reset option count mismatch");
  sc_core::sc_spawn(std::function<void()>(std::bind(&Fixture::reset_worker, &fixture, label, asynchronous)), label,
                    &option);
}
}  // namespace

/** @brief Run native option callbacks and print their reference timeline. */
int sc_main(int, char**)
{
  const int before = Options::destroyed;
  {
    Options sentinel;
  }
  if (Options::destroyed != before + 1) return 2;
  std::cout << "OPTION_DESTRUCTOR\t" << Options::destroyed - before << "\n";

  Fixture fixture("spawn_options");
  register_method(fixture, "event", "option_event", set_event_sensitivity, 1, 0, 0, 0);
  register_method(fixture, "port", "option_port", set_port_sensitivity, 0, 0, 0, 1);
  register_method(fixture, "interface", "option_interface", set_interface_sensitivity, 0, 0, 1, 0);
  register_method(fixture, "export", "option_export", set_export_sensitivity, 0, 0, 1, 0);
  register_method(fixture, "finder", "option_finder", set_finder_sensitivity, 0, 1, 0, 0);

  register_reset(fixture, "async_in", true, set_async_reset_in);
  register_reset(fixture, "async_inout", true, set_async_reset_inout);
  register_reset(fixture, "async_out", true, set_async_reset_out);
  register_reset(fixture, "async_if", true, set_async_reset_interface);
  register_reset(fixture, "sync_in", false, set_sync_reset_in);
  register_reset(fixture, "sync_inout", false, set_sync_reset_inout);
  register_reset(fixture, "sync_out", false, set_sync_reset_out);
  register_reset(fixture, "sync_if", false, set_sync_reset_interface);

  sc_core::sc_spawn(std::function<void()>(std::bind(&Fixture::drive, &fixture)), "drive_options");

  sc_core::sc_start();
  std::sort(callback_records.begin(), callback_records.end());
  for (const auto& [label, time] : callback_records) std::cout << "OPTION_CALLBACK\t" << label << '\t' << time << "\n";

  std::sort(reset_records.begin(), reset_records.end());
  for (const auto& [label, action, time] : reset_records)
    std::cout << "OPTION_RESET\t" << label << '\t' << action << '\t' << time << "\n";

  if (callback_records.size() != 5 || reset_records.size() != 24 ||
      sc_core::sc_time_stamp() != sc_core::sc_time(5, sc_core::SC_NS))
    return 3;
  std::cout << "OPTION_FINAL\t" << sc_core::sc_time_stamp().value() << '\t' << callback_records.size() << '\t'
            << reset_records.size() << "\n";
  return 0;
}

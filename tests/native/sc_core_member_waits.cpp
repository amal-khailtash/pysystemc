/** @file sc_core_member_waits.cpp
 * @brief Native member-wait timing and reset/kill cleanup reference.
 * @details The top owns events and channel through simulation. Native threads
 * run without Python or GIL state; scheduler unwind is always rethrown.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <systemc>
#include <utility>
#include <vector>

using namespace sc_core;

/** @brief Expose native protected channel waits for the reference model. */
struct Channel : sc_prim_channel
{
  explicit Channel(const char* name) : sc_prim_channel(name)
  {
  }
  using sc_prim_channel::wait;
};

/** @brief Forward the same overload arguments directly to native free waits. */
struct NamespaceWait
{
  /** @brief Borrow arguments for one native wait; no Python or GIL is involved. */
  template <class... Arguments>
  void wait(Arguments&&... arguments)
  {
    sc_core::wait(std::forward<Arguments>(arguments)...);
  }
};

/** @brief Retain all wait targets and handles until simulation finishes. */
struct Top : sc_module
{
  Channel channel{"channel"};
  sc_event first{"first"}, second{"second"};
  std::vector<std::string> kinds{"static",     "count",   "time",     "units",       "event",    "or",       "and",
                                 "time_event", "time_or", "time_and", "units_event", "units_or", "units_and"};
  std::vector<sc_process_handle> handles;
  std::vector<std::string> records;
  std::map<std::string, int> starts, cleanups;
  bool use_channel, use_namespace, normal;
  using sc_module::wait;

  /** @brief Invoke each member overload; all arguments stay owned by top. */
  template <class Owner>
  void suspend(Owner& owner, const std::string& kind)
  {
    sc_time duration(normal ? 3 : 100, SC_NS);
    if (kind == "static")
      owner.wait();
    else if (kind == "count")
      owner.wait(2);
    else if (kind == "time")
      owner.wait(duration);
    else if (kind == "units")
      owner.wait(normal ? 3 : 100, SC_NS);
    else if (kind == "event")
      owner.wait(first);
    else if (kind == "or")
      owner.wait(first | second);
    else if (kind == "and")
      owner.wait(first & second);
    else if (kind == "time_event")
      owner.wait(duration, first);
    else if (kind == "time_or")
      owner.wait(duration, first | second);
    else if (kind == "time_and")
      owner.wait(duration, first & second);
    else if (kind == "units_event")
      owner.wait(100, SC_NS, first);
    else if (kind == "units_or")
      owner.wait(100, SC_NS, first | second);
    else
      owner.wait(100, SC_NS, first & second);
  }

  /** @brief Record native scheduler cleanup and preserve its control exception. */
  void worker(const std::string& kind)
  {
    ++starts[kind];
    try
    {
      if (use_namespace)
      {
        NamespaceWait owner;
        suspend(owner, kind);
      }
      else if (use_channel)
        suspend(channel, kind);
      else
        suspend(*this, kind);
      if (!normal) throw std::runtime_error("interrupted wait returned");
      records.push_back("MEMBER_WAIT " + kind + " " + sc_time_stamp().to_string());
    }
    catch (const sc_unwind_exception&)
    {
      ++cleanups[kind];
      records.push_back("MEMBER_CLEANUP " + kind + " " + sc_time_stamp().to_string());
      throw;
    }
    ++cleanups[kind];
  }

  /** @brief Drive events or interrupt every pending wait at deterministic times. */
  void control()
  {
    if (normal)
    {
      wait(1, SC_NS);
      first.notify(SC_ZERO_TIME);
      wait(1, SC_NS);
      first.notify(SC_ZERO_TIME);
      second.notify(SC_ZERO_TIME);
    }
    else
    {
      wait(2, SC_NS);
      for (std::size_t index = 0; index < kinds.size(); ++index) handles[index].reset();
      wait(1, SC_NS);
      for (std::size_t index = 0; index < kinds.size(); ++index) handles[index].kill();
    }
  }

  /** @brief Register callbacks borrowing this model for their native lifetime. */
  Top(sc_module_name name, bool use_channel, bool use_namespace, bool normal)
    : sc_module(name), use_channel(use_channel), use_namespace(use_namespace), normal(normal)
  {
    for (const auto& kind : kinds)
    {
      sc_spawn_options options;
      options.set_sensitivity(&first);
      handles.push_back(sc_spawn([this, kind] { worker(kind); }, kind.c_str(), &options));
    }
    handles.push_back(sc_spawn([this] { control(); }, "control"));
  }
};

/** @brief Run a finite fresh-kernel reference and assert process termination. */
int sc_main(int argc, char** argv)
{
  if (argc != 3) throw std::runtime_error("arguments");
  Top top("top", std::string(argv[1]) == "channel", std::string(argv[1]) == "namespace",
          std::string(argv[2]) == "normal");
  sc_start(4, SC_NS);
  for (const auto& handle : top.handles)
    if (!handle.terminated()) throw std::runtime_error("termination");
  for (const auto& kind : top.kinds)
  {
    if (top.starts[kind] != (top.normal ? 1 : 2))
      throw std::runtime_error("start count " + kind + " " + std::to_string(top.starts[kind]));
    if (top.cleanups[kind] != top.starts[kind])
      throw std::runtime_error("cleanup count " + kind + " " + std::to_string(top.cleanups[kind]));
  }
  sc_stop();
  std::sort(top.records.begin(), top.records.end());
  for (const auto& record : top.records) std::cout << record << '\n';
  return 0;
}

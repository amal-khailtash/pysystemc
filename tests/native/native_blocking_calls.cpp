/** @file native_blocking_calls.cpp
 * @brief Independent native timing, output, error and reset/kill oracle.
 */
#include "native_blocking_calls.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace sc_core;
using namespace blocking_fixture;
/** @brief Retain native functors, channels and sensitivity through a finite simulation. */
struct BlockingTop : sc_module
{
  sc_event first, second;
  ForeignChannel foreign{"foreign"};
  int output = -17;
  std::string family, mode;
  bool normal;
  std::vector<sc_process_handle> handles;
  std::vector<std::string> records;
  std::map<int, int> starts, cleanups;
  /** @brief Invoke raw native pointers or foreign methods, preserving native unwind. */
  void worker(int kind)
  {
    ++starts[kind];
    try
    {
      if (family == "cached")
      {
        CachedWait operation(kind, normal, first, second, sc_get_curr_simcontext());
        operation();
      }
      else if (family == "copy_only")
      {
        CopyOnlyOperation operation(foreign);
        auto result = operation();
        if (result.value != 73) throw std::logic_error("copy-only value");
      }
      else
      {
        ForeignOperation operation(foreign, output, mode == "error");
        if (operation() != 41 || output != 73) throw std::logic_error("result");
      }
      if (!normal) throw std::logic_error("interrupted return");
      records.push_back("BLOCKING_WAIT " + std::to_string(kind) + " " + sc_time_stamp().to_string());
    }
    catch (const sc_unwind_exception&)
    {
      ++cleanups[kind];
      if (output != -17) throw std::logic_error("interrupted output");
      records.push_back("BLOCKING_CLEANUP " + std::to_string(kind) + " " + sc_time_stamp().to_string());
      throw;
    }
    catch (const std::runtime_error& error)
    {
      if (mode != "error" || std::string(error.what()) != "foreign operation failure" || output != -17) throw;
      records.push_back("BLOCKING_ERROR " + std::to_string(kind) + " " + sc_time_stamp().to_string());
      foreign.ready.notify(SC_ZERO_TIME);
      ForeignOperation recovery(foreign, output, false);
      if (recovery() != 41 || output != 73) throw std::logic_error("recovery");
      records.push_back("BLOCKING_RECOVERY " + std::to_string(kind) + " " + sc_time_stamp().to_string());
    }
    ++cleanups[kind];
  }
  /** @brief Complete waits or interrupt each process exactly twice. */
  void control()
  {
    if (normal)
    {
      wait(1, SC_NS);
      first.notify(SC_ZERO_TIME);
      foreign.ready.notify(SC_ZERO_TIME);
      wait(1, SC_NS);
      first.notify(SC_ZERO_TIME);
      second.notify(SC_ZERO_TIME);
    }
    else
    {
      wait(2, SC_NS);
      for (auto& handle : handles) handle.reset();
      wait(1, SC_NS);
      for (auto& handle : handles) handle.kill();
    }
  }
  /** @brief Register independent native processes borrowing this model until simulation ends. */
  BlockingTop(sc_module_name name, const std::string& family, const std::string& mode)
    : sc_module(name), family(family), mode(mode), normal(mode != "reset")
  {
    for (int kind = 0; kind < (family == "cached" ? 13 : 1); ++kind)
    {
      sc_spawn_options options;
      options.set_sensitivity(&first);
      handles.push_back(sc_spawn([this, kind] { worker(kind); }, nullptr, &options));
    }
    sc_spawn([this] { control(); });
  }
};
/** @brief Run the selected finite matrix and require exact cleanup counts and termination. */
int sc_main(int argc, char** argv)
{
  if (argc != 3) throw std::invalid_argument("family mode");
  BlockingTop top("top", argv[1], argv[2]);
  sc_start(4, SC_NS);
  for (const auto& handle : top.handles)
    if (!handle.terminated()) throw std::logic_error("termination");
  for (const auto& entry : top.starts)
    if (entry.second != (top.normal ? 1 : 2) || top.cleanups[entry.first] != entry.second)
      throw std::logic_error("cleanup count");
  sc_stop();
  std::sort(top.records.begin(), top.records.end());
  for (const auto& record : top.records) std::cout << record << '\n';
  return 0;
}

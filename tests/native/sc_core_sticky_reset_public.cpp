/** @file sc_core_sticky_reset_public.cpp
 * @brief Native sticky-reset and descendant-control lifecycle reference.
 * @details All callback storage and handles outlive their native threads. RAII
 * cleanup records scheduler unwind and ordinary return; no Python/GIL involved.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <functional>
#include <iostream>
#include <string>
#include <systemc>
#include <vector>

/** @brief Record an execution marker with the current native simulation time. */
void record(std::vector<std::string>& records, const std::string& kind, const std::string& label)
{
  records.push_back("STICKY_RESET " + kind + ' ' + label + ' ' + std::to_string(sc_core::sc_time_stamp().value()));
}

/** @brief Record cleanup exactly once when its callback frame exits. */
struct cleanup_guard
{
  std::vector<std::string>& records;
  std::string label;
  ~cleanup_guard()
  {
    record(records, "cleanup", label);
  }
};

/** @brief Run the default-policy or recursive native synchronous-reset scenario. */
int sc_main(int argc, char** argv)
{
  using namespace sc_core;
  if (argc != 2) return 2;
  const bool descendants = std::string(argv[1]) == "descendants";
  std::vector<std::string> records;
  sc_process_handle child_handle;
  auto child = [&]
  {
    record(records, "entry", "child");
    cleanup_guard guard{records, "child"};
    wait(1, SC_NS);
    record(records, "done", "child");
  };
  auto parent = [&]
  {
    record(records, "entry", "parent");
    if (descendants && !child_handle.valid()) child_handle = sc_spawn(child, "child");
    cleanup_guard guard{records, "parent"};
    wait(1, SC_NS);
    record(records, "done", "parent");
  };
  auto parent_handle = sc_spawn(parent, "parent");
  sc_spawn(
    [&]
    {
      wait(0.25, SC_NS);
      if (descendants)
        parent_handle.sync_reset_on(SC_INCLUDE_DESCENDANTS);
      else
        parent_handle.sync_reset_on();
      wait(2, SC_NS);
      if (descendants)
        parent_handle.sync_reset_off(SC_INCLUDE_DESCENDANTS);
      else
        parent_handle.sync_reset_off();
    },
    "controller");
  sc_start(4, SC_NS);
  for (const auto& line : records) std::cout << line << '\n';
  sc_stop();
  return 0;
}

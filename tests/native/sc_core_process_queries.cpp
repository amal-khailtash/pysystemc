/** @file sc_core_process_queries.cpp
 * @brief Native current process/kind and handle fallback reference.
 * @details Raw pointers are borrowed; handles retain native reference counts.
 */
#include <algorithm>
#include <sstream>
#include <systemc>
#include <vector>
std::vector<std::string> records;
int entries = 0;
int cleanups = 0;
/** @brief Check current process queries without modifying process state. */
void observe(const char* label, sc_core::sc_curr_proc_kind expected)
{
  using namespace sc_core;
  auto kind = sc_get_curr_process_kind();
  auto* raw = sc_get_curr_process_handle();
  auto handle = sc_get_current_process_handle();
  if (kind != expected || sc_is_unwinding()) throw std::runtime_error("kind/unwind");
  if (expected == SC_NO_PROC_)
  {
    if (raw) throw std::runtime_error("raw outside process");
  }
  else if (!raw || !handle.valid() || std::string(raw->name()) != handle.name() || raw->proc_kind() != kind)
    throw std::runtime_error("identity");
  std::ostringstream out;
  out << "PROCESS_QUERY_REFERENCE " << label << " " << int(kind) << " " << bool(raw) << " " << handle.valid() << " "
      << sc_is_unwinding();
  records.push_back(out.str());
}
/** @brief Own independently registered method and pthread thread callbacks. */
struct model : sc_core::sc_module
{
  SC_HAS_PROCESS(model);
  model(sc_core::sc_module_name name) : sc_module(name)
  {
    SC_METHOD(method);
    if (std::string(sc_core::sc_get_last_created_process_handle().name()) != "queries.method")
      throw std::runtime_error("last method");
    SC_THREAD(thread);
    if (std::string(sc_core::sc_get_current_process_handle().name()) != "queries.thread")
      throw std::runtime_error("construction fallback");
  }
  void method()
  {
    observe("method", sc_core::SC_METHOD_PROC_);
  }
  void thread()
  {
    observe("thread", sc_core::SC_THREAD_PROC_);
  }
};
/** @brief Compare callback and outside-callback handle fallback states. */
int sc_main(int, char**)
{
  using namespace sc_core;
  observe("before", SC_NO_PROC_);
  model top("queries");
  if (sc_get_last_created_process_handle().valid()) return 2;
  observe("registered", SC_NO_PROC_);
  auto worker = sc_spawn(
    []()
    {
      ++entries;
      try
      {
        wait(100, SC_NS);
      }
      catch (const sc_unwind_exception&)
      {
        if (!sc_is_unwinding() || !sc_get_current_process_handle().is_unwinding())
          throw std::runtime_error("unwind state");
        ++cleanups;
        records.push_back("PROCESS_UNWIND_REFERENCE " + std::to_string(entries) + " 1");
        throw;
      }
    },
    "unwind_worker");
  auto controller = sc_spawn(
    [&]()
    {
      wait(1, SC_NS);
      worker.reset();
      wait(1, SC_NS);
      worker.kill();
    },
    "controller");
  sc_start(3, SC_NS);
  if (cleanups != 2 || !worker.terminated() || !controller.terminated()) return 4;
  observe("paused", SC_NO_PROC_);
  sc_stop();
  observe("stopped", SC_NO_PROC_);
  std::sort(records.begin(), records.end());
  for (auto& record : records) std::cout << record << "\n";
  return 0;
}

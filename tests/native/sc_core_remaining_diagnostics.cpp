/** @file sc_core_remaining_diagnostics.cpp
 * @brief Native references for the remaining recoverable F4 kernel diagnostics.
 * @details Run one scenario per process because the native SystemC kernel is a
 * singleton. The report callback copies borrowed metadata synchronously and
 * delegates selected throwing actions to the default handler. No Python state
 * or references cross the native callback boundary.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <systemc>
#include <vector>

using namespace sc_core;
namespace sc_core
{
void sc_cycle(const sc_time& duration);
}

namespace remaining_diag
{
/** @brief Copy one native report row while borrowed metadata remains valid. */
struct Row
{
  std::string label;
  int id;
  int severity;
  std::string type;
  std::string message;
  std::uint64_t time;
  std::string process;
  int verbosity;
  unsigned actions;
};

std::string scenario;
std::string current_label;
std::vector<Row> rows;

/** @brief Normalize report whitespace for stable Python/native comparisons. */
std::string normalized(const char* message)
{
  std::istringstream input(message ? message : "");
  std::string word, result;
  while (input >> word)
  {
    if (!result.empty()) result += ' ';
    result += word;
  }
  return result;
}

/** @brief Capture labeled diagnostic metadata and preserve the chosen action.
 * @details The callback copies all borrowed strings synchronously and delegates
 * the original report/actions to SystemC's default handler before returning.
 */
void capture(const sc_report& report, const sc_actions& actions)
{
  if (!current_label.empty() && report.get_severity() != SC_INFO)
  {
    std::string label = current_label;
    if (label == "w509_missing_end" && report.get_id() == 510) label = "w510_hierarchy";
    rows.push_back({label, report.get_id(), static_cast<int>(report.get_severity()), report.get_msg_type(),
                    normalized(report.get_msg()), report.get_time().value(),
                    report.get_process_name() ? report.get_process_name() : "", report.get_verbosity(), actions});
  }
  sc_report_handler::default_handler(report, actions);
}

/** @brief Require one throwing native report without changing report identity.
 * @param label Stable output label copied into the captured row.
 * @param id Expected SystemC report identifier.
 * @param operation Native call configured to throw exactly one report.
 */
template <class Operation>
void attempt(const std::string& label, int id, Operation&& operation)
{
  current_label = label;
  const auto before = rows.size();
  try
  {
    operation();
  }
  catch (const sc_report& report)
  {
    if (rows.size() != before + 1 || report.get_id() != id || report.get_severity() != SC_ERROR ||
        rows.back().time != sc_time_stamp().value())
      throw std::runtime_error("native report mismatch: " + label);
    return;
  }
  throw std::runtime_error("configured native report did not throw: " + label);
}

std::string state()
{
  std::ostringstream output;
  output << sc_time_stamp().value() << '|' << sc_delta_count() << '|' << static_cast<int>(sc_get_status());
  return output.str();
}

void print_rows()
{
  for (const auto& row : rows)
    std::cout << "REMAINING_DIAGNOSTIC_REPORT|" << scenario << '|' << row.label << '|' << row.id << '|' << row.severity
              << '|' << row.type << '|' << row.message << '|' << row.time << '|' << row.process << '|' << row.verbosity
              << '|' << row.actions << '\n';
  std::cout << "REMAINING_DIAGNOSTIC_STATE|" << scenario << '|' << state() << '\n';
}

class OrphanModule : public sc_module
{
  SC_HAS_PROCESS(OrphanModule);

 public:
  explicit OrphanModule(sc_module_name name) : sc_module(name)
  {
    SC_METHOD(orphan);
    dont_initialize();
  }
  void orphan()
  {
    ++calls;
  }
  int calls = 0;
};

class StackModule : public sc_module
{
  SC_HAS_PROCESS(StackModule);

 public:
  explicit StackModule(sc_module_name name) : sc_module(name)
  {
    SC_METHOD(method);
  }
  void method()
  {
  }
  void set_method_stack_size(std::size_t size)
  {
    set_stack_size(size);
  }
};

class LegacyModule : public sc_module
{
 public:
  explicit LegacyModule(const char* name) : sc_module(name)
  {
  }
};

class ErrorModule : public sc_module
{
  SC_HAS_PROCESS(ErrorModule);

 public:
  explicit ErrorModule(sc_module_name name) : sc_module(name)
  {
    SC_METHOD(fail);
    sensitive << event;
  }
  void fail()
  {
    throw std::runtime_error("prior kernel error");
  }
  sc_event event{"prior_error_event"};
};

/** @brief Own an input port for positional binding error and recovery checks. */
struct BindModule : sc_module
{
  explicit BindModule(sc_module_name name) : sc_module(name), input("input")
  {
  }
  sc_in<bool> input;
};

/** @brief Route a native interface through sc_module's positional bind path. */
void positional_bind_interface(sc_module& module, sc_interface& interface_)
{
  module << interface_;
}

int target_entries = 0;
int unwind_cleanups = 0;
int controller_done = 0;
sc_process_handle unwind_target;

struct ReenterReset
{
  sc_process_handle* target;
  ~ReenterReset()
  {
    ++unwind_cleanups;
    current_label = "w540_reentrant_reset";
    target->reset();
    current_label.clear();
  }
};

void unwind_worker()
{
  ++target_entries;
  if (target_entries == 1)
  {
    ReenterReset cleanup{&unwind_target};
    wait(20, SC_NS);
  }
  else
  {
    wait(20, SC_NS);
  }
}

void unwind_controller()
{
  wait(1, SC_NS);
  unwind_target.reset();
  wait(1, SC_NS);
  controller_done = static_cast<int>(sc_time_stamp().value());
  sc_stop();
}

int self_calls = 0;
sc_event self_event{"self_notify_event"};

void run_orphan()
{
  sc_report_handler::set_actions(SC_WARNING, SC_DO_NOTHING);
  OrphanModule owner("orphan_owner");
  current_label = "w558_orphan";
  sc_start(SC_ZERO_TIME);
  current_label.clear();
  if (owner.calls != 0 || rows.size() != 1 || rows[0].id != 558 || rows[0].severity != SC_WARNING ||
      sc_time_stamp() != SC_ZERO_TIME)
    throw std::runtime_error("W558 scenario did not preserve orphan state");
}

void run_idle_start()
{
  sc_report_handler::set_actions(SC_WARNING, SC_DO_NOTHING);
  sc_start(SC_ZERO_TIME);
  if (!rows.empty()) throw std::runtime_error("first initialization start unexpectedly reported");
  current_label = "w571_idle";
  sc_start(SC_ZERO_TIME);
  current_label.clear();
  if (rows.size() != 1 || rows[0].id != 571 || rows[0].severity != SC_WARNING || sc_time_stamp() != SC_ZERO_TIME)
    throw std::runtime_error("W571 scenario did not preserve idle state");
}

void start_overflow()
{
  sc_start(sc_max_time(), SC_RUN_TO_TIME);
}

void run_overflow()
{
  sc_start(1, SC_PS, SC_RUN_TO_TIME);
  const auto before = state();
  const sc_actions old = sc_report_handler::set_actions(SC_ID_SIMULATION_TIME_OVERFLOW_, SC_ERROR, SC_THROW);
  attempt("e544_overflow", 544, start_overflow);
  if (state() != before) throw std::runtime_error("E544 changed simulation state");
  sc_report_handler::set_actions(SC_ID_SIMULATION_TIME_OVERFLOW_, SC_ERROR, old);
}

struct UnwindModule : sc_module
{
  SC_HAS_PROCESS(UnwindModule);
  explicit UnwindModule(sc_module_name name) : sc_module(name)
  {
    SC_THREAD(target);
    unwind_target = sc_get_last_created_process_handle();
    SC_THREAD(controller);
  }
  void target()
  {
    unwind_worker();
  }
  void controller()
  {
    unwind_controller();
  }
};

void run_unwinding_case()
{
  sc_report_handler::set_actions(SC_WARNING, SC_DO_NOTHING);
  sc_module_name name("unwind_owner");
  UnwindModule owner(name);
  sc_start(5, SC_NS, SC_RUN_TO_TIME);
  if (rows.size() != 1 || rows[0].id != 540 || rows[0].severity != SC_WARNING || target_entries != 2 ||
      unwind_cleanups != 1 || controller_done != 2000 || sc_time_stamp().value() != 2000 || unwind_target.terminated())
  {
    std::cerr << "W540_DEBUG " << rows.size() << ' ' << target_entries << ' ' << unwind_cleanups << ' '
              << controller_done << ' ' << sc_time_stamp().value() << ' ' << unwind_target.terminated() << '\n';
    throw std::runtime_error("W540 unwind recovery mismatch");
  }
}

struct SelfNotifyModule : sc_module
{
  SC_HAS_PROCESS(SelfNotifyModule);
  explicit SelfNotifyModule(sc_module_name name) : sc_module(name)
  {
    SC_METHOD(notify_self);
    sensitive << self_event;
    dont_initialize();
  }
  void notify_self()
  {
    ++self_calls;
    current_label = "w536_self_notify";
    self_event.notify();
    current_label.clear();
  }
};

void run_self_notify()
{
  sc_report_handler::set_actions(SC_WARNING, SC_DO_NOTHING);
  sc_module_name name("self_notify_owner");
  SelfNotifyModule owner(name);
  self_event.notify(1, SC_NS);
  sc_start(2, SC_NS, SC_RUN_TO_TIME);
  if (rows.size() != 1 || rows[0].id != 536 || rows[0].severity != SC_WARNING || self_calls != 1 ||
      sc_time_stamp().value() != 2000)
    throw std::runtime_error("W536 self-notification mismatch");
}

void run_module_warnings()
{
  sc_report_handler::set_actions(SC_WARNING, SC_DO_NOTHING);
  current_label = "w569_legacy_one";
  LegacyModule first("legacy_one");
  current_label = "w569_legacy_two";
  LegacyModule second("legacy_two");
  current_label = "w509_missing_end";
  sc_start(SC_ZERO_TIME);
  current_label.clear();
  const std::vector<int> ids{569, 569, 509, 509, 510};
  if (rows.size() != ids.size()) throw std::runtime_error("module warning count mismatch");
  for (std::size_t index = 0; index < ids.size(); ++index)
    if (rows[index].id != ids[index] || rows[index].severity != SC_WARNING)
      throw std::runtime_error("module warning metadata mismatch");
  if (std::strcmp(first.name(), "legacy_one") != 0 || std::strcmp(second.name(), "legacy_one.legacy_two") != 0 ||
      sc_get_status() != SC_PAUSED)
    throw std::runtime_error("module warning state mismatch");
}

void run_module_stack()
{
  sc_report_handler::set_actions(SC_WARNING, SC_DO_NOTHING);
  sc_module_name name("stack_owner");
  StackModule module(name);
  current_label = "w511_method_stack";
  module.set_method_stack_size(1024);
  current_label.clear();
  if (rows.size() != 1 || rows[0].id != 511 || rows[0].severity != SC_WARNING ||
      std::strcmp(module.name(), "stack_owner") != 0)
    throw std::runtime_error("W511 stack warning mismatch");
}

void restart_after_error()
{
  sc_start(SC_ZERO_TIME);
}
void reentrant_start()
{
  sc_start(SC_ZERO_TIME);
}

void run_after_error()
{
  sc_report_handler::set_handler(nullptr);
  auto* name = new sc_module_name("prior_error_owner");
  ErrorModule owner(*name);
  delete name;
  try
  {
    sc_start(SC_ZERO_TIME);
  }
  catch (const sc_report& report)
  {
    if (report.get_id() != 549) throw;
  }
  const int status_before = static_cast<int>(sc_get_status());
  if (sc_get_curr_simcontext()->sim_status() != SC_SIM_ERROR)
    throw std::runtime_error("native error setup did not enter SC_SIM_ERROR");
  sc_report_handler::set_handler(capture);
  sc_report_handler::set_actions(SC_ID_SIMULATION_START_AFTER_ERROR_, SC_ERROR, SC_THROW);
  attempt("e548_restart_after_error", 548, restart_after_error);
  if (static_cast<int>(sc_get_status()) != status_before || sc_get_curr_simcontext()->sim_status() != SC_SIM_ERROR)
    throw std::runtime_error("E548 changed kernel status");
}

sc_process_handle raw_target;
int nested_callbacks = 0;
sc_event nested_event{"nested_start_event"};
void nested_process()
{
  ++nested_callbacks;
  attempt("e554_nested_start", 554, reentrant_start);
}

struct NestedStartModule : sc_module
{
  SC_HAS_PROCESS(NestedStartModule);
  explicit NestedStartModule(sc_module_name name) : sc_module(name)
  {
    SC_METHOD(nested);
    sensitive << nested_event;
    dont_initialize();
  }
  void nested()
  {
    nested_process();
  }
};

int cycle_callbacks = 0;
sc_event cycle_event{"cycle_pending_event"};
struct CycleModule : sc_module
{
  SC_HAS_PROCESS(CycleModule);
  explicit CycleModule(sc_module_name name) : sc_module(name)
  {
    SC_METHOD(observe);
    sensitive << cycle_event;
    dont_initialize();
  }
  void observe()
  {
    ++cycle_callbacks;
  }
};

/** @brief Probe forbidden process creation during registered simulation phases.
 * @details The kernel invokes this object's callback synchronously until its
 * destruction unregisters it. Spawn closures are C++-owned and retain no Python
 * references, callbacks, or GIL state.
 */
struct PhaseProbe : sc_object
{
  /** @brief Register for update and pre-timestep callbacks before simulation.
   * @param name Native object name used as the callback hierarchy scope.
   */
  explicit PhaseProbe(const char* name) : sc_object(name)
  {
    register_simulation_phase_callback(SC_END_OF_UPDATE | SC_BEFORE_TIMESTEP);
  }

  int update_calls = 0;
  int timestep_calls = 0;

 private:
  /** @brief Attempt one method/thread pair during each selected phase. */
  void simulation_phase_callback() override
  {
    const auto phase = sc_get_status();
    if (phase == SC_END_OF_UPDATE && update_calls++ == 0)
      spawn_pair("end_update");
    else if (phase == SC_BEFORE_TIMESTEP && timestep_calls++ == 0)
      spawn_pair("before_timestep");
  }

  /** @brief Request immediate native spawns that the active phase forbids.
   * @param phase Suffix distinguishing end-of-update and pre-timestep probes.
   */
  void spawn_pair(const char* phase)
  {
    const std::string method_name = std::string("method_") + phase;
    const std::string thread_name = std::string("thread_") + phase;
    sc_spawn_options method_options;
    method_options.spawn_method();
    sc_spawn([] {}, method_name.c_str(), &method_options);
    sc_spawn([] { wait(1, SC_NS); }, thread_name.c_str());
  }
};

void run_nested_start()
{
  const sc_actions old = sc_report_handler::set_actions(SC_ID_SIMULATION_START_UNEXPECTED_, SC_ERROR, SC_THROW);
  auto* name = new sc_module_name("nested_start_owner");
  NestedStartModule owner(*name);
  delete name;
  nested_event.notify(1, SC_NS);
  sc_start(2, SC_NS, SC_RUN_TO_TIME);
  if (rows.size() != 1 || rows[0].id != 554 || rows[0].severity != SC_ERROR || nested_callbacks != 1 ||
      sc_time_stamp().value() != 2000 || sc_get_status() != SC_PAUSED)
    throw std::runtime_error("E554 reentrant start mismatch");
  sc_report_handler::set_actions(SC_ID_SIMULATION_START_UNEXPECTED_, SC_ERROR, old);
}

void raw_kill(sc_process_handle& handle)
{
  handle.kill();
}
void raw_reset(sc_process_handle& handle)
{
  handle.reset();
}
void raw_kill_target()
{
  raw_kill(raw_target);
}
void raw_reset_target()
{
  raw_reset(raw_target);
}

struct RawControlModule : sc_module
{
  SC_HAS_PROCESS(RawControlModule);
  explicit RawControlModule(sc_module_name name) : sc_module(name)
  {
    SC_THREAD(target);
  }
  void target()
  {
    wait(10, SC_NS);
  }
};

void run_raw_controls()
{
  sc_module_name name("raw_control_owner");
  RawControlModule owner(name);
  sc_process_handle handle = sc_get_last_created_process_handle();
  sc_report_handler::set_actions(SC_ID_KILL_PROCESS_WHILE_UNITIALIZED_, SC_ERROR, SC_THROW);
  sc_report_handler::set_actions(SC_ID_RESET_PROCESS_WHILE_NOT_RUNNING_, SC_ERROR, SC_THROW);
  raw_target = handle;
  attempt("e572_raw_kill_before_start", 572, raw_kill_target);
  if (!handle.valid() || std::strcmp(handle.name(), "raw_control_owner.target") != 0 || handle.terminated())
    throw std::runtime_error("E572 changed target process");
  attempt("e573_raw_reset_before_start", 573, raw_reset_target);
  if (!handle.valid() || std::strcmp(handle.name(), "raw_control_owner.target") != 0 || handle.terminated() ||
      sc_get_status() != SC_ELABORATION)
    throw std::runtime_error("E573 changed target process or kernel state");
}

void run_cycle_misses()
{
  sc_report_handler::set_actions(SC_WARNING, SC_DO_NOTHING);
  auto* name = new sc_module_name("cycle_owner");
  CycleModule owner(*name);
  delete name;
  sc_start(SC_ZERO_TIME);
  cycle_event.notify(1, SC_NS);
  current_label = "w538_cycle_misses_events";
  sc_cycle(sc_time(1, SC_NS));
  current_label.clear();
  if (rows.size() != 1 || rows[0].id != 538 || rows[0].severity != SC_WARNING || cycle_callbacks != 0 ||
      sc_time_stamp().value() != 1000)
    throw std::runtime_error("W538 missed-cycle state mismatch");
  sc_start(1, SC_NS, SC_RUN_TO_TIME);
  if (cycle_callbacks != 1 || sc_time_stamp().value() != 2000) throw std::runtime_error("W538 event recovery mismatch");
}

void run_phase_callback_forbidden()
{
  PhaseProbe probe("phase_probe");
  sc_event timer("phase_probe_timer");
  timer.notify(1, SC_NS);
  current_label = "w553_phase_callback";
  sc_start(2, SC_NS, SC_RUN_TO_TIME);
  current_label.clear();
  if (rows.size() != 4 || probe.update_calls < 1 || probe.timestep_calls < 1 || sc_time_stamp().value() != 2000)
    throw std::runtime_error("W553 phase callback recovery mismatch");
  for (const auto& row : rows)
    if (row.id != 553 || row.severity != SC_WARNING)
      throw std::runtime_error("W553 phase callback report metadata mismatch");
}

void run_module_binding_error()
{
  BindModule owner("bind_owner");
  sc_signal<int> wrong("wrong_signal");
  sc_signal<bool> correct("correct_signal");
  const sc_actions old = sc_report_handler::set_actions(SC_ID_BIND_IF_TO_PORT_, SC_ERROR, SC_THROW);
  const auto before = state();
  current_label = "e107_module_binding_type_mismatch";
  attempt("e107_module_binding_type_mismatch", 107, [&] { positional_bind_interface(owner, wrong); });
  current_label.clear();
  if (state() != before || rows.back().message != "type mismatch on port 0 of module `bind_owner'")
    throw std::runtime_error("E107 changed elaboration state or report message");
  positional_bind_interface(owner, correct);
  sc_start(SC_ZERO_TIME);
  if (owner.input.read() || sc_get_status() != SC_PAUSED || sc_time_stamp() != SC_ZERO_TIME)
    throw std::runtime_error("E107 binding recovery failed");
  sc_report_handler::set_actions(SC_ID_BIND_IF_TO_PORT_, SC_ERROR, old);
}
}  // namespace remaining_diag

int sc_main(int argc, char** argv)
{
  using namespace remaining_diag;
  if (argc < 2) return 2;
  scenario = argv[1];
  sc_report_handler::set_handler(capture);
  if (scenario == "orphan")
    run_orphan();
  else if (scenario == "idle_start")
    run_idle_start();
  else if (scenario == "overflow")
    run_overflow();
  else if (scenario == "unwinding")
    run_unwinding_case();
  else if (scenario == "self_notify")
    run_self_notify();
  else if (scenario == "module_warnings")
    run_module_warnings();
  else if (scenario == "module_stack")
    run_module_stack();
  else if (scenario == "after_error")
    run_after_error();
  else if (scenario == "raw_controls")
    run_raw_controls();
  else if (scenario == "nested_start")
    run_nested_start();
  else if (scenario == "cycle_misses")
    run_cycle_misses();
  else if (scenario == "phase_callback_forbidden")
    run_phase_callback_forbidden();
  else if (scenario == "module_binding_error")
    run_module_binding_error();
  else
    return 3;
  print_rows();
  return 0;
}

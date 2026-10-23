/**
 * @file sc_module_new.cpp
 * @brief Native process and tracing adapters for cppyy Python modules.
 * @details Callers release the GIL for blocking waits; cppyy reacquires it on Python callback entry. Callback
 * exceptions propagate through the simulation boundary. Guarded waits preserve reset/kill scheduler unwind.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <exception>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <systemc>

#include "binding_log.h"

namespace sc_core
{

/**
 * @brief Public constructor for Python-defined native interfaces.
 * @details Native ports borrow interfaces; Python must retain the implementing object through simulation.
 * cppyy dispatch acquires the GIL for Python overrides. Overrides must not return temporary events.
 */
class PythonInterface : public virtual sc_interface
{
 public:
  PythonInterface() = default;
  ~PythonInterface() override = default;
};

/**
 * @brief Public primitive-channel constructor and virtual update hooks for Python subclasses.
 * @details Kernel registration borrows the channel for its simulation lifetime. Retain the Python owner.
 * cppyy acquires the GIL for overrides; update must never wait. Callback errors propagate to simulation.
 */
class PythonPrimChannel : public sc_prim_channel
{
 public:
  explicit PythonPrimChannel(const char* name) : sc_prim_channel(name)
  {
  }
  ~PythonPrimChannel() override = default;

  using sc_prim_channel::async_attach_suspending;
  using sc_prim_channel::async_detach_suspending;

  void update() override
  {
  }
  void before_end_of_elaboration() override
  {
  }
  void end_of_elaboration() override
  {
  }
  void start_of_simulation() override
  {
  }
  void end_of_simulation() override
  {
  }
};

/**
 * @brief Stable native storage for Python-controlled signal interfaces.
 * @details Ports borrow this channel and its references. Retain the owner through simulation.
 * Native update scheduling and events belong to sc_signal; no Python reference-return dispatcher is used.
 * No callbacks or GIL release occur in reads/writes; Python processes may drive the channel normally.
 */
template <class T>
class PythonSignalChannel : public sc_signal<T>
{
 public:
  explicit PythonSignalChannel(const char* name) : sc_signal<T>(name)
  {
  }
  ~PythonSignalChannel() override = default;
};

/**
 * @brief Python-subclassable FIFO with stable native reference arguments and events.
 * @details Native ports borrow the channel; retain its Python owner through simulation.
 * sc_fifo owns storage, delta-cycle accounting and blocking behavior. Blocking read/write calls
 * must release the GIL so other Python processes can run. Exceptions propagate to the caller;
 * overrides of reference-return methods are unsupported by cppyy and must use the native methods.
 */
template <class T>
class PythonFifoChannel : public sc_fifo<T>
{
 public:
  PythonFifoChannel(const char* name, int size = 16) : sc_fifo<T>(name, size)
  {
  }
  ~PythonFifoChannel() override = default;
};

/**
 * @brief Adapt a Python value callback to stable native signal storage.
 * @details Caller retains callback/channel through simulation. cppyy acquires the GIL for callback
 * dispatch. notify_changed samples once and schedules a native write; reads never call Python.
 * Native sc_signal owns stable value/event/edge references and deferred update scheduling.
 * Call sampling from the simulation thread, never a separate host thread; callback must not wait.
 */
template <class T>
class PythonReadInterface : public sc_signal<T>
{
  std::function<T()> reader;

 public:
  PythonReadInterface(const char* name, std::function<T()> callback) : sc_signal<T>(name), reader(std::move(callback))
  {
  }
  ~PythonReadInterface() override = default;
  void notify_changed()
  {
    this->write(reader());
  }
};

/**
 * @brief Read resolved logic through its native signal interface.
 * @param signal Borrowed resolved channel retained by the caller.
 * @return A value copy, avoiding borrowed storage escaping to Python.
 * @details No callbacks or GIL release required; native update scheduling is unchanged.
 */
sc_dt::sc_logic python_read_resolved(const sc_signal_resolved& signal)
{
  return signal.read();
}

/**
 * @brief Select native time modulo without cppyy operator discovery ambiguity.
 * @param left Borrowed dividend time.
 * @param right Borrowed divisor time.
 * @return Native remainder; inputs are not modified.
 * @details No callbacks or GIL release are required. A zero divisor is rejected before native integer division.
 */
sc_time python_time_modulo(const sc_time& left, const sc_time& right)
{
  if (right == SC_ZERO_TIME) throw std::invalid_argument("time modulo divisor must be nonzero");
  return left % right;
}

/**
 * @brief Read a native argv entry without cppyy pointer-array conversion.
 * @param index Argument position during sc_main execution.
 * @return Copy of the original native argument.
 * @details Native argv is borrowed only during entry; out-of-range indices throw std::out_of_range.
 */
std::string python_argv_at(int index)
{
  if (!sc_in_action) throw std::runtime_error("native argv is only valid during sc_main");
  if (index < 0 || index >= sc_argc()) throw std::out_of_range("native argv index");
  return sc_argv()[index];
}

/**
 * @brief Own pending scheduler unwind on the suspended process's OS thread.
 * @details Guarded waits and self-control capture it; callback dispatch rethrows and clears it before returning to the
 * kernel.
 */
thread_local std::exception_ptr python_unwind;

/** @brief Capture scheduler unwind from a borrowed native utility operation.
 * @details Release GIL; operation stays alive during suspension. Other errors propagate.
 * @return False requests Python cleanup and restoration by dispatch_python.
 */
bool python_guarded_operation(const std::function<void()>& operation)
{
  try
  {
    operation();
    return true;
  }
  catch (const sc_unwind_exception&)
  {
    python_unwind = std::current_exception();
    return false;
  }
}

/**
 * @brief Run a wait without allowing cppyy to consume scheduler unwind.
 * @details Call with GIL released. False tells Python to unwind its frames; the original exception stays owned here.
 */
template <class Operation>
bool guarded_wait(Operation operation)
{
  try
  {
    operation();
    return true;
  }
  catch (const sc_unwind_exception&)
  {
    python_unwind = std::current_exception();
    return false;
  }
}

/** @brief Guard native FIFO read against cppyy consuming scheduler unwind.
 * @details Borrow channel for this call, release GIL before entry. On unwind
 * Python must raise its private unwind signal; output is assigned only on success.
 */
template <class T>
std::pair<T, bool> python_fifo_read(sc_fifo_blocking_in_if<T>& channel)
{
  T value{};
  bool complete = python_guarded_operation([&] { channel.read(value); });
  return {value, complete};
}

/** @brief Guard native FIFO writes; borrowed input is copied by the channel.
 * @details Release GIL; false requires Python stack unwind. Exceptions propagate.
 */
template <class T>
bool python_fifo_write(sc_fifo_blocking_out_if<T>& channel, const T& value)
{
  return python_guarded_operation([&] { channel.write(value); });
}

/** @brief Guard a borrowed mutex lock while preserving scheduler unwind.
 * @details Caller releases GIL and retains channel throughout suspension.
 * Ordinary errors propagate; false completion requests Python frame cleanup.
 */
std::pair<int, bool> python_mutex_lock(sc_mutex_if& channel)
{
  int result = -1;
  bool complete = python_guarded_operation([&] { result = channel.lock(); });
  return {result, complete};
}

/** @brief Guard a borrowed semaphore wait with the same lifetime/GIL contract.
 * @return Native status and completion; false requires Python unwind.
 */
std::pair<int, bool> python_semaphore_wait(sc_semaphore_if& channel)
{
  int result = -1;
  bool complete = python_guarded_operation([&] { result = channel.wait(); });
  return {result, complete};
}

/**
 * @brief Kill a process from an active native scheduler process.
 * @param handle Borrowed process handle, retained by the caller.
 * @param descendants Native descendant inclusion policy.
 * @details Release the GIL. Calls outside a running process are rejected rather than entering unsafe kernel context.
 */
bool python_kill(sc_process_handle& handle, sc_descendant_inclusion_info descendants = SC_NO_DESCENDANTS)
{
  if (!sc_get_current_process_handle().valid() || sc_get_status() != SC_RUNNING)
    throw std::runtime_error("process kill requires a running SystemC process");
  return guarded_wait([&]() { handle.kill(descendants); });
}

/** @brief Reset from a running process; ownership, GIL and context requirements match python_kill. */
bool python_reset(sc_process_handle& handle, sc_descendant_inclusion_info descendants = SC_NO_DESCENDANTS)
{
  if (!sc_get_current_process_handle().valid() || sc_get_status() != SC_RUNNING)
    throw std::runtime_error("process reset requires a running SystemC process");
  return guarded_wait([&]() { handle.reset(descendants); });
}

/** @brief Suspend a retained process, including the current Python process.
 * @param handle Borrowed handle retained throughout suspension.
 * @param descendants Native descendant inclusion policy.
 * @return False when native reset/kill requires Python frame cleanup.
 * @details Caller releases the GIL. Current-process suspension may switch pthreads;
 * guarded_wait preserves native unwind after the Python callback stack returns.
 * Native reports propagate. Called only from a running scheduler process.
 */
bool python_suspend(sc_process_handle& handle, sc_descendant_inclusion_info descendants = SC_NO_DESCENDANTS)
{
  if (!sc_get_current_process_handle().valid() || sc_get_status() != SC_RUNNING)
    throw std::runtime_error("process suspension requires a running SystemC process");
  return guarded_wait([&]() { handle.suspend(descendants); });
}

/** @brief Set the next method trigger from an OR expression, consuming its native auto-delete list. */
void python_next_trigger(const sc_event_expr<sc_event_or_list>& expression)
{
  next_trigger(expression.release());
}

/** @brief Set the next method trigger from an AND expression, consuming its native auto-delete list. */
void python_next_trigger(const sc_event_expr<sc_event_and_list>& expression)
{
  next_trigger(expression.release());
}

/**
 * @brief Dispatch Python and restore original native scheduler control flow.
 * @details cppyy acquires the GIL for callback entry. Even if Python catches its private unwind signal and returns,
 * a pending native unwind is rethrown. Callers must propagate Python unwind immediately and execute cleanup only.
 */
void dispatch_python(const std::function<void()>& callback)
{
  try
  {
    callback();
  }
  catch (...)
  {
    if (python_unwind)
    {
      auto original = python_unwind;
      python_unwind = nullptr;
      std::rethrow_exception(original);
    }
    throw;
  }
  if (python_unwind)
  {
    auto original = python_unwind;
    python_unwind = nullptr;
    std::rethrow_exception(original);
  }
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait()
{
  return guarded_wait([&]() { wait(); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(int cycles)
{
  return guarded_wait([&]() { wait(cycles); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(const sc_time& time)
{
  return guarded_wait([&]() { wait(time); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(double value, sc_time_unit unit)
{
  return guarded_wait([&]() { wait(value, unit); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(const sc_event& event)
{
  return guarded_wait([&]() { wait(event); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(const sc_time& time, const sc_event& event)
{
  return guarded_wait([&]() { wait(time, event); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(double value, sc_time_unit unit, const sc_event& event)
{
  return guarded_wait([&]() { wait(value, unit, event); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(const sc_event_or_list& event)
{
  return guarded_wait([&]() { wait(event); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(const sc_time& time, const sc_event_or_list& event)
{
  return guarded_wait([&]() { wait(time, event); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(double value, sc_time_unit unit, const sc_event_or_list& event)
{
  return guarded_wait([&]() { wait(value, unit, event); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(const sc_event_and_list& event)
{
  return guarded_wait([&]() { wait(event); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(const sc_time& time, const sc_event_and_list& event)
{
  return guarded_wait([&]() { wait(time, event); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(double value, sc_time_unit unit, const sc_event_and_list& event)
{
  return guarded_wait([&]() { wait(value, unit, event); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(const sc_event_expr<sc_event_or_list>& event)
{
  return guarded_wait([&]() { wait(event.release()); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(const sc_time& time, const sc_event_expr<sc_event_or_list>& event)
{
  return guarded_wait([&]() { wait(time, event.release()); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(double value, sc_time_unit unit, const sc_event_expr<sc_event_or_list>& event)
{
  return guarded_wait([&]() { wait(value, unit, event.release()); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(const sc_event_expr<sc_event_and_list>& event)
{
  return guarded_wait([&]() { wait(event.release()); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(const sc_time& time, const sc_event_expr<sc_event_and_list>& event)
{
  return guarded_wait([&]() { wait(time, event.release()); });
}

/** @brief Guard a native wait overload; targets must outlive suspension and callers release the GIL. */
bool python_wait(double value, sc_time_unit unit, const sc_event_expr<sc_event_and_list>& event)
{
  return guarded_wait([&]() { wait(value, unit, event.release()); });
}

/**
 * @brief Store a report callback with actions passed by value to Python.
 */
inline std::function<void(const sc_report&, unsigned int)> python_report_handler;

/**
 * @brief Adapt native const-reference action bits to a Python integer.
 */
inline void dispatch_report(const sc_report& report, const sc_actions& actions)
{
  python_report_handler(report, actions);
}

/**
 * @brief Install the adapter, or restore the native default handler.
 */
void set_python_report_handler(std::function<void(const sc_report&, unsigned int)> callback)
{
  python_report_handler = std::move(callback);
  sc_report_handler::set_handler(dispatch_report);
}

/**
 * @brief Restore the default handler and release the retained callback.
 */
void restore_report_handler()
{
  sc_report_handler::set_handler(sc_report_handler::default_handler);
  python_report_handler = nullptr;
}

/**
 * @brief Spawn an on-demand Python callback through native SystemC.
 * @param callback Callable retained by the Python wrapper until simulation exit.
 * @param name Native process name.
 * @param options Optional native process configuration.
 */
sc_process_handle spawn_callback(std::function<void()> callback, const char* name,
                                 const sc_spawn_options* options = nullptr)
{
  return sc_spawn([callback = std::move(callback)]() { dispatch_python(callback); }, name, options);
}

/**
 * @brief Trace a signal's native storage rather than a temporary Python value.
 * @tparam T Signal value type, instantiated on demand by cppyy.
 * @param file Open native trace file.
 * @param signal Signal retained by the caller for the simulation lifetime.
 * @details A native method refreshes structural scalar reference storage on
 * value changes; callbacks acquire GIL. Signal must outlive that method.
 * @param name Waveform variable name.
 */
template <class T>
void trace_signal(sc_trace_file* file, const sc_signal_in_if<T>& signal, const std::string& name)
{
  sc_trace(file, signal.read(), name);
  // Structural scalar references are refreshed by read(), unlike sc_signal
  // storage which is updated directly by the primitive channel.
  sc_spawn_options options;
  options.spawn_method();
  options.dont_initialize();
  options.set_sensitivity(&signal.value_changed_event());
  sc_spawn([&signal] { (void)signal.read(); }, sc_gen_unique_name("trace_refresh"), &options);
}

/**
 * @brief SystemC module with callbacks supplied by Python. Python retains bound callbacks on its module proxy. Native
 * processes use this object as their host; callback identity is the native process object. cppyy manages callback entry
 * into Python. Coroutine backend compatibility must be verified separately for suspended Python threads.
 */
class ScModule : public sc_module
{
 public:
  /**
   * @brief Construct within a live sc_module_name elaboration scope.
   */
  ScModule(const sc_module_name& name) : sc_module(name)
  {
  }

  /**
   * @brief Override in Python before binding validation. cppyy acquires the GIL for dispatch.
   */
  virtual void before_end_of_elaboration() override
  {
  }

  /**
   * @brief Override in Python after elaboration completes. cppyy acquires the GIL for dispatch.
   */
  virtual void end_of_elaboration() override
  {
  }

  /**
   * @brief Override in Python before processes start. cppyy acquires the GIL for dispatch.
   */
  virtual void start_of_simulation() override
  {
  }

  /**
   * @brief Override in Python after kernel stop. cppyy acquires the GIL for dispatch.
   */
  virtual void end_of_simulation() override
  {
  }

  /**
   * @brief Register a method; waits are illegal in a method process.
   */
  void sc_method(const char* name, std::function<void()> callback)
  {
    register_process(name, callback, SC_METHOD_PROC_);
  }

  /**
   * @brief Register a thread on the configured native coroutine backend.
   */
  void sc_thread(const char* name, std::function<void()> callback)
  {
    register_process(name, callback, SC_THREAD_PROC_);
  }

  /**
   * @brief Register a clocked thread sensitive to the supplied edge.
   */
  void sc_cthread(const char* name, std::function<void()> callback, sc_event_finder& edge)
  {
    auto handle = register_process(name, callback, SC_CTHREAD_PROC_);
    sc_module::sensitive(handle, edge);
  }

  using sc_module::async_reset_signal_is;
  using sc_module::dont_initialize;
  using sc_module::reset_signal_is;

  /**
   * @brief Add static sensitivity to an interface default event; caller retains the object.
   */
  void sensitive(const sc_interface& value)
  {
    sc_module::sensitive << value;
  }

  /**
   * @brief Add static sensitivity to a bound port; caller retains the object.
   */
  void sensitive(const sc_port_base& value)
  {
    sc_module::sensitive << value;
  }

  /**
   * @brief Add static sensitivity to a port edge finder; caller retains the object.
   */
  void sensitive(sc_event_finder& value)
  {
    sc_module::sensitive << value;
  }

  /**
   * @brief Add static sensitivity to a native event; caller retains the object.
   */
  void sensitive(const sc_event& value)
  {
    sc_module::sensitive << value;
  }

 private:
  /**
   * @brief Callback ownership lasts until this native module is destroyed.
   */
  std::map<sc_object*, std::function<void()>> callbacks;

  /**
   * @brief Dispatch the current process to its retained Python callback.
   */
  void call_process()
  {
    auto process = sc_get_current_process_handle();
    pysystemc::log(false, fmt::format("Dispatch {} at {}", process.name(), sc_time_stamp().to_string()));
    dispatch_python(callbacks.at(process.get_process_object()));
  }

  /**
   * @brief Create a process with this module as host and select its sensitivity.
   * @details Allocate callback storage before touching the kernel. Native creation failures destroy the pending
   * callback; attaching it to the completed process uses a map node and cannot allocate. Python callers retain
   * their callable after successful registration. cppyy supplies Python callback entry and GIL acquisition.
   */
  sc_process_handle register_process(const char* name, std::function<void()> callback, sc_curr_proc_kind kind)
  {
    if (!callback) throw std::invalid_argument("Process callback must be callable");
    std::map<sc_object*, std::function<void()>> pending;
    pending.emplace(nullptr, std::move(callback));
    auto callback_node = pending.extract(pending.begin());
    auto context = sc_get_curr_simcontext();
    sc_process_handle handle;
    auto entry = SC_MAKE_FUNC_PTR(ScModule, call_process);
    switch (kind)
    {
    case SC_METHOD_PROC_:
      handle = context->create_method_process(name, false, entry, this, nullptr);
      break;
    case SC_THREAD_PROC_:
      handle = context->create_thread_process(name, false, entry, this, nullptr);
      break;
    case SC_CTHREAD_PROC_:
      handle = context->create_cthread_process(name, false, entry, this, nullptr);
      break;
    default:
      throw std::invalid_argument("Unknown SystemC process kind");
    }
    callback_node.key() = handle.get_process_object();
    callbacks.insert(std::move(callback_node));
    pysystemc::log(false, fmt::format("Registered process {} (kind {})", handle.name(), static_cast<int>(kind)));
    sc_module::sensitive << handle;
    sc_module::sensitive_pos << handle;
    sc_module::sensitive_neg << handle;
    return handle;
  }
};

}  // namespace sc_core

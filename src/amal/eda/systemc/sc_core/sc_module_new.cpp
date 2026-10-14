/** @file sc_module_new.cpp
 * @brief Native process and tracing adapters for cppyy Python modules.
 */
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc>
#include "binding_log.h"
#include <functional>
#include <map>
#include <string>
#include <stdexcept>

namespace sc_core {
/** @brief Select the zero-argument cycle wait without cppyy overload ambiguity. */
void wait_cycle() { wait(); }

/** @brief Store a report callback with actions passed by value to Python. */
inline std::function<void(const sc_report&, unsigned int)> python_report_handler;
/** @brief Adapt native const-reference action bits to a Python integer. */
inline void dispatch_report(const sc_report& report, const sc_actions& actions) {
    python_report_handler(report, actions);
}
/** @brief Install the adapter, or restore the native default handler. */
void set_python_report_handler(std::function<void(const sc_report&, unsigned int)> callback) {
    python_report_handler = std::move(callback);
    sc_report_handler::set_handler(dispatch_report);
}
void restore_report_handler() {
    sc_report_handler::set_handler(sc_report_handler::default_handler);
    python_report_handler = nullptr;
}

/** @brief Spawn an on-demand Python callback through native SystemC.
 * @param callback Callable retained by the Python wrapper until simulation exit.
 * @param name Native process name.
 * @param options Optional native process configuration.
 */
sc_process_handle spawn_callback(std::function<void()> callback, const char* name,
                                const sc_spawn_options* options = nullptr) {
    return sc_spawn(std::move(callback), name, options);
}

/** @brief Trace a signal's native storage rather than a temporary Python value.
 * @tparam T Signal value type, instantiated on demand by cppyy.
 * @param file Open native trace file.
 * @param signal Signal retained by the caller for the simulation lifetime.
 * @param name Waveform variable name.
 */
template<class T>
void trace_signal(sc_trace_file* file, const sc_signal_in_if<T>& signal,
                  const std::string& name) {
    sc_trace(file, signal.read(), name);
}

/** @brief SystemC module with callbacks supplied by Python.
 * Python retains bound callbacks on its module proxy. Native processes use this
 * object as their host; callback identity is the native process object.
 * cppyy manages callback entry into Python. Coroutine backend compatibility
 * must be verified separately for suspended Python threads.
 */
class ScModule : public sc_module {
public:
    /** @brief Construct within a live sc_module_name elaboration scope. */
    ScModule(const sc_module_name& name) : sc_module(name) {}
    virtual void before_end_of_elaboration() override {}
    virtual void end_of_elaboration() override {}
    virtual void start_of_simulation() override {}
    virtual void end_of_simulation() override {}
    /** @brief Register a method; waits are illegal in a method process. */
    void sc_method(const char* name, std::function<void()> callback) {
        register_process(name, callback, SC_METHOD_PROC_);
    }
    /** @brief Register a thread on the configured native coroutine backend. */
    void sc_thread(const char* name, std::function<void()> callback) {
        register_process(name, callback, SC_THREAD_PROC_);
    }
    /** @brief Register a clocked thread sensitive to the supplied edge. */
    void sc_cthread(const char* name, std::function<void()> callback, sc_event_finder& edge) {
        auto handle = register_process(name, callback, SC_CTHREAD_PROC_);
        sc_module::sensitive(handle, edge);
    }
    using sc_module::async_reset_signal_is;
    using sc_module::reset_signal_is;
    using sc_module::dont_initialize;
    void sensitive(const sc_interface& value) { sc_module::sensitive << value; }
    void sensitive(const sc_port_base& value) { sc_module::sensitive << value; }
    void sensitive(sc_event_finder& value) { sc_module::sensitive << value; }
    void sensitive(const sc_event& value) { sc_module::sensitive << value; }
private:
    std::map<sc_object*, std::function<void()>> callbacks;
    /** @brief Dispatch the current process to its retained Python callback. */
    void call_process() {
        auto process = sc_get_current_process_handle();
        pysystemc::log(false, fmt::format("Dispatch {} at {}", process.name(), sc_time_stamp().to_string()));
        callbacks.at(process.get_process_object())();
    }
    /** @brief Create a process with this module as host and select its sensitivity. */
    sc_process_handle register_process(const char* name, std::function<void()> callback,
                                      sc_curr_proc_kind kind) {
        auto context = sc_get_curr_simcontext();
        sc_process_handle handle;
        auto entry = SC_MAKE_FUNC_PTR(ScModule, call_process);
        switch (kind) {
        case SC_METHOD_PROC_: handle = context->create_method_process(name, false, entry, this, nullptr); break;
        case SC_THREAD_PROC_: handle = context->create_thread_process(name, false, entry, this, nullptr); break;
        case SC_CTHREAD_PROC_: handle = context->create_cthread_process(name, false, entry, this, nullptr); break;
        default: throw std::invalid_argument("Unknown SystemC process kind");
        }
        callbacks.emplace(handle.get_process_object(), std::move(callback));
        pysystemc::log(false, fmt::format("Registered process {} (kind {})", handle.name(), static_cast<int>(kind)));
        sc_module::sensitive << handle;
        sc_module::sensitive_pos << handle;
        sc_module::sensitive_neg << handle;
        return handle;
    }
};
}

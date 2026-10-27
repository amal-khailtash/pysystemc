/** @file sc_core_process_error_matrix.cpp
 * @brief Native report reference for process-handle restriction errors.
 * Each report is converted to SC_THROW so the operation can be checked without
 * stopping the simulation. The model owns all process targets until completion.
 */
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <systemc>

namespace
{
using namespace sc_core;

/** @brief Record one native report's stable type, message and severity. */
void capture(const char* label, const std::function<void()>& operation)
{
  try
  {
    operation();
  }
  catch (const sc_report& report)
  {
    std::cout << "PROCESS_ERROR " << label << " " << report.get_msg_type() << " | " << report.get_msg() << " | "
              << static_cast<int>(report.get_severity()) << '\n';
    return;
  }
  throw std::runtime_error(std::string("missing native report: ") + label);
}

/** @brief Own the method, reset-sensitive method, thread and error controller. */
struct Model : sc_module
{
  sc_event event{"method_event"};
  sc_signal<bool> reset{"reset"};
  sc_process_handle method_handle;
  sc_process_handle reset_method_handle;
  sc_process_handle thread_handle;
  sc_process_handle completed_handle;

  SC_HAS_PROCESS(Model);
  explicit Model(sc_module_name name) : sc_module(name)
  {
    SC_METHOD(method);
    sensitive << event;
    method_handle = sc_get_last_created_process_handle();

    SC_METHOD(reset_method);
    sensitive << event;
    reset_signal_is(reset, true);
    reset_method_handle = sc_get_last_created_process_handle();

    SC_THREAD(thread);
    thread_handle = sc_get_last_created_process_handle();

    SC_THREAD(completed);
    completed_handle = sc_get_last_created_process_handle();

    SC_THREAD(controller);
  }

  /** @brief Remain statically sensitive so handle operations target a live method. */
  void method()
  {
  }
  /** @brief Remain statically sensitive while carrying reset-signal metadata. */
  void reset_method()
  {
  }
  /** @brief Keep a live thread handle throughout the report checks. */
  void thread()
  {
    wait(5, SC_NS);
  }
  /** @brief Finish during initialization to check throw_it on a terminal target. */
  void completed()
  {
  }

  /** @brief Exercise method and empty-handle restrictions while the kernel runs. */
  void controller()
  {
    wait(1, SC_NS);
    sc_process_handle empty;
    std::runtime_error exception("unused native exception");
    capture("empty_throw", [&] { empty.throw_it(exception); });
    capture("method_throw", [&] { method_handle.throw_it(exception); });
    capture("throw_terminated_thread", [&] { completed_handle.throw_it(exception); });
    capture("suspend_reset_method", [&] { reset_method_handle.suspend(); });

    method_handle.disable();
    method_handle.suspend();
    capture("resume_disabled_suspended_method", [&] { method_handle.resume(); });
    method_handle.enable();
    sc_stop();
  }
};
}  // namespace

/** @brief Compare native pre-simulation throw_it and in-simulation restrictions. */
int sc_main(int, char**)
{
  using namespace sc_core;
  Model model("process_error_matrix");
  const sc_actions old_warning = sc_report_handler::set_actions(SC_WARNING, SC_THROW);
  const sc_actions old_error = sc_report_handler::set_actions(SC_ERROR, SC_THROW);
  std::runtime_error exception("unused native exception");
  capture("throw_before_simulation", [&] { model.thread_handle.throw_it(exception); });
  sc_start(SC_ZERO_TIME);
  capture("throw_while_paused", [&] { model.thread_handle.throw_it(exception); });
  sc_start(3, SC_NS);
  capture("throw_after_stop", [&] { model.thread_handle.throw_it(exception); });
  sc_report_handler::set_actions(SC_WARNING, old_warning);
  sc_report_handler::set_actions(SC_ERROR, old_error);
  return 0;
}

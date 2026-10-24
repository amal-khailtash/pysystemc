/** @file sc_core_process_handles.cpp
 * @brief Independent process-handle reference counting and control reference.
 * Handles own references; event/object views borrow their retained process.
 */
#include <stdexcept>
#include <string>
#include <systemc>
using namespace sc_core;
/** Verify kind-specific borrowed process pointer conversions. */
int conversions(sc_process_handle h)
{
  return bool(static_cast<sc_method_handle>(h)) + 2 * bool(static_cast<sc_thread_handle>(h)) +
         4 * bool(static_cast<sc_cthread_handle>(h));
}
/** Own the method, sensitivity event and independent controller thread. */
struct Model : sc_module
{
  SC_HAS_PROCESS(Model);
  sc_event event{"tick"};
  sc_process_handle original;
  int visits = 0;
  sc_clock clock{"clock", sc_time(2, SC_NS)};
  sc_in<bool> clock_port{"clock_port"};
  Model(sc_module_name name) : sc_module(name)
  {
    SC_METHOD(method);
    sensitive << event;
    original = sc_get_last_created_process_handle();
    clock_port(clock);
    SC_CTHREAD(clocked, clock_port.pos());
  }
  /** Verify native clocked-thread conversion and wait semantics. */
  void clocked()
  {
    auto h = sc_get_current_process_handle();
    if (h.proc_kind() != SC_CTHREAD_PROC_ || conversions(h) != 6) throw std::runtime_error("cthread conversion");
    wait();
    std::cout << "HANDLE_REFERENCE clocked " << int(h.proc_kind()) << " " << sc_time_stamp() << "\n";
  }
  /** Verify callback metadata and exact invocation time. */
  void method()
  {
    auto h = sc_get_current_process_handle();
    sc_process_b* raw = h;
    if (raw != sc_get_curr_process_handle() || h.proc_kind() != SC_METHOD_PROC_ || h.dynamic() || h.is_unwinding() ||
        std::string(h.basename()) != "method" || std::string(h.get_parent_object()->name()) != "handles" ||
        std::string(h.get_process_object()->name()) != "handles.method")
      throw std::runtime_error("metadata");
    std::cout << "HANDLE_REFERENCE visit " << ++visits << " " << sc_time_stamp() << "\n";
  }
  /** Exercise disabled versus suspended pending dispatch semantics. */
  void control()
  {
    wait(1, SC_NS);
    auto child = sc_spawn([] {}, "runtime_child");
    sc_event child_event("child_event");
    auto current = sc_get_current_process_handle();
    if (current.get_child_objects().size() != 1 || current.get_child_events().size() != 1)
      throw std::runtime_error("child hierarchy");
    if (!child.dynamic() || std::string(child.get_parent_object()->name()) != "controller")
      throw std::runtime_error("runtime dynamic");
    std::cout << "HANDLE_REFERENCE dynamic " << child.dynamic() << " " << child.get_parent_object()->name() << "\n";
    original.disable();
    event.notify(SC_ZERO_TIME);
    wait(1, SC_NS);
    if (visits != 1) throw std::runtime_error("disabled");
    original.enable();
    event.notify(SC_ZERO_TIME);
    wait(1, SC_NS);
    if (visits != 2) throw std::runtime_error("enabled");
    original.suspend();
    event.notify(SC_ZERO_TIME);
    wait(1, SC_NS);
    if (visits != 2) throw std::runtime_error("suspended");
    original.resume();
    wait(1, SC_NS);
    if (visits != 3) throw std::runtime_error("resumed");
    original.resume();
    wait(1, SC_NS);
    if (visits != 3) throw std::runtime_error("duplicate resume");
    std::cout << "HANDLE_REFERENCE controls " << visits << " " << sc_time_stamp() << "\n";
    sc_process_handle empty;
    auto old = sc_report_handler::set_actions(SC_WARNING, SC_THROW);
    int reports = 0;
    auto verify = [&](auto operation)
    {
      try
      {
        operation();
      }
      catch (const sc_report& e)
      {
        if (std::string(e.get_msg_type()).find("empty process handle") == std::string::npos)
          throw std::runtime_error("unexpected empty warning");
        ++reports;
        return;
      }
      throw std::runtime_error("missing empty warning");
    };
    verify([&] { empty.disable(); });
    verify([&] { empty.enable(); });
    verify([&] { empty.suspend(); });
    verify([&] { empty.resume(); });
    verify([&] { empty.sync_reset_on(); });
    verify([&] { empty.sync_reset_off(); });
    verify([&] { empty.reset(); });
    verify([&] { empty.kill(); });
    verify([&] { empty.reset_event(); });
    verify([&] { empty.terminated_event(); });
    verify([&] { empty.is_unwinding(); });
    sc_report_handler::set_actions(SC_WARNING, old);
    std::cout << "HANDLE_REFERENCE invalid " << reports << "\n";
  }
};
/** Compare owning constructor, copy/assignment/swap and post-termination lifetime. */
int sc_main(int, char**)
{
  sc_process_handle empty;
  sc_semaphore plain("plain", 1);
  if (empty.valid() || empty.dynamic() || empty.terminated() || std::string(empty.name()) != "" ||
      std::string(empty.basename()) != "" || empty.proc_kind() != SC_NO_PROC_ || empty.get_parent_object() ||
      empty.get_process_object() || !empty.get_child_objects().empty() || !empty.get_child_events().empty() ||
      empty.dump_state() != "NO TARGET" || sc_process_handle(&plain).valid())
    return 2;
  std::cout << "HANDLE_REFERENCE empty 0 0 0\n";
  Model model("handles");
  auto original = model.original;
  sc_process_handle copy(original), from_object(original.get_process_object());
  sc_process_b* raw = original;
  sc_process_handle from_raw(raw), assigned;
  if (!copy.valid() || !from_object.valid() || !from_raw.valid() || conversions(original) != 1) return 3;
  assigned = copy;
  assigned = assigned;
  assigned.swap(empty);
  if (!empty.valid() || assigned.valid()) return 4;
  assigned.swap(empty);
  if (!assigned.valid() || empty.valid() || !original.get_child_objects().empty()) return 5;
  if (&original.reset_event() != &original.reset_event() ||
      &original.terminated_event() != &original.terminated_event())
    return 6;
  std::cout << "HANDLE_REFERENCE owned " << original.valid() << " " << original.dynamic() << " " << original.name()
            << " " << int(original.proc_kind()) << "\n";
  auto controller = sc_spawn([&] { model.control(); }, "controller");
  if (controller.dynamic() || controller.proc_kind() != SC_THREAD_PROC_ || conversions(controller) != 2) return 7;
  sc_process_handle retained(controller);
  original.suspend();
  original.resume();
  sc_start(7, SC_NS);
  if (!retained.valid() || !retained.terminated() || original.terminated() || retained.is_unwinding()) return 8;
  original.suspend();
  original.resume();
  controller = sc_process_handle();
  if (!retained.valid() || !retained.terminated()) return 9;
  std::cout << "HANDLE_REFERENCE retained " << retained.valid() << " " << retained.terminated() << " "
            << retained.name() << "\n";
  sc_stop();
  return 0;
}

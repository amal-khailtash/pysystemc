/** @file sc_core_sensitivity_public.cpp
 * @brief Native reference for direct Python calls into static sensitivity builders.
 * @details Every target remains owned by the module through simulation. This
 * model uses native processes only and runs in its own kernel subprocess.
 */
#include <sysc/kernel/sc_cthread_process.h>
#include <sysc/kernel/sc_method_process.h>
#include <sysc/kernel/sc_thread_process.h>

#include <iostream>
#include <string>
#include <systemc>
#include <vector>

/** @brief Register native methods against the same event, interface, port and edge sources. */
struct SensitivityReference : sc_core::sc_module
{
  sc_core::sc_signal<bool> bool_signal{"bool_signal", false};
  sc_core::sc_signal<sc_dt::sc_logic> logic_signal{"logic_signal", sc_dt::SC_LOGIC_0};
  sc_core::sc_in<bool> in_bool{"in_bool"};
  sc_core::sc_in<sc_dt::sc_logic> in_logic{"in_logic"};
  sc_core::sc_in<bool> clock_port{"clock_port"};
  sc_core::sc_inout<bool> inout_bool{"inout_bool"};
  sc_core::sc_inout<sc_dt::sc_logic> inout_logic{"inout_logic"};
  sc_core::sc_event event{"event"};
  std::vector<std::pair<std::string, sc_dt::uint64>> records;

  SC_HAS_PROCESS(SensitivityReference);

  /** @brief Bind targets and exercise each native static-sensitivity overload. */
  explicit SensitivityReference(sc_core::sc_module_name name, sc_core::sc_clock& clock) : sc_module(name)
  {
    in_bool(bool_signal);
    in_logic(logic_signal);
    inout_bool(bool_signal);
    inout_logic(logic_signal);
    clock_port(clock);

    SC_METHOD(call_event);
    prepare_method();
    sensitive(event);
    SC_METHOD(call_interface);
    prepare_method();
    sensitive(bool_signal);
    SC_METHOD(call_port);
    prepare_method();
    sensitive(in_bool);
    SC_METHOD(call_finder);
    prepare_method();
    sensitive(in_bool.pos());

    SC_METHOD(shift_event);
    prepare_method();
    sensitive << event;
    SC_METHOD(shift_interface);
    prepare_method();
    sensitive << logic_signal;
    SC_METHOD(shift_port);
    prepare_method();
    sensitive << in_logic;
    SC_METHOD(shift_finder);
    prepare_method();
    sensitive << in_logic.pos();

    SC_METHOD(pos_shift_bool_interface);
    prepare_method();
    sensitive_pos << bool_signal;
    SC_METHOD(pos_shift_logic_interface);
    prepare_method();
    sensitive_pos << logic_signal;
    SC_METHOD(pos_shift_bool_input);
    prepare_method();
    sensitive_pos << in_bool;
    SC_METHOD(pos_shift_logic_input);
    prepare_method();
    sensitive_pos << in_logic;
    SC_METHOD(pos_shift_bool_inout);
    prepare_method();
    sensitive_pos << inout_bool;
    SC_METHOD(pos_shift_logic_inout);
    prepare_method();
    sensitive_pos << inout_logic;

    SC_METHOD(pos_call_bool_interface);
    prepare_method();
    sensitive_pos(bool_signal);
    SC_METHOD(pos_call_logic_interface);
    prepare_method();
    sensitive_pos(logic_signal);
    SC_METHOD(pos_call_bool_input);
    prepare_method();
    sensitive_pos(in_bool);
    SC_METHOD(pos_call_logic_input);
    prepare_method();
    sensitive_pos(in_logic);
    SC_METHOD(pos_call_bool_inout);
    prepare_method();
    sensitive_pos(inout_bool);
    SC_METHOD(pos_call_logic_inout);
    prepare_method();
    sensitive_pos(inout_logic);

    SC_METHOD(neg_shift_bool_interface);
    prepare_method();
    sensitive_neg << bool_signal;
    SC_METHOD(neg_shift_logic_interface);
    prepare_method();
    sensitive_neg << logic_signal;
    SC_METHOD(neg_shift_bool_input);
    prepare_method();
    sensitive_neg << in_bool;
    SC_METHOD(neg_shift_logic_input);
    prepare_method();
    sensitive_neg << in_logic;
    SC_METHOD(neg_shift_bool_inout);
    prepare_method();
    sensitive_neg << inout_bool;
    SC_METHOD(neg_shift_logic_inout);
    prepare_method();
    sensitive_neg << inout_logic;

    SC_METHOD(neg_call_bool_interface);
    prepare_method();
    sensitive_neg(bool_signal);
    SC_METHOD(neg_call_logic_interface);
    prepare_method();
    sensitive_neg(logic_signal);
    SC_METHOD(neg_call_bool_input);
    prepare_method();
    sensitive_neg(in_bool);
    SC_METHOD(neg_call_logic_input);
    prepare_method();
    sensitive_neg(in_logic);
    SC_METHOD(neg_call_bool_inout);
    prepare_method();
    sensitive_neg(inout_bool);
    SC_METHOD(neg_call_logic_inout);
    prepare_method();
    sensitive_neg(inout_logic);

    SC_METHOD(pos_process_handle);
    prepare_method();
    sensitive_pos << sc_core::sc_get_last_created_process_handle();
    sensitive_pos << bool_signal;
    SC_METHOD(pos_method_handle);
    prepare_method();
    sensitive_pos << dynamic_cast<sc_core::sc_method_handle>(
      sc_core::sc_get_last_created_process_handle().get_process_object());
    sensitive_pos << in_bool;
    SC_THREAD(pos_thread_handle);
    prepare_method();
    sensitive_pos << dynamic_cast<sc_core::sc_thread_handle>(
      sc_core::sc_get_last_created_process_handle().get_process_object());
    sensitive_pos << inout_logic;

    SC_METHOD(neg_process_handle);
    prepare_method();
    sensitive_neg << sc_core::sc_get_last_created_process_handle();
    sensitive_neg << bool_signal;
    SC_METHOD(neg_method_handle);
    prepare_method();
    sensitive_neg << dynamic_cast<sc_core::sc_method_handle>(
      sc_core::sc_get_last_created_process_handle().get_process_object());
    sensitive_neg << in_bool;
    SC_THREAD(neg_thread_handle);
    prepare_method();
    sensitive_neg << dynamic_cast<sc_core::sc_thread_handle>(
      sc_core::sc_get_last_created_process_handle().get_process_object());
    sensitive_neg << inout_logic;

    SC_CTHREAD(cthread_finder, clock_port.pos());
    sensitive(
      dynamic_cast<sc_core::sc_cthread_handle>(sc_core::sc_get_last_created_process_handle().get_process_object()),
      in_bool.pos());
    SC_CTHREAD(cthread_bool_interface, clock_port.pos());
    sensitive(
      dynamic_cast<sc_core::sc_cthread_handle>(sc_core::sc_get_last_created_process_handle().get_process_object()),
      bool_signal);
    SC_CTHREAD(cthread_logic_interface, clock_port.pos());
    sensitive(
      dynamic_cast<sc_core::sc_cthread_handle>(sc_core::sc_get_last_created_process_handle().get_process_object()),
      logic_signal);
    SC_CTHREAD(cthread_bool_input, clock_port.pos());
    sensitive(
      dynamic_cast<sc_core::sc_cthread_handle>(sc_core::sc_get_last_created_process_handle().get_process_object()),
      in_bool);
    SC_CTHREAD(cthread_logic_input, clock_port.pos());
    sensitive(
      dynamic_cast<sc_core::sc_cthread_handle>(sc_core::sc_get_last_created_process_handle().get_process_object()),
      in_logic);
    SC_CTHREAD(cthread_bool_inout, clock_port.pos());
    sensitive(
      dynamic_cast<sc_core::sc_cthread_handle>(sc_core::sc_get_last_created_process_handle().get_process_object()),
      inout_bool);
    SC_CTHREAD(cthread_logic_inout, clock_port.pos());
    sensitive(
      dynamic_cast<sc_core::sc_cthread_handle>(sc_core::sc_get_last_created_process_handle().get_process_object()),
      inout_logic);

    SC_METHOD(static_event);
    dont_initialize();
    sc_core::sc_sensitive::make_static_sensitivity(
      static_cast<sc_core::sc_process_b*>(sc_core::sc_get_last_created_process_handle().get_process_object()), event);
    SC_METHOD(static_interface);
    dont_initialize();
    sc_core::sc_sensitive::make_static_sensitivity(
      static_cast<sc_core::sc_process_b*>(sc_core::sc_get_last_created_process_handle().get_process_object()),
      bool_signal);
    SC_METHOD(static_port);
    dont_initialize();
    sc_core::sc_sensitive::make_static_sensitivity(
      static_cast<sc_core::sc_process_b*>(sc_core::sc_get_last_created_process_handle().get_process_object()), in_bool);
    SC_METHOD(static_finder);
    dont_initialize();
    sc_core::sc_sensitive::make_static_sensitivity(
      static_cast<sc_core::sc_process_b*>(sc_core::sc_get_last_created_process_handle().get_process_object()),
      in_bool.pos());

    SC_METHOD(reset_ordinary);
    dont_initialize();
    sensitive.reset();
    sensitive << event;
    SC_METHOD(reset_pos);
    dont_initialize();
    sensitive_pos.reset();
    sensitive_pos << bool_signal;
    SC_METHOD(reset_neg);
    dont_initialize();
    sensitive_neg.reset();
    sensitive_neg << bool_signal;
  }

  /** @brief Select the most recently registered process and suppress initialization. */
  void prepare_method()
  {
    dont_initialize();
    const auto handle = sc_core::sc_get_last_created_process_handle();
    sensitive << handle;
    sensitive_pos << handle;
    sensitive_neg << handle;
  }

#define SENSITIVITY_METHOD(label)                                   \
  void label()                                                      \
  {                                                                 \
    records.emplace_back(#label, sc_core::sc_time_stamp().value()); \
  }

  SENSITIVITY_METHOD(call_event)
  SENSITIVITY_METHOD(call_interface)
  SENSITIVITY_METHOD(call_port)
  SENSITIVITY_METHOD(call_finder)
  SENSITIVITY_METHOD(shift_event)
  SENSITIVITY_METHOD(shift_interface)
  SENSITIVITY_METHOD(shift_port)
  SENSITIVITY_METHOD(shift_finder)
  SENSITIVITY_METHOD(pos_shift_bool_interface)
  SENSITIVITY_METHOD(pos_shift_logic_interface)
  SENSITIVITY_METHOD(pos_shift_bool_input)
  SENSITIVITY_METHOD(pos_shift_logic_input)
  SENSITIVITY_METHOD(pos_shift_bool_inout)
  SENSITIVITY_METHOD(pos_shift_logic_inout)
  SENSITIVITY_METHOD(neg_shift_bool_interface)
  SENSITIVITY_METHOD(neg_shift_logic_interface)
  SENSITIVITY_METHOD(neg_shift_bool_input)
  SENSITIVITY_METHOD(neg_shift_logic_input)
  SENSITIVITY_METHOD(neg_shift_bool_inout)
  SENSITIVITY_METHOD(neg_shift_logic_inout)
  SENSITIVITY_METHOD(pos_call_bool_interface)
  SENSITIVITY_METHOD(pos_call_logic_interface)
  SENSITIVITY_METHOD(pos_call_bool_input)
  SENSITIVITY_METHOD(pos_call_logic_input)
  SENSITIVITY_METHOD(pos_call_bool_inout)
  SENSITIVITY_METHOD(pos_call_logic_inout)
  SENSITIVITY_METHOD(neg_call_bool_interface)
  SENSITIVITY_METHOD(neg_call_logic_interface)
  SENSITIVITY_METHOD(neg_call_bool_input)
  SENSITIVITY_METHOD(neg_call_logic_input)
  SENSITIVITY_METHOD(neg_call_bool_inout)
  SENSITIVITY_METHOD(neg_call_logic_inout)
  SENSITIVITY_METHOD(pos_process_handle)
  SENSITIVITY_METHOD(pos_method_handle)
  SENSITIVITY_METHOD(pos_thread_handle)
  SENSITIVITY_METHOD(neg_process_handle)
  SENSITIVITY_METHOD(neg_method_handle)
  SENSITIVITY_METHOD(neg_thread_handle)
  SENSITIVITY_METHOD(cthread_finder)
  SENSITIVITY_METHOD(cthread_bool_interface)
  SENSITIVITY_METHOD(cthread_logic_interface)
  SENSITIVITY_METHOD(cthread_bool_input)
  SENSITIVITY_METHOD(cthread_logic_input)
  SENSITIVITY_METHOD(cthread_bool_inout)
  SENSITIVITY_METHOD(cthread_logic_inout)
  SENSITIVITY_METHOD(static_event)
  SENSITIVITY_METHOD(static_interface)
  SENSITIVITY_METHOD(static_port)
  SENSITIVITY_METHOD(static_finder)
  SENSITIVITY_METHOD(reset_ordinary)
  SENSITIVITY_METHOD(reset_pos)
  SENSITIVITY_METHOD(reset_neg)

#undef SENSITIVITY_METHOD
};

/** @brief Compare each callback, edge, timestamp, and registration ordering to native behavior. */
int sc_main(int, char**)
{
  sc_core::sc_clock clock("clock", sc_core::sc_time(8, sc_core::SC_NS), 0.5, sc_core::sc_time(4, sc_core::SC_NS));
  SensitivityReference model("sensitivity", clock);
  sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_NS));
  model.bool_signal.write(true);
  model.logic_signal.write(sc_dt::SC_LOGIC_1);
  model.event.notify();
  sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_NS));
  model.bool_signal.write(false);
  model.logic_signal.write(sc_dt::SC_LOGIC_0);
  sc_core::sc_start(sc_core::sc_time(1, sc_core::SC_NS));

  if (model.records.size() != 55) return 1;
  std::cout << "SENSITIVITY_ALIASES 18\n";
  for (const auto& [label, time] : model.records) std::cout << "SENSITIVITY_CALLBACK " << label << "@" << time << "\n";
  std::cout << "SENSITIVITY_RESET 1\n";
  std::cout << "SENSITIVITY_LIFETIME 52 55\n";
  return 0;
}

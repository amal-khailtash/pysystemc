/** @file sc_core_edge_waits_public.cpp
 * @brief Native references for timeout queries and sampled-edge waits.
 * @details The model owns its signals, events and process handles throughout
 * simulation. No Python callbacks or cross-thread calls occur in this model.
 */
#include <iostream>
#include <string>
#include <systemc>
#include <vector>

/** @brief Exercise public timeout and edge-wait helpers in native processes. */
struct EdgeWaits : sc_core::sc_module
{
  sc_core::sc_in<bool> clock_port;
  sc_core::sc_signal<bool> edge_bool_pos{"edge_bool_pos", false};
  sc_core::sc_signal<bool> edge_bool_neg{"edge_bool_neg", false};
  sc_core::sc_signal<sc_dt::sc_logic> edge_logic_pos{"edge_logic_pos", sc_dt::SC_LOGIC_0};
  sc_core::sc_signal<sc_dt::sc_logic> edge_logic_neg{"edge_logic_neg", sc_dt::SC_LOGIC_0};
  sc_core::sc_signal<bool> reset_signal{"reset_signal", false};
  sc_core::sc_signal<bool> kill_signal{"kill_signal", false};
  sc_core::sc_event early_event{"early_event"};
  sc_core::sc_event late_event{"late_event"};
  sc_core::sc_process_handle reset_handle;
  sc_core::sc_process_handle kill_handle;
  std::vector<std::string> edge_records;
  std::vector<sc_dt::uint64> reset_starts;
  std::vector<sc_dt::uint64> reset_cleanups;
  std::vector<sc_dt::uint64> kill_starts;
  std::vector<sc_dt::uint64> kill_cleanups;
  std::vector<bool> reset_unwinds;
  std::vector<bool> kill_unwinds;
  bool early_timed_out = true;
  bool early_explicit_timed_out = true;
  bool late_timed_out = false;
  bool late_explicit_timed_out = false;
  sc_dt::uint64 early_time = 0;
  sc_dt::uint64 late_time = 0;

  SC_HAS_PROCESS(EdgeWaits);

  /** @brief Bind clock and register each native reference process.
   * @param name Native module name, borrowed for registration.
   * @param clock Clock borrowed for the module lifetime.
   */
  EdgeWaits(sc_core::sc_module_name name, sc_core::sc_clock& clock) : sc_module(name), clock_port("clock_port")
  {
    clock_port(clock);
    SC_CTHREAD(positive_bool, clock_port.pos());
    SC_CTHREAD(negative_bool, clock_port.pos());
    SC_CTHREAD(positive_logic, clock_port.pos());
    SC_CTHREAD(negative_logic, clock_port.pos());
    SC_CTHREAD(reset_edge_wait, clock_port.pos());
    reset_handle = sc_core::sc_get_last_created_process_handle();
    SC_CTHREAD(kill_edge_wait, clock_port.pos());
    kill_handle = sc_core::sc_get_last_created_process_handle();
    SC_THREAD(early_timeout);
    SC_THREAD(late_timeout);
    SC_THREAD(drive);
    SC_THREAD(control_unwinds);
  }

  /** @brief Record one edge helper's completion label and exact time. */
  void record_edge(const char* label)
  {
    edge_records.emplace_back(std::string(label) + "@" + std::to_string(sc_core::sc_time_stamp().value()));
  }

  /** @brief Verify bool positive-edge helper and record its completion. */
  void positive_bool()
  {
    sc_core::at_posedge(edge_bool_pos);
    record_edge("pos_bool");
  }

  /** @brief Verify bool negative-edge helper and record its completion. */
  void negative_bool()
  {
    sc_core::at_negedge(edge_bool_neg);
    record_edge("neg_bool");
  }

  /** @brief Verify sc_logic positive-edge helper and record completion. */
  void positive_logic()
  {
    sc_core::at_posedge(edge_logic_pos);
    record_edge("pos_logic");
  }

  /** @brief Verify sc_logic negative-edge helper and record completion. */
  void negative_logic()
  {
    sc_core::at_negedge(edge_logic_neg);
    record_edge("neg_logic");
  }

  /** @brief Confirm event completion makes timed_out false. */
  void early_timeout()
  {
    sc_core::wait(5, sc_core::SC_NS, early_event);
    early_timed_out = sc_core::timed_out();
    early_explicit_timed_out = sc_core::timed_out(sc_core::sc_get_curr_simcontext());
    early_time = sc_core::sc_time_stamp().value();
  }

  /** @brief Confirm timeout completion makes timed_out true. */
  void late_timeout()
  {
    sc_core::wait(3, sc_core::SC_NS, late_event);
    late_timed_out = sc_core::timed_out();
    late_explicit_timed_out = sc_core::timed_out(sc_core::sc_get_curr_simcontext());
    late_time = sc_core::sc_time_stamp().value();
  }

  /** @brief Capture reset unwind cleanup while preserving native propagation. */
  void reset_edge_wait()
  {
    reset_starts.push_back(sc_core::sc_time_stamp().value());
    try
    {
      sc_core::at_posedge(reset_signal);
    }
    catch (const sc_core::sc_unwind_exception& unwind)
    {
      reset_unwinds.push_back(unwind.is_reset());
      reset_cleanups.push_back(sc_core::sc_time_stamp().value());
      throw;
    }
  }

  /** @brief Capture kill unwind cleanup while preserving native propagation. */
  void kill_edge_wait()
  {
    kill_starts.push_back(sc_core::sc_time_stamp().value());
    try
    {
      sc_core::at_posedge(kill_signal);
    }
    catch (const sc_core::sc_unwind_exception& unwind)
    {
      kill_unwinds.push_back(unwind.is_reset());
      kill_cleanups.push_back(sc_core::sc_time_stamp().value());
      throw;
    }
  }

  /** @brief Drive event and signal changes independently of observers. */
  void drive()
  {
    using namespace sc_core;
    wait(2, SC_NS);
    early_event.notify();
    edge_bool_pos.write(true);
    edge_bool_neg.write(true);
    edge_logic_pos.write(sc_dt::SC_LOGIC_1);
    edge_logic_neg.write(sc_dt::SC_LOGIC_1);
    wait(2, SC_NS);
    edge_bool_pos.write(false);
    edge_bool_neg.write(false);
    edge_logic_pos.write(sc_dt::SC_LOGIC_0);
    edge_logic_neg.write(sc_dt::SC_LOGIC_0);
  }

  /** @brief Reset and kill suspended edge waiters, then stop simulation. */
  void control_unwinds()
  {
    using namespace sc_core;
    wait(500, SC_PS);
    reset_handle.reset();
    wait(500, SC_PS);
    reset_handle.kill();
    kill_handle.kill();
    wait(5, SC_NS);
    sc_stop();
  }
};

/** @brief Run the native comparison and verify every result and lifecycle step. */
int sc_main(int, char**)
{
  sc_core::sc_clock clock("clock", sc_core::sc_time(2, sc_core::SC_NS));
  EdgeWaits model("edge_waits", clock);
  sc_core::sc_start();

  const std::vector<std::string> expected_edges{"pos_logic@2000", "pos_bool@2000", "neg_bool@4000", "neg_logic@4000"};
  if (model.edge_records != expected_edges || model.early_timed_out || model.early_explicit_timed_out ||
      !model.late_timed_out || !model.late_explicit_timed_out || model.early_time != 2000 || model.late_time != 3000 ||
      model.reset_starts != std::vector<sc_dt::uint64>{0, 500} ||
      model.reset_cleanups != std::vector<sc_dt::uint64>{500, 1000} ||
      model.reset_unwinds != std::vector<bool>{true, false} || model.kill_starts != std::vector<sc_dt::uint64>{0} ||
      model.kill_cleanups != std::vector<sc_dt::uint64>{1000} || model.kill_unwinds != std::vector<bool>{false})
    return 1;

  std::cout << "EDGE_WAIT_REFERENCE";
  for (const auto& record : model.edge_records) std::cout << " " << record;
  std::cout << "\n";
  std::cout << "TIMED_OUT_REFERENCE early=" << model.early_timed_out << "/" << model.early_explicit_timed_out << "@"
            << model.early_time << " late=" << model.late_timed_out << "/" << model.late_explicit_timed_out << "@"
            << model.late_time << "\n";
  std::cout << "EDGE_CLEANUP_REFERENCE reset=500,1000 kill=1000\n";
  return 0;
}

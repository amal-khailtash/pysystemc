/** @file sc_core_scheduler_queries.cpp
 * @brief Native scheduler queries; no callbacks, borrowed context remains live.
 */
#include <systemc>
/** @brief Emit scheduler state without advancing the singleton kernel. */
void snapshot(const char* label)
{
  using namespace sc_core;
  auto* context = sc_get_curr_simcontext();
  if (sc_pending_activity_at_current_time() != sc_pending_activity_at_current_time(context) ||
      sc_pending_activity_at_future_time() != sc_pending_activity_at_future_time(context) ||
      sc_pending_activity() != sc_pending_activity(context) ||
      sc_time_to_pending_activity() != sc_time_to_pending_activity(context) ||
      sc_is_running() != sc_is_running(context))
    throw std::runtime_error("context mismatch");
  std::cout << "SCHEDULER_REFERENCE " << label << " " << int(sc_get_status()) << " " << sc_delta_count() << " "
            << sc_delta_count_at_current_time() << " " << sc_start_of_simulation_invoked() << " "
            << sc_end_of_simulation_invoked() << " " << sc_pending_activity_at_current_time() << " "
            << sc_pending_activity_at_future_time() << " " << sc_pending_activity() << " "
            << sc_time_to_pending_activity().value() << " " << sc_is_running() << " " << sc_time_stamp().value()
            << "\n";
}
/** @brief Observe pending notification at construction, pause and stop boundaries. */
int sc_main(int, char**)
{
  using namespace sc_core;
  sc_event event("future");
  snapshot("construction");
  sc_start(SC_ZERO_TIME);
  snapshot("initialized");
  event.notify(5, SC_NS);
  snapshot("scheduled");
  sc_start(2, SC_NS, SC_RUN_TO_TIME);
  snapshot("two_ns");
  sc_start(3, SC_NS, SC_RUN_TO_TIME);
  snapshot("five_ns");
  sc_start(SC_ZERO_TIME);
  snapshot("drained");
  sc_stop();
  snapshot("stopped");
  return 0;
}

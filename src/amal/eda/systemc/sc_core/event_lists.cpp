#include <systemc>

namespace sc_core
{
/** @brief Notify in the next delta; native reports propagate with the GIL held.
 * The event is borrowed and must survive pending notifications on the kernel thread.
 */
void python_event_notify_delayed(sc_event& event)
{
  event.notify_delayed();
}
/** @brief Schedule a borrowed event after a native delay, preserving report exceptions.
 * Call with the GIL held on the kernel thread; event storage must outlive delivery.
 */
void python_event_notify_delayed_time(sc_event& event, const sc_time& time)
{
  event.notify_delayed(time);
}
/** @brief Schedule a borrowed event using native time/unit semantics and reports.
 * Call with the GIL held on the kernel thread; event storage must outlive delivery.
 */
void python_event_notify_delayed_units(sc_event& event, double value, sc_time_unit unit)
{
  event.notify_delayed(value, unit);
}
/**
 * @brief Consume an event expression into an existing, inactive native list.
 *
 * The target owns its list storage but borrows events, which must outlive all
 * sensitivity registrations. Assignment consumes and deletes the expression's
 * automatic list through native SystemC semantics. The expression must be used
 * exactly once. Native report exceptions propagate; call with the GIL held on
 * the kernel thread, and never modify a list while a process is waiting on it.
 */
template <typename List>
void python_assign_event_expression(List& target, const sc_event_expr<List>& expression)
{
  target = static_cast<const List&>(expression);
}
}  // namespace sc_core

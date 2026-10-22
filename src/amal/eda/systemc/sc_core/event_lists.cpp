#include <systemc>

namespace sc_core
{
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

/** @file event_expressions.cpp
 * @brief Select native event-expression append operators explicitly.
 */
#include <systemc>
#include <type_traits>
namespace pysystemc_public
{
/** @brief Consume a left expression with native disjunction/conjunction semantics.
 * @tparam List Native OR or AND list type, never a precompiled value registry.
 * @details The native expression copy transfers its automatic list; the old
 * expression cannot be reused. Referenced events remain caller-owned and must
 * outlive the expression/list and every waiting process. GIL stays held and no
 * Python callback is stored. Native allocation/conversion errors propagate.
 */
template <class List>
struct event_expression_operations
{
  /** @brief Append a borrowed event using the exact native free operator. */
  static sc_core::sc_event_expr<List> append(const sc_core::sc_event_expr<List>& left, const sc_core::sc_event& right)
  {
    if constexpr (std::is_same_v<List, sc_core::sc_event_or_list>)
      return sc_core::operator|(left, right);
    else
      return sc_core::operator&(left, right);
  }
  /** @brief Append a borrowed list using the exact native free operator. */
  static sc_core::sc_event_expr<List> append(const sc_core::sc_event_expr<List>& left, const List& right)
  {
    if constexpr (std::is_same_v<List, sc_core::sc_event_or_list>)
      return sc_core::operator|(left, right);
    else
      return sc_core::operator&(left, right);
  }
};
}  // namespace pysystemc_public

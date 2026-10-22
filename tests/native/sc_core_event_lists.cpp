/** @file sc_core_event_lists.cpp
 * @brief Native reference for public SystemC event-list operations.
 * @details Events outlive every list and expression copy. Event lists retain
 * borrowed event pointers, so this scope order avoids dangling references.
 */
#include <iostream>
#include <systemc>
#include <vector>

/** @brief Exercise OR-list construction, union, copy, swap and assignment.
 * @param a,b,c Events retained until after all lists have been destroyed.
 * @param counts Destination for the Python-comparable final sizes.
 * @return Zero when every native list operation has the expected result.
 */
int exercise_or(const sc_core::sc_event& a, const sc_core::sc_event& b, const sc_core::sc_event& c,
                std::vector<int>& counts)
{
  using namespace sc_core;
  sc_event_or_list empty;
  sc_event_or_list first(a);
  sc_event_or_list second(b);
  if (!empty.empty() || empty.size() != 0 || first.empty() || first.size() != 1) return 1;

  first |= a;
  first |= second;
  first |= second;  // Existing event from the right-hand list must be suppressed.
  if (first.size() != 2) return 2;

  auto expression = first | c;
  expression.push_back(c);
  expression.push_back(second);
  sc_event_or_expr moved(expression);
  sc_event_or_list expanded(moved);
  auto release_expression = first | second;
  const sc_event_or_list& released = release_expression.release();
  if (released.size() != 2) return 8;
  sc_event_or_list repeated(released);
  if (expanded.size() != 3 || repeated.size() != 2) return 3;

  sc_event_or_list copied(first);  // Uses the implicit derived copy constructor.
  if (copied.size() != 2) return 4;

  empty.swap(expanded);
  if (empty.size() != 3 || !expanded.empty()) return 5;

  sc_event_list& copied_base = copied;
  const sc_event_list& second_base = second;
  copied_base = second_base;
  if (copied.size() != 1) return 6;
  copied_base = copied_base;
  if (copied.size() != 1) return 7;

  counts.insert(counts.end(), {first.size(), empty.size(), expanded.size(), repeated.size(), copied.size()});
  return 0;
}

/** @brief Exercise AND-list construction, union, copy, swap and assignment.
 * @param a,b,c Events retained until after all lists have been destroyed.
 * @param counts Destination for the Python-comparable final sizes.
 * @return Zero when every native list operation has the expected result.
 */
int exercise_and(const sc_core::sc_event& a, const sc_core::sc_event& b, const sc_core::sc_event& c,
                 std::vector<int>& counts)
{
  using namespace sc_core;
  sc_event_and_list empty;
  sc_event_and_list first(a);
  sc_event_and_list second(b);
  if (!empty.empty() || empty.size() != 0 || first.empty() || first.size() != 1) return 11;

  first &= a;
  first &= second;
  first &= second;  // Existing event from the right-hand list must be suppressed.
  if (first.size() != 2) return 12;

  auto expression = first & c;
  expression.push_back(c);
  expression.push_back(second);
  sc_event_and_expr moved(expression);
  sc_event_and_list expanded(moved);
  auto release_expression = first & second;
  const sc_event_and_list& released = release_expression.release();
  if (released.size() != 2) return 18;
  sc_event_and_list repeated(released);
  if (expanded.size() != 3 || repeated.size() != 2) return 13;

  sc_event_and_list copied(first);  // Uses the implicit derived copy constructor.
  if (copied.size() != 2) return 14;

  empty.swap(expanded);
  if (empty.size() != 3 || !expanded.empty()) return 15;

  sc_event_list& copied_base = copied;
  const sc_event_list& second_base = second;
  copied_base = second_base;
  if (copied.size() != 1) return 16;
  copied_base = copied_base;
  if (copied.size() != 1) return 17;

  counts.insert(counts.end(), {first.size(), empty.size(), expanded.size(), repeated.size(), copied.size()});
  return 0;
}

/** @brief Run both native list families with persistent events. */
int sc_main(int, char**)
{
  sc_core::sc_event a("a");
  sc_core::sc_event b("b");
  sc_core::sc_event c("c");
  std::vector<int> counts;
  int status = exercise_or(a, b, c, counts);
  if (status != 0) return status;
  status = exercise_and(a, b, c, counts);
  if (status != 0) return status;

  std::cout << "EVENT_LIST_REFERENCE";
  for (int count : counts) std::cout << " " << count;
  std::cout << "\n";
  return 0;
}

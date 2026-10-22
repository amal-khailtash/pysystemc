/** @file sc_core_event_list_busy.cpp
 * @brief Native reference for event-list mutation while processes are waiting.
 * @details Persistent lists and their borrowed events remain alive until both
 * waiters resume, then each list family is mutated again to verify recovery.
 */
#include <string>
#include <systemc>
#include <vector>

namespace
{

using namespace sc_core;

/** @brief Exercise four forbidden mutations while a list has active waiters.
 * @param busy The list retained by a process suspended in wait().
 * @param idle A separate one-event list used as the mutation source/target.
 * @return Number of expected native errors, or -1 if behavior differed.
 * @details Both lists are owned by the caller and outlive this routine. The
 * default SC_ERROR action throws sc_report; this function catches each report
 * locally and does not change report policy or retain report references.
 */
template <typename List>
int expect_busy_errors(List& busy, List& idle)
{
  sc_event_list& busy_base = busy;
  const sc_event_list& idle_base = idle;
  int errors = 0;

  const auto expect_error = [&errors](auto&& operation)
  {
    try
    {
      operation();
    }
    catch (const sc_report& report)
    {
      if (std::string(report.get_msg_type()) != SC_ID_EVENT_LIST_FAILED_ ||
          std::string(report.get_msg()) != "list modified while being waited on")
        return false;
      ++errors;
      return true;
    }
    return false;
  };

  if (!expect_error([&]() { busy_base = idle_base; }) || busy.size() != 2 || idle.size() != 1) return -1;
  if (!expect_error([&]() { busy.swap(idle); }) || busy.size() != 2 || idle.size() != 1) return -1;
  if (!expect_error([&]() { idle.swap(busy); }) || busy.size() != 2 || idle.size() != 1) return -1;
  if (!expect_error([&]() { busy.swap(busy); }) || busy.size() != 2 || idle.size() != 1) return -1;
  return errors;
}

/** @brief Confirm later assignment and swap work after all waiters resume.
 * @param former The list that was busy during the earlier mutation attempts.
 * @param event The event borrowed by the recovery source list.
 * @return True when both recovery operations have their expected list sizes.
 * @details All temporary lists are local to the driver after the waiters have
 * resumed, so none can be destroyed while a process still borrows the list.
 */
template <typename List>
bool recover_list(List& former, const sc_event& event)
{
  List idle(event);
  sc_event_list& former_base = former;
  const sc_event_list& idle_base = idle;
  former_base = idle_base;
  if (former.size() != 1 || idle.size() != 1) return false;

  List empty;
  former.swap(empty);
  return former.empty() && empty.size() == 1;
}

/** @brief Native module with persistent OR/AND wait lists and a driver. */
class BusyLists : public sc_module
{
 public:
  SC_HAS_PROCESS(BusyLists);

  /** @brief Construct events/lists before registering their waiting threads.
   * @param name Native module name.
   * @details Member declaration order keeps events alive longer than the lists
   * that borrow them; the module owns all callbacks for the simulation lifetime.
   */
  explicit BusyLists(sc_module_name name) : sc_module(name), a("a"), b("b"), c("c"), or_list(a), and_list(a)
  {
    or_list |= b;
    and_list &= b;
    SC_THREAD(wait_or);
    SC_THREAD(wait_and);
    SC_THREAD(drive);
  }

  sc_event a;
  sc_event b;
  sc_event c;
  sc_event_or_list or_list;
  sc_event_and_list and_list;
  std::vector<int> errors;
  std::vector<std::string> times;
  bool valid = true;

 private:
  /** @brief Wait on the persistent OR list and record its wake timestamp. */
  void wait_or()
  {
    sc_core::wait(or_list);
    times.push_back(sc_time_stamp().to_string());
  }

  /** @brief Wait until both events in the persistent AND list have fired. */
  void wait_and()
  {
    sc_core::wait(and_list);
    times.push_back(sc_time_stamp().to_string());
  }

  /** @brief Check busy errors, notify events in order, then test recovery. */
  void drive()
  {
    sc_core::wait(1, SC_NS);
    sc_event_or_list or_idle(c);
    sc_event_and_list and_idle(c);
    errors.push_back(expect_busy_errors(or_list, or_idle));
    errors.push_back(expect_busy_errors(and_list, and_idle));
    if (errors != std::vector<int>{4, 4}) valid = false;

    sc_core::wait(1, SC_NS);
    a.notify();
    sc_core::wait(1, SC_NS);
    b.notify();
    sc_core::wait(1, SC_NS);

    if (!recover_list(or_list, c) || !recover_list(and_list, c)) valid = false;
    sc_stop();
  }
};

}  // namespace

/** @brief Run the native event-list busy/recovery reference simulation. */
int sc_main(int, char**)
{
  BusyLists model("busy");
  sc_start();

  if (!model.valid || model.errors != std::vector<int>{4, 4} || model.times != std::vector<std::string>{"2 ns", "3 ns"})
    return 1;

  std::cout << "EVENT_BUSY_REFERENCE";
  for (const int count : model.errors) std::cout << " " << count;
  for (const std::string& time : model.times) std::cout << " " << time;
  std::cout << "\n";
  return 0;
}

/** @file event_registration_rollback_probe.h
 * @brief Test-only access adapters for simulated event-list registration failure.
 * @details The probe registers the actual process handle on a real expression
 * list and, when requested, its native timeout event before throwing
 * std::bad_alloc. It invokes the first-party expression wrappers so their
 * owner, rollback, and deletion-counter paths remain under test. The helpers
 * run on the SystemC simulation thread and retain no Python references.
 */
#ifndef PYSYSTEMC_EVENT_REGISTRATION_ROLLBACK_PROBE_H
#define PYSYSTEMC_EVENT_REGISTRATION_ROLLBACK_PROBE_H

#include <sysc/kernel/sc_method_process.h>
#include <sysc/kernel/sc_process.h>
#include <sysc/kernel/sc_thread_process.h>

#include <atomic>
#include <new>
#include <stdexcept>
#include <systemc>

namespace sc_core::event_rollback_probe
{

/** @brief Form legal protected-member pointers for live native objects.
 * @details Accessors apply pointers to actual SystemC base objects; they add no
 * state, layout assumptions, or ownership. Calls require the simulation thread
 * and do not acquire the GIL or invoke Python callbacks.
 */
struct NativeAccess : sc_process_b, sc_event_list
{
  /** @brief Borrow a process's native timeout event.
   * @param process Live process whose timeout event is retained by the kernel.
   * @return Borrowed timeout event; valid only while @p process remains alive.
   */
  static sc_event& timeout_event(sc_process_b& process)
  {
    sc_event* const timeout = process.*&NativeAccess::m_timeout_event_p;
    if (!timeout) throw std::logic_error("process has no timeout event");
    return *timeout;
  }

  /** @brief Register a method handle on a real event list.
   * @param list Borrowed target list, retained by the expression wrapper.
   * @param process Live method handle registered on the simulation thread.
   */
  static void add(const sc_event_list& list, sc_method_handle process)
  {
    using Member = void (sc_event_list::*)(sc_method_handle) const;
    const Member member = &NativeAccess::add_dynamic;
    (list.*member)(process);
  }

  /** @brief Register a thread or clocked-thread handle on a real event list.
   * @param list Borrowed target list, retained by the expression wrapper.
   * @param process Live thread handle registered on the simulation thread.
   */
  static void add(const sc_event_list& list, sc_thread_handle process)
  {
    using Member = void (sc_event_list::*)(sc_thread_handle) const;
    const Member member = &NativeAccess::add_dynamic;
    (list.*member)(process);
  }

  /** @brief Remove one selected registration through a one-event list.
   * @param list Ordinary list containing only the event to remove.
   * @param process Live process whose selected dynamic registration is removed.
   * @details The native list's busy count and other target-list members are
   * untouched. This creates a deterministic partially registered target state.
   */
  static void remove_one(const sc_event_list& list, sc_method_handle process)
  {
    using Member = void (sc_event_list::*)(sc_method_handle, const sc_event*) const;
    const Member member = &NativeAccess::remove_dynamic;
    (list.*member)(process, nullptr);
  }

  /** @copydoc remove_one(const sc_event_list&, sc_method_handle) */
  static void remove_one(const sc_event_list& list, sc_thread_handle process)
  {
    using Member = void (sc_event_list::*)(sc_thread_handle, const sc_event*) const;
    const Member member = &NativeAccess::remove_dynamic;
    (list.*member)(process, nullptr);
  }

  /** @brief Balance one registration on a non-auto-delete staging list.
   * @param list Borrowed ordinary storage; this call retains no pointer.
   * @details Does not remove registrations from the event members.
   */
  static void finish(const sc_event_list& list)
  {
    const auto member = &NativeAccess::auto_delete;
    (list.*member)();
  }
};

/** @brief Access a method process's protected trigger reset.
 * @details The borrowed process is live on the simulation thread. The operation
 * retains no callback or Python reference and preserves native clear order.
 */
struct MethodAccess
{
  /** @brief Clear an earlier dynamic trigger before replacement registration. */
  static void clear(sc_method_handle process)
  {
    if (process != sc_get_current_process_b()) throw std::logic_error("method is not the current process");
    sc_core::next_trigger();
  }
};

/** @brief Count test-only timeout schedules staged before simulated failure. */
inline std::atomic<unsigned long> staged_timeout_notifications{0};
/** @brief Count deliberate failures reached after all selected registrations. */
inline std::atomic<unsigned long> injected_bad_allocations{0};

/** @brief Return a process's borrowed timeout event for a static observer.
 * @param handle Retained live process handle owned by the model.
 * @return Reference to the process timeout event; do not retain beyond the
 * process lifetime.
 * @throws std::logic_error If the handle has no live process object.
 * @details Call during elaboration to add an observer sensitivity. This is a
 * read-only test accessor and does not change process state.
 */
inline sc_event& timeout_event(const sc_process_handle& handle)
{
  auto* const process = dynamic_cast<sc_process_b*>(handle.get_process_object());
  if (!process) throw std::logic_error("process handle has no live process object");
  return NativeAccess::timeout_event(*process);
}

/** @brief Return the number of timeout notifications staged by this probe.
 * @return Relaxed snapshot of test-only notification setup count.
 */
inline unsigned long staged_timeout_count()
{
  return staged_timeout_notifications.load(std::memory_order_relaxed);
}

/** @brief Return the number of deliberate std::bad_alloc throws reached.
 * @return Relaxed snapshot incremented immediately before the test's throw.
 */
inline unsigned long injected_bad_alloc_count()
{
  return injected_bad_allocations.load(std::memory_order_relaxed);
}

/** @brief Stage a real timeout event registration before throwing.
 * @param process Current native process; event remains kernel-owned.
 * @param delay Positive native delay used for the pending timeout notification.
 * @details A one-event ordinary list invokes SystemC's protected list
 * @c add_dynamic overload, which registers the same process handle on the
 * timeout event. The list's busy increment is balanced immediately through
 * @c auto_delete; its dynamic event registration remains until rollback clears
 * it. SystemC's public delayed notify is canceled by that rollback. No event
 * internals or allocator behavior are replaced.
 */
inline void stage_timeout(sc_process_b& process, sc_method_handle handle, const sc_time& delay)
{
  sc_event& timeout = NativeAccess::timeout_event(process);
  timeout.notify(delay);
  sc_event_or_list registration(timeout);
  NativeAccess::add(registration, handle);
  NativeAccess::finish(registration);
  staged_timeout_notifications.fetch_add(1, std::memory_order_relaxed);
}

/** @copydoc stage_timeout(sc_process_b&, sc_method_handle, const sc_time&) */
inline void stage_timeout(sc_process_b& process, sc_thread_handle handle, const sc_time& delay)
{
  sc_event& timeout = NativeAccess::timeout_event(process);
  timeout.notify(delay);
  sc_event_or_list registration(timeout);
  NativeAccess::add(registration, handle);
  NativeAccess::finish(registration);
  staged_timeout_notifications.fetch_add(1, std::memory_order_relaxed);
}

/** @brief Register a live process on an event-list expression, then fail.
 * @tparam List Native OR/AND list type carried by the expression.
 * @param expression Expression consumed by the actual Python bridge wrapper.
 * @param timed Whether to stage the process timeout before list registration.
 * @param partial Whether to remove one member registration before failure.
 * @param removed_event Event member removed to create the deterministic partial state.
 * @param delay Positive timeout when @p timed is true; ignored otherwise.
 * @details This is a deterministic synthetic std::bad_alloc after native
 * registrations succeed and before the wrapper publishes process list state.
 * In partial mode it removes one member using an ordinary one-event list.
 * SystemC registers target members in reverse storage order, so removing the
 * second registration preserves the first while leaving the target list busy.
 * It does not claim to trigger allocator failure. Events, process, and callback
 * owner must remain alive for the simulation lifetime. Called on the simulation
 * thread; the bridge wrapper controls its normal GIL behavior.
 * @throws std::bad_alloc Deliberate synthetic failure, rethrown unchanged by
 * the actual expression wrapper.
 */
template <class List>
void wait_bad_alloc(const sc_event_expr<List>& expression, bool timed, bool partial, const sc_event& removed_event,
                    const sc_time& delay)
{
  sc_process_b* const process = sc_get_current_process_b();
  if (!process) throw std::logic_error("wait probe requires a live process");
  const sc_curr_proc_kind kind = process->proc_kind();
  if (kind != SC_THREAD_PROC_ && kind != SC_CTHREAD_PROC_)
    throw std::logic_error("wait probe requires SC_THREAD or SC_CTHREAD");

  (void)python_wait_expression(expression,
                               [&](const List& list)
                               {
                                 if (timed) stage_timeout(*process, static_cast<sc_thread_handle>(process), delay);
                                 NativeAccess::add(list, static_cast<sc_thread_handle>(process));
                                 if (partial)
                                 {
                                   sc_event_or_list selected(removed_event);
                                   NativeAccess::remove_one(selected, static_cast<sc_thread_handle>(process));
                                 }
                                 injected_bad_allocations.fetch_add(1, std::memory_order_relaxed);
                                 throw std::bad_alloc();
                               });
}

/** @brief Replace a method trigger with a staged list and then fail.
 * @tparam List Native OR/AND list type carried by the expression.
 * @param expression Expression consumed by the actual Python bridge wrapper.
 * @param timed Whether to stage the process timeout before list registration.
 * @param partial Whether to remove one member registration before failure.
 * @param removed_event Event member removed to create the deterministic partial state.
 * @param delay Positive timeout when @p timed is true; ignored otherwise.
 * @throws std::bad_alloc Deliberately thrown after registrations and before
 * process list publication; the actual wrapper must rethrow it unchanged.
 * @details Clears the previous method trigger first, matching native
 * next_trigger order. This synthetic post-registration failure is not an
 * allocator-failure test. Events and method owner must outlive simulation.
 */
template <class List>
void next_trigger_bad_alloc(const sc_event_expr<List>& expression, bool timed, bool partial,
                            const sc_event& removed_event, const sc_time& delay)
{
  sc_process_b* const process = sc_get_current_process_b();
  if (!process || process->proc_kind() != SC_METHOD_PROC_)
    throw std::logic_error("next_trigger probe requires a live SC_METHOD");
  const auto method = static_cast<sc_method_handle>(process);

  (void)python_next_trigger_expression(expression,
                                       [&](const List& list)
                                       {
                                         MethodAccess::clear(method);
                                         if (timed) stage_timeout(*process, method, delay);
                                         NativeAccess::add(list, method);
                                         if (partial)
                                         {
                                           sc_event_or_list selected(removed_event);
                                           NativeAccess::remove_one(selected, method);
                                         }
                                         injected_bad_allocations.fetch_add(1, std::memory_order_relaxed);
                                         throw std::bad_alloc();
                                       });
}

/** @brief Invoke the actual wrapper for an OR wait expression.
 * @param expression Native expression consumed by the actual wrapper.
 * @param timed Whether to stage a timeout before list registration.
 * @param partial Whether to remove one member registration before failure.
 * @param removed_event Event member removed to create the deterministic partial state.
 * @param delay Positive delay when timed; otherwise ignored.
 * @throws std::bad_alloc After registration, rethrown unchanged by the wrapper.
 * @details Simulation-thread helper; no Python reference is retained.
 */
inline void wait_or_bad_alloc(const sc_event_expr<sc_event_or_list>& expression, bool timed, bool partial,
                              const sc_event& removed_event, const sc_time& delay)
{
  wait_bad_alloc(expression, timed, partial, removed_event, delay);
}

/** @brief Invoke the actual wrapper for an AND wait expression.
 * @param expression Native expression consumed by the actual wrapper.
 * @param timed Whether to stage a timeout before list registration.
 * @param partial Whether to remove one member registration before failure.
 * @param removed_event Event member removed to create the deterministic partial state.
 * @param delay Positive delay when timed; otherwise ignored.
 * @throws std::bad_alloc After registration, rethrown unchanged by the wrapper.
 * @details Simulation-thread helper; no Python reference is retained.
 */
inline void wait_and_bad_alloc(const sc_event_expr<sc_event_and_list>& expression, bool timed, bool partial,
                               const sc_event& removed_event, const sc_time& delay)
{
  wait_bad_alloc(expression, timed, partial, removed_event, delay);
}

/** @brief Invoke the actual wrapper for an OR next_trigger expression.
 * @param expression Native expression consumed by the actual wrapper.
 * @param timed Whether to stage a timeout before list registration.
 * @param partial Whether to remove one member registration before failure.
 * @param removed_event Event member removed to create the deterministic partial state.
 * @param delay Positive delay when timed; otherwise ignored.
 * @throws std::bad_alloc After registration, rethrown unchanged by the wrapper.
 * @details Simulation-thread helper; no Python reference is retained.
 */
inline void next_or_bad_alloc(const sc_event_expr<sc_event_or_list>& expression, bool timed, bool partial,
                              const sc_event& removed_event, const sc_time& delay)
{
  next_trigger_bad_alloc(expression, timed, partial, removed_event, delay);
}

/** @brief Invoke the actual wrapper for an AND next_trigger expression.
 * @param expression Native expression consumed by the actual wrapper.
 * @param timed Whether to stage a timeout before list registration.
 * @param partial Whether to remove one member registration before failure.
 * @param removed_event Event member removed to create the deterministic partial state.
 * @param delay Positive delay when timed; otherwise ignored.
 * @throws std::bad_alloc After registration, rethrown unchanged by the wrapper.
 * @details Simulation-thread helper; no Python reference is retained.
 */
inline void next_and_bad_alloc(const sc_event_expr<sc_event_and_list>& expression, bool timed, bool partial,
                               const sc_event& removed_event, const sc_time& delay)
{
  next_trigger_bad_alloc(expression, timed, partial, removed_event, delay);
}

}  // namespace sc_core::event_rollback_probe

#endif  // PYSYSTEMC_EVENT_REGISTRATION_ROLLBACK_PROBE_H

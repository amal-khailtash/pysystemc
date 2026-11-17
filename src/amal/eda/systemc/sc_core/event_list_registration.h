/** @file Transactional cleanup for unpublished dynamic event-list registration. */
#pragma once

#include <sysc/kernel/sc_method_process.h>
#include <sysc/kernel/sc_thread_process.h>

#include <systemc>

namespace pysystemc_detail
{
/** @brief Form legal protected-member pointers on actual native base objects.
 * @details Adds no state or layout assumptions. Calls borrow live native lists
 * on the simulation thread, retain no Python objects, and require no GIL.
 */
struct EventListAccess : sc_core::sc_event_list
{
  /** @brief Read whether any native registration retains the borrowed list. */
  static bool is_busy(const sc_core::sc_event_list& list)
  {
    return (list.*&EventListAccess::busy)();
  }

  /** @brief Remove one method's registrations without altering the busy count. */
  static void remove(const sc_core::sc_event_list& list, sc_core::sc_method_handle process)
  {
    using Member = void (sc_core::sc_event_list::*)(sc_core::sc_method_handle, const sc_core::sc_event*) const;
    const Member member = &EventListAccess::remove_dynamic;
    (list.*member)(process, nullptr);
  }

  /** @brief Remove one thread's registrations without altering the busy count. */
  static void remove(const sc_core::sc_event_list& list, sc_core::sc_thread_handle process)
  {
    using Member = void (sc_core::sc_event_list::*)(sc_core::sc_thread_handle, const sc_core::sc_event*) const;
    const Member member = &EventListAccess::remove_dynamic;
    (list.*member)(process, nullptr);
  }

  /** @brief Balance one registration; an idle temporary list may delete itself. */
  static void finish(const sc_core::sc_event_list& list)
  {
    (list.*&EventListAccess::auto_delete)();
  }
};

/** @brief Inspect and clear native process registrations through protected members.
 * @details Member pointers apply to the actual process base, without downcasting
 * it to this adapter. Simulation-thread calls retain no references or callbacks.
 */
struct ProcessAccess : sc_core::sc_process_b
{
  /** @brief Return the borrowed published dynamic event-list pointer. */
  static const sc_core::sc_event_list* list(sc_core::sc_process_b& process)
  {
    return process.*&ProcessAccess::m_event_list_p;
  }

  /** @brief Return the borrowed published dynamic single-event pointer. */
  static const sc_core::sc_event* event(sc_core::sc_process_b& process)
  {
    return process.*&ProcessAccess::m_event_p;
  }

  /** @brief Return whether native reset or kill unwind is already active. */
  static bool unwinding(sc_core::sc_process_b& process)
  {
    return process.*&ProcessAccess::m_unwinding;
  }

  /** @brief Cancel unpublished timeout registration and notification.
   * @details Call only with no published event/list pointer and after verifying
   * the captured process identity. Native cleanup does not allocate or invoke
   * Python. Existing accepted sensitivity must not be cleared by this operation.
   */
  static void clear(sc_core::sc_process_b& process)
  {
    (process.*&ProcessAccess::remove_dynamic_events)(false);
  }
};

/** @brief Snapshot ownership before a native event-list scheduling operation.
 * @details The caller exclusively owns an initially idle list. On failure,
 * rollback is allowed only for the same process, before the list is published,
 * and outside an existing unwind. Successful registration and suppressed unwind
 * reports keep native scheduling semantics. No callback or Python object is
 * retained. Real timed-priority-queue allocation corruption is outside this
 * transaction and remains a native kernel limitation.
 */
class EventListRegistration
{
 public:
  /** @brief Capture borrowed list/process state before the native call.
   * @param list Live event list owned by the calling bridge frame.
   * @param method True for next_trigger, false for blocking wait.
   * @details Runs on the simulation thread, with or without the GIL; no allocation.
   */
  EventListRegistration(const sc_core::sc_event_list& list, bool method)
    : process_(sc_core::sc_get_current_process_b()), method_(method), idle_(!EventListAccess::is_busy(list))
  {
    if (process_)
    {
      kind_ = process_->proc_kind();
      previous_list_ = ProcessAccess::list(*process_);
      previous_event_ = ProcessAccess::event(*process_);
      unwinding_ = ProcessAccess::unwinding(*process_);
    }
  }

  /** @brief Roll back only unpublished registrations made by the failed call.
   * @tparam Owner Unique owner whose deleter safely destroys an idle list.
   * @param owner Live list owner; emptied before native temporary self-deletion.
   * @return True if cleanup was eligible, false if native ownership is uncertain.
   * @details Called while preserving an active native exception. No allocation,
   * GIL acquisition, or Python callback occurs. Existing/committed registrations
   * are never removed. For next_trigger, native auto_delete balances busy state
   * and deletes the released expression; blocking waits keep ordinary storage.
   */
  template <class Owner>
  bool rollback(Owner& owner) const
  {
    auto& list = *owner;
    if (!process_ || !idle_ || unwinding_ || previous_list_ == &list ||
        sc_core::sc_get_current_process_b() != process_ || process_->proc_kind() != kind_)
      return false;
    if (method_)
    {
      if (kind_ != sc_core::SC_METHOD_PROC_ || list.size() == 0) return false;
    }
    else if ((kind_ != sc_core::SC_THREAD_PROC_ && kind_ != sc_core::SC_CTHREAD_PROC_) || previous_list_ ||
             previous_event_)
      return false;
    if (ProcessAccess::list(*process_) || ProcessAccess::event(*process_)) return false;

    // Native publication has not happened. This can only remove the staged
    // timeout; old method sensitivity was cleared before list registration.
    ProcessAccess::clear(*process_);
    if (EventListAccess::is_busy(list))
    {
      if (method_)
        EventListAccess::remove(list, static_cast<sc_core::sc_method_handle>(process_));
      else
        EventListAccess::remove(list, static_cast<sc_core::sc_thread_handle>(process_));
      if (method_) owner.release();
      EventListAccess::finish(list);
    }
    return true;
  }

 private:
  sc_core::sc_process_b* process_ = nullptr;
  sc_core::sc_curr_proc_kind kind_ = sc_core::SC_NO_PROC_;
  const sc_core::sc_event_list* previous_list_ = nullptr;
  const sc_core::sc_event* previous_event_ = nullptr;
  bool method_;
  bool idle_;
  bool unwinding_ = false;
};
}  // namespace pysystemc_detail

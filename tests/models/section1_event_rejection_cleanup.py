"""Measure first-party native event-list destruction on rejected scheduler calls."""

from collections import Counter
import traceback

from amal.eda.systemc import sc_core as sc
import cppyy


WAIT_ERROR = 519
TRIGGER_ERROR = 520
EMPTY_LIST_ERROR = 565
WAIT_MESSAGE = "in SC_METHODs use next_trigger() instead"
TRIGGER_MESSAGE = "in SC_THREADs and SC_CTHREADs use wait() instead"
EMPTY_WAIT_MESSAGE = "wait() on empty event list not allowed"
EMPTY_TRIGGER_MESSAGE = "next_trigger() on empty event list not allowed"
report_calls = []
recovery = []
callback_failures = []
method_invocations = []
reset_wait_starts = []
reset_wait_unwinds = []
kill_wait_unwinds = []


def capture_report(report, actions):
    """Record native report metadata and delegate the configured action unchanged."""
    report_calls.append(
        (
            int(report.get_id()),
            report.get_msg_type(),
            " ".join(report.get_msg().split()),
            int(actions),
            report.get_process_name(),
        )
    )
    sc.sc_report_handler.default_handler(report, actions)


def assert_throw(label, operation, report_id, message, process_name=""):
    """Assert one original native report and unchanged SC_THROW action."""
    before = len(report_calls)
    try:
        operation()
    except sc.sc_report as error:
        assert len(report_calls) == before + 1, (label, report_calls[before:])
        record = report_calls[-1]
        assert error.get_id() == report_id, (label, error.get_id())
        assert error.get_severity() == sc.SC_ERROR, (label, error.get_severity())
        assert error.get_msg_type() == record[1] == report_type[report_id], (label, error.get_msg_type(), record)
        assert record[0] == report_id and record[2] == message, (label, record)
        assert record[3] == sc.SC_THROW, (label, record)
        assert record[4] == process_name, (label, record[4], process_name)
        assert " ".join(error.get_msg().split()) == message, (label, error.get_msg())
        assert error.__cause__ is None, (label, error.__cause__)
        assert error.get_time().value() == sc.sc_time_stamp().value()
    else:
        raise AssertionError(f"{label}: SC_THROW report returned normally")


def assert_suppressed(label, operation, report_id, message, process_name=""):
    """Assert one original native report observed with the SC_DO_NOTHING action."""
    before = len(report_calls)
    operation()
    assert len(report_calls) == before + 1, (label, report_calls[before:])
    record = report_calls[-1]
    assert record == (report_id, report_type[report_id], message, sc.SC_DO_NOTHING, process_name), (label, record)


def assert_report_sequence(label, operation, expected, thrown_report=None):
    """Assert ordered native reports and either a selected throw or normal return."""
    before = len(report_calls)
    try:
        operation()
    except sc.sc_report as error:
        assert thrown_report is not None, (label, error.get_id())
        assert error.get_id() == thrown_report[0], (label, error.get_id())
        assert error.get_severity() == sc.SC_ERROR
        assert error.get_msg_type() == report_type[thrown_report[0]]
        assert " ".join(error.get_msg().split()) == thrown_report[1]
        assert error.__cause__ is None
    else:
        assert thrown_report is None, label
    actual = report_calls[before:]
    assert actual == [
        (report_id, report_type[report_id], message, actions, "")
        for report_id, message, actions in expected
    ], (label, actual, expected)


report_type = {
    WAIT_ERROR: sc.SC_ID_WAIT_NOT_ALLOWED_,
    TRIGGER_ERROR: sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_,
    EMPTY_LIST_ERROR: sc.SC_ID_EVENT_LIST_FAILED_,
}


def configure(report_id, actions):
    """Set one error action while returning its previous policy."""
    return sc.sc_report_handler.set_actions(report_type[report_id], sc.SC_ERROR, actions)


previous_wait = configure(WAIT_ERROR, sc.SC_THROW)
previous_trigger = configure(TRIGGER_ERROR, sc.SC_THROW)
previous_empty = configure(EMPTY_LIST_ERROR, sc.SC_THROW)
sc.set_report_handler(capture_report)
native_bridge = cppyy.gbl.sc_core
wait_deletions_before = int(native_bridge.python_wait_expression_list_deletion_count())
trigger_deletions_before = int(native_bridge.python_next_trigger_expression_list_deletion_count())

outside_first = sc.sc_event("outside_first")
outside_second = sc.sc_event("outside_second")
empty_or_first = native_bridge.sc_event_or_list()
empty_or_second = native_bridge.sc_event_or_list()
empty_and_first = native_bridge.sc_event_and_list()
empty_and_second = native_bridge.sc_event_and_list()


def outside_wait_forms():
    """Yield free OR/AND expression wait forms, including explicit context."""
    return (
        ("outside_wait_or", lambda: sc.wait(outside_first | outside_second)),
        (
            "outside_wait_timed_and_explicit",
            lambda: sc.wait(1, sc.SC_NS, outside_first & outside_second, sc.sc_get_curr_simcontext()),
        ),
    )


def outside_empty_wait_forms():
    """Yield empty OR/AND expressions built from public native list operators."""
    return (
        ("outside_wait_empty_or", lambda: sc.wait(empty_or_first | empty_or_second)),
        ("outside_wait_empty_and", lambda: sc.wait(empty_and_first & empty_and_second)),
    )


def outside_trigger_forms():
    """Yield free OR/AND expression trigger forms and explicit context."""
    return (
        ("outside_trigger_or", lambda: sc.next_trigger(outside_first | outside_second)),
        (
            "outside_trigger_timed_and_explicit",
            lambda: sc.next_trigger(
                sc.sc_time(1, sc.SC_NS), outside_first & outside_second, sc.sc_get_curr_simcontext()
            ),
        ),
    )


def outside_empty_trigger_forms():
    """Yield empty OR/AND method-sensitivity expressions outside a process."""
    return (
        ("outside_trigger_empty_or", lambda: sc.next_trigger(empty_or_first | empty_or_second)),
        ("outside_trigger_empty_and", lambda: sc.next_trigger(empty_and_first & empty_and_second)),
    )


empty_wait_forms = outside_empty_wait_forms()
empty_trigger_forms = outside_empty_trigger_forms()

# Throw on the empty-list report first; native context validation must not run.
for label, operation in empty_wait_forms:
    assert_throw(label + "_empty_first", operation, EMPTY_LIST_ERROR, EMPTY_WAIT_MESSAGE)
for label, operation in empty_trigger_forms:
    assert_throw(label + "_empty_first", operation, EMPTY_LIST_ERROR, EMPTY_TRIGGER_MESSAGE)

# Suppress empty-list validation, then let the process-context report throw.
configure(EMPTY_LIST_ERROR, sc.SC_DO_NOTHING)
for label, operation in empty_wait_forms:
    assert_report_sequence(
        label + "_empty_suppressed_context_throw",
        operation,
        [
            (EMPTY_LIST_ERROR, EMPTY_WAIT_MESSAGE, sc.SC_DO_NOTHING),
            (WAIT_ERROR, WAIT_MESSAGE, sc.SC_THROW),
        ],
        (WAIT_ERROR, WAIT_MESSAGE),
    )
for label, operation in empty_trigger_forms:
    assert_report_sequence(
        label + "_empty_suppressed_context_throw",
        operation,
        [
            (EMPTY_LIST_ERROR, EMPTY_TRIGGER_MESSAGE, sc.SC_DO_NOTHING),
            (TRIGGER_ERROR, TRIGGER_MESSAGE, sc.SC_THROW),
        ],
        (TRIGGER_ERROR, TRIGGER_MESSAGE),
    )

# Suppress both native reports and verify each call returns with the same order.
configure(WAIT_ERROR, sc.SC_DO_NOTHING)
for label, operation in empty_wait_forms:
    assert_report_sequence(
        label + "_both_suppressed",
        operation,
        [
            (EMPTY_LIST_ERROR, EMPTY_WAIT_MESSAGE, sc.SC_DO_NOTHING),
            (WAIT_ERROR, WAIT_MESSAGE, sc.SC_DO_NOTHING),
        ],
    )
configure(TRIGGER_ERROR, sc.SC_DO_NOTHING)
for label, operation in empty_trigger_forms:
    assert_report_sequence(
        label + "_both_suppressed",
        operation,
        [
            (EMPTY_LIST_ERROR, EMPTY_TRIGGER_MESSAGE, sc.SC_DO_NOTHING),
            (TRIGGER_ERROR, TRIGGER_MESSAGE, sc.SC_DO_NOTHING),
        ],
    )

configure(WAIT_ERROR, sc.SC_THROW)
configure(TRIGGER_ERROR, sc.SC_THROW)
for label, operation in outside_wait_forms():
    assert_throw(label, operation, WAIT_ERROR, WAIT_MESSAGE)
for label, operation in outside_trigger_forms():
    assert_throw(label, operation, TRIGGER_ERROR, TRIGGER_MESSAGE)

configure(WAIT_ERROR, sc.SC_DO_NOTHING)
for label, operation in outside_wait_forms():
    assert_suppressed(label + "_suppressed", operation, WAIT_ERROR, WAIT_MESSAGE)
configure(TRIGGER_ERROR, sc.SC_DO_NOTHING)
for label, operation in outside_trigger_forms():
    assert_suppressed(label + "_suppressed", operation, TRIGGER_ERROR, TRIGGER_MESSAGE)


class RejectionModel(sc.sc_module):
    """Retain native owners while method/thread contexts reject expressions."""

    def __init__(self):
        """Create event sources, rejected callbacks, and a valid recovery driver."""
        super().__init__(sc.sc_module_name("section1_event_rejection"))
        self.channel = sc.sc_prim_channel("channel")
        self.first = sc.sc_event("first")
        self.second = sc.sc_event("second")
        self.kick = sc.sc_event("kick")
        self.reset_first = sc.sc_event("reset_first")
        self.reset_second = sc.sc_event("reset_second")
        self.kill_first = sc.sc_event("kill_first")
        self.kill_second = sc.sc_event("kill_second")
        self.sc_method("method_recovery", self.method_recovery)
        self.sensitive(self.kick)
        self.sc_thread("thread_recovery", self.thread_recovery)
        self.sc_thread("reset_waiter", self.reset_waiter)
        self.reset_handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("kill_waiter", self.kill_waiter)
        self.kill_handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("driver", self.driver)

    def method_recovery(self):
        """Catch thrown and suppressed method-wait reports before valid trigger."""
        method_invocations.append(int(sc.sc_time_stamp().value()))
        if len(method_invocations) > 1:
            recovery.append("method")
            return
        try:
            self._reject_method_waits()
        except BaseException:
            callback_failures.append(traceback.format_exc())

    def _reject_method_waits(self):
        """Reject method waits under both throw and suppression actions."""
        configure(WAIT_ERROR, sc.SC_THROW)
        assert_throw(
            "method_wait_free_or",
            lambda: sc.wait(self.first | self.second),
            WAIT_ERROR,
            WAIT_MESSAGE,
            "method_recovery",
        )
        assert_throw(
            "method_wait_member_timed_and",
            lambda: self.channel.wait(2, sc.SC_NS, self.first & self.second),
            WAIT_ERROR,
            WAIT_MESSAGE,
            "method_recovery",
        )
        configure(WAIT_ERROR, sc.SC_DO_NOTHING)
        assert_suppressed(
            "method_wait_free_or_suppressed",
            lambda: sc.wait(self.first | self.second),
            WAIT_ERROR,
            WAIT_MESSAGE,
            "method_recovery",
        )
        assert_suppressed(
            "method_wait_member_timed_and_suppressed",
            lambda: self.channel.wait(2, sc.SC_NS, self.first & self.second),
            WAIT_ERROR,
            WAIT_MESSAGE,
            "method_recovery",
        )
        configure(TRIGGER_ERROR, sc.SC_DO_NOTHING)
        self.next_trigger(self.first | self.second)

    def thread_recovery(self):
        """Catch thread next-trigger reports then resume on a valid event wait."""
        try:
            self._reject_thread_triggers()
        except BaseException:
            callback_failures.append(traceback.format_exc())

    def _reject_thread_triggers(self):
        """Reject thread triggers under both throw and suppression actions."""
        configure(TRIGGER_ERROR, sc.SC_THROW)
        assert_throw(
            "thread_trigger_free_or",
            lambda: sc.next_trigger(self.first | self.second),
            TRIGGER_ERROR,
            TRIGGER_MESSAGE,
            "thread_recovery",
        )
        assert_throw(
            "thread_trigger_member_timed_and",
            lambda: self.channel.next_trigger(2, sc.SC_NS, self.first & self.second),
            TRIGGER_ERROR,
            TRIGGER_MESSAGE,
            "thread_recovery",
        )
        configure(TRIGGER_ERROR, sc.SC_DO_NOTHING)
        assert_suppressed(
            "thread_trigger_free_or_suppressed",
            lambda: sc.next_trigger(self.first | self.second, sc.sc_get_curr_simcontext()),
            TRIGGER_ERROR,
            TRIGGER_MESSAGE,
            "thread_recovery",
        )
        assert_suppressed(
            "thread_trigger_member_timed_and_suppressed",
            lambda: self.channel.next_trigger(2, sc.SC_NS, self.first & self.second),
            TRIGGER_ERROR,
            TRIGGER_MESSAGE,
            "thread_recovery",
        )
        sc.wait(self.first | self.second)
        recovery.append("thread")

    def reset_waiter(self):
        """Hold an accepted expression wait across reset and return after restart."""
        reset_wait_starts.append(int(sc.sc_time_stamp().value()))
        try:
            sc.wait(self.reset_first | self.reset_second)
        except BaseException:
            reset_wait_unwinds.append(int(sc.sc_time_stamp().value()))

    def kill_waiter(self):
        """Hold an accepted expression wait across kill and record scheduler unwind."""
        try:
            sc.wait(self.kill_first | self.kill_second)
        except BaseException:
            kill_wait_unwinds.append(int(sc.sc_time_stamp().value()))

    def driver(self):
        """Reset and kill accepted waiters, wake recovery callbacks and stop cleanly."""
        self.kick.notify(sc.SC_ZERO_TIME)
        sc.wait(1, sc.SC_NS)
        self.reset_handle.reset()
        self.kill_handle.kill()
        self._yield_until(lambda: len(reset_wait_starts) == 2 and self.kill_handle.terminated())
        self.first.notify()
        sc.wait(1, sc.SC_NS)
        sc.sc_stop()

    @staticmethod
    def _yield_until(predicate):
        """Advance delta cycles until reset restart and kill termination complete."""
        for _ in range(16):
            if predicate():
                return
            sc.wait(sc.SC_ZERO_TIME)
        raise AssertionError("expression-wait reset/kill lifecycle did not finish within 16 deltas")


model = RejectionModel()
sc.sc_start()
assert not callback_failures, "\n".join(callback_failures)
assert recovery.count("method") == 1 and recovery.count("thread") == 1, (recovery, method_invocations)
assert method_invocations == [0, 1000], method_invocations
assert sc.sc_time_stamp() == sc.sc_time(2, sc.SC_NS)
assert reset_wait_starts == [0, 1000], reset_wait_starts
assert reset_wait_unwinds == [1000], reset_wait_unwinds
assert kill_wait_unwinds == [1000], kill_wait_unwinds
assert model.reset_handle.terminated() is False and model.kill_handle.terminated()
assert sc.sc_report_handler.get_count(report_type[WAIT_ERROR]) == 12
assert sc.sc_report_handler.get_count(report_type[TRIGGER_ERROR]) == 12
assert sc.sc_report_handler.get_count(report_type[EMPTY_LIST_ERROR]) == 12
assert len(report_calls) == 37, (len(report_calls), report_calls, recovery, method_invocations)
stop_actions = int(sc.SC_DISPLAY | sc.SC_LOG)
assert Counter((row[0], row[3]) for row in report_calls) == Counter(
    {
        (int(WAIT_ERROR), int(sc.SC_THROW)): 6,
        (int(WAIT_ERROR), int(sc.SC_DO_NOTHING)): 6,
        (int(TRIGGER_ERROR), int(sc.SC_THROW)): 6,
        (int(TRIGGER_ERROR), int(sc.SC_DO_NOTHING)): 6,
        (int(EMPTY_LIST_ERROR), int(sc.SC_THROW)): 4,
        (int(EMPTY_LIST_ERROR), int(sc.SC_DO_NOTHING)): 8,
        (-1, stop_actions): 1,
    }
)
assert [(row[0], row[3], row[4]) for row in report_calls] == [
    (int(EMPTY_LIST_ERROR), int(sc.SC_THROW), ""),
    (int(EMPTY_LIST_ERROR), int(sc.SC_THROW), ""),
    (int(EMPTY_LIST_ERROR), int(sc.SC_THROW), ""),
    (int(EMPTY_LIST_ERROR), int(sc.SC_THROW), ""),
    (int(EMPTY_LIST_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(WAIT_ERROR), int(sc.SC_THROW), ""),
    (int(EMPTY_LIST_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(WAIT_ERROR), int(sc.SC_THROW), ""),
    (int(EMPTY_LIST_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(TRIGGER_ERROR), int(sc.SC_THROW), ""),
    (int(EMPTY_LIST_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(TRIGGER_ERROR), int(sc.SC_THROW), ""),
    (int(EMPTY_LIST_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(WAIT_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(EMPTY_LIST_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(WAIT_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(EMPTY_LIST_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(TRIGGER_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(EMPTY_LIST_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(TRIGGER_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(WAIT_ERROR), int(sc.SC_THROW), ""),
    (int(WAIT_ERROR), int(sc.SC_THROW), ""),
    (int(TRIGGER_ERROR), int(sc.SC_THROW), ""),
    (int(TRIGGER_ERROR), int(sc.SC_THROW), ""),
    (int(WAIT_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(WAIT_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(TRIGGER_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(TRIGGER_ERROR), int(sc.SC_DO_NOTHING), ""),
    (int(WAIT_ERROR), int(sc.SC_THROW), "method_recovery"),
    (int(WAIT_ERROR), int(sc.SC_THROW), "method_recovery"),
    (int(WAIT_ERROR), int(sc.SC_DO_NOTHING), "method_recovery"),
    (int(WAIT_ERROR), int(sc.SC_DO_NOTHING), "method_recovery"),
    (int(TRIGGER_ERROR), int(sc.SC_THROW), "thread_recovery"),
    (int(TRIGGER_ERROR), int(sc.SC_THROW), "thread_recovery"),
    (int(TRIGGER_ERROR), int(sc.SC_DO_NOTHING), "thread_recovery"),
    (int(TRIGGER_ERROR), int(sc.SC_DO_NOTHING), "thread_recovery"),
    (-1, stop_actions, ""),
], report_calls
assert report_calls[-1][1:3] == ("/OSCI/SystemC", "Simulation stopped by user.")
wait_deletions = int(native_bridge.python_wait_expression_list_deletion_count()) - wait_deletions_before
trigger_deletions = (
    int(native_bridge.python_next_trigger_expression_list_deletion_count()) - trigger_deletions_before
)
assert wait_deletions == 17, wait_deletions
assert trigger_deletions == 14, trigger_deletions
sc.set_report_handler()
sc.sc_report_handler.set_actions(report_type[EMPTY_LIST_ERROR], sc.SC_ERROR, previous_empty)
sc.sc_report_handler.set_actions(report_type[WAIT_ERROR], sc.SC_ERROR, previous_wait)
sc.sc_report_handler.set_actions(report_type[TRIGGER_ERROR], sc.SC_ERROR, previous_trigger)
print(f"SECTION1_REJECTED_EXPR wait_deletes={wait_deletions} trigger_deletes={trigger_deletions} reports={len(report_calls) - 1}")
print("SECTION1_REJECTED_EXPR_RECOVERY method=1 thread=1 reset=1 kill=1 empty=throw/suppress channel=numeric actions=throw/suppress")

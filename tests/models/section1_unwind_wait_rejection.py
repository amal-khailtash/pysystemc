"""Probe expression waits attempted inside self-reset and self-kill unwind."""

from collections import Counter
import sys

from amal.eda.systemc import sc_core as sc
import cppyy


WAIT_DURING_UNWINDING = 537
suppressed = "--suppress" in sys.argv[1:]
report_action = sc.SC_DO_NOTHING if suppressed else sc.SC_THROW
restart_time = 1000 if suppressed else 0
release_time = 2000 if suppressed else 1000
report_type = sc.SC_ID_WAIT_DURING_UNWINDING_
reports = []
reset_starts = []
kill_starts = []
reset_unwind_markers = []
kill_unwind_markers = []
reset_resumed = []


def capture_report(report, actions):
    """Record native report identity and preserve the configured action."""
    reports.append(
        (
            int(report.get_id()),
            report.get_msg_type(),
            " ".join(report.get_msg().split()),
            int(report.get_severity()),
            int(actions),
            report.get_process_name(),
            int(report.get_time().value()),
        )
    )
    sc.sc_report_handler.default_handler(report, actions)


class Model(sc.sc_module):
    """Own self-controlled workers and observe expression-list cleanup."""

    def __init__(self):
        """Register reset, kill and controller threads with separate events."""
        super().__init__(sc.sc_module_name("section1_unwind_wait_rejection"))
        self.reset_first = sc.sc_event("reset_first")
        self.reset_second = sc.sc_event("reset_second")
        self.kill_first = sc.sc_event("kill_first")
        self.kill_second = sc.sc_event("kill_second")
        self.reset_release = sc.sc_event("reset_release")
        self.sc_thread("reset_worker", self.reset_worker)
        self.reset_handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("kill_worker", self.kill_worker)
        self.kill_handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("controller", self.controller)

    def _attempt_expression_wait_and_rethrow(self, label, first, second):
        """Observe one forbidden wait report, then restore the original unwind.

        Args:
            label: Worker name used to associate the report with its control path.
            first: First native event in the expression.
            second: Second native event in the expression.

        Raises:
            AssertionError: Throwing native wait returns or emits a report
                with the wrong ID, type, or process message.

        The global handler records the report with `SC_THROW` or
        `SC_DO_NOTHING`. The throwing case catches `sc_report`; the suppressed
        case requires normal return. The caller raises its original unwind.
        """
        try:
            sc.wait(first | second)
        except sc.sc_report as report:
            assert int(report.get_id()) == WAIT_DURING_UNWINDING
            assert report.get_msg_type() == report_type
            assert " ".join(report.get_msg().split()) == label
        else:
            assert suppressed, f"{label}: wait returned during native unwind"

    def reset_worker(self):
        """Self-reset once, reject one expression wait, then park after restart."""
        reset_starts.append(int(sc.sc_time_stamp().value()))
        if len(reset_starts) == 1:
            try:
                self.reset_handle.reset()
            except sc._ProcessUnwind as unwind:
                reset_unwind_markers.append(type(unwind).__name__)
                self._attempt_expression_wait_and_rethrow("reset_worker", self.reset_first, self.reset_second)
                raise unwind
            raise AssertionError("self-reset returned without scheduler unwind")

        assert len(reset_starts) == 2, reset_starts
        sc.wait(self.reset_release)
        reset_resumed.append(int(sc.sc_time_stamp().value()))

    def kill_worker(self):
        """Self-kill after one forbidden expression wait and terminate."""
        kill_starts.append(int(sc.sc_time_stamp().value()))
        try:
            self.kill_handle.kill()
        except sc._ProcessUnwind as unwind:
            kill_unwind_markers.append(type(unwind).__name__)
            self._attempt_expression_wait_and_rethrow("kill_worker", self.kill_first, self.kill_second)
            raise unwind
        raise AssertionError("self-kill returned without scheduler unwind")

    def controller(self):
        """Exercise expression events after restart, then release and stop."""
        sc.wait(1, sc.SC_NS)
        if suppressed:
            # Native SC_DO_NOTHING continues into event registration and waits.
            assert reset_starts == [0] and kill_starts == [0]
            assert not self.reset_handle.terminated() and not self.kill_handle.terminated()
            self.reset_first.notify(sc.SC_ZERO_TIME)
            self.kill_first.notify(sc.SC_ZERO_TIME)
            self._yield_deltas(4)
            sc.wait(1, sc.SC_NS)
        assert reset_starts == [0, restart_time], reset_starts
        assert kill_starts == [0], kill_starts
        assert self.kill_handle.terminated(), "self-killed thread did not terminate"
        assert not self.reset_handle.terminated(), "reset worker did not restart"

        for event in (self.reset_first, self.reset_second, self.kill_first, self.kill_second):
            event.notify(sc.SC_ZERO_TIME)
        self._yield_deltas(4)
        assert reset_resumed == [], "an expression event woke the restarted worker"
        assert self.kill_handle.terminated(), "an expression event revived the killed worker"

        self.reset_release.notify(sc.SC_ZERO_TIME)
        self._yield_until(lambda: self.reset_handle.terminated())
        assert reset_resumed == [release_time], reset_resumed
        sc.sc_stop()

    @staticmethod
    def _yield_deltas(count):
        """Advance a fixed number of delta cycles for zero-time notifications."""
        for _ in range(count):
            sc.wait(sc.SC_ZERO_TIME)

    @staticmethod
    def _yield_until(predicate):
        """Advance at most sixteen deltas while waiting for process termination."""
        for _ in range(16):
            if predicate():
                return
            sc.wait(sc.SC_ZERO_TIME)
        raise AssertionError("reset worker did not terminate after release")


native = cppyy.gbl.sc_core
wait_deletions_before = int(native.python_wait_expression_list_deletion_count())
sc.sc_report_handler.set_actions(report_type, sc.SC_ERROR, report_action)
sc.set_report_handler(capture_report)
model = Model()
sc.sc_start()

wait_deletions = int(native.python_wait_expression_list_deletion_count()) - wait_deletions_before
assert reset_starts == [0, restart_time], reset_starts
assert kill_starts == [0], kill_starts
assert reset_unwind_markers == kill_unwind_markers == ["_ProcessUnwind"]
assert wait_deletions == 2, wait_deletions
target_reports = [row for row in reports if row[0] == WAIT_DURING_UNWINDING]
stop_reports = [row for row in reports if row[0] != WAIT_DURING_UNWINDING]
assert len(target_reports) == 2 and len(stop_reports) == 1, reports
assert Counter((row[0], row[1], row[2], row[3], row[4], row[5], row[6]) for row in target_reports) == Counter(
    [
        (
            WAIT_DURING_UNWINDING,
            report_type,
            "reset_worker",
            int(sc.SC_ERROR),
            int(report_action),
            "reset_worker",
            0,
        ),
        (
            WAIT_DURING_UNWINDING,
            report_type,
            "kill_worker",
            int(sc.SC_ERROR),
            int(report_action),
            "kill_worker",
            0,
        ),
    ]
), target_reports
assert stop_reports[0][:3] == (-1, "/OSCI/SystemC", "Simulation stopped by user."), stop_reports
assert model.reset_handle.terminated() and model.kill_handle.terminated()
assert reset_resumed == [release_time], reset_resumed
assert int(sc.sc_time_stamp().value()) == release_time
sc.set_report_handler()
print(
    f"SECTION1_UNWIND_WAIT reset_starts={len(reset_starts)} kill_starts={len(kill_starts)} "
    f"reports={len(target_reports)} wait_deletes={wait_deletions} stray_wakes=0 "
    f"action={'suppressed' if suppressed else 'throw'}"
)

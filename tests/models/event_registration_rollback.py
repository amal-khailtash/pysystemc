"""Exercise simulated rollback after real dynamic event registrations."""

from pathlib import Path
import sys

from amal.eda.systemc import sc_core as sc
import cppyy


ROOT = Path(__file__).resolve().parents[2]
cppyy.cppdef((ROOT / "tests/native/event_registration_rollback_probe.h").read_text(encoding="utf-8"))
native = cppyy.gbl.sc_core
probe = native.event_rollback_probe


def report_counts():
    """Return all severity totals so synthetic failure cannot hide a report."""
    severities = (sc.SC_INFO, sc.SC_WARNING, sc.SC_ERROR, sc.SC_FATAL)
    return tuple(sc.sc_report_handler.get_count(severity) for severity in severities)


def current_ticks():
    """Return the current native simulation time in resolution ticks."""
    return int(sc.sc_time_stamp().value())


class RollbackModel(sc.sc_module):
    """Run one isolated process-kind, list-kind and timedness scenario."""

    def __init__(self, process_kind, expression_kind, timed, partial):
        """Create live event owners and register one worker plus its observers.

        Args:
            process_kind: ``thread``, ``cthread`` or ``method``.
            expression_kind: ``OR`` or ``AND``.
            timed: Whether the probe stages a native process timeout.
            partial: Whether to remove one member registration before injecting
                failure, leaving a deterministic partial target-list state.
        """
        module_name = sc.sc_module_name("event_rollback")
        super().__init__(module_name)
        self.process_kind = process_kind
        self.expression_kind = expression_kind
        self.timed = timed
        self.partial = partial
        self.delay = sc.sc_time(3, sc.SC_NS)
        self.first = sc.sc_event("first")
        self.second = sc.sc_event("second")
        self.previous = sc.sc_event("previous_trigger")
        self.kick = sc.sc_event("method_kick")
        self.recovery = sc.sc_event("recovery")
        self.ready = sc.sc_event("ready")
        self.done = sc.sc_event("done")
        self.timeout_observations = []
        self.callback_records = []
        self.injection_start = None
        self.report_counts_before = None
        self.deletion_delta = None
        self.staged_timeout_delta = None

        if process_kind == "cthread":
            self.clock = sc.sc_clock("clock", sc.sc_time(1, sc.SC_NS))
            self.clock_port = sc.sc_in[bool]("clock_port")
            self.clock_port.bind(self.clock)
            self.sc_cthread("worker", self.wait_worker, self.clock_port.pos())
        elif process_kind == "thread":
            self.sc_thread("worker", self.wait_worker)
        elif process_kind == "method":
            self.sc_method("worker", self.method_worker)
            self.dont_initialize()
            self.sensitive(self.kick)
        else:
            raise ValueError(f"unknown process kind: {process_kind}")

        self.worker_handle = sc.sc_get_last_created_process_handle()
        timeout_event = probe.timeout_event(self.worker_handle)
        self.sc_method("timeout_observer", self.timeout_observer)
        self.dont_initialize()
        self.sensitive(timeout_event)
        self.sc_thread("driver", self.driver)

    def timeout_observer(self):
        """Record any staged timeout that escaped rollback cancellation."""
        self.timeout_observations.append(current_ticks())

    def _expression(self):
        """Build the requested temporary expression from live source events."""
        return self.first | self.second if self.expression_kind == "OR" else self.first & self.second

    def _assert_injection(self, operation, deletion_counter):
        """Require the deliberate allocation exception and exact cleanup delta.

        Args:
            operation: Probe call that stages native registrations then throws.
            deletion_counter: Native bridge deletion-counter accessor.

        Raises:
            AssertionError: The injection mapping, reports or owner cleanup differ.
            cppyy.gbl.std.bad_alloc: Propagates unless it is the expected synthetic failure.
        """
        before_deletions = int(deletion_counter())
        before_timeout_stages = int(probe.staged_timeout_count())
        before_injected = int(probe.injected_bad_alloc_count())
        self.report_counts_before = report_counts()
        try:
            operation(self._expression(), self.timed, self.partial, self.first, self.delay)
        except cppyy.gbl.std.bad_alloc as error:
            assert isinstance(error, cppyy.gbl.std.bad_alloc), type(error)
        else:
            raise AssertionError("synthetic std::bad_alloc returned normally")

        self.deletion_delta = int(deletion_counter()) - before_deletions
        self.staged_timeout_delta = int(probe.staged_timeout_count()) - before_timeout_stages
        injected_delta = int(probe.injected_bad_alloc_count()) - before_injected
        assert self.deletion_delta == 1, self.deletion_delta
        assert self.staged_timeout_delta == int(self.timed), self.staged_timeout_delta
        assert injected_delta == 1, injected_delta
        assert report_counts() == self.report_counts_before

    def wait_worker(self):
        """Inject from a live thread and check recovery, timeout and quiet timing.

        For the native CTHREAD form, the event wait resumes at 5 ns and the
        following timed wait completes at 8 ns with a 1 ns clock. The reference
        control separately verifies native event winners at 1/2 ns and timeout
        winners at 3 ns; CTHREAD's cycle wait aligns with an edge already at
        those timestamps in that control sequence.
        """
        self.injection_start = current_ticks()
        self.ready.notify(sc.SC_ZERO_TIME)
        operation = probe.wait_or_bad_alloc if self.expression_kind == "OR" else probe.wait_and_bad_alloc
        self._assert_injection(operation, native.python_wait_expression_list_deletion_count)
        self.callback_records.append(("injected", current_ticks() - self.injection_start))
        sc.wait(self.recovery)
        assert self.timeout_observations == [], self.timeout_observations
        self.callback_records.append(("recovery", current_ticks() - self.injection_start))
        sc.wait(3, sc.SC_NS)
        quiet_delta = current_ticks() - self.injection_start
        expected_quiet = 8 if self.process_kind == "cthread" else 7
        assert quiet_delta == sc.sc_time(expected_quiet, sc.SC_NS).value(), quiet_delta
        self.callback_records.append(("quiet", quiet_delta))
        self.done.notify(sc.SC_ZERO_TIME)

    def method_worker(self):
        """Replace an old dynamic trigger, inject, and verify the recovery trigger."""
        if not self.callback_records:
            sc.next_trigger(self.previous)
            self.callback_records.append(("previous_armed", current_ticks()))
            self.ready.notify(sc.SC_ZERO_TIME)
            return
        if self.callback_records[-1][0] == "previous_armed":
            self.injection_start = current_ticks()
            operation = probe.next_or_bad_alloc if self.expression_kind == "OR" else probe.next_and_bad_alloc
            self._assert_injection(operation, native.python_next_trigger_expression_list_deletion_count)
            self.ready.notify(sc.SC_ZERO_TIME)
            self.callback_records.clear()
            self.callback_records.append(("injected", current_ticks() - self.injection_start))
            sc.next_trigger(self.recovery)
            return
        if self.callback_records[-1][0] == "injected":
            assert self.timeout_observations == [], self.timeout_observations
            recovery_delta = current_ticks() - self.injection_start
            expected_recovery = 5 if self.process_kind == "cthread" else 4
            assert recovery_delta == sc.sc_time(expected_recovery, sc.SC_NS).value(), recovery_delta
            self.callback_records.append(("recovery", recovery_delta))
            sc.next_trigger(3, sc.SC_NS)
            return
        if self.callback_records[-1][0] == "recovery":
            quiet_delta = current_ticks() - self.injection_start
            assert quiet_delta == sc.sc_time(7, sc.SC_NS).value(), quiet_delta
            self.callback_records.append(("quiet", quiet_delta))
            self.done.notify(sc.SC_ZERO_TIME)
            return
        raise AssertionError(f"unexpected method callback state: {self.callback_records}")

    def driver(self):
        """Notify failed targets, cross the timeout, then release recovery once."""
        if self.process_kind == "method":
            self.kick.notify(sc.SC_ZERO_TIME)
            sc.wait(self.ready)
            self.previous.notify(1, sc.SC_NS)
            sc.wait(self.ready)
        else:
            sc.wait(self.ready)

        self.first.notify(1, sc.SC_NS)
        self.second.notify(2, sc.SC_NS)
        sc.wait(3, sc.SC_NS)
        sc.wait(1, sc.SC_NS)
        self.recovery.notify(sc.SC_ZERO_TIME)
        sc.wait(1, sc.SC_NS)
        self.first.notify(sc.SC_ZERO_TIME)
        sc.wait(1, sc.SC_NS)
        self.second.notify(sc.SC_ZERO_TIME)
        sc.wait(1, sc.SC_NS)
        sc.wait(self.done)
        expected_timeout = 8 if self.process_kind == "cthread" else 7
        assert self.timeout_observations == [
            self.injection_start + sc.sc_time(expected_timeout, sc.SC_NS).value()
        ], self.timeout_observations
        sc.sc_stop()


def main():
    """Run one command-line scenario and print its canonical observations."""
    process_kind, expression_kind, timed_arg, state_arg = sys.argv[1:5]
    if process_kind not in ("thread", "cthread", "method"):
        raise ValueError(process_kind)
    if expression_kind not in ("OR", "AND"):
        raise ValueError(expression_kind)
    if timed_arg not in ("timed", "untimed"):
        raise ValueError(timed_arg)
    if state_arg not in ("full", "partial"):
        raise ValueError(state_arg)
    timed = timed_arg == "timed"
    partial = state_arg == "partial"
    model = RollbackModel(process_kind, expression_kind, timed, partial)
    sc.sc_start()

    expected = ["injected", "recovery", "quiet"]
    assert [record[0] for record in model.callback_records] == expected, model.callback_records
    expected_timeout = 8 if process_kind == "cthread" else 7
    assert model.timeout_observations == [
        model.injection_start + sc.sc_time(expected_timeout, sc.SC_NS).value()
    ], model.timeout_observations
    assert model.deletion_delta == 1, model.deletion_delta
    assert model.staged_timeout_delta == int(timed), model.staged_timeout_delta
    recovery_ns = 5 if process_kind == "cthread" else 4
    quiet_ns = 8 if process_kind == "cthread" else 7
    recovery_ticks = sc.sc_time(recovery_ns, sc.SC_NS).value()
    quiet_ticks = sc.sc_time(quiet_ns, sc.SC_NS).value()
    assert model.callback_records[1][1] == recovery_ticks
    assert model.callback_records[2][1] == quiet_ticks
    print(
        "EVENT_REGISTRATION_ROLLBACK",
        process_kind,
        expression_kind,
        timed_arg,
        state_arg,
        f"recovery={sc.sc_time(recovery_ns, sc.SC_NS)}",
        f"quiet={sc.sc_time(quiet_ns, sc.SC_NS)}",
        f"deletions={model.deletion_delta}",
        "injected=1",
        f"timeouts_staged={model.staged_timeout_delta}",
        "staged_timeout_callbacks=0",
        f"legitimate_timeout_callbacks={len(model.timeout_observations)}",
    )


main()

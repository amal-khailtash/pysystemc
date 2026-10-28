"""Compare recoverable kernel boundary reports with the native C++ reference.

The public Python ``sc_stop`` wrapper is idempotent after shutdown. Its zero
repeat-report behavior is asserted before raw native calls exercise W545.
"""

import sys

from amal.eda.systemc import sc_core as sc
import cppyy


SCENARIOS = {"wait_n", "empty_lists", "stop_start"}
scenario = sys.argv[1]
assert scenario in SCENARIOS, scenario
records = []


def attempt(label, report_id, severity, operation):
    """Catch one configured report and record its metadata and recovery.

    Args:
        label: Stable public operation name shared with the native reference.
        report_id: Expected native SystemC report number.
        severity: Expected SystemC report severity.
        operation: Callable that must raise the selected ``sc_report``.

    Raises:
        AssertionError: If the operation returns or emits different metadata.
    """
    try:
        operation()
    except sc.sc_report as report:
        assert report.get_id() == report_id, (label, report.get_id())
        assert report.get_severity() == severity, (label, report.get_severity())
        message_type = report.get_msg_type()
        assert message_type == report_type[int(report_id)], (label, message_type)
        time_value = report.get_time().value()
        assert time_value == sc.sc_time_stamp().value(), (label, time_value)
        count = sc.sc_report_handler.get_count(message_type)
        records.append(
            f"KERNEL_BOUNDARY_REPORT {scenario}|{label}|{report_id}|{int(severity)}|{message_type}|"
            f"{' '.join(report.get_msg().split())}|{time_value}|{count}"
        )
        records.append(f"KERNEL_BOUNDARY_RECOVERED {scenario}|{label}")
    else:
        raise AssertionError(f"{label}: configured report did not throw")


report_type = {
    525: "wait(n) is only valid for n > 0",
    565: "invalid use of sc_(and|or)_event_list",
    545: "sc_stop has already been called",
    546: "sc_start called after sc_stop has been called",
}


def set_throw(report_id, severity):
    """Configure one report ID to throw and retain its previous action mask."""
    return sc.sc_report_handler.set_actions(report_id, severity, sc.SC_THROW)


def run_wait_n():
    """Exercise zero and negative integer waits from a recoverable thread."""
    previous = set_throw(sc.SC_ID_WAIT_N_INVALID_, sc.SC_ERROR)
    finished = []

    def worker():
        """Recover from both invalid waits and prove the thread continued."""
        attempt("wait_zero", 525, sc.SC_ERROR, lambda: sc.wait(0))
        attempt("wait_negative", 525, sc.SC_ERROR, lambda: sc.wait(-1))
        finished.append("thread")

    handle = sc.sc_spawn(worker, "invalid_wait_n")
    sc.sc_start(1, sc.SC_NS)
    assert handle.terminated() and finished == ["thread"], (handle.terminated(), finished)
    assert sc.sc_report_handler.get_count(report_type[525]) == 2
    sc.sc_report_handler.set_actions(sc.SC_ID_WAIT_N_INVALID_, sc.SC_ERROR, previous)


class EmptyListModel(sc.sc_module):
    """Keep empty lists and callbacks alive while each public trigger reports."""

    def __init__(self, name):
        """Create a thread and a method for empty-list wait and trigger calls."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.kick = sc.sc_event("empty_list_kick")
        self.or_list = sc.sc_event_or_list()
        self.and_list = sc.sc_event_and_list()
        self.wait_done = False
        self.trigger_done = False
        self.sc_thread("wait_empty_lists", self.wait_empty_lists)
        self.sc_method("trigger_empty_lists", self.trigger_empty_lists)
        self.sensitive(self.kick)
        self.dont_initialize()

    def wait_empty_lists(self):
        """Catch empty OR and AND waits, then mark the thread recovered."""
        attempt("wait_empty_or", 565, sc.SC_ERROR, lambda: sc.wait(self.or_list))
        attempt("wait_empty_and", 565, sc.SC_ERROR, lambda: sc.wait(self.and_list))
        self.wait_done = True

    def trigger_empty_lists(self):
        """Catch empty OR and AND next_trigger calls in a method process."""
        attempt("next_trigger_empty_or", 565, sc.SC_ERROR, lambda: sc.next_trigger(self.or_list))
        attempt("next_trigger_empty_and", 565, sc.SC_ERROR, lambda: sc.next_trigger(self.and_list))
        self.trigger_done = True


def run_empty_lists():
    """Exercise empty OR/AND lists through wait and dynamic next_trigger."""
    previous = set_throw(sc.SC_ID_EVENT_LIST_FAILED_, sc.SC_ERROR)
    model = EmptyListModel("empty_list_boundary")
    model.kick.notify(sc.SC_ZERO_TIME)
    sc.sc_start(1, sc.SC_NS)
    assert model.wait_done and model.trigger_done
    assert sc.sc_report_handler.get_count(report_type[565]) == 4
    sc.sc_report_handler.set_actions(sc.SC_ID_EVENT_LIST_FAILED_, sc.SC_ERROR, previous)


def run_stop_start():
    """Check wrapper idempotence, native W545 suppression and E546 recovery."""
    warning_previous = set_throw(sc.SC_ID_SIMULATION_STOP_CALLED_TWICE_, sc.SC_WARNING)
    error_previous = set_throw(sc.SC_ID_SIMULATION_START_AFTER_STOP_, sc.SC_ERROR)
    sc.sc_stop()
    sc.sc_stop()
    sc.sc_stop()
    assert sc.sc_report_handler.get_count(report_type[545]) == 0
    native_stop = cppyy.gbl.sc_core.sc_stop
    attempt("duplicate_stop", 545, sc.SC_WARNING, native_stop)
    warning_count = sc.sc_report_handler.get_count(report_type[545])
    assert warning_count == 1
    native_stop()
    assert sc.sc_report_handler.get_count(report_type[545]) == warning_count
    attempt("start_after_stop", 546, sc.SC_ERROR, lambda: sc.sc_start(sc.SC_ZERO_TIME))
    assert sc.sc_report_handler.get_count(report_type[546]) == 1
    sc.sc_report_handler.set_actions(sc.SC_ID_SIMULATION_STOP_CALLED_TWICE_, sc.SC_WARNING, warning_previous)
    sc.sc_report_handler.set_actions(sc.SC_ID_SIMULATION_START_AFTER_STOP_, sc.SC_ERROR, error_previous)


if scenario == "wait_n":
    run_wait_n()
elif scenario == "empty_lists":
    run_empty_lists()
else:
    run_stop_start()

for record in records:
    print(record)
print(f"KERNEL_BOUNDARY_OK {scenario} reports={len(records) // 2}")

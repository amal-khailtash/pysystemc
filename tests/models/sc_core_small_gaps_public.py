"""Close event-list, event-operator and small kernel configuration contracts."""

import gc
from pathlib import Path

from amal.eda.systemc import sc_core as sc
import cppyy

cppyy.include(str(Path(__file__).resolve().parents[1] / "native/sc_core_small_gaps_probe.h"))
probe = cppyy.gbl.small_gaps_probe
assert sc.sc_exception.__cpp_name__ == cppyy.gbl.std.exception.__cpp_name__
exception_owner = cppyy.gbl.std.runtime_error("alias failure")
assert exception_owner.what() == "alias failure"
for index, expected in enumerate(
    ("native runtime failure", "native text failure", "UNKNOWN EXCEPTION", "native report failure")
):
    report = probe.exception(index)
    assert report and report.get_severity() == sc.SC_ERROR and report.get_msg() == expected
    report.__python_owns__ = True
    print("SMALL_EXCEPTION", index, report.get_id(), report.get_msg_type(), report.get_msg(), sep="|")
    del report
    gc.collect()
previous = sc.sc_report_handler.set_actions(sc.SC_ERROR, sc.SC_DO_NOTHING)
assert not probe.exception(0)
sc.sc_report_handler.set_actions(sc.SC_ERROR, previous)
print("SMALL_EXCEPTION_SUPPRESSED 1")
assert sc.SC_ID_REGISTER_ID_FAILED_ == "register_id failed"
try:
    sc.sc_report.register_id(-1, "invalid registration")
except sc.sc_report as report:
    assert report.get_msg_type() == sc.SC_ID_REGISTER_ID_FAILED_
    assert report.get_id() == 800 and report.get_msg() == "invalid report id"
    print("SMALL_REGISTER_REPORT", report.get_id(), report.get_msg_type(), report.get_msg(), sep="|")
else:
    raise AssertionError("invalid report registration was accepted")

reports = []


def capture(report, actions):
    """Copy warning diagnostics while native name checking and seed calls execute."""
    reports.append((report.get_id(), str(report.get_msg_type()), str(report.get_msg())))


sc.set_report_handler(capture)
assert sc.sc_enable_name_checking is True
sc.sc_enable_name_checking = True
checked = sc.sc_event("checked name")
# Event names retain native event semantics; object name checking is separate.
checked_object = sc.sc_signal[bool]("checked object")
assert checked_object.name() == "checked_object"
sc.sc_enable_name_checking = False
assert cppyy.gbl.sc_core.sc_enable_name_checking is False
unchecked_object = sc.sc_signal[bool]("unchecked object")
assert unchecked_object.name() == "unchecked object"
sc.sc_enable_name_checking = True
sc.sc_set_random_seed(123)
sc.set_report_handler()
assert len(reports) == 2, reports
for row in reports:
    print("SMALL_CONFIG_REPORT", *row, sep="|")
print("SMALL_NAMES", checked_object.name(), unchecked_object.name(), sep="|")
for member in ("sc_enable_name_checking", "sc_allow_process_control_corners"):
    try:
        setattr(sc, member, 1)
    except TypeError:
        pass
    else:
        raise AssertionError("nonboolean native flag assignment was accepted")

# Destruction is safe only after dynamic/static consumers detach.
owner = sc.sc_event("temporary_owned")
owner.notify(9, sc.SC_NS)
assert sc.sc_find_event("temporary_owned")
del owner
gc.collect()
assert not sc.sc_find_event("temporary_owned")
print("SMALL_EVENT_DESTRUCTION 1")
a, b, c = [sc.sc_event(name) for name in ("a", "b", "c")]
extra_or = sc.sc_event_or_list(c)
extra_and = sc.sc_event_and_list(c)
or_event = sc.sc_event_or_list((a | b) | c)
or_list = sc.sc_event_or_list((a | b) | extra_or)
and_event = sc.sc_event_and_list((a & b) & c)
and_list = sc.sc_event_and_list((a & b) & extra_and)
for values in (or_event, or_list, and_event, and_list):
    base = cppyy.bind_object(cppyy.addressof(values), sc.sc_event_list)
    copied = probe.List(base)
    assert copied.size() == 3 and values.size() == 3
    del copied
assert all(event.name() == name for event, name in ((a, "a"), (b, "b"), (c, "c")))
print("SMALL_EVENT_LIST_COPY 3 3 3 3")
records = []


class Model(sc.sc_module):
    """Own event-expression consumers and a timed thread-control experiment."""

    def __init__(self):
        """Retain callbacks and register four event-list waits and corner controls."""
        name = sc.sc_module_name("small_gaps")
        super().__init__(name)
        for label, events in (
            ("or_event", or_event),
            ("or_list", or_list),
            ("and_event", and_event),
            ("and_list", and_list),
        ):
            self.sc_thread(label, self.waiter(label, events))
        self.sc_thread("timed", self.timed)
        self.handle = sc.sc_get_last_created_process_handle()
        self.sc_thread("drive", self.drive)

    def waiter(self, label, events):
        """Build a retained wait callback for an owned list of borrowed events."""

        def callback():
            """Record exact completion time of the native expression list."""
            sc.wait(events)
            records.append((label, sc.sc_time_stamp().value()))

        return callback

    def timed(self):
        """Wait on a timeout to exercise the native process-control corner flag."""
        sc.wait(4, sc.SC_NS)
        records.append(("timed", sc.sc_time_stamp().value()))

    def drive(self):
        """Check strict/corner control policies and drive event conjunctions."""
        context = sc.sc_get_curr_simcontext()
        assert isinstance(context, sc.sc_simcontext) and not context.__python_owns__
        assert cppyy.addressof(self.simcontext()) == cppyy.addressof(context)
        process = sc.sc_get_current_process_b()
        assert isinstance(process, sc.sc_process_b) and not process.__python_owns__
        print("SMALL_CONTEXT_PROCESS 1 1")
        sc.wait(1, sc.SC_NS)
        assert sc.sc_allow_process_control_corners is False
        try:
            self.handle.disable()
        except sc.sc_report as report:
            assert report.get_id() == 559
            print("SMALL_CORNER_REPORT", report.get_id(), report.get_msg(), sep="|")
        else:
            raise AssertionError("strict timeout disable did not report")
        sc.sc_allow_process_control_corners = True
        assert cppyy.gbl.sc_core.sc_allow_process_control_corners is True
        self.handle.disable()
        a.notify()
        sc.wait(1, sc.SC_NS)
        b.notify()
        sc.wait(1, sc.SC_NS)
        c.notify()
        self.handle.enable()
        sc.sc_allow_process_control_corners = False
        sc.wait(2, sc.SC_NS)
        sc.sc_stop()


model = Model()
sc.sc_start()
assert sorted(records) == [
    ("and_event", 3000),
    ("and_list", 3000),
    ("or_event", 1000),
    ("or_list", 1000),
    ("timed", 4000),
], records
for row in sorted(records):
    print("SMALL_EVENT_RECORD", *row)
assert sc.sc_enable_name_checking is True and sc.sc_allow_process_control_corners is False

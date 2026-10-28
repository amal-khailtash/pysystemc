"""Compare invalid member scheduling and stop-mode reports with native SystemC."""

import sys

from amal.eda.systemc import sc_core as sc


scenario = sys.argv[1]
assert scenario in {"module", "channel", "stop_mode"}, scenario
records = []


def normalized(message):
    """Normalize report whitespace to match the native reference output."""
    return " ".join(message.split())


def report_record(route, label, report, expected_id, message_type, expected_status=sc.SC_RUNNING):
    """Validate and record one report's metadata and live kernel state.

    Args:
        route: Member owner or public function route under test.
        label: Stable operation label.
        report: Caught native ``sc_report`` instance.
        expected_id: Expected SystemC report number.
        message_type: Expected registered report message type.
        expected_status: Kernel status required at report time.

    Raises:
        AssertionError: A report field, count, time, or simulation state differs.
    """
    assert report.get_id() == expected_id, (route, label, report.get_id())
    assert report.get_severity() == sc.SC_ERROR, (route, label, report.get_severity())
    assert report.get_msg_type() == message_type, (route, label, report.get_msg_type())
    time_value = report.get_time().value()
    assert time_value == sc.sc_time_stamp().value(), (route, label, time_value)
    status = int(sc.sc_get_status())
    delta = sc.sc_delta_count()
    assert status == int(expected_status), (route, label, status)
    count = sc.sc_report_handler.get_count(message_type)
    records.append(
        f"MEMBER_BOUNDARY_REPORT {route}|{label}|{expected_id}|{int(sc.SC_ERROR)}|{message_type}|"
        f"{normalized(report.get_msg())}|{time_value}|{delta}|{status}|{count}"
    )
    records.append(f"MEMBER_BOUNDARY_RECOVERED {route}|{label}")


class BoundaryModel(sc.sc_module):
    """Keep module and channel owners alive while a thread triggers invalid members."""

    def __init__(self, owner_kind):
        """Create one thread that probes the selected member route.

        Args:
            owner_kind: Either ``module`` or ``channel``.
        """
        name = sc.sc_module_name("member_boundary")
        super().__init__(name)
        self.channel = sc.sc_prim_channel("member_boundary_channel")
        self.owner = self if owner_kind == "module" else self.channel
        self.owner_kind = owner_kind
        self.handle = None
        self.sc_thread("invalid_member_calls", self.invalid_member_calls)
        self.handle = sc.sc_get_last_created_process_handle()
        del name

    def arguments(self):
        """Return every argument shape accepted by the native member overloads."""
        first = sc.sc_event("first")
        second = sc.sc_event("second")
        or_list = sc.sc_event_or_list(first | second)
        and_list = sc.sc_event_and_list(first & second)
        duration = sc.sc_time(3, sc.SC_NS)
        return first, second, or_list, and_list, duration

    def invalid_member_calls(self):
        """Recover from all member E520 reports and applicable E522 routes."""
        report_520 = sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_
        forms = (
            ("none", lambda: self.owner.next_trigger()),
            ("event", lambda: self.owner.next_trigger(first)),
            ("or_list", lambda: self.owner.next_trigger(or_list)),
            ("and_list", lambda: self.owner.next_trigger(and_list)),
            ("time", lambda: self.owner.next_trigger(duration)),
            ("units", lambda: self.owner.next_trigger(3.0, sc.SC_NS)),
            ("time_event", lambda: self.owner.next_trigger(duration, first)),
            ("units_event", lambda: self.owner.next_trigger(3.0, sc.SC_NS, first)),
            ("time_or", lambda: self.owner.next_trigger(duration, or_list)),
            ("units_or", lambda: self.owner.next_trigger(3.0, sc.SC_NS, or_list)),
            ("time_and", lambda: self.owner.next_trigger(duration, and_list)),
            ("units_and", lambda: self.owner.next_trigger(3.0, sc.SC_NS, and_list)),
        )
        first, second, or_list, and_list, duration = self.arguments()
        for label, operation in forms:
            try:
                operation()
            except sc.sc_report as report:
                report_record(
                    self.owner_kind,
                    label,
                    report,
                    520,
                    report_520,
                )
            else:
                raise AssertionError(f"{self.owner_kind}.{label}: invalid thread next_trigger returned")

        if self.owner_kind == "module":
            report_522 = sc.SC_ID_HALT_NOT_ALLOWED_
            for route, operation in (
                ("module_halt", self.halt),
                ("free_halt", sc.halt),
            ):
                try:
                    operation()
                except sc.sc_report as report:
                    report_record(
                        route,
                        "halt",
                        report,
                        522,
                        report_522,
                    )
                else:
                    raise AssertionError(f"{route}: halt outside SC_CTHREAD returned")


def run_member_route(owner_kind):
    """Run one member owner route and verify reports, counts, and final state."""
    previous = sc.sc_report_handler.set_actions(sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, sc.SC_ERROR, sc.SC_THROW)
    halt_previous = sc.sc_report_handler.set_actions(sc.SC_ID_HALT_NOT_ALLOWED_, sc.SC_ERROR, sc.SC_THROW)
    model = BoundaryModel(owner_kind)
    sc.sc_start(1, sc.SC_NS)
    expected_520 = 12
    expected_522 = 2 if owner_kind == "module" else 0
    assert model.handle.terminated()
    assert sc.sc_report_handler.get_count(sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_) == expected_520
    assert sc.sc_report_handler.get_count(sc.SC_ID_HALT_NOT_ALLOWED_) == expected_522
    assert sc.sc_time_stamp() == sc.sc_time(1, sc.SC_NS)
    assert sc.sc_get_status() == sc.SC_PAUSED
    final_status = int(sc.sc_get_status())
    records.append(
        f"MEMBER_BOUNDARY_FINAL {owner_kind}|{expected_520}|{expected_522}|"
        f"{sc.sc_time_stamp().value()}|{sc.sc_delta_count()}|{final_status}|1"
    )
    sc.sc_report_handler.set_actions(sc.SC_ID_NEXT_TRIGGER_NOT_ALLOWED_, sc.SC_ERROR, previous)
    sc.sc_report_handler.set_actions(sc.SC_ID_HALT_NOT_ALLOWED_, sc.SC_ERROR, halt_previous)


def run_stop_mode():
    """Require the exact recoverable W547 report and unchanged stop mode."""
    report_type = sc.SC_ID_STOP_MODE_AFTER_START_
    previous = sc.sc_report_handler.set_actions(report_type, sc.SC_ERROR, sc.SC_THROW)
    initial_mode = sc.sc_get_stop_mode()
    assert initial_mode == sc.SC_STOP_FINISH_DELTA
    sc.sc_start(1, sc.SC_NS)
    before = sc.sc_report_handler.get_count(report_type)
    try:
        sc.sc_set_stop_mode(sc.SC_STOP_IMMEDIATE)
    except sc.sc_report as report:
        report_record("stop_mode", "set_after_start", report, 547, report_type, expected_status=sc.SC_PAUSED)
    else:
        raise AssertionError("stop-mode change after start did not report W547")
    assert sc.sc_report_handler.get_count(report_type) - before == 1
    assert sc.sc_get_stop_mode() == initial_mode
    assert sc.sc_time_stamp() == sc.sc_time(1, sc.SC_NS)
    assert sc.sc_get_status() == sc.SC_PAUSED
    records.append(
        f"MEMBER_BOUNDARY_FINAL stop_mode|0|1|{sc.sc_time_stamp().value()}|{sc.sc_delta_count()}|"
        f"{int(sc.sc_get_status())}|{int(sc.sc_get_stop_mode())}"
    )
    sc.sc_report_handler.set_actions(report_type, sc.SC_ERROR, previous)


if scenario == "stop_mode":
    run_stop_mode()
else:
    run_member_route(scenario)

for record in records:
    print(record)
print(f"MEMBER_BOUNDARY_OK {scenario} reports={len(records) // 2}")

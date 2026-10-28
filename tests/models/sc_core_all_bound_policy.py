"""Exercise bounded and unbounded SC_ALL_BOUND ports in a fresh kernel."""

import sys

from amal.eda.systemc import sc_core as sc


def capture_report(report, actions):
    """Copy an elaboration diagnostic while suppressing the default error action."""
    reports.append(
        (
            report.get_id(),
            int(report.get_severity()),
            report.get_msg_type(),
            report.get_msg(),
        )
    )


scenario = sys.argv[1]
cases = {
    "bounded_full": (2, 2),
    "bounded_under": (2, 1),
    "bounded_none": (2, 0),
    "unbounded_one": (0, 1),
    "unbounded_none": (0, 0),
}
try:
    max_size, binding_count = cases[scenario]
except KeyError as error:
    raise SystemExit(f"unknown all-bound scenario: {scenario}") from error

reports = []
name = sc.sc_module_name("all_bound")
top = sc.sc_module(name)
left = sc.sc_signal[int]("left", 17)
right = sc.sc_signal[int]("right", 29)
Port = sc.sc_port[sc.sc_signal_in_if[int], max_size, sc.SC_ALL_BOUND]
port = Port("required")
channels = (left, right)
for index in range(binding_count):
    port.bind(channels[index])
assert port.bind_count() == binding_count
del name
sc.set_report_handler(capture_report)
sc.sc_start(sc.SC_ZERO_TIME)
sc.set_report_handler()
assert sc.sc_get_status() == sc.SC_PAUSED
assert port.size() == binding_count
values = [int(port.get_interface(index).read()) for index in range(binding_count)]
assert values == [17, 29][:binding_count]

if binding_count == max_size and binding_count > 0:
    assert not reports, reports
elif max_size == 0 and binding_count == 1:
    assert not reports, reports
else:
    expected = f"{binding_count} actual binds is less than required {max_size}: port 'all_bound.required' (sc_port)"
    assert reports == [(109, int(sc.SC_ERROR), sc.SC_ID_COMPLETE_BINDING_, expected)], reports

report_details = ";".join(
    f"{report_id};{severity};{message_type};{message}" for report_id, severity, message_type, message in reports
)
print(
    "ALL_BOUND_REFERENCE|"
    f"{scenario}|{max_size}|{port.bind_count()}|{port.size()}|"
    f"{','.join(map(str, values))}|{len(reports)}"
    f"{f'|{report_details}' if report_details else ''}"
)

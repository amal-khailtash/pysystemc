"""Compare stop-after overload scopes, limits and rejection behavior."""

from amal.eda.systemc import sc_core as sc

h = sc.sc_report_handler
records = []


def capture(report, actions):
    """Retain scalar report data while allowing SC_STOP without stopping."""
    if report.get_msg_type().startswith("limit/"):
        records.append((report.get_msg_type(), int(report.get_severity()), int(actions)))


sc.set_report_handler(capture)
labels = ("omitted", "minus_one", "zero", "one", "two")
limits = (-1, -1, 0, 1, 2)
scopes = ("severity", "message", "message_severity")
severity = sc.SC_INFO

for scope in scopes:
    message = f"limit/{scope}"
    expected_previous = -1
    for label, limit in zip(labels, limits, strict=True):
        # initialize resets report counters only; this process never resets the kernel.
        h.initialize()
        if scope == "severity":
            previous = h.stop_after(severity) if label == "omitted" else h.stop_after(severity, limit)
        elif scope == "message":
            previous = h.stop_after(message) if label == "omitted" else h.stop_after(message, limit)
        else:
            previous = h.stop_after(message, severity) if label == "omitted" else h.stop_after(message, severity, limit)
        assert previous == expected_previous, (scope, label, previous, expected_previous)

        for index in range(1, 4):
            h.report(severity, message, "threshold", "report_limit_matrix.py", index)
            actual_type, actual_severity, actions = records[-1]
            assert actual_type == message and actual_severity == int(severity)
            expected_stop = limit > 0 and index >= limit
            assert bool(actions & int(sc.SC_STOP)) == expected_stop, (scope, label, index, actions)
            count = h.get_count(message, severity)
            message_count = h.get_count(message)
            severity_count = h.get_count(severity)
            assert (count, message_count, severity_count) == (index, index, index)
            print(
                "REPORT_LIMIT",
                scope,
                label,
                previous,
                limit,
                index,
                actions,
                count,
                message_count,
                severity_count,
            )
        expected_previous = limit if limit >= 0 else -1
    h.initialize()
    cleared = (
        h.stop_after(severity)
        if scope == "severity"
        else h.stop_after(message)
        if scope == "message"
        else h.stop_after(message, severity)
    )
    assert cleared == 2, (scope, cleared)


h.initialize()
precedence_message = "limit/precedence"
assert h.stop_after(severity, 2) == -1
assert h.stop_after(precedence_message, 3) == -1
assert h.stop_after(precedence_message, severity, 1) == -1
h.report(severity, precedence_message, "combined", "report_limit_matrix.py", 40)
actions = records[-1][2]
assert actions & int(sc.SC_STOP)
print(
    "REPORT_PRECEDENCE",
    "message_severity",
    1,
    actions,
    h.get_count(precedence_message, severity),
    h.get_count(precedence_message),
    h.get_count(severity),
)
assert h.stop_after(precedence_message, severity) == 1

h.initialize()
for index in range(1, 4):
    h.report(severity, precedence_message, "message", "report_limit_matrix.py", 40 + index)
    actions = records[-1][2]
    assert bool(actions & int(sc.SC_STOP)) == (index >= 3), ("message precedence", index, actions)
    print(
        "REPORT_PRECEDENCE",
        "message",
        index,
        actions,
        h.get_count(precedence_message, severity),
        h.get_count(precedence_message),
        h.get_count(severity),
    )
assert h.stop_after(precedence_message) == 3

h.initialize()
for index in range(1, 3):
    h.report(severity, precedence_message, "severity", "report_limit_matrix.py", 50 + index)
    actions = records[-1][2]
    assert bool(actions & int(sc.SC_STOP)) == (index >= 2), ("severity precedence", index, actions)
    print(
        "REPORT_PRECEDENCE",
        "severity",
        index,
        actions,
        h.get_count(precedence_message, severity),
        h.get_count(precedence_message),
        h.get_count(severity),
    )
assert h.stop_after(severity) == 2


# Seed all three selected policy entries and their counters for rejection checks.
h.initialize()
reject_message = "limit/reject"
assert h.stop_after(sc.SC_WARNING, 2) == -1
assert h.stop_after(reject_message, 2) == -1
assert h.stop_after(reject_message, sc.SC_WARNING, 2) == -1
h.report(sc.SC_WARNING, reject_message, "seed", "report_limit_matrix.py", 90)


def rejection_state():
    """Read the three thresholds without changing their configured values."""
    previous = (
        h.stop_after(sc.SC_WARNING, 2),
        h.stop_after(reject_message, 2),
        h.stop_after(reject_message, sc.SC_WARNING, 2),
    )
    assert previous == (2, 2, 2), previous
    return (
        previous,
        h.get_count(sc.SC_WARNING),
        h.get_count(reject_message),
        h.get_count(reject_message, sc.SC_WARNING),
    )


def expect_rejection(label, expected_exception, invoke):
    """Assert a rejected call preserves selected limits and all report counts."""
    before = rejection_state()
    try:
        invoke()
    except expected_exception:
        pass
    except Exception as error:
        raise AssertionError(
            f"{label} raised {type(error).__name__}, expected {expected_exception.__name__}"
        ) from error
    else:
        raise AssertionError(f"{label} was accepted")
    after = rejection_state()
    assert after == before, (label, before, after)
    print("REPORT_REJECT", label, expected_exception.__name__)


bad_int = 1 << 100
for label, invoke in (
    ("no_arguments", lambda: h.stop_after()),
    ("severity_arity", lambda: h.stop_after(sc.SC_WARNING, 2, 3)),
    ("message_arity", lambda: h.stop_after(reject_message, 2, 3)),
    ("message_severity_arity", lambda: h.stop_after(reject_message, sc.SC_WARNING, 2, 3)),
    ("subject_type", lambda: h.stop_after(object(), 2)),
):
    expect_rejection(label, TypeError, invoke)

for label, invoke in (
    ("severity_type", lambda: h.stop_after(sc.SC_WARNING, "bad")),
    ("message_type", lambda: h.stop_after(reject_message, object())),
    ("message_severity_type", lambda: h.stop_after(reject_message, sc.SC_WARNING, "bad")),
):
    expect_rejection(label, TypeError, invoke)

for label, invoke in (
    ("severity_overflow", lambda: h.stop_after(sc.SC_WARNING, bad_int)),
    ("message_overflow", lambda: h.stop_after(reject_message, bad_int)),
    ("message_severity_overflow", lambda: h.stop_after(reject_message, sc.SC_WARNING, bad_int)),
):
    expect_rejection(label, OverflowError, invoke)

sc.set_report_handler()

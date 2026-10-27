"""Reject malformed report overloads without changing native report policy."""

from amal.eda.systemc import sc_core as sc


handler = sc.sc_report_handler
legacy_id = 61137
legacy_message_type = "matrix/arguments/legacy"
seed_message_type = "matrix/arguments/seed"
rejected_message_type = "matrix/arguments/rejected"
recovery_message_type = "matrix/arguments/recovery"
bad_int = 1 << 100
records = []


def capture(report, actions):
    """Record copied callback fields so rejected dispatches are observable."""
    records.append(
        (
            report.get_msg_type(),
            report.get_msg(),
            report.get_file_name(),
            report.get_line_number(),
            report.get_verbosity(),
            int(report.get_severity()),
            int(actions),
        )
    )


def report_string(
    severity=sc.SC_WARNING, message_type=rejected_message_type, message="probe", file="arguments.py", line=10
):
    """Call the message-string report signature with selectable fields."""
    handler.report(severity, message_type, message, file, line)


def report_verbosity(
    severity=sc.SC_WARNING,
    message_type=rejected_message_type,
    message="probe",
    verbosity=sc.SC_LOW,
    file="arguments.py",
    line=10,
):
    """Call the explicit-verbosity report signature with selectable fields."""
    handler.report(severity, message_type, message, verbosity, file, line)


def report_legacy(severity=sc.SC_WARNING, identifier=legacy_id, message="probe", file="arguments.py", line=10):
    """Call the legacy-identifier report signature with selectable fields."""
    handler.report(severity, identifier, message, file, line)


def report_state():
    """Return severity, message and legacy-related counters plus the cached report."""
    cached = handler.get_cached_report()
    cached_fields = None
    if cached:
        cached_fields = (
            cached.get_id(),
            int(cached.get_severity()),
            cached.get_msg_type(),
            cached.get_msg(),
            cached.get_file_name(),
            cached.get_line_number(),
            cached.get_verbosity(),
        )
    return (
        tuple(handler.get_count(severity) for severity in (sc.SC_INFO, sc.SC_WARNING, sc.SC_ERROR, sc.SC_FATAL)),
        handler.get_count(seed_message_type),
        handler.get_count(rejected_message_type),
        handler.get_count(legacy_message_type),
        handler.get_count(legacy_message_type, sc.SC_WARNING),
        cached_fields,
        handler.stop_after(sc.SC_WARNING, 2),
        handler.stop_after(rejected_message_type, sc.SC_WARNING, 2),
    )


sc.sc_report.register_id(legacy_id, legacy_message_type)
handler.clear_cached_report()
assert handler.stop_after(sc.SC_WARNING, 2) == -1
assert handler.stop_after(rejected_message_type, sc.SC_WARNING, 2) == -1
assert handler.set_actions(seed_message_type, sc.SC_WARNING, sc.SC_CACHE_REPORT | sc.SC_DO_NOTHING) == sc.SC_UNSPECIFIED
handler.report(sc.SC_WARNING, seed_message_type, "retained seed", "seed.cpp", 9)
seed_cache = report_state()[-3]
assert seed_cache == (
    -1,
    int(sc.SC_WARNING),
    seed_message_type,
    "retained seed",
    "seed.cpp",
    9,
    sc.SC_MEDIUM,
)
sc.set_report_handler(capture)


def expect_rejection(label, expected_exception, invoke):
    """Require one malformed call to preserve all tracked report state."""
    before = report_state()
    try:
        invoke()
    except expected_exception as error:
        assert type(error) is expected_exception, (label, type(error).__name__, expected_exception.__name__)
    except Exception as error:
        raise AssertionError(
            f"{label} raised {type(error).__name__}, expected {expected_exception.__name__}"
        ) from error
    else:
        raise AssertionError(f"{label} was accepted")
    after = report_state()
    assert after == before, (label, before, after)
    assert not records, (label, records)


rejections = (
    (
        "wrong_arity_string",
        TypeError,
        lambda: handler.report(sc.SC_WARNING, rejected_message_type, "probe", "arguments.py"),
    ),
    (
        "wrong_arity_verbosity",
        TypeError,
        lambda: handler.report(sc.SC_WARNING, rejected_message_type, "probe", 1, "arguments.py", 10, "extra"),
    ),
    ("wrong_arity_legacy", TypeError, lambda: handler.report(sc.SC_WARNING, legacy_id, "probe", "arguments.py")),
    ("object_severity_string", TypeError, lambda: report_string(severity=object())),
    ("object_severity_verbosity", TypeError, lambda: report_verbosity(severity=object())),
    ("object_severity_legacy", TypeError, lambda: report_legacy(severity=object())),
    ("object_message_type_string", TypeError, lambda: report_string(message_type=object())),
    ("object_message_type_verbosity", TypeError, lambda: report_verbosity(message_type=object())),
    ("object_message_type_legacy", TypeError, lambda: report_legacy(identifier=object())),
    ("object_message_string", TypeError, lambda: report_string(message=object())),
    ("object_message_verbosity", TypeError, lambda: report_verbosity(message=object())),
    ("object_message_legacy", TypeError, lambda: report_legacy(message=object())),
    ("object_file_string", TypeError, lambda: report_string(file=object())),
    ("object_file_verbosity", TypeError, lambda: report_verbosity(file=object())),
    ("object_file_legacy", TypeError, lambda: report_legacy(file=object())),
    ("object_line_string", TypeError, lambda: report_string(line=object())),
    ("object_line_verbosity", TypeError, lambda: report_verbosity(line=object())),
    ("object_line_legacy", TypeError, lambda: report_legacy(line=object())),
    ("overflow_id", OverflowError, lambda: report_legacy(identifier=bad_int)),
    ("overflow_line_string", OverflowError, lambda: report_string(line=bad_int)),
    ("overflow_line_verbosity", OverflowError, lambda: report_verbosity(line=bad_int)),
    ("overflow_line_legacy", OverflowError, lambda: report_legacy(line=bad_int)),
    ("overflow_verbosity", OverflowError, lambda: report_verbosity(verbosity=bad_int)),
)

for rejection_label, exception_type, call in rejections:
    expect_rejection(rejection_label, exception_type, call)

recovery_before = report_state()
handler.set_actions(recovery_message_type, sc.SC_INFO, sc.SC_CACHE_REPORT)
report_verbosity(
    severity=sc.SC_INFO,
    message_type=recovery_message_type,
    message="recovered after malformed calls",
    verbosity=-1234567,
    file="recovery.py",
    line=72,
)
recovery_after = report_state()
assert len(records) == 1, records
assert records[0][:6] == (
    recovery_message_type,
    "recovered after malformed calls",
    "recovery.py",
    72,
    -1234567,
    int(sc.SC_INFO),
), records[0]
assert recovery_after[0] == (
    recovery_before[0][0] + 1,
    recovery_before[0][1],
    recovery_before[0][2],
    recovery_before[0][3],
), (recovery_before, recovery_after)
assert recovery_after[3] == recovery_before[3]
assert handler.get_count(recovery_message_type) == 1
assert handler.get_count(recovery_message_type, sc.SC_INFO) == 1
assert handler.get_count(legacy_message_type) == 0
assert recovery_after[-3] == (
    -1,
    int(sc.SC_INFO),
    recovery_message_type,
    "recovered after malformed calls",
    "recovery.py",
    72,
    -1234567,
)
sc.set_report_handler()
print(f"REPORT_ARGUMENT_REJECTIONS {len(rejections)}")
print("REPORT_ARGUMENT_RECOVERY", records[0][0], records[0][4], len(records), handler.get_count(recovery_message_type))

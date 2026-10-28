"""Compare report action precedence, masks and thrown error reports."""

from amal.eda.systemc import sc_core as sc

h = sc.sc_report_handler
records = []


def capture(report, actions):
    """Record primitive callback values without retaining the borrowed report."""
    if report.get_msg_type().startswith("matrix/"):
        records.append((report.get_msg_type(), report.get_msg(), int(actions), int(report.get_severity())))


h.initialize()
old_severity_actions = h.set_actions(sc.SC_WARNING, sc.SC_DISPLAY | sc.SC_LOG)
old_message_actions = h.set_actions("matrix/policy", sc.SC_LOG)
old_specific_actions = h.set_actions("matrix/policy", sc.SC_WARNING, sc.SC_CACHE_REPORT)
assert old_severity_actions == sc.SC_DEFAULT_WARNING_ACTIONS
assert old_message_actions == old_specific_actions == sc.SC_UNSPECIFIED
sc.set_report_handler(capture)
h.report(sc.SC_WARNING, "matrix/policy", "specific", "error_matrix.cpp", 31)
assert h.get_cached_report().get_msg() == "specific"
h.report(sc.SC_INFO, "matrix/policy", "message", "error_matrix.cpp", 32)
assert h.get_count("matrix/policy") == 2
assert h.get_count("matrix/policy", sc.SC_WARNING) == h.get_count(sc.SC_WARNING) == 1
assert h.get_count(sc.SC_INFO) == 1

old_suppress = h.suppress(sc.SC_DISPLAY | sc.SC_LOG)
old_force = h.force(sc.SC_THROW)
assert old_suppress == old_force == sc.SC_UNSPECIFIED
h.report(sc.SC_INFO, "matrix/policy", "masked", "error_matrix.cpp", 33)
assert h.suppress() == sc.SC_DISPLAY | sc.SC_LOG
assert h.force() == sc.SC_THROW
assert h.get_count("matrix/policy") == 3
assert records[-1][2] == sc.SC_THROW
h.clear_cached_report()
assert not h.get_cached_report()
assert h.stop_after("matrix/zero", sc.SC_ERROR, 0) == -1
sc.set_report_handler(capture)
h.report(sc.SC_ERROR, "matrix/zero", "zero", "error_matrix.cpp", 34)
h.report(sc.SC_ERROR, "matrix/zero", "zero", "error_matrix.cpp", 35)
sc.set_report_handler()
assert h.stop_after("matrix/zero", sc.SC_ERROR) == 0
assert h.get_count("matrix/zero") == h.get_count("matrix/zero", sc.SC_ERROR) == 2
assert not any(row[2] & sc.SC_STOP for row in records[-2:])
print("REPORT_STOP_ZERO", h.get_count("matrix/zero"), 0)
sc.set_report_handler()
h.set_actions(sc.SC_WARNING, old_severity_actions)

error_actions = sc.SC_THROW | sc.SC_CACHE_REPORT


def check_thrown_report(label, message_type, report_id, verbosity, line, invoke, expected_count):
    """Check one direct overload's throw, metadata, count and independent cache copy."""
    old_actions = h.set_actions(message_type, sc.SC_ERROR, error_actions)
    assert old_actions == sc.SC_UNSPECIFIED
    try:
        invoke()
    except sc.sc_report as report:
        assert report.get_severity() == sc.SC_ERROR
        assert report.get_msg_type() == message_type
        assert report.get_id() == report_id
        assert report.get_msg() == label
        assert report.get_file_name() == "error_matrix.cpp" and report.get_line_number() == line
        assert report.get_verbosity() == verbosity
    else:
        raise AssertionError(f"SC_THROW error report returned normally: {label}")
    assert h.get_count(message_type) == h.get_count(message_type, sc.SC_ERROR) == 1
    assert h.get_count(sc.SC_ERROR) == expected_count
    cached = h.get_cached_report()
    assert cached and cached.get_msg() == label
    copy = sc.sc_report(cached)
    h.clear_cached_report()
    assert not h.get_cached_report() and copy.get_msg() == label
    print("REPORT_ERROR_THROW", label, message_type, report_id, verbosity, h.get_count(sc.SC_ERROR))


check_thrown_report(
    "message",
    "matrix/error_message",
    -1,
    sc.SC_MEDIUM,
    41,
    lambda: h.report(sc.SC_ERROR, "matrix/error_message", "message", "error_matrix.cpp", 41),
    3,
)
check_thrown_report(
    "verbosity",
    "matrix/error_verbosity",
    -1,
    sc.SC_HIGH,
    42,
    lambda: h.report(sc.SC_ERROR, "matrix/error_verbosity", "verbosity", sc.SC_HIGH, "error_matrix.cpp", 42),
    4,
)
sc.sc_report.register_id(61011, "matrix/error_legacy")
check_thrown_report(
    "legacy",
    "matrix/error_legacy",
    61011,
    sc.SC_MEDIUM,
    43,
    lambda: h.report(sc.SC_ERROR, 61011, "legacy", "error_matrix.cpp", 43),
    5,
)

callback_calls = []


def fail_once(report, actions):
    """Raise a Python exception once while preserving report callback evidence."""
    callback_calls.append((report.get_msg_type(), report.get_msg(), int(actions)))
    raise ValueError("report_callback_matrix_marker")


h.set_actions("matrix/callback", sc.SC_ERROR, sc.SC_CACHE_REPORT)
sc.set_report_handler(fail_once)
try:
    h.report(sc.SC_ERROR, "matrix/callback", "callback", "error_matrix.cpp", 44)
except ValueError as error:
    assert type(error) is ValueError and "report_callback_matrix_marker" in str(error)
else:
    raise AssertionError("Python report callback exception did not propagate")
finally:
    sc.set_report_handler()
assert callback_calls == [("matrix/callback", "callback", int(sc.SC_CACHE_REPORT))]
assert h.get_count("matrix/callback") == h.get_count("matrix/callback", sc.SC_ERROR) == 1
assert h.get_count(sc.SC_ERROR) == 6
print("REPORT_CALLBACK_ERROR", callback_calls[0][0], callback_calls[0][1], callback_calls[0][2], len(callback_calls))

sc.set_report_handler(capture)
h.report(sc.SC_ERROR, "matrix/error_action", "action", "error_matrix.cpp", 45)
sc.set_report_handler()
assert records[-1][2] == sc.SC_DEFAULT_ERROR_ACTIONS

for row in records:
    print("REPORT_ERROR_ACTION", *row)
print(
    "REPORT_ERROR_COUNTS",
    h.get_count("matrix/policy"),
    h.get_count(sc.SC_WARNING),
    h.get_count(sc.SC_INFO),
    h.get_count(sc.SC_ERROR),
)

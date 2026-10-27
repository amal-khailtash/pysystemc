"""Validate every legacy sc_report control and explicit report-copy destruction."""

from amal.eda.systemc import sc_core as sc
import cppyy

records = []
owned = []


def capture(report, actions):
    """Copy legacy reports and retain the native action/severity evidence."""
    if report.get_msg_type().startswith("legacy/") or report.get_msg_type() == "unknown id":
        owned.append(sc.sc_report(report))
        records.append((report.get_id(), int(report.get_severity()), int(actions)))


sc.sc_report.register_id(61001, "legacy/first")
sc.sc_report.register_id(61002, "legacy/second")
sc.sc_report.register_id(61001, "legacy/first")
assert sc.sc_report.get_message(61001) == "legacy/first"
assert sc.sc_report.get_message(61999) == "unknown id"
assert not sc.sc_report.is_suppressed(61999)
sc.sc_report.suppress_id(61999, True)
for label, arguments in (
    ("negative", (-1, "bad")),
    ("conflict", (61001, "conflict")),
    ("null_message", (61003, cppyy.nullptr)),
):
    before = sc.sc_report_handler.get_count(sc.SC_ID_REGISTER_ID_FAILED_)
    try:
        sc.sc_report.register_id(*arguments)
    except sc.sc_report as error:
        count = sc.sc_report_handler.get_count(sc.SC_ID_REGISTER_ID_FAILED_) - before
        assert count == 1
        assert error.get_msg_type() == sc.SC_ID_REGISTER_ID_FAILED_
        assert error.get_severity() == sc.SC_ERROR
        assert error.get_time() == sc.SC_ZERO_TIME
        print(
            f"REPORT_REGISTER_ERROR {label}|{error.get_id()}|{int(error.get_severity())}|"
            f"{error.get_msg_type()}|{error.get_msg()}|{count}"
        )
    else:
        raise AssertionError("invalid ID registration accepted")
assert sc.sc_report.get_message(61001) == "legacy/first"
assert sc.sc_report.get_message(61003) == "unknown id"
sc.sc_report.register_id(61004, "")
# The empty message already has a predefined native ID; registration is a no-op.
assert sc.sc_report.get_message(61004) == "unknown id"
print("REPORT_REGISTER_EMPTY unknown id")
before = sc.sc_report_handler.get_count(sc.SC_ID_REGISTER_ID_FAILED_)
try:
    sc.sc_report.register_id(object(), "bad")
except TypeError:
    pass
else:
    raise AssertionError("invalid ID conversion accepted")
assert sc.sc_report_handler.get_count(sc.SC_ID_REGISTER_ID_FAILED_) == before
sc.set_report_handler(capture)
sc.sc_report.suppress_id(61001, True)
assert sc.sc_report.is_suppressed(61001)
sc.sc_report_handler.report(sc.SC_INFO, 61001, "suppressed", "legacy.cpp", 9)
sc.sc_report.suppress_id(61001, False)
assert not sc.sc_report.is_suppressed(61001)
sc.sc_report.suppress_infos(True)
sc.sc_report_handler.report(sc.SC_INFO, 61001, "info", "legacy.cpp", 9)
sc.sc_report.suppress_infos(False)
sc.sc_report_handler.report(sc.SC_INFO, 61001, "normal", "legacy.cpp", 9)
sc.sc_report.suppress_warnings(True)
sc.sc_report_handler.report(sc.SC_WARNING, 61002, "warning", "legacy.cpp", 9)
sc.sc_report.suppress_warnings(False)
sc.sc_report.make_warnings_errors(True)
sc.sc_report_handler.report(sc.SC_WARNING, 61002, "promoted", "legacy.cpp", 9)
sc.sc_report.make_warnings_errors(False)
sc.sc_report_handler.report(sc.SC_WARNING, 61002, "normal", "legacy.cpp", 9)
sc.sc_report_handler.report(sc.SC_INFO, 61999, "missing", "legacy.cpp", 9)
sc.set_report_handler()
assert records[0][2] == records[1][2] == records[3][2] == sc.SC_DO_NOTHING
assert records[2][2] == sc.SC_DEFAULT_INFO_ACTIONS
assert records[4][1] == sc.SC_ERROR and records[4][2] & sc.SC_THROW
assert records[5][1] == sc.SC_WARNING
left, right = owned[0], owned[4]
getattr(left, "__assign__")(right)
assert left.get_msg() == "promoted" and left.get_severity() == sc.SC_ERROR
getattr(left, "__assign__")(left)
assert left.get_msg() == "promoted"
copy = sc.sc_report(left)
copy.__destruct__()
assert not copy
for row in records:
    print("REPORT_LEGACY_REFERENCE", *row)
print("REPORT_ASSIGN_REFERENCE promoted 2")

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
for arguments in ((-1, "bad"), (61001, "conflict"), (61003, cppyy.nullptr)):
    try:
        sc.sc_report.register_id(*arguments)
    except Exception as error:
        assert "register" in str(error)
    else:
        raise AssertionError("invalid ID registration accepted")
assert sc.sc_report.get_message(61001) == "legacy/first"
try:
    sc.sc_report.register_id(object(), "bad")
except TypeError:
    pass
else:
    raise AssertionError("invalid ID conversion accepted")
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

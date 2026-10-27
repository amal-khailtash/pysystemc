"""Exercise error routes through module and primitive-channel member waits."""

import sys

from amal.eda.systemc import sc_core as sc


owner_kind, scenario = sys.argv[1:]
module_name = sc.sc_module_name("member_wait_error")
module = sc.sc_module(module_name)
del module_name
channel = sc.sc_prim_channel("member_wait_channel")
owner = module if owner_kind == "module" else channel


def check_host_type_error():
    """Reject a malformed wait argument before the kernel enters simulation."""
    try:
        owner.wait(object())
    except TypeError:
        pass
    else:
        raise AssertionError("member wait accepted a non-native argument")
    assert sc.sc_get_status() == sc.SC_ELABORATION
    assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
    assert sc.sc_delta_count() == 0
    print(f"MEMBER_WAIT_TYPE_ERROR_OK {owner_kind} status={int(sc.sc_get_status())}")


def check_method_process_error():
    """Preserve the native wait-not-allowed report from an SC_METHOD callback."""
    event = sc.sc_event("trigger")
    attempts = []
    completed = []
    reports = []

    def invalid_wait():
        """Call the selected owner's native wait member from method context."""
        attempts.append((sc.sc_time_stamp().value(), sc.sc_delta_count()))
        try:
            owner.wait(sc.sc_time(1, sc.SC_NS))
        except sc.sc_report as report:
            reports.append((report.get_id(), report.get_msg_type(), report.get_severity(), report.get_msg()))
        else:
            raise AssertionError("wait from an SC_METHOD unexpectedly succeeded")
        completed.append(True)

    module.sc_method("invalid_wait", invalid_wait)
    module.sensitive(event)
    module.dont_initialize()
    sc.sc_report_handler.set_actions(sc.SC_ID_WAIT_NOT_ALLOWED_, sc.SC_ERROR, sc.SC_THROW)
    event.notify(sc.SC_ZERO_TIME)
    sc.sc_start(1, sc.SC_NS)
    assert attempts == [(0, 0)]
    assert len(reports) == 1
    report_id, report_type, severity, message = reports[0]
    assert report_id == 519
    assert report_type == sc.SC_ID_WAIT_NOT_ALLOWED_
    assert severity == sc.SC_ERROR
    assert "SC_METHOD" in message and "next_trigger()" in message
    assert completed == [True]
    print(f"MEMBER_WAIT_METHOD_ERROR_OK {owner_kind} report={report_id} attempts={len(attempts)} completed=1")


if scenario == "type":
    check_host_type_error()
elif scenario == "method":
    check_method_process_error()
else:
    raise SystemExit(f"unknown error scenario: {scenario}")

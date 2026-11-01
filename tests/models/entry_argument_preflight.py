"""Verify public main rejects malformed argv before native entry and recovers."""

from contextlib import redirect_stderr
from io import StringIO
import sys

from amal.eda.systemc import main, sc_core as sc
import cppyy


def check_preflight_failure(bad_argv, diagnostic):
    """Assert one malformed process argv exits before any native entry call."""
    before_status = sc.sc_get_status()
    before_time = sc.sc_time_stamp()
    assert before_status == sc.SC_ELABORATION
    assert not cppyy.gbl.sc_core.sc_in_action
    calls = []

    def callback(arguments):
        """Record an accidental callback if malformed argv reaches sc_main."""
        calls.append(arguments)
        return 0

    sys.modules["__main__"].sc_main = callback
    sys.argv = list(bad_argv)
    stderr = StringIO()
    try:
        with redirect_stderr(stderr):
            main()
    except SystemExit as error:
        assert error.code == 1, error.code
    else:
        raise AssertionError("public main did not exit for malformed argv")

    rendered = stderr.getvalue()
    assert "Exception while executing sc_main:" in rendered, rendered
    assert diagnostic in rendered, rendered
    assert calls == [], calls
    assert sc.sc_get_status() == before_status
    assert sc.sc_time_stamp() == before_time
    assert not cppyy.gbl.sc_core.sc_in_action


scenario = sys.argv[1]
script_path = sys.argv[0]
if scenario == "nonstring":
    check_preflight_failure([script_path, 17], "argv entries must be strings")
elif scenario == "nul":
    check_preflight_failure([script_path, "bad\0argument"], "argv entries must not contain NUL")
else:
    raise SystemExit(f"unknown entry preflight scenario: {scenario}")

expected_argv = [script_path, "café", "--value=17"]
sys.argv = list(expected_argv)
entry_records = []


def sc_main(arguments):
    """Capture the successful native callback and borrowed argv snapshot."""
    snapshot = sc.sc_argv()
    record = (list(arguments), sys.argv.copy(), snapshot)
    entry_records.append(record)
    assert arguments == expected_argv
    assert sys.argv == expected_argv
    assert snapshot == tuple(expected_argv)
    assert sc.sc_argc() == len(expected_argv)
    return 0


main()
assert entry_records == [(expected_argv, expected_argv, tuple(expected_argv))], entry_records
assert not cppyy.gbl.sc_core.sc_in_action
assert sc.sc_get_status() == sc.SC_ELABORATION
print(f"ENTRY_ARGUMENT_PREFLIGHT_OK scenario={scenario}")

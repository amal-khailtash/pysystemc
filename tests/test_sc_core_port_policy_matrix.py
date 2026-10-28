"""Compare every port policy binding boundary against independent native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_port_policy_matrix(tmp_path):
    """Check all three policies with bounded/unlimited ports and exact report diagnostics."""
    output = run_example(tmp_path, "tests/models/sc_core_port_policy_matrix.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_port_policy_matrix.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("PORT_POLICY|")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("PORT_POLICY|")]
    assert len(actual) == 21 and actual == expected
    for index, record in enumerate(actual):
        prefix, label, maximum, count, size, values, report_count, message = record.split("|", 7)
        maximum, count = int(maximum), int(count)
        assert int(size) == count
        assert values == ",".join(map(str, (17, 29, 43)[:count]))
        reason = ""
        if maximum and count > maximum:
            reason = f"{count} binds exceeds maximum of {maximum} allowed"
        elif label == "one" and count == 0:
            reason = "port not bound"
        elif label == "all" and (count == 0 or maximum and count < maximum):
            reason = f"{count} actual binds is less than required {maximum}"
        assert int(report_count) == int(bool(reason))
        suffix = f": port 'port_policy_{index}.required' (sc_port)"
        expected_message = reason + suffix if reason else ""
        assert message == expected_message
    assert "PORT_POLICY_DONE cases=21 reports=8 time=0" in output
    assert "PORT_POLICY_DONE cases=21 reports=8 time=0" in native.stdout

"""Compare report-handler utility contracts with an independent native model."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_report_policy_utilities(tmp_path):
    """Verify filename clearing, suppression and native action-bit exhaustion."""
    output = run_example(tmp_path, "tests/models/sc_core_report_policy_utilities.py")
    native_dir = tmp_path / "native"
    native_dir.mkdir()
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_report_policy_utilities.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = (
        "REPORT_POLICY_LOG",
        "REPORT_POLICY_ABSENT_ID",
        "REPORT_POLICY_NEW_ACTION",
        "REPORT_POLICY_ACTION_EXHAUSTION",
    )
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert actual == expected
    assert len(actual) == 4

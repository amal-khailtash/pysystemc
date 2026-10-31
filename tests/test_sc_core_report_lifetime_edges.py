"""Compare report assignment and borrowed timestamp lifetime edges."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_report_lifetime_edges(tmp_path):
    """Verify report timestamp identity, assignment invalidation, and swap transfer."""
    output = run_example(tmp_path, "tests/models/sc_core_report_lifetime_edges.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_report_lifetime_edges.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = (
        "REPORT_TIME_REFERENCE",
        "REPORT_TIME_ASSIGN",
        "REPORT_TIME_SELF",
        "REPORT_TIME_CLASS_ASSIGN",
        "REPORT_TIME_SWAP",
    )
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert actual == expected
    assert len(actual) == len(prefixes)

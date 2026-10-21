"""Compare public explicit entry calls with native argument and status semantics."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_explicit_entry(tmp_path):
    """Verify pre-entry validation, count slicing, immutable originals and status."""
    output = run_example(tmp_path, "tests/models/sc_core_explicit_entry.py")
    native = subprocess.run(
        [
            sys.executable,
            "-m",
            "amal.eda.systemc._runner",
            str(ROOT / "tests/native/sc_core_explicit_entry.cpp"),
            "--",
            "café",
            "--flag",
            "",
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 7, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("EXPLICIT_ENTRY_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("EXPLICIT_ENTRY_REFERENCE")]
    assert len(actual) == 1
    assert actual == expected


def test_sc_core_zero_entry(tmp_path):
    """Verify zero-count entry passes an empty list and immutable empty snapshot."""
    assert "ZERO_ENTRY_REFERENCE 0" in run_example(tmp_path, "tests/models/sc_core_explicit_entry.py", ("zero",))

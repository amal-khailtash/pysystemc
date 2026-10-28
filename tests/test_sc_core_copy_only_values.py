"""Check structural callback copies against native copy-only value semantics."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_copy_only_callback_values(tmp_path):
    """Check eight interface views, independent storage and rejected native copies."""
    output = run_example(tmp_path, "tests/models/sc_core_copy_only_values.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_copy_only_values.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    records = [line for line in output.splitlines() if line.startswith(("COPY_ONLY_VALUE ", "COPY_ONLY_FAILURE "))]
    expected = [
        line for line in native.stdout.splitlines() if line.startswith(("COPY_ONLY_VALUE ", "COPY_ONLY_FAILURE "))
    ]
    assert len(records) == len(expected) == 9
    assert records == expected
    assert "COPY_ONLY_FAILURE 1 999 0" in output
    assert "COPY_ONLY_CALLBACK_RELEASE_OK" in output

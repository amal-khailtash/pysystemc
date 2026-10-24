"""Compare process descendant controls and teardown to native SystemC."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize("mode", ("no_descendants", "include_descendants"))
def test_sc_core_process_descendants(tmp_path, mode):
    """Assert exact root/child event counts, times and cleanup for both policies."""
    output = run_example(tmp_path, "tests/models/sc_core_process_descendants.py", (mode,))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_process_descendants.cpp"),
            "--",
            mode,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("DESCENDANT_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("DESCENDANT_REFERENCE")]
    assert actual == expected

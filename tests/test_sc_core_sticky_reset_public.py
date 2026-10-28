"""Compare sticky synchronous reset timing, descendant policy and cleanup."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize("scenario", ("default", "descendants"))
def test_sc_core_sticky_reset_public(tmp_path, scenario):
    """Match every native reset entry, successful return and stack-cleanup marker."""
    output = run_example(tmp_path, "tests/models/sc_core_sticky_reset_public.py", (scenario,))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_sticky_reset_public.cpp"),
            "--",
            scenario,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("STICKY_RESET ")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("STICKY_RESET ")]
    assert actual == expected
    assert len(actual) == (14 if scenario == "descendants" else 7)

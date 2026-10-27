"""Verify guarded module/channel member waits against native SystemC."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize("owner", ("module", "channel", "namespace"))
@pytest.mark.parametrize("mode", ("normal", "reset"))
def test_sc_core_member_waits(tmp_path, owner, mode):
    """Match all 13 overload timings and reset/kill cleanup exactly to native."""
    output = run_example(tmp_path, "tests/models/sc_core_member_waits.py", (owner, mode))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_member_waits.cpp"),
            "--",
            owner,
            mode,
        ],
        cwd=tmp_path,
        text=True,
        capture_output=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefix = ("MEMBER_WAIT ", "MEMBER_CLEANUP ")
    assert [line for line in output.splitlines() if line.startswith(prefix)] == [
        line for line in native.stdout.splitlines() if line.startswith(prefix)
    ]
    assert f"MEMBER_WAIT_OK {owner} {mode} 13" in output

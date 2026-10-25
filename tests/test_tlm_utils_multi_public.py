"""Verify exact multi-socket callbacks and both native hierarchy binding forms."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize("mode", ("bind", "call"))
def test_multi_socket_public_matches_native(tmp_path, mode):
    """Compare two indexed connections, native callbacks, aliases and lifecycle."""
    output = run_example(tmp_path, "tests/models/tlm_utils_multi_public.py", (mode,))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_utils_multi_public.cpp"),
            "--",
            mode,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("MULTI_SOCKET_EVENT ")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("MULTI_SOCKET_EVENT ")]
    assert len(actual) == len(expected) == 20
    assert actual == expected
    for marker in ("MULTI_SOCKET_LIFETIME 2 2", "MULTI_SOCKET_TIME 3 ns", "TLM_UTILS_MULTI_PUBLIC_OK"):
        assert marker in output
        assert marker in native.stdout

"""Compare timeout and sampled-edge helpers with an independent native run."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_edge_waits_public(tmp_path):
    """Verify timeout values, edge overload order/times, and reset/kill cleanup."""
    output = run_example(tmp_path, "tests/models/sc_core_edge_waits_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_edge_waits_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = ("EDGE_WAIT_REFERENCE", "TIMED_OUT_REFERENCE", "EDGE_CLEANUP_REFERENCE")
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert actual == expected
    assert actual == [
        "EDGE_WAIT_REFERENCE pos_logic@2000 pos_bool@2000 neg_bool@4000 neg_logic@4000",
        "TIMED_OUT_REFERENCE early=0/0@2000 late=1/1@3000",
        "EDGE_CLEANUP_REFERENCE reset=500,1000 kill=1000",
    ]
    assert "TIMED_OUT_CONTEXT_GUARD timed_out requires a running SystemC process" in output.splitlines()

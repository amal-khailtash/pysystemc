"""Compare sc_ppq base and typed pointer families with a native reference."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_priority_queue_public(tmp_path):
    """Verify queue ordering, capacity growth, callback lifetime and pointer ownership."""
    expected = (
        "SC_PPQ_REFERENCE base=5,4,3,1,1 typed=8,8,5,3,2 time=1,2,2,4 event_timed=null protected=3,8,9 borrowed=1"
    )
    output = run_example(tmp_path, "tests/models/sc_core_priority_queue_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_priority_queue_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert expected in output
    assert expected in native.stdout

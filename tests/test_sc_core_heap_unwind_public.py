"""Compare public heap, time parameter and unwind probes with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_heap_unwind_public(tmp_path):
    """Verify exact public values and lifecycle behavior in independent processes."""
    output = run_example(tmp_path, "tests/models/sc_core_heap_unwind_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_heap_unwind_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = (
        "HEAP_DEFAULT_REFERENCE",
        "HEAP_REFERENCE",
        "HEAP_RESIZE_REFERENCE",
        "HEAP_DESTROY_REFERENCE",
        "TIME_PARAMS_REFERENCE",
        "TIME_PARAMS_DESTROY_REFERENCE",
        "UNWIND_REFERENCE",
    )
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert len(actual) == 8 and sorted(actual) == sorted(expected), (actual, expected)

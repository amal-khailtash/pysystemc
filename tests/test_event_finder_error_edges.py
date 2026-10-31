"""Compare event finder validation edges and callback semantics with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_event_finder_error_edges(tmp_path):
    """Check inherited finder lookup, cache recovery, and callback parity."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    output = run_example(python_dir, "tests/models/event_finder_error_edges.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/event_finder_error_edges.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    actual_reference = [line for line in output.splitlines() if line.startswith("FINDER_EDGE_REFERENCE")]
    expected_reference = [line for line in native.stdout.splitlines() if line.startswith("FINDER_EDGE_REFERENCE")]
    assert len(actual_reference) == 1 and actual_reference == expected_reference
    assert [line for line in output.splitlines() if line.startswith("FINDER_EDGE_VALIDATION")] == [
        "FINDER_EDGE_VALIDATION constructor_errors=5 cache_errors=4 empty_after_errors=1 "
        "usable_after_errors=1 call_spelling=1 metadata=1 direct_owned=1 cache_borrowed=1"
    ]

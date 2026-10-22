"""Compare the complete public time tuple family with independent C++."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_time_tuple(tmp_path):
    """Verify every public constructor/getter/conversion across normalized units."""
    output = run_example(tmp_path, "tests/models/sc_core_time_tuple.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_time_tuple.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("TIME_TUPLE_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("TIME_TUPLE_REFERENCE")]
    assert len(actual) == 5 and actual == expected


def test_sc_core_time_tuple_overflow(tmp_path):
    """Verify normalization overflow reports and safe value recovery at 10 fs."""
    output = run_example(tmp_path, "tests/models/sc_core_time_tuple.py", ("overflow",))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_time_tuple.cpp"),
            "--",
            "overflow",
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "TIME_TUPLE_OVERFLOW_REFERENCE 0 10" in output
    assert "TIME_TUPLE_OVERFLOW_REFERENCE 0 10" in native.stdout

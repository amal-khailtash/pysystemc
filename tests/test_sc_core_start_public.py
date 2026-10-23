"""Compare each public ``sc_start`` overload with an independent native kernel."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize(
    "scenario",
    (
        "noarg",
        "time_run",
        "time_exit",
        "int_run",
        "int_exit",
        "double_run",
        "double_exit",
    ),
)
def test_sc_core_start_overloads(tmp_path, scenario):
    """Check callbacks on either side of the boundary and final time/delta/status."""
    output = run_example(tmp_path, "tests/models/sc_core_start_public.py", (scenario,))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_start_public.cpp"),
            "--",
            scenario,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("START_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("START_")]
    assert actual == expected
    callback_count = 2 if scenario.endswith("run") or scenario == "noarg" else 1
    assert len(actual) == callback_count + 1


def test_sc_core_start_callback_error_is_not_retried(tmp_path):
    """Preserve native report fields when a timed callback throws inside sc_start."""
    output = run_example(tmp_path, "tests/models/sc_core_start_public.py", ("error",))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_start_public.cpp"),
            "--",
            "error",
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("START_ERROR")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("START_ERROR")]
    assert len(actual) == len(expected) == 1
    assert actual == expected
    fields = actual[0].split("\t")
    assert fields[1:3] == ["549", "uncaught exception"]
    assert fields[3] == "2" and fields[4] == "1000"

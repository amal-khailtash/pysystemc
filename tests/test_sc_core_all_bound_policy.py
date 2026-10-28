"""Compare SC_ALL_BOUND elaboration rules and diagnostics with native SystemC."""

import os
import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize(
    "scenario,max_size,bindings,values,diagnostic",
    (
        ("bounded_full", 2, 2, "17,29", None),
        (
            "bounded_under",
            2,
            1,
            "17",
            "1 actual binds is less than required 2: port 'all_bound.required' (sc_port)",
        ),
        (
            "bounded_none",
            2,
            0,
            "",
            "0 actual binds is less than required 2: port 'all_bound.required' (sc_port)",
        ),
        ("unbounded_one", 0, 1, "17", None),
        (
            "unbounded_none",
            0,
            0,
            "",
            "0 actual binds is less than required 0: port 'all_bound.required' (sc_port)",
        ),
    ),
)
def test_sc_core_all_bound_policy(tmp_path, scenario, max_size, bindings, values, diagnostic):
    """Assert binding counts, values, and report identity for each policy case."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/sc_core_all_bound_policy.py", (scenario,))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_all_bound_policy.cpp"),
            "--",
            scenario,
        ],
        cwd=native_dir,
        env=dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc"))),
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    marker = "ALL_BOUND_REFERENCE|"
    python_records = [line for line in python_output.splitlines() if line.startswith(marker)]
    native_records = [line for line in native.stdout.splitlines() if line.startswith(marker)]
    assert len(python_records) == len(native_records) == 1
    assert python_records == native_records
    fields = python_records[0].split("|")
    assert fields[:6] == [
        "ALL_BOUND_REFERENCE",
        scenario,
        str(max_size),
        str(bindings),
        str(bindings),
        values,
    ]
    if diagnostic is None:
        assert fields[6:] == ["0"]
    else:
        assert fields[6:] == ["1", f"109;2;complete binding failed;{diagnostic}"]

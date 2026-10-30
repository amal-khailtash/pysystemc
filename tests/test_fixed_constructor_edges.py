"""Compare remaining fixed-point constructor adapter paths with SystemC."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path):
    """Compile and execute the native constructor reference in a fresh process.

    Args:
        tmp_path: Isolated native runner working directory.

    Returns:
        Captured stdout from the completed native process.

    Raises:
        AssertionError: The native compiler or simulation returns failure.
        subprocess.TimeoutExpired: Compilation or execution exceeds 60 seconds.
    """
    environment = dict(os.environ)
    environment.setdefault("SYSTEMC_HOME", str(ROOT / "submodules/systemc"))
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/fixed_constructor_edges.cpp"),
        ],
        cwd=tmp_path,
        env=environment,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def selected_records(output):
    """Return constructor evidence while excluding native kernel banners."""
    return [line for line in output.splitlines() if line.startswith("FCE\t")]


def test_remaining_fixed_constructor_paths_match_native(tmp_path):
    """Compare scalar, policy, observer, fast, copy, and rejection behavior."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/fixed_constructor_edges.py")
    native_output = run_native(native_dir)
    python_records = selected_records(python_output)
    native_records = selected_records(native_output)

    python_parity = [
        line for line in python_records if not line.startswith("FCE\tadapt_") and not line.startswith("FCE\tmalformed_")
    ]
    assert python_parity == native_records
    assert [record.split("\t", 2)[1] for record in native_records] == [
        "small_int",
        "floating",
        "native_scalar",
        "fixed_value",
        "template_source_cast_observer",
        "config_modes",
        "config_width_modes_n",
        "fast_native_scalar",
        "default",
        "copy",
    ]
    assert "FCE\tadapt_idempotent\t1" in python_records
    assert "FCE\tmalformed_trailing\t1" in python_records
    assert all("|" in line for line in native_records)

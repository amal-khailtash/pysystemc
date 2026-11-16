"""Compare arbitrary-width signal event and lifecycle contracts with native SystemC."""

import os
import subprocess
import sys

import pytest

from .support import ROOT, run_example


def _signal_records(output):
    """Select stable value-change event records from one simulator run."""
    return [line for line in output.splitlines() if line.startswith("WIDE_SIGNAL_EVENT|")]


@pytest.mark.parametrize("width", (65, 137, 257))
def test_wide_signal_value_change_contract_matches_native(tmp_path, width):
    """Compare deferred writes, event counts, full values, times and deltas at a width."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/wide_signal_contracts.py", (str(width),))
    environment = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/wide_signal_contracts.cpp"),
            "--",
            str(width),
        ],
        cwd=native_dir,
        env=environment,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    python_records = _signal_records(python_output)
    native_records = _signal_records(native.stdout)
    assert len(python_records) == len(native_records) == 3
    assert python_records == native_records
    assert f"WIDE_SIGNAL_RESULT|{width}|events=3|lifetime=retained" in python_output
    assert f"WIDE_SIGNAL_RESULT|{width}|events=3|lifetime=native" in native.stdout

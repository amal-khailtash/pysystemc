"""Compare selected omitted defaults with explicit native SystemC defaults."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


def reference_rows(tmp_path, scenario, policy):
    """Return matching Python/native evidence from separate bounded kernels.

    Args:
        tmp_path: Isolated directory for runner artifacts.
        scenario: Start overload shape or constructor comparison.
        policy: Omitted, explicit run, or explicit starvation policy.

    Returns:
        Normalized rows whose values have also been asserted in both models.

    Raises:
        AssertionError: Either model fails or their evidence differs.
        subprocess.TimeoutExpired: Either model exceeds its 60-second limit.
    """
    output = run_example(tmp_path, "tests/models/effective_defaults.py", (scenario, policy))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/effective_defaults.cpp"),
            "--",
            scenario,
            policy,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("DEFAULT_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("DEFAULT_")]
    assert actual and actual == expected
    return actual


@pytest.mark.parametrize("scenario", ("time", "int", "double", "fifo", "queue"))
def test_effective_defaults(tmp_path, scenario):
    """Check empty-kernel run policy, FIFO capacity, and queue naming/deliveries."""
    omitted = reference_rows(tmp_path, scenario, "omitted")
    if scenario in ("time", "int", "double"):
        explicit = reference_rows(tmp_path, scenario, "run")
        starvation = reference_rows(tmp_path, scenario, "exit")
        assert omitted == explicit == ["DEFAULT_START\t7000"]
        assert starvation == ["DEFAULT_START\t0"]
    elif scenario == "fifo":
        assert omitted == ["DEFAULT_FIFO\t16\t120\t16"]
    else:
        assert omitted == ["DEFAULT_QUEUE\t1000\t3000\t2\t1"]

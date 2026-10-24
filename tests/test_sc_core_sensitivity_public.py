"""Compare direct Python sensitivity overloads with a native SystemC model."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_sensitivity_public(tmp_path):
    """Verify every observed callback, edge time, source family, and lifetime count."""
    output = run_example(tmp_path, "tests/models/sc_core_sensitivity_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_sensitivity_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("SENSITIVITY_CALLBACK")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("SENSITIVITY_CALLBACK")]
    assert len(actual) == len(expected) == 55
    assert actual == expected
    assert [line for line in output.splitlines() if line.startswith("SENSITIVITY_LIFETIME")] == [
        "SENSITIVITY_LIFETIME 52 55"
    ]
    assert [line for line in output.splitlines() if line.startswith("SENSITIVITY_RESET")] == ["SENSITIVITY_RESET 1"]
    assert [line for line in native.stdout.splitlines() if line.startswith("SENSITIVITY_RESET")] == [
        "SENSITIVITY_RESET 1"
    ]
    for transcript in (output, native.stdout):
        assert [line for line in transcript.splitlines() if line.startswith("SENSITIVITY_ALIASES")] == [
            "SENSITIVITY_ALIASES 18"
        ]
    assert [line for line in output.splitlines() if line.startswith("SENSITIVITY_INVALID_SOURCE")] == [
        "SENSITIVITY_INVALID_SOURCE TypeError"
    ]

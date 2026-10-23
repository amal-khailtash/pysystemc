"""Compare public Python sc_time operators with native free functions."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_time_free_operators(tmp_path):
    """Check all seven overload rows, exact ticks, invalid products, and copies."""
    output = run_example(tmp_path, "tests/models/sc_core_time_free_operators.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_time_free_operators.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = (
        "TIME_ADD ",
        "TIME_SUB ",
        "TIME_MUL_RIGHT ",
        "TIME_MUL_LEFT ",
        "TIME_DIV_DOUBLE ",
        "TIME_RATIO ",
        "TIME_MOD ",
        "TIME_PRODUCT_INVALID ",
    )
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert actual == expected
    assert len(actual) == 22
    assert actual[-1] == "TIME_PRODUCT_INVALID 1"
    operations = (
        "TIME_ADD",
        "TIME_SUB",
        "TIME_MUL_RIGHT",
        "TIME_MUL_LEFT",
        "TIME_DIV_DOUBLE",
        "TIME_RATIO",
        "TIME_MOD",
    )
    assert all([line.split()[0] for line in actual].count(operation) == 3 for operation in operations)
    for line in actual[:-1]:
        assert line.split()[-1] == "1", line
    expected = {
        "basic": {
            "TIME_ADD": "1251",
            "TIME_SUB": "751",
            "TIME_MUL_RIGHT": "1502",
            "TIME_MUL_LEFT": "1502",
            "TIME_DIV_DOUBLE": "667",
            "TIME_RATIO": "4.0039999999999996",
            "TIME_MOD": "1",
        },
        "fractional": {
            "TIME_ADD": "750",
            "TIME_SUB": "500",
            "TIME_MUL_RIGHT": "781",
            "TIME_MUL_LEFT": "781",
            "TIME_DIV_DOUBLE": "500",
            "TIME_RATIO": "5",
            "TIME_MOD": "0",
        },
        "large": {
            "TIME_ADD": "5629499534213312",
            "TIME_SUB": "3377699720527936",
            "TIME_MUL_RIGHT": "6755399441055936",
            "TIME_MUL_LEFT": "6755399441055936",
            "TIME_DIV_DOUBLE": "3002399751580416",
            "TIME_RATIO": "3.9999999999998863",
            "TIME_MOD": "1125899906842560",
        },
    }
    for label, values in expected.items():
        actual_values = {line.split()[0]: line.split()[-2] for line in actual if f" {label} " in line}
        assert actual_values == values

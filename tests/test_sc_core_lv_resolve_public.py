"""Compare public sc_lv_resolve calls against an independent native process."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_lv_resolve_public(tmp_path):
    """Compare the complete scalar table and vector cases across runtimes."""
    output = run_example(tmp_path, "tests/models/sc_core_lv_resolve_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_lv_resolve_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = ("RESOLVE_PAIR ", "RESOLVE_SINGLETON ", "RESOLVE_MULTI ", "RESOLVE_WIDE ")
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert actual == expected

    pair_lines = [line for line in actual if line.startswith("RESOLVE_PAIR ")]
    expected_rows = {
        "0": {"0": "0", "1": "X", "X": "X", "Z": "0"},
        "1": {"0": "X", "1": "1", "X": "X", "Z": "1"},
        "X": {"0": "X", "1": "X", "X": "X", "Z": "X"},
        "Z": {"0": "0", "1": "1", "X": "X", "Z": "Z"},
    }
    assert len(pair_lines) == 16
    assert pair_lines == [
        f"RESOLVE_PAIR {left} {right} {expected_rows[left][right]}" for left in "01XZ" for right in "01XZ"
    ]
    assert len([line for line in actual if line.startswith("RESOLVE_SINGLETON ")]) == 1
    assert len([line for line in actual if line.startswith("RESOLVE_MULTI ")]) == 1
    assert len([line for line in actual if line.startswith("RESOLVE_WIDE ")]) == 1
    for line in actual:
        if not line.startswith("RESOLVE_PAIR "):
            fields = line.split()
            assert fields[2:4] == ["1", "1"], line

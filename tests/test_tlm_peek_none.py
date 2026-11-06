"""Compare Python blocking-peek None handling with its native TLM reference."""

import subprocess
import sys

from .support import ROOT, run_example


def test_tlm_peek_none_matches_native(tmp_path):
    """Check None normalization, full-width output and pre-native errors."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/tlm_peek_none.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_peek_none.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    prefixes = ("TLM_PEEK_NONE_INT ", "TLM_PEEK_NONE_WIDE ")
    python_records = [line for line in python_output.splitlines() if line.startswith(prefixes)]
    native_records = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert python_records == native_records
    assert python_records == [
        "TLM_PEEK_NONE_INT 23 23 23 23 23 23 5 0 0",
        "TLM_PEEK_NONE_WIDE 7 1 7 1 7 1 7 1 7 1 7 1 7 1 6 0 0",
    ]
    assert "TLM_PEEK_NONE_ERRORS TypeError TypeError native_calls=0" in python_output.splitlines()

"""Compare Python and native exact hash callback-pointer behavior."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_hash_callback_pointers(tmp_path):
    """Check typedef identity, pointer holders, native consumers and table results."""
    expected = "SC_HASH_CALLBACK_POINTERS exact-overloads table-insert duplicate-update count=1"
    python_output = run_example(tmp_path, "tests/models/sc_core_hash_callback_pointers.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_hash_callback_pointers.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert expected in python_output
    assert expected in native.stdout

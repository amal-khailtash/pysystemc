"""Compare native name-generator behavior with its Python public class."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_name_generator(tmp_path):
    """Verify counters, preserve-first defaults, returned string copies and recovery."""
    output = run_example(tmp_path, "tests/models/sc_core_name_generator.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_name_generator.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert [line for line in output.splitlines() if line.startswith("NAME_GENERATOR_REFERENCE")] == [
        line for line in native.stdout.splitlines() if line.startswith("NAME_GENERATOR_REFERENCE")
    ]

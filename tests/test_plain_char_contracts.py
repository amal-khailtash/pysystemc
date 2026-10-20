"""Compare plain char structural callback contracts with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_plain_char_native_contracts(tmp_path):
    """Assert endpoints, native overloads, rejected samples and exact reference records."""
    output = run_example(tmp_path, "tests/models/plain_char_contracts.py")
    assert "PLAIN_CHAR_CONTRACTS_OK" in output
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/plain_char.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    records = [line for line in output.splitlines() if line.startswith("PLAIN_CHAR ")]
    assert len(records) == 4
    assert records == [line for line in native.stdout.splitlines() if line.startswith("PLAIN_CHAR ")]

"""Compare TLM write-only native declarations with Python callback behavior."""

import subprocess
import sys
from .support import ROOT, run_example


def test_tlm_write_contracts(tmp_path):
    """Verify wide input copies, delay values and callback error recovery."""
    output = run_example(tmp_path, "tests/models/tlm_write_contracts.py")
    assert "TLM_WRITE_CONTRACTS_OK" in output
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/tlm_write_contracts.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert [line for line in output.splitlines() if line.startswith("TLM_WRITE_REFERENCE")] == [
        line for line in native.stdout.splitlines() if line.startswith("TLM_WRITE_REFERENCE")
    ]

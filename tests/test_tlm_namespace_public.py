"""Compare public TLM namespace values with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_tlm_namespace_public_matches_native_reference(tmp_path):
    """Verify enum identity, metadata, host predicates and extension registration."""
    output = run_example(tmp_path, "tests/models/tlm_namespace_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/tlm_namespace_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    records = [line for line in output.splitlines() if line.startswith("TLM_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("TLM_")]
    assert len(records) == 57
    assert records == expected

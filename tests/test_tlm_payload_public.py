"""Compare generic payload field and ownership behavior with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_generic_payload_public_matches_native(tmp_path):
    """Verify fields, masked reads, exact extensions and native manager lifetimes."""
    output = run_example(tmp_path, "tests/models/tlm_payload_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/tlm_payload_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("PAYLOAD_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("PAYLOAD_")]
    assert len(actual) == len(expected) == 14
    assert actual == expected
    assert "TLM_PAYLOAD_PUBLIC_OK" in output
    assert "TLM_PAYLOAD_PUBLIC_OK" in native.stdout

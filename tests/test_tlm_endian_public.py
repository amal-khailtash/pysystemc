"""Compare all public endian request/response families with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_tlm_endian_public_matches_native(tmp_path):
    """Verify 384 read/write pairs, masking, pointers, metadata and custom words."""
    output = run_example(tmp_path, "tests/models/tlm_endian_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/tlm_endian_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    records = sorted(line for line in output.splitlines() if line.startswith("ENDIAN_PUBLIC|"))
    expected = sorted(line for line in native.stdout.splitlines() if line.startswith("ENDIAN_PUBLIC|"))
    assert len(records) == 384
    assert records == expected
    reused = [line for line in output.splitlines() if line.startswith("ENDIAN_REUSE|")]
    native_reused = [line for line in native.stdout.splitlines() if line.startswith("ENDIAN_REUSE|")]
    assert len(reused) == 4
    assert reused == native_reused
    assert "ENDIAN_PUBLIC_OK 384" in output
    assert "ENDIAN_PUBLIC_OK 384" in native.stdout

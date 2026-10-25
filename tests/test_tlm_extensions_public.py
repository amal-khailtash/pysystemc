"""Check native TLM extension and memory-manager public API behavior."""

import subprocess
import sys

from .support import ROOT, run_example


def test_extension_families_match_native_reference(tmp_path):
    """Compare extension IDs, callback lifecycle and private accessor records."""
    python_output = run_example(tmp_path, "tests/models/tlm_extensions_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_extensions_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [
        line
        for line in python_output.splitlines()
        if line.startswith(("EXTENSION_", "BASE_EXTENSION_FREE", "PRIVATE_ACCESS", "MEMORY_MANAGER"))
    ]
    native_records = [
        line
        for line in native.stdout.splitlines()
        if line.startswith(("EXTENSION_", "BASE_EXTENSION_FREE", "PRIVATE_ACCESS", "MEMORY_MANAGER"))
    ]
    assert python_records == native_records
    assert "TLM_EXTENSIONS_PUBLIC_OK" in python_output
    assert "TLM_EXTENSIONS_PUBLIC_OK" in native.stdout

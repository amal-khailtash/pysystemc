"""Compare opaque reset and report-handler native declaration contracts."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_kernel_types_public(tmp_path):
    """Verify borrowed reset identity and handler typedef transport with C++."""
    output = run_example(tmp_path, "tests/models/sc_core_kernel_types_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_kernel_types_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    expected = "SMALL_TYPES_OK 1 1"
    assert expected in output and expected in native.stdout

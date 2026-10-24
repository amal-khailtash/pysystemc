"""Compare complete sc_port_base surface behavior with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_port_base_public(tmp_path):
    """Verify constructors, interface/port binds, lifecycle hooks and errors."""
    output = run_example(tmp_path, "tests/models/sc_core_port_base_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_port_base_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    marker = next(line for line in output.splitlines() if line.startswith("SC_PORT_BASE_PUBLIC_REFERENCE "))
    assert marker in native.stdout

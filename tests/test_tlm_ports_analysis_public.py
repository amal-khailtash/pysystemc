"""Check TLM1 ports, event finders, analysis delivery, and FIFO semantics."""

import subprocess
import sys

from .support import ROOT, run_example


def test_tlm_ports_and_analysis_match_native_reference(tmp_path):
    """Compare custom-width port events, subscribers, FIFO values, and lifetimes."""
    python_output = run_example(tmp_path, "tests/models/tlm_ports_analysis_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_ports_analysis_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = (
        "PORT_ANALYSIS_EVENT ",
        "ANALYSIS_PORT ",
        "ANALYSIS_FIFO ",
        "ANALYSIS_FIFO_INT ",
        "ANALYSIS_TRIPLE ",
    )
    python_records = [line for line in python_output.splitlines() if line.startswith(prefixes)]
    native_records = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert python_records == native_records
    assert "PORT_ANALYSIS_LIFETIME 1" in python_output
    assert "PORT_ANALYSIS_LIFETIME 1" in native.stdout
    assert "TLM_PORTS_ANALYSIS_PUBLIC_OK" in python_output
    assert "TLM_PORTS_ANALYSIS_PUBLIC_OK" in native.stdout

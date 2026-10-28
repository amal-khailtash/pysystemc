"""Compare Python-derived TLM put/compound/FIFO probes to native references."""

import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path):
    """Run the standalone C++ reference in a fresh SystemC kernel.

    Args:
        tmp_path: Isolated native compile and simulation directory.

    Returns:
        Captured native standard output.

    Raises:
        AssertionError: If native compilation or execution fails.
        subprocess.TimeoutExpired: If the reference exceeds sixty seconds.
    """
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_put_compound_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def interface_records(output):
    """Return the deterministic method/value records from one probe output."""
    prefixes = ("TLM_PUT ", "TLM_MASTER ", "TLM_SLAVE ", "TLM_FIFO ", "TLM_TRANSPORT ")
    return [line for line in output.splitlines() if line.startswith(prefixes)]


def test_tlm_put_compound_families_match_native(tmp_path):
    """Check wide/custom templates, inherited overloads, tags and callbacks."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/tlm_put_compound_public.py")
    native_output = run_native(native_dir)
    actual = interface_records(python_output)
    expected = interface_records(native_output)
    assert actual == expected, (actual, expected)
    assert actual == [
        "TLM_PUT 4 2 2 4 2 1",
        "TLM_MASTER 4 2 4 3 2 2 4 2 2 2 47 1",
        "TLM_SLAVE 4 2 4 3 2 2 4 2 2 2 47 1",
        "TLM_FIFO 0 17 1 23 1 91 1 1 1 2 3 3 2 2 4 4 3 3 1",
        "TLM_TRANSPORT 46 46 2",
    ]
    assert "TLM_PUT_COMPOUND_PUBLIC_OK" in python_output

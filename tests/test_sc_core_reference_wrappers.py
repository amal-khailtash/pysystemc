"""Compare public sc_ref/sc_cref wrappers with native forwarding templates."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_reference_wrappers(tmp_path):
    """Verify generic identity, constness, bound spawn, and Python owner lifetime."""
    output = run_example(tmp_path, "tests/models/sc_core_reference_wrappers.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_reference_wrappers.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = ("REF_CUSTOM_REFERENCE", "REF_WIDE_REFERENCE", "REF_BOUND_REFERENCE")
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert actual == expected
    assert actual == [
        "REF_CUSTOM_REFERENCE value=22 address=1 copies=1 const=17 consumed=15 bound=17",
        "REF_WIDE_REFERENCE width=137 address=1 low=7 high=1",
        "REF_BOUND_REFERENCE start@0=17 end@2000=22",
    ]
    assert "REF_LIFETIME_REFERENCE ref=retained-released cref=retained-released" in output.splitlines()

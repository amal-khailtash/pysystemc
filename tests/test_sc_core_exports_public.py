"""Compare native exports and protected base hooks against Python calls."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_exports_public(tmp_path):
    """Check bindings, const access, reports, wide values and complete lifecycle."""
    output = run_example(tmp_path, "tests/models/sc_core_exports_public.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_exports_public.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("EXPORT_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("EXPORT_")]
    assert len(actual) == 6 and actual == expected
    assert "EXPORT_ACCESS\t1 1 1 1 1 1 87112285931760246646623899502532662132737" in actual
    assert "EXPORT_HOOKS\tBESF\tBESF" in actual
    assert "EXPORT_DESTRUCTION\t2" in actual


def test_sc_core_exports_unbound(tmp_path):
    """Compare null interface access and required export elaboration errors."""
    output = run_example(tmp_path, "tests/models/sc_core_exports_unbound.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_exports_unbound.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("EXPORT_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("EXPORT_")]
    assert len(actual) == 2 and actual == expected

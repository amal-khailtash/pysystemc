"""Compare hierarchical registry overloads against an independent native model."""

import subprocess
import sys
from .support import ROOT, run_example


def test_sc_core_name_registry(tmp_path):
    """Verify duplicate/missing results and external versus object/event names."""
    output = run_example(tmp_path, "tests/models/sc_core_name_registry.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_name_registry.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("NAME_REGISTRY_")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("NAME_REGISTRY_")]
    assert len(actual) == 3
    assert actual == expected

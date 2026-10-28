"""Compare public sc_module declarations and guarded member behavior."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_module_public(tmp_path):
    """Match all wait/trigger members, edges, hierarchy, stack and lifecycle natively."""
    output = run_example(tmp_path, "tests/models/sc_core_module_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_module_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = (
        "MODULE_HIERARCHY",
        "MODULE_STACK",
        "MODULE_BINDINGS",
        "MODULE_SENSITIVITY",
        "MODULE_DESTRUCTOR",
        "MODULE_WAIT",
        "MODULE_TRIGGER",
        "MODULE_EDGE",
        "MODULE_LIFECYCLE",
        "MODULE_CONTEXT",
    )
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert sorted(actual) == sorted(expected)
    assert any(line.startswith("MODULE_BINDINGS 6 ") for line in actual)
    assert any(line.startswith("MODULE_SENSITIVITY ") and int(line.split()[1]) > 1 for line in actual)
    assert any(line.endswith(" 1 1") for line in actual if line.startswith("MODULE_DESTRUCTOR"))
    assert len([line for line in actual if line.startswith("MODULE_WAIT")]) == 13
    assert len([line for line in actual if line.startswith("MODULE_TRIGGER")]) == 12
    assert len([line for line in actual if line.startswith("MODULE_EDGE")]) == 4
    assert "MODULE_LIFECYCLE before elaboration start end" in actual

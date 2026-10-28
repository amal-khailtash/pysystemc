"""Compare primitive-channel behavior and overloads with native SystemC."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_prim_channel_public_matches_native(tmp_path):
    """Check protected scheduler overloads, updates, async lifetime and hooks."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python_result = run_example(python_dir, "tests/models/sc_core_prim_channel_public.py")
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_prim_channel_public.cpp"),
        ],
        cwd=native_dir,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    prefixes = (
        "PRIM_DEFAULT",
        "PRIM_EXPLICIT_DESTRUCTION",
        "PRIM_WAIT",
        "PRIM_TRIGGER",
        "PRIM_UPDATE",
        "PRIM_ASYNC",
        "PRIM_LIFECYCLE",
    )
    actual = sorted(line for line in python_result.splitlines() if line.startswith(prefixes))
    expected = sorted(line for line in native.stdout.splitlines() if line.startswith(prefixes))
    assert actual == expected
    assert len([line for line in actual if line.startswith("PRIM_WAIT\t")]) == 13
    assert len([line for line in actual if line.startswith("PRIM_TRIGGER\t")]) == 12
    assert "PRIM_UPDATE\t1\t1\t1" in actual
    assert "PRIM_ASYNC\t1\t1\t1\t1" in actual
    assert "PRIM_LIFECYCLE\tBBEESSFF" in actual
    assert "PRIM_DESTRUCTION\t2\t1" in native.stdout.splitlines()

"""Compare public Python and native sc_main entry contracts."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize("entry_mode", ("global", "direct", "elab", "public_elab"))
def test_sc_core_main_public(tmp_path, entry_mode):
    """Compare direct and wrapped callback invocation with native timed behavior."""
    output = run_example(tmp_path, "tests/models/sc_core_main_public.py", (entry_mode,))
    native = subprocess.run(
        [
            sys.executable,
            "-m",
            "amal.eda.systemc._runner",
            str(ROOT / "tests/native/sc_core_main_public.cpp"),
            "--",
            entry_mode,
            "café",
            "",
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 7, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("MAIN_PUBLIC_REFERENCE")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("MAIN_PUBLIC_REFERENCE")]
    assert (
        actual
        == expected
        == [f"MAIN_PUBLIC_REFERENCE {entry_mode} {entry_mode}|café|<empty> thread:1 ns;event:1 ns;thread:2 ns 3 ns"]
    )


def test_sc_core_main_public_errors(tmp_path):
    """Verify callback validation failures return -1 and release Python references."""
    output = run_example(tmp_path, "tests/models/sc_core_main_public.py", ("errors",))
    assert (
        "MAIN_PUBLIC_ERRORS missing noncallable bad_return int_overflow overflow system_exit invalid_count null_argv "
        "cleanup recovery" in output
    )

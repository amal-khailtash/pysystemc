"""Compare native user-exception injection through Python waits and descendants."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize("route", ("public", "namespace"))
@pytest.mark.parametrize("policy", ("default", "descendants"))
@pytest.mark.parametrize("kind", ("custom", "runtime"))
def test_sc_core_user_exception_public(tmp_path, policy, kind, route):
    """Assert native exception values, catches, yields, cleanup and termination."""
    arguments = (policy, kind)
    actual = run_example(tmp_path, "tests/models/sc_core_user_exception_public.py", (*arguments, route))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_user_exception_public.cpp"),
            "--",
            *arguments,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    records = [line for line in actual.splitlines() if line.startswith("USER_EXCEPTION")]
    assert records == [line for line in native.stdout.splitlines() if line.startswith("USER_EXCEPTION")]
    assert len(records) == (10 if policy == "descendants" else 9)

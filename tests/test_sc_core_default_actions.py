"""Compare default-report action branches and ordering in fresh subprocesses."""

import os
import signal
import subprocess
import sys
import pytest
from .support import ROOT


@pytest.mark.parametrize(
    "mode", ("none", "display", "log", "interrupt", "throw", "stop", "stop_throw", "combined", "abort")
)
def test_sc_core_default_actions(tmp_path, mode):
    """Assert action results, logged/displayed content, throw ordering and abort."""
    results = []
    for label, command in (
        ("python", [sys.executable, str(ROOT / "tests/models/sc_core_default_actions.py"), mode]),
        (
            "native",
            [
                sys.executable,
                "-m",
                "amal.eda.systemc._runner",
                str(ROOT / "tests/native/sc_core_default_actions.cpp"),
                "--",
                mode,
            ],
        ),
    ):
        directory = tmp_path / label
        directory.mkdir()
        result = subprocess.run(
            command, cwd=directory, capture_output=True, text=True, timeout=60, env=os.environ.copy()
        )
        if mode == "abort":
            assert result.returncode == -signal.SIGABRT, result.stdout + result.stderr
            assert f"DEFAULT_ACTION_ENTER {mode}" in result.stdout
            assert "DEFAULT_ACTION_RETURN" not in result.stdout
        else:
            assert result.returncode == 0, result.stdout + result.stderr
            assert f"DEFAULT_ACTION_RETURN {mode}" in result.stdout
        if mode in ("display", "combined"):
            assert "Warning: actions/test: message\nIn file: actions.cpp:17" in result.stdout
        if mode == "none":
            assert "Warning: actions/test: message" not in result.stdout
            assert not (directory / "actions.log").exists()
        results.append([line for line in result.stdout.splitlines() if line.startswith("DEFAULT_ACTION_")])
    assert results[0] == results[1]

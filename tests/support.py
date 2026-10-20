"""Shared fresh-process SystemC simulation helpers."""

import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def run_example(tmp_path, path, arguments=()):
    """Run a finite simulation in a fresh process and return captured stdout.

    Args:
        tmp_path: Isolated working directory for simulation outputs.
        path: Repository-relative example or semantic test script.
        arguments: Optional command-line arguments for a bounded simulation matrix.

    Raises:
        AssertionError: If native/Python execution returns a failure status.
        subprocess.TimeoutExpired: If the simulation exceeds 60 seconds.
    """
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [sys.executable, str(ROOT / path), *arguments],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout

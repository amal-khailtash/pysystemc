"""Compare native hierarchy text and finite FIFO model behavior."""

import os
import re
import subprocess
import sys
from pathlib import Path

from .support import ROOT

_MESSAGE = "Visit www.accellera.org and see what SystemC can do for you today!\n"


def _hierarchy_paths(output: str) -> list[str]:
    """Extract full `top` instance paths from module hierarchy rows."""
    return re.findall(r"\((top(?:\.[^()\s]+)*)\)\s*$", output, re.MULTILINE)


def _hierarchy_rows(output: str) -> list[tuple[str, str]]:
    """Extract displayed module type labels and full `top` instance paths."""
    return re.findall(r"^[ \t]*(.+?) \((top(?:\.[^()\s]+)*)\)\s*$", output, re.MULTILINE)


def _run_console(
    name: str,
    arguments: list[str],
    cwd: Path,
    env: dict[str, str],
) -> subprocess.CompletedProcess[str]:
    """Load an installed console entry point and run it in a bounded process.

    Args:
        name: Installed `systemc-py` or `systemc-cpp` console command.
        arguments: Console arguments excluding the executable name.
        cwd: Isolated working directory for the native kernel.
        env: Child environment with the selected SystemC installation.

    Returns:
        Captured output and exit status from the console entry point.
    """
    entrypoint_script = """
import importlib.metadata
import sys
entry = next(item for item in importlib.metadata.entry_points(group='console_scripts') if item.name == sys.argv[1])
sys.argv = [entry.name, *sys.argv[2:]]
sys.exit(entry.load()())
"""
    return subprocess.run(
        [sys.executable, "-c", entrypoint_script, name, *arguments],
        cwd=cwd,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )


def test_simple_fifo_python_and_cpp_hierarchy_and_values(tmp_path):
    """Match full hierarchy paths and assert each implementation transfers the message."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    env = dict(os.environ)
    python_result = _run_console(
        "systemc-py",
        ["run", str(ROOT / "examples/simple_fifo/py/simple_fifo.py")],
        cwd=python_dir,
        env=env,
    )
    assert python_result.returncode == 0, python_result.stdout + python_result.stderr
    python_output = python_result.stdout
    native_result = _run_console(
        "systemc-cpp",
        ["run", str(ROOT / "examples/simple_fifo/cpp/simple_fifo.cpp")],
        cwd=native_dir,
        env=env,
    )
    assert native_result.returncode == 0, native_result.stdout + native_result.stderr
    assert _MESSAGE in python_output
    assert _MESSAGE in native_result.stdout
    assert python_output.count(_MESSAGE) == native_result.stdout.count(_MESSAGE) == 1
    assert python_output.index("Design Hierarchy:") < python_output.index(_MESSAGE)
    assert native_result.stdout.index("Design Hierarchy:") < native_result.stdout.index(_MESSAGE)

    expected_paths = ["top", "top.producer", "top.consumer"]
    python_paths = _hierarchy_paths(python_output)
    native_paths = _hierarchy_paths(native_result.stdout)
    assert python_paths == native_paths == expected_paths
    expected_rows = [("Top", "top"), ("Producer", "top.producer"), ("Consumer", "top.consumer")]
    assert _hierarchy_rows(python_output) == expected_rows
    assert _hierarchy_rows(native_result.stdout) == expected_rows
    for output in (python_output, native_result.stdout):
        last_row = "Consumer (top.consumer)"
        last_row_end = output.index(last_row) + len(last_row)
        hierarchy = output[output.index("Design Hierarchy:") - 1 : last_row_end + 2]
        assert hierarchy.startswith("\nDesign Hierarchy:\n")
        assert hierarchy.endswith("\n\n")
        assert "__cppyy_internal" not in hierarchy
        assert "(anonymous namespace)" not in hierarchy
    assert all("fifo" not in path for path in python_paths + native_paths)

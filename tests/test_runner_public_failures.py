"""Exercise public runner failures and fresh-process recovery."""

import os
from pathlib import Path
import subprocess
import sys


def _run_systemc_python(arguments: list[str], *, cwd: Path) -> subprocess.CompletedProcess[str]:
    """Run the Python CLI module in a new process.

    Args:
        arguments: Arguments after the ``systemc-py`` command name.
        cwd: Working directory for the child process.

    Returns:
        Captured process output and exit status.
    """
    return subprocess.run(
        [sys.executable, "-m", "amal.eda.cli", *arguments],
        cwd=cwd,
        env=dict(os.environ),
        capture_output=True,
        text=True,
        timeout=60,
    )


def _run_systemc_cpp(source: Path, *, cwd: Path) -> subprocess.CompletedProcess[str]:
    """Run the installed C++ CLI entry point in a new process.

    Args:
        source: C++ source passed to ``systemc-cpp run``.
        cwd: Working directory for the child process.

    Returns:
        Captured process output and exit status.
    """
    script = """
import importlib.metadata
import sys

entry_point = next(
    entry
    for entry in importlib.metadata.distribution("pysystemc").entry_points
    if entry.group == "console_scripts" and entry.name == "systemc-cpp"
)
sys.exit(entry_point.load()())
"""
    return subprocess.run(
        [sys.executable, "-c", script, "run", str(source)],
        cwd=cwd,
        env=dict(os.environ),
        capture_output=True,
        text=True,
        timeout=60,
    )


def test_run_file_missing_source_raises_before_native_startup(tmp_path: Path) -> None:
    """Raise FileNotFoundError for a missing Python source without importing SystemC."""
    missing = tmp_path / "missing.py"
    script = """
import sys
from importlib.abc import MetaPathFinder


class NativeImportBlocker(MetaPathFinder):
    '''Fail if the Python runner requests its native runtime.'''

    def find_spec(self, fullname, path=None, target=None):
        '''Reject SystemC or cppyy module resolution.'''
        if fullname == "cppyy" or fullname.startswith("cppyy."):
            raise AssertionError(f"unexpected cppyy import: {fullname}")
        if fullname == "amal.eda.systemc" or fullname.startswith("amal.eda.systemc."):
            raise AssertionError(f"unexpected SystemC import: {fullname}")
        return None


sys.meta_path.insert(0, NativeImportBlocker())
from amal.eda.cli._run import run_file

try:
    run_file(sys.argv[1])
except FileNotFoundError as error:
    assert str(error) == f"Python file not found: {sys.argv[1]}"
    print(f"MISSING_SOURCE {type(error).__name__}: {error}")
else:
    raise AssertionError("missing Python source was accepted")

assert "amal.eda.systemc" not in sys.modules
assert "cppyy" not in sys.modules
"""
    result = subprocess.run(
        [sys.executable, "-c", script, str(missing)],
        cwd=tmp_path,
        env=dict(os.environ),
        capture_output=True,
        text=True,
        timeout=30,
    )

    assert result.returncode == 0, result.stdout + result.stderr
    assert result.stdout == f"MISSING_SOURCE FileNotFoundError: Python file not found: {missing}\n"
    assert result.stderr == ""
    assert "Simulation started" not in result.stdout + result.stderr


def test_python_cli_missing_source_returns_error_without_simulation(tmp_path: Path) -> None:
    """Report a missing run source through systemc-py before native startup."""
    missing = tmp_path / "missing.py"
    result = _run_systemc_python(["run", str(missing)], cwd=tmp_path)
    output = result.stdout + result.stderr

    assert result.returncode == 1, output
    assert result.stdout == ""
    assert result.stderr == f"systemc-py run: Python file not found: {missing}\n"
    assert "SystemC 2.3.4" not in output
    assert "Simulation" not in output


def test_cpp_cli_reports_a_real_compilation_failure(tmp_path: Path) -> None:
    """Format Cling's diagnostic for an invalid C++ source and return failure."""
    source = tmp_path / "invalid.cpp"
    invalid_code = "this is not valid C++\n"
    source.write_text(invalid_code)

    result = _run_systemc_cpp(source, cwd=tmp_path)
    output = result.stdout + result.stderr

    assert result.returncode == 1, output
    assert f"[pysystemc:C++:info] Reading {source}" in result.stderr
    assert "systemc-cpp: Failed to parse the given C++ code" in result.stderr
    assert invalid_code.rstrip() in result.stderr
    assert "error:" in result.stderr
    assert "Traceback" not in result.stderr
    assert "Simulation started" not in output


def test_python_sc_main_exception_and_fresh_cli_recovery(tmp_path: Path) -> None:
    """Report the native callback failure, then run a valid model in a fresh process."""
    failing_source = tmp_path / "sc_main_failure.py"
    failing_source.write_text(
        '"""Raise a marker from the native Python sc_main callback."""\n'
        "from amal.eda.systemc import main\n\n"
        "def sc_main(_arguments):\n"
        '    """Raise a recognizable callback error."""\n'
        '    raise LookupError("sc_main_exception_marker")\n\n'
        "main()\n"
    )

    failed = _run_systemc_python(["run", str(failing_source)], cwd=tmp_path)
    failed_output = failed.stdout + failed.stderr

    assert failed.returncode == 255, failed_output
    assert "LookupError: sc_main_exception_marker" in failed.stderr
    assert "[pysystemc:C++:ERROR] Python sc_main failed" in failed.stderr
    assert "[pysystemc:Python:ERROR] sc_main returned status -1" in failed.stderr
    assert "Exception while executing sc_main" not in failed.stderr

    recovered_source = tmp_path / "sc_main_recovery.py"
    recovered_source.write_text(
        '"""Print a marker after successful native Python sc_main dispatch."""\n'
        "from amal.eda.systemc import main\n\n"
        "def sc_main(_arguments):\n"
        '    """Prove callback execution in a fresh process."""\n'
        '    print("SC_MAIN_RECOVERY_CALLBACK")\n'
        "    return 0\n\n"
        "main()\n"
    )

    recovered = _run_systemc_python(["run", str(recovered_source)], cwd=tmp_path)
    recovered_output = recovered.stdout + recovered.stderr

    assert recovered.returncode == 0, recovered_output
    assert "SC_MAIN_RECOVERY_CALLBACK" in recovered.stdout
    assert "sc_main returned status -1" not in recovered.stderr

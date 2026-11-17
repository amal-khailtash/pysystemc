"""Fresh-process checks for ``systemc-py run`` script execution."""

import json
import os
from pathlib import Path
import re
import subprocess
import sys

import pytest

from .support import ROOT


def _run_cli(*arguments: str, cwd: Path | None = None, timeout: int = 30) -> subprocess.CompletedProcess[str]:
    """Run systemc-py in a fresh process and capture its output.

    Args:
        *arguments: CLI arguments, excluding the module invocation.
        cwd: Optional working directory for the child process.
        timeout: Maximum child-process runtime in seconds.

    Returns:
        Captured standard output, standard error, and process status.
    """
    return subprocess.run(
        [sys.executable, "-m", "amal.eda.cli", *arguments],
        capture_output=True,
        text=True,
        cwd=cwd,
        timeout=timeout,
    )


def test_run_help_does_not_import_native_runtime() -> None:
    """Render run help while a subprocess import hook blocks native modules."""
    script = """
import sys
from importlib.abc import MetaPathFinder

class BlockNativeImports(MetaPathFinder):
    \"\"\"Reject SystemC and cppyy imports during lightweight help.\"\"\"

    def find_spec(self, fullname, path=None, target=None):
        \"\"\"Raise if import resolution requests either native dependency.\"\"\"
        if fullname == 'cppyy' or fullname.startswith('cppyy.'):
            raise AssertionError(f'unexpected native import: {fullname}')
        if fullname == 'amal.eda.systemc' or fullname.startswith('amal.eda.systemc.'):
            raise AssertionError(f'unexpected native import: {fullname}')
        return None

sys.meta_path.insert(0, BlockNativeImports())
from amal.eda.cli import main
try:
    main(['run', '--help'])
except SystemExit as error:
    assert error.code == 0
assert 'amal.eda.systemc' not in sys.modules
assert 'cppyy' not in sys.modules
"""
    environment = dict(os.environ, PYSYSTEMC_COLOR="always", NO_COLOR="1")
    result = subprocess.run([sys.executable, "-c", script], capture_output=True, text=True, env=environment, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr
    plain_output = re.sub(r"\x1b\[[0-9;]*m", "", result.stdout)
    assert "Usage: systemc-py run [OPTIONS] source [ARGS ...]" in plain_output
    assert plain_output.index("Run a Python SystemC model") < plain_output.index("Usage:")
    assert "source" in plain_output
    assert "arguments passed to the Python file" in plain_output
    assert "\x1b[35msystemc-py" in result.stdout
    assert "\x1b[96mrun" in result.stdout
    assert "\x1b[2;36m[OPTIONS]" in result.stdout
    assert "\x1b[2;36m-h" in result.stdout
    assert "\x1b[32msource" in result.stdout


def test_help_and_version_command_help_stay_lightweight() -> None:
    """Render help and version command pages under a native import blocker."""
    script = """
import sys
from importlib.abc import MetaPathFinder

class BlockNativeImports(MetaPathFinder):
    \"\"\"Reject native dependency imports while displaying command pages.\"\"\"

    def find_spec(self, fullname, path=None, target=None):
        \"\"\"Raise when Python tries to resolve SystemC or cppyy.\"\"\"
        if fullname == 'cppyy' or fullname.startswith('cppyy.'):
            raise AssertionError(f'unexpected native import: {fullname}')
        if fullname == 'amal.eda.systemc' or fullname.startswith('amal.eda.systemc.'):
            raise AssertionError(f'unexpected native import: {fullname}')
        return None

sys.meta_path.insert(0, BlockNativeImports())
from amal.eda.cli import main
for arguments in (['help', '-h'], ['version', '-h'], ['help', 'help'], ['help', 'version']):
    try:
        main(arguments)
    except SystemExit as error:
        assert error.code == 0
assert 'amal.eda.systemc' not in sys.modules
assert 'cppyy' not in sys.modules
"""
    environment = dict(os.environ, PYSYSTEMC_COLOR="never")
    result = subprocess.run([sys.executable, "-c", script], capture_output=True, text=True, env=environment, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr
    assert "Usage: systemc-py help [OPTIONS] [<COMMAND>]" in result.stdout
    assert "Show help for systemc-py or one of its commands." in result.stdout
    assert "Optional command (run, repl, help, version)" in result.stdout
    assert "Usage: systemc-py version [OPTIONS]" in result.stdout
    assert "Display the installed SystemC Python bindings version and exit." in result.stdout


def test_run_executes_as_main_with_arguments_siblings_and_exit_status(tmp_path: Path) -> None:
    """Preserve script argv, Unicode flags, sibling imports, and SystemExit status."""
    (tmp_path / "helper.py").write_text('TOKEN = "sibling-import-ok"\n')
    source = tmp_path / "script.py"
    source.write_text(
        "import json\n"
        "import sys\n"
        "from helper import TOKEN\n"
        'print(json.dumps({"name": __name__, "argv": sys.argv, "token": TOKEN}, ensure_ascii=False))\n'
        "raise SystemExit(7)\n"
    )
    source_argument = source.name
    result = _run_cli("run", source_argument, "--", "café", "--flag", cwd=tmp_path)
    assert result.returncode == 7, result.stdout + result.stderr
    assert json.loads(result.stdout) == {
        "name": "__main__",
        "argv": [source_argument, "café", "--flag"],
        "token": "sibling-import-ok",
    }


def test_run_restores_process_arguments_and_path_after_script_exit(tmp_path: Path) -> None:
    """Restore sys.argv and sys.path when a programmatic run exits explicitly."""
    source = tmp_path / "exit.py"
    source.write_text("raise SystemExit(7)\n")
    script = f"""
import sys
from amal.eda.cli._run import run_file
original_argv = sys.argv
original_argv_values = original_argv[:]
original_path = sys.path
original_path_values = original_path[:]
try:
    run_file({str(source)!r}, ["café", "--flag"])
except SystemExit as error:
    assert error.code == 7
else:
    raise AssertionError("script SystemExit was swallowed")
assert sys.argv is original_argv
assert sys.argv == original_argv_values
assert sys.path is original_path
assert sys.path == original_path_values
"""
    result = subprocess.run([sys.executable, "-c", script], capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr


def test_run_missing_file_reports_clear_error(tmp_path: Path) -> None:
    """Return a clear nonzero status without a traceback for a missing script."""
    missing = tmp_path / "missing.py"
    result = _run_cli("run", str(missing), cwd=tmp_path)
    assert result.returncode == 1
    assert f"systemc-py run: Python file not found: {missing}" in result.stderr
    assert "Traceback" not in result.stderr


def test_run_systemc_example_checks_callback_and_lifecycle(tmp_path: Path) -> None:
    """Run a real SystemC example and verify its callback and lifecycle output."""
    source = ROOT / "examples/helloworld/py/helloworld.py"
    result = subprocess.run(
        [sys.executable, "-m", "amal.eda.cli", "run", str(source)],
        cwd=tmp_path,
        env=dict(os.environ),
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    expected = [
        "Hello world using approach 1",
        "Before End of Elaboration",
        "Elaboration finished",
        "Simulation started",
        "0 s - [ helloworld   ] - hello2()",
        "Hello world using approach 3",
        "Simulation finished",
    ]
    positions = [result.stdout.index(marker) for marker in expected]
    assert positions == sorted(positions)
    assert "Simulation stopped" in result.stdout


@pytest.mark.parametrize("script_arguments", [(), ("--", "café", "--flag")])
def test_run_accepts_empty_and_separator_prefixed_arguments(tmp_path: Path, script_arguments: tuple[str, ...]) -> None:
    """Allow scripts to receive no arguments or a leading -- separator."""
    source = tmp_path / "arguments.py"
    source.write_text("import sys\nprint(repr(sys.argv[1:]))\n")
    result = _run_cli("run", str(source), *script_arguments, cwd=tmp_path)
    assert result.returncode == 0, result.stderr
    expected_arguments = [] if not script_arguments else ["café", "--flag"]
    assert result.stdout.strip() == repr(expected_arguments)

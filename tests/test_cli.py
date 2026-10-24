"""Fresh-process checks for the lightweight pysystemc CLI."""

import subprocess
import sys


def run_cli(*arguments: str) -> subprocess.CompletedProcess[str]:
    """Run the source-tree CLI in a child process.

    Args:
        *arguments: CLI arguments, excluding the Python module invocation.

    Returns:
        Captured child process output and status.
    """
    return subprocess.run(
        [sys.executable, "-m", "amal.eda.cli", *arguments],
        capture_output=True,
        text=True,
        timeout=30,
    )


def test_cli_help_version_and_help_command() -> None:
    """Show top-level and command help and version without native imports."""
    help_result = run_cli()
    assert help_result.returncode == 0, help_result.stderr
    assert "usage: pysystemc" in help_result.stdout
    assert "repl" in help_result.stdout and "version" in help_result.stdout

    option_help = run_cli("--help")
    assert option_help.returncode == 0, option_help.stderr
    assert "interactive SystemC Python playground" in option_help.stdout

    repl_help = run_cli("help", "repl")
    assert repl_help.returncode == 0, repl_help.stderr
    assert "--history PATH" in repl_help.stdout
    assert "--no-history" in repl_help.stdout

    version_option = run_cli("--version")
    version_command = run_cli("version")
    assert version_option.returncode == version_command.returncode == 0
    assert version_option.stdout.startswith("pysystemc ")
    assert version_command.stdout.startswith("pysystemc v")


def test_cli_metadata_commands_do_not_import_native_runtime() -> None:
    """Importing or running version metadata leaves SystemC unloaded."""
    script = """
import sys
from amal.eda.cli import main
assert 'amal.eda.systemc' not in sys.modules
try:
    main(['--version'])
except SystemExit as error:
    assert error.code == 0
else:
    raise AssertionError('--version did not exit through argparse')
assert 'amal.eda.systemc' not in sys.modules
"""
    result = subprocess.run(
        [sys.executable, "-c", script],
        capture_output=True,
        text=True,
        timeout=30,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert result.stdout.startswith("pysystemc ")

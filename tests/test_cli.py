"""Fresh-process checks for the lightweight systemc-py CLI."""

import os
import subprocess
import sys


def run_cli(*arguments: str, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    """Run the source-tree CLI in a child process.

    Args:
        *arguments: CLI arguments, excluding the Python module invocation.
        env: Optional child-process environment; defaults to the current one.

    Returns:
        Captured child process output and status.
    """
    return subprocess.run(
        [sys.executable, "-m", "amal.eda.cli", *arguments],
        capture_output=True,
        text=True,
        env=env,
        timeout=30,
    )


def test_cli_help_version_and_help_command() -> None:
    """Show top-level and command help and version without native imports."""
    help_result = run_cli()
    assert help_result.returncode == 0, help_result.stderr
    assert "Usage: systemc-py [OPTIONS] <COMMAND>" in help_result.stdout
    assert help_result.stdout.index("Explore native SystemC") < help_result.stdout.index("Usage:")
    repl_row = "start the interactive SystemC Python playground"
    help_row = "show help for systemc-py or a command"
    version_row = "print the installed systemc-py version"
    assert repl_row in help_result.stdout
    assert help_row in help_result.stdout
    assert version_row in help_result.stdout
    assert (
        help_result.stdout.index(repl_row) < help_result.stdout.index(help_row) < help_result.stdout.index(version_row)
    )

    colored_help = run_cli("--help", env=os.environ | {"PYSYSTEMC_COLOR": "always"})
    assert colored_help.returncode == 0, colored_help.stderr
    assert "\x1b[2;36m[OPTIONS]\x1b[0m" in colored_help.stdout
    assert "\x1b[96m<COMMAND>\x1b[0m" in colored_help.stdout
    assert "repl" in help_result.stdout and "version" in help_result.stdout
    assert "Global options:" in help_result.stdout
    assert "-h, --help" in help_result.stdout
    assert "Show this message and exit" in help_result.stdout
    assert "-V, --version" in help_result.stdout
    assert "Show version and exit" in help_result.stdout

    option_help = run_cli("--help")
    assert option_help.returncode == 0, option_help.stderr
    assert "interactive SystemC Python playground" in option_help.stdout

    repl_help = run_cli("help", "repl")
    assert repl_help.returncode == 0, repl_help.stderr
    assert "Usage: systemc-py repl" in repl_help.stdout
    assert "Options:" in repl_help.stdout
    assert "--history PATH" in repl_help.stdout
    assert "--no-history" in repl_help.stdout

    help_command_help = run_cli("help", "--help")
    assert help_command_help.returncode == 0, help_command_help.stderr
    assert "Usage: systemc-py help" in help_command_help.stdout
    assert "Positional arguments:" in help_command_help.stdout

    short_version_option = run_cli("-V")
    long_version_option = run_cli("--version")
    version_command = run_cli("version")
    assert short_version_option.returncode == long_version_option.returncode == version_command.returncode == 0
    assert short_version_option.stdout.startswith("systemc-py v")
    assert long_version_option.stdout.startswith("systemc-py v")
    assert version_command.stdout.startswith("systemc-py v")


def test_cli_metadata_commands_do_not_import_native_runtime() -> None:
    """Load the installed entry point and run version without native imports."""
    script = """
import sys
from importlib.metadata import entry_points
class BlockNativeImports:
    '''Reject imports that would initialize either native dependency.'''

    def find_spec(self, fullname, path=None, target=None):
        '''Raise if import resolution requests cppyy or the SystemC package.'''
        if fullname == 'cppyy' or fullname.startswith('cppyy.'):
            raise AssertionError(f'unexpected native import: {fullname}')
        if fullname == 'amal.eda.systemc' or fullname.startswith('amal.eda.systemc.'):
            raise AssertionError(f'unexpected native import: {fullname}')
        return None
sys.meta_path.insert(0, BlockNativeImports())
from amal.eda.cli import main
console_entry = next(entry for entry in entry_points(group='console_scripts') if entry.name == 'systemc-py')
assert console_entry.value == 'amal.eda.cli:main'
assert console_entry.load() is main
assert 'amal.eda.systemc' not in sys.modules
assert 'cppyy' not in sys.modules
for version_flag in ('-V', '--version'):
    try:
        console_entry.load()([version_flag])
    except SystemExit as error:
        assert error.code == 0
    else:
        raise AssertionError(f'{version_flag} did not exit through argparse')
assert 'amal.eda.systemc' not in sys.modules
assert 'cppyy' not in sys.modules
"""
    result = subprocess.run(
        [sys.executable, "-c", script],
        capture_output=True,
        text=True,
        timeout=30,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert result.stdout.splitlines()
    assert all(line.startswith("systemc-py v") for line in result.stdout.splitlines())

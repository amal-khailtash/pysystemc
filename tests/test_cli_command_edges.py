"""Exercise lightweight CLI dispatch edges without starting native SystemC."""

import builtins
from collections.abc import Iterator
from importlib.metadata import PackageNotFoundError
from pathlib import Path
import sys
from types import ModuleType

import pytest

from amal.eda.cli import commands
from amal.eda.cli import cpp


@pytest.fixture
def native_import_attempts(monkeypatch: pytest.MonkeyPatch) -> Iterator[list[str]]:
    """Reject native imports and record the one injected REPL boundary."""
    original_import = builtins.__import__
    attempted: list[str] = []

    def guarded_import(name, globals=None, locals=None, fromlist=(), level=0):
        """Reject SystemC and cppyy imports except the injected REPL module."""
        if name == "cppyy" or name.startswith("cppyy."):
            attempted.append(name)
            raise AssertionError(f"unexpected native import: {name}")
        if name == "amal.eda.systemc" or name.startswith("amal.eda.systemc."):
            if name == "amal.eda.systemc.repl" and getattr(sys.modules.get(name), "__cli_test_fake__", False):
                attempted.append(f"injected:{name}")
                return original_import(name, globals, locals, fromlist, level)
            attempted.append(name)
            raise AssertionError(f"unexpected SystemC import: {name}")
        return original_import(name, globals, locals, fromlist, level)

    monkeypatch.setattr(builtins, "__import__", guarded_import)
    yield attempted


def test_version_reports_unknown_when_distribution_metadata_is_missing(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    native_import_attempts: list[str],
) -> None:
    """Render the documented unknown version when package metadata is absent."""

    def missing_metadata(distribution_name: str) -> str:
        """Model importlib metadata lookup when the distribution is unavailable."""
        assert distribution_name == "pysystemc"
        raise PackageNotFoundError(distribution_name)

    monkeypatch.setenv("PYSYSTEMC_COLOR", "never")
    monkeypatch.setattr(commands, "version", missing_metadata)

    assert commands.main(["version"]) == 0
    assert capsys.readouterr().out == "systemc-py vunknown\n"
    assert native_import_attempts == []


def test_help_without_command_prints_top_level_help(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    native_import_attempts: list[str],
) -> None:
    """Show top-level help for the bare help command without native imports."""
    monkeypatch.setenv("PYSYSTEMC_COLOR", "never")
    assert commands.main(["help"]) == 0
    output = capsys.readouterr().out
    assert "Usage: systemc-py [OPTIONS] <COMMAND>" in output
    assert "Explore native SystemC" in output
    assert native_import_attempts == []


def test_missing_command_prints_top_level_help(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    native_import_attempts: list[str],
) -> None:
    """Show top-level help when the CLI receives no arguments."""
    monkeypatch.setenv("PYSYSTEMC_COLOR", "never")
    assert commands.main([]) == 0
    assert "Usage: systemc-py [OPTIONS] <COMMAND>" in capsys.readouterr().out
    assert native_import_attempts == []


def test_help_for_named_command_uses_argparse_help_exit(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    native_import_attempts: list[str],
) -> None:
    """Display named-command help and preserve argparse's successful exit."""
    monkeypatch.setenv("PYSYSTEMC_COLOR", "never")
    with pytest.raises(SystemExit) as exit_info:
        commands.main(["help", "repl"])
    assert exit_info.value.code == 0
    assert "Usage: systemc-py repl" in capsys.readouterr().out
    assert native_import_attempts == []


@pytest.mark.parametrize(
    ("arguments", "diagnostic"),
    [
        (("--bogus",), "unrecognized arguments"),
        (("--history",), "expected one argument"),
        (("--history", "session.hist", "--no-history"), "not allowed with argument"),
    ],
)
def test_repl_argument_errors_exit_before_native_import(
    arguments: tuple[str, ...],
    diagnostic: str,
    capsys: pytest.CaptureFixture[str],
    native_import_attempts: list[str],
) -> None:
    """Report malformed REPL options through argparse before importing SystemC."""
    with pytest.raises(SystemExit) as exit_info:
        commands.main(["repl", *arguments])
    assert exit_info.value.code == 2
    assert diagnostic in capsys.readouterr().err
    assert native_import_attempts == []


@pytest.mark.parametrize(
    ("arguments", "history_path", "no_history"),
    [
        (("--history", "session.hist"), Path("session.hist"), False),
        (("--no-history",), None, True),
    ],
)
def test_repl_dispatch_passes_history_options_to_repl_boundary(
    monkeypatch: pytest.MonkeyPatch,
    arguments: tuple[str, ...],
    history_path: Path | None,
    no_history: bool,
    native_import_attempts: list[str],
) -> None:
    """Pass parsed history settings to the REPL and return its successful status."""
    calls: list[tuple[str, object]] = []

    class FakeRepl:
        """Capture the CLI-to-REPL boundary without loading the native runtime."""

        def __init__(self, *, history_path: Path | None, no_history: bool) -> None:
            """Record construction options supplied by command dispatch."""
            calls.append(("init", (history_path, no_history)))

        def run(self) -> None:
            """Record that the CLI starts the constructed REPL once."""
            calls.append(("run", None))

    fake_systemc = ModuleType("amal.eda.systemc")
    fake_systemc.__path__ = []
    injected_repl = ModuleType("amal.eda.systemc.repl")
    injected_repl.Repl = FakeRepl
    injected_repl.__cli_test_fake__ = True
    monkeypatch.setitem(sys.modules, "amal.eda.systemc", fake_systemc)
    monkeypatch.setitem(sys.modules, "amal.eda.systemc.repl", injected_repl)

    assert commands.main(["repl", *arguments]) == 0
    assert calls == [("init", (history_path, no_history)), ("run", None)]
    assert native_import_attempts == ["injected:amal.eda.systemc.repl"]


@pytest.mark.parametrize(
    ("script_arguments", "expected_arguments"),
    [
        (("--", "alpha", "--flag"), ("alpha", "--flag")),
        (("--", "--", "alpha"), ("--", "alpha")),
        (("--", "--"), ("--",)),
        (("first", "--", "alpha"), ("first", "--", "alpha")),
    ],
)
def test_run_forwards_arguments_without_dropping_literal_separators(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    native_import_attempts: list[str],
    script_arguments: tuple[str, ...],
    expected_arguments: tuple[str, ...],
) -> None:
    """Preserve literal script separators after argparse consumes its delimiter."""
    source = tmp_path / "arguments.py"
    source.write_text("import sys\nprint(repr(sys.argv[1:]))\n")

    # The real lightweight runner proves forwarding without a mock or SystemC.
    assert commands.main(["run", str(source), *script_arguments]) == 0
    assert capsys.readouterr().out == f"{list(expected_arguments)!r}\n"
    assert native_import_attempts == []


def test_run_missing_source_returns_error_without_native_import(
    tmp_path: Path,
    capsys: pytest.CaptureFixture[str],
    native_import_attempts: list[str],
) -> None:
    """Name a missing Python source on stderr and return status one."""
    missing_source = tmp_path / "missing.py"
    assert commands.main(["run", str(missing_source)]) == 1
    captured = capsys.readouterr()
    assert captured.out == ""
    assert captured.err == f"systemc-py run: Python file not found: {missing_source}\n"
    assert native_import_attempts == []


def test_cpp_separator_without_command_reports_parser_error(
    capsys: pytest.CaptureFixture[str],
    native_import_attempts: list[str],
) -> None:
    """Reject an empty C++ command before trying to initialize native SystemC."""
    with pytest.raises(SystemExit) as exit_info:
        cpp.main(["--"])
    assert exit_info.value.code == 2
    assert "systemc-cpp: error: no command selected" in capsys.readouterr().err
    assert native_import_attempts == []

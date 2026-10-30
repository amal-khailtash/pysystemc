"""Exercise public CLI formatting behavior at metadata and layout edges."""

import argparse
import builtins
import re
from collections.abc import Iterator

from importlib.metadata import PackageNotFoundError

import pytest

from amal.eda.cli import _formatting


@pytest.fixture(autouse=True)
def _block_native_imports(monkeypatch: pytest.MonkeyPatch) -> Iterator[None]:
    """Fail if a formatting path tries to initialize cppyy or SystemC."""
    original_import = builtins.__import__

    def importing(name, globals=None, locals=None, fromlist=(), level=0):
        """Reject native package imports and delegate ordinary imports."""
        if name == "cppyy" or name.startswith("cppyy."):
            raise AssertionError(f"unexpected native import: {name}")
        if name == "amal.eda.systemc" or name.startswith("amal.eda.systemc."):
            raise AssertionError(f"unexpected native import: {name}")
        return original_import(name, globals, locals, fromlist, level)

    monkeypatch.setattr(builtins, "__import__", importing)
    yield


def _without_ansi(text: str) -> str:
    """Remove ANSI color sequences while retaining help layout and wording."""
    return re.sub(r"\x1b\[[0-9;]*m", "", text)


@pytest.mark.parametrize(
    ("color_mode", "no_color", "colored"),
    [("always", True, True), ("never", False, False), ("auto", True, False)],
)
def test_print_version_falls_back_and_obeys_color_environment(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
    color_mode: str,
    no_color: bool,
    colored: bool,
) -> None:
    """Render an unknown version marker and honor all explicit color policies."""

    def missing_metadata(distribution_name: str) -> str:
        """Represent a lightweight install with no distribution metadata."""
        assert distribution_name == "pysystemc"
        raise PackageNotFoundError(distribution_name)

    monkeypatch.setattr(_formatting, "distribution_version", missing_metadata)
    monkeypatch.setenv("PYSYSTEMC_COLOR", color_mode)
    if no_color:
        monkeypatch.setenv("NO_COLOR", "1")
    else:
        monkeypatch.delenv("NO_COLOR", raising=False)

    _formatting.print_version("systemc-format")

    output = capsys.readouterr().out
    assert _without_ansi(output) == "systemc-format vunknown\n"
    assert ("\x1b[" in output) is colored
    if colored:
        assert "\x1b[35msystemc-format \x1b[0m" in output
        assert "\x1b[36mvunknown\x1b[0m" in output


def test_help_parser_colors_wrapped_usage_metavariables_options_and_commands(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    """Keep custom multiline usage, token colors, help text, and section order."""
    parser = _formatting.HelpParser(
        prog="systemc-format",
        description="Render command help without loading native runtime.",
        usage=(
            "%(prog)s OPTIONS COMMAND [OPTIONS] <COMMAND> <SOURCE> <INCLUDE> input\n"
            "              {build,check} [--mode <MODE>]"
        ),
    )
    parser.add_argument("source", metavar="<SOURCE>", help="Input source file")
    parser.add_argument("--mode", metavar="<MODE>", choices=("fast", "safe"), help="Select a mode")
    commands = parser.add_subparsers(dest="command")
    commands.add_parser("build", help="Build a model")
    commands.add_parser("check", help="Check a model")

    monkeypatch.setenv("PYSYSTEMC_COLOR", "always")
    monkeypatch.setenv("NO_COLOR", "1")
    formatted = parser.format_help()
    parser.print_help()
    output = capsys.readouterr().out

    assert formatted.index("Render command help") < formatted.index("Usage:")
    assert "Usage: systemc-format OPTIONS COMMAND [OPTIONS] <COMMAND> <SOURCE> <INCLUDE> input\n" in formatted
    assert "              {build,check} [--mode <MODE>]" in formatted
    assert "Positional arguments:" in formatted
    assert "Options:" in formatted
    assert "Global options:" in formatted
    assert formatted.index("Positional arguments:") < formatted.index("Options:") < formatted.index("Global options:")

    assert _without_ansi(output) == formatted
    assert "\x1b[34mUsage:" in output
    assert "\x1b[35msystemc-format" in output
    assert "\x1b[2;38;5;247mOPTIONS" in output
    assert "\x1b[2;38;5;247mCOMMAND" in output
    assert "\x1b[96mbuild" in output
    assert "\x1b[33m<INCLUDE>" in output
    assert "\x1b[32minput" in output
    assert "\x1b[2;36m--mode" in output
    assert "\x1b[33mSOURCE" in output
    assert "\x1b[0m" in output


def test_help_without_usage_or_subcommands_and_lazy_global_options_group(
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
) -> None:
    """Format syntax-only positional help and create global options on plain parsers."""
    monkeypatch.setenv("PYSYSTEMC_COLOR", "never")
    parser = _formatting.HelpParser(prog="minimal", usage=argparse.SUPPRESS)
    parser.add_argument("source", metavar="<SOURCE>")
    help_text = parser.format_help()
    parser.print_help()
    output = capsys.readouterr().out
    assert not help_text.startswith("Usage:")
    assert "Positional arguments:" in help_text
    assert "Global options:" in help_text
    assert "<SOURCE>" in help_text
    assert output == help_text
    assert "\x1b[" not in output

    plain_parser = argparse.ArgumentParser(prog="plain", add_help=False)
    group = _formatting.global_options_group(plain_parser)
    assert _formatting.global_options_group(plain_parser) is group
    _formatting.add_version_argument(plain_parser)
    plain_parser.add_argument("source", help="Source model")
    plain_help = plain_parser.format_help()
    assert "Global options:" in plain_help
    assert "-V, --version" in plain_help
    assert "positional arguments:" in plain_help

    with pytest.raises(SystemExit) as version_exit:
        plain_parser.parse_args(["--version"])
    assert version_exit.value.code == 0
    version_output = capsys.readouterr().out
    assert version_output.startswith("plain v")

"""Shared lightweight formatting for pysystemc console commands."""

import argparse
from importlib.metadata import PackageNotFoundError, version as distribution_version
import os
import re
import sys
from typing import TextIO

from rich.console import Console
from rich.text import Text

_MUTED = "dim grey62"


def _installed_version() -> str:
    """Read the installed package version without importing the native runtime."""
    try:
        return distribution_version("pysystemc")
    except PackageNotFoundError:
        return "unknown"


def _console(file: TextIO | None = None) -> Console:
    """Build a console following the binding's PYSYSTEMC_COLOR policy.

    Auto mode follows terminal detection and NO_COLOR. Explicit always mode
    forces ANSI output and overrides NO_COLOR; never mode always disables it.

    Args:
        file: Output stream; defaults to stdout.

    Returns:
        Rich console configured for the selected stream.
    """
    mode = os.environ.get("PYSYSTEMC_COLOR", "auto")
    force_terminal = True if mode == "always" else False if mode == "never" else None
    no_color = mode == "never" or (mode != "always" and "NO_COLOR" in os.environ)
    return Console(
        file=sys.stdout if file is None else file,
        force_terminal=force_terminal,
        no_color=no_color,
        highlight=False,
        markup=False,
        soft_wrap=True,
    )


def print_version(program: str, version: str | None = None) -> None:
    """Print a program/version label with the configured Rich color behavior.

    Args:
        program: Console program name shown before the version.
        version: Optional version text; defaults to the installed package version.
    """
    label = Text(program, style="magenta")
    label.append(" ")
    label.append(version if version is not None else f"v{_installed_version()}", style="cyan")
    _console().print(label)


def _usage_token_style(token: str, command_names: set[str]) -> str:
    """Choose a usage-token style by syntax and known command names."""
    if token == "[OPTIONS]":
        return "dim cyan"
    if token == "<COMMAND>":
        return "bright_cyan"
    if token == "<INCLUDE>":
        return "yellow"
    if token in {"[", "]", "{", "}", "<", ">", "|", ",", "...", "."}:
        return _MUTED
    if token in {"OPTIONS", "COMMAND"}:
        return _MUTED
    if token in command_names:
        return "bright_cyan"
    if token.startswith("-"):
        return "dim cyan"
    if token.isalpha() and token.upper() == token:
        return "yellow"
    return "green"


def _append_usage_tokens(
    rendered: Text,
    usage: str,
    *,
    program_first: bool,
    command_names: set[str],
) -> None:
    """Append token-aware argparse usage text without changing its spacing."""
    is_program = program_first
    token_pattern = r"\s+|\.{3}|\[OPTIONS\]|<COMMAND>|<INCLUDE>|[<>\[\]{}|,]|[^\s.<>\[\]{}|,]+|\."
    for match in re.finditer(token_pattern, usage):
        token = match.group()
        if token.isspace():
            rendered.append(token)
            continue
        if is_program:
            rendered.append(token, style="magenta")
            is_program = False
        else:
            rendered.append(token, style=_usage_token_style(token, command_names))


def _append_option_column(rendered: Text, option_names: str, command_names: set[str]) -> None:
    """Color option, positional, and choice names within an argparse help row."""
    token_pattern = r"\s+|\.{3}|\[OPTIONS\]|<COMMAND>|<INCLUDE>|[<>\[\]{}|,]|[^\s.<>\[\]{}|,]+|\."
    for match in re.finditer(token_pattern, option_names):
        token = match.group()
        rendered.append(token, style=None if token.isspace() else _usage_token_style(token, command_names))


def _append_help_line(
    rendered: Text,
    line: str,
    *,
    in_usage: bool,
    command_names: set[str],
) -> bool:
    """Append one argparse help line with styles and return usage-block state."""
    if not line:
        rendered.append("\n")
        return False
    if line.startswith("Usage:"):
        rendered.append("Usage:", style="blue")
        remainder = line[len("Usage:") :]
        _append_usage_tokens(rendered, remainder, program_first=True, command_names=command_names)
        rendered.append("\n")
        return True
    if in_usage and line[0].isspace():
        indentation = line[: len(line) - len(line.lstrip())]
        rendered.append(indentation)
        _append_usage_tokens(
            rendered,
            line[len(indentation) :],
            program_first=False,
            command_names=command_names,
        )
        rendered.append("\n")
        return True
    if line == line.strip() and line.endswith(":"):
        rendered.append(line, style="bold green")
        rendered.append("\n")
        return False
    # Argparse uses two-space indentation for ordinary options and four spaces
    # for subcommand rows. Keep continuation and descriptive text muted, while
    # resolving the names in either kind of row with the same token rules used
    # for Usage (flags, metavariables, source positionals, and commands).
    match = re.match(r"^(\s{2,})(.*?)(\s{2,})(\S.*)$", line)
    if match:
        rendered.append(match.group(1))
        _append_option_column(rendered, match.group(2), command_names)
        rendered.append(match.group(3))
        rendered.append(match.group(4), style=_MUTED)
    elif line.startswith("  ") and (
        line.lstrip().startswith("-")
        or (
            len(line.strip().split()) == 1 and (line.strip() == "source" or any(name in line for name in command_names))
        )
    ):
        # argparse omits the description column entirely for arguments without
        # help text, so these syntax-only rows need their own token styling.
        indentation = line[: len(line) - len(line.lstrip())]
        rendered.append(indentation)
        _append_option_column(rendered, line[len(indentation) :], command_names)
    else:
        rendered.append(line, style=_MUTED)
    rendered.append("\n")
    return False


def _styled_help(help_text: str, command_names: set[str]) -> Text:
    """Convert argparse's plain help text to the shared Rich color scheme."""
    rendered = Text()
    in_usage = False
    for line in help_text.splitlines():
        in_usage = _append_help_line(rendered, line, in_usage=in_usage, command_names=command_names)
    return rendered


class HelpParser(argparse.ArgumentParser):
    """Argument parser that renders its existing help layout with Rich colors."""

    def __init__(self, *args, **kwargs):
        """Initialize a parser with the shared Global options group.

        Args:
            *args: Positional arguments forwarded to ArgumentParser.
            **kwargs: Parser options; default argparse help and color handling
                are disabled so this class controls both behaviors.
        """
        kwargs["add_help"] = False
        kwargs["color"] = False
        super().__init__(*args, **kwargs)
        self._global_options_group = self.add_argument_group("Global options")
        self._global_options_group.add_argument("-h", "--help", action="help", help="Show this message and exit")

    def format_help(self) -> str:
        """Return capitalized help text with the description above its usage.

        Returns:
            Plain help text with original wording and wrapping, except for the
            project's section capitalization and long version usage spelling.
        """
        lines = super().format_help().splitlines(keepends=True)
        normalized = []
        for line in lines:
            body = line.rstrip("\r\n")
            ending = line[len(body) :]
            if body.startswith("usage:"):
                body = "Usage:" + body[len("usage:") :]
            elif body == "positional arguments:":
                body = "Positional arguments:"
            elif body == "options:":
                body = "Options:"
            body = body.replace("[-V]", "[--version]")
            normalized.append(body + ending)

        usage_start = next((i for i, line in enumerate(normalized) if line.startswith("Usage:")), None)
        if usage_start is None:
            return "".join(normalized)
        usage_end = next(
            (i for i in range(usage_start + 1, len(normalized)) if not normalized[i].strip()),
            len(normalized),
        )
        section_names = {"Positional arguments:", "Options:", "Global options:"}
        section_start = next(
            (i for i in range(usage_end + 1, len(normalized)) if normalized[i].strip() in section_names),
            len(normalized),
        )
        description = [line for line in normalized[usage_end + 1 : section_start] if line.strip()]
        usage = normalized[usage_start:usage_end]
        sections = normalized[section_start:]
        output = []
        if description:
            output.extend(description)
            output.append("\n")
        output.extend(usage)
        if sections:
            output.append("\n")
            output.extend(sections)
        return "".join(output)

    def print_help(self, file: TextIO | None = None) -> None:
        """Print argparse's help text with shared Rich styling.

        Args:
            file: Output stream; defaults to stdout.
        """
        destination = sys.stdout if file is None else file
        command_names = set(self.prog.split()[1:])
        for action in self._actions:
            choices = action.choices
            if isinstance(choices, dict):
                command_names.update(str(choice) for choice in choices)
            elif isinstance(choices, (list, tuple, set)):
                command_names.update(str(choice) for choice in choices)
        _console(destination).print(_styled_help(self.format_help(), command_names), end="")


class _VersionAction(argparse.Action):
    """Print the styled console program and installed version, then exit."""

    def __init__(self, option_strings, dest, **kwargs):
        """Configure this action as a zero-argument argparse option.

        Args:
            option_strings: Names accepted for the action.
            dest: Namespace attribute name supplied by argparse.
            **kwargs: Remaining argparse action configuration.
        """
        super().__init__(option_strings, dest, nargs=0, **kwargs)

    def __call__(self, parser, namespace, values, option_string=None):
        """Print parser.prog and exit before argparse validates required positionals.

        Args:
            parser: The parser that owns this version option.
            namespace: Parsed argument namespace (unused).
            values: The action value (always None).
            option_string: The spelling used for this option.

        Raises:
            SystemExit: Always, with status zero after printing the version.
        """
        print_version(parser.prog)
        parser.exit()


def global_options_group(parser: argparse.ArgumentParser):
    """Return the parser's shared Global options group, creating one if needed.

    Args:
        parser: Argument parser receiving global command-line options.

    Returns:
        The argument group used for help and version options.
    """
    group = getattr(parser, "_global_options_group", None)
    if group is None:
        group = parser.add_argument_group("Global options")
        setattr(parser, "_global_options_group", group)
    return group


def add_version_argument(parser: argparse.ArgumentParser) -> None:
    """Add the shared Rich-styled --version option to a lightweight parser.

    Args:
        parser: An argparse parser whose prog value is the desired console name.

    The action adds -V/--version to the shared Global options group, prints the
    installed package version, and exits before required source arguments are
    checked. It does not import the native SystemC runtime.

    Returns:
        None. The parser is updated in place.
    """
    global_options_group(parser).add_argument("-V", "--version", action=_VersionAction, help="Show version and exit")

"""Lightweight command-line dispatcher for native C++ SystemC models."""

import argparse
from collections.abc import Sequence
import sys

from amal.eda.cli._formatting import HelpParser, add_version_argument, print_version


def _build_parser() -> argparse.ArgumentParser:
    """Build the C++ runner parser without loading SystemC or cppyy."""
    parser = HelpParser(
        prog="systemc-cpp",
        description="Run native SystemC C++ models.",
        usage="%(prog)s [OPTIONS] <COMMAND>",
    )
    add_version_argument(parser)
    commands = parser.add_subparsers(dest="command")

    run_parser = commands.add_parser(
        "run",
        help="compile and run native SystemC sources",
        description="Compile native SystemC sources and invoke their entry in a fresh interpreter.",
        usage="%(prog)s [OPTIONS] source [source ...]",
    )
    run_parser.add_argument("-I", "--include", action="append", default=[], metavar="<INCLUDE>")
    run_parser.add_argument("source", nargs="+")

    help_parser = commands.add_parser(
        "help",
        help="show help for systemc-cpp or one of its commands",
        description="Show help for systemc-cpp or one of its commands.",
        usage="%(prog)s [OPTIONS] [<COMMAND>]",
    )
    help_parser.add_argument(
        "command_name",
        nargs="?",
        choices=("run", "help", "version"),
        metavar="<COMMAND>",
        help="Optional command (run, help, version); omit to show top-level help.",
    )

    version_parser = commands.add_parser(
        "version",
        help="display the installed systemc-cpp version",
        description="Display the installed SystemC C++ runner version and exit.",
        usage="%(prog)s [OPTIONS]",
    )
    version_parser.set_defaults(command_action="version")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    """Parse C++ runner arguments and invoke native SystemC only when valid.

    Args:
        argv: Arguments without the executable name; defaults to sys.argv[1:].
            Arguments after -- are forwarded unchanged to native sc_main.

    Returns:
        The native sc_main status, or 1 after displaying native initialization
        or runner errors.

    Raises:
        SystemExit: For help, version, or malformed command-line arguments.
    """
    values = list(sys.argv[1:] if argv is None else argv)
    separator = values.index("--") if "--" in values else len(values)
    parser = _build_parser()
    if not values:
        parser.print_help()
        return 0
    parse_values = values[:separator]
    top_options = {"-h", "--help", "-V", "--version"}
    if parse_values and parse_values[0] not in {"run", "help", "version"} | top_options:
        first = parse_values[0]
        if first in {"-I", "--include"} or first.startswith("-I") or first.startswith("--include="):
            parse_values.insert(0, "run")
        elif not first.startswith("-"):
            # Preserve the original direct-source invocation as shorthand.
            parse_values.insert(0, "run")
    options = parser.parse_args(parse_values)

    if options.command == "help":
        if options.command_name is None:
            parser.print_help()
            return 0
        parser.parse_args([options.command_name, "--help"])
        return 0
    if options.command == "version":
        print_version(parser.prog)
        return 0
    if options.command != "run":
        parser.error("no command selected")

    try:
        # Keep argument parsing, help, and version independent from native startup.
        # Import here so runtime initialization failures use the runner's error format.
        from amal.eda.systemc._runner import run_sources

        return run_sources(options.source, values[separator + 1 :], options.include)
    except Exception as error:
        print(f"systemc-cpp: {error}", file=sys.stderr)
        return 1

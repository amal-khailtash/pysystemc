"""Command-line entry point for the systemc-py Python playground."""

import argparse
from importlib.metadata import PackageNotFoundError, version
from pathlib import Path
from collections.abc import Sequence
import sys

from ._formatting import HelpParser, add_version_argument, print_version


def _version() -> str:
    """Read installed distribution metadata without importing native bindings."""
    try:
        return version("pysystemc")
    except PackageNotFoundError:
        return "unknown"


def _build_parser() -> argparse.ArgumentParser:
    """Build the lightweight CLI parser without importing SystemC or cppyy."""
    parser = HelpParser(
        prog="systemc-py",
        usage="%(prog)s [OPTIONS] <COMMAND>",
        description="Explore native SystemC from an interactive Python session.",
    )
    add_version_argument(parser)
    commands = parser.add_subparsers(dest="command")

    run_parser = commands.add_parser(
        "run",
        help="run a Python file with optional arguments",
        description="Run a Python SystemC model as __main__ with optional script arguments.",
    )
    run_parser.usage = "%(prog)s [OPTIONS] source [ARGS ...]"
    run_parser.add_argument("source", metavar="source", help="Python file to execute")
    run_parser.add_argument(
        "script_args",
        nargs=argparse.REMAINDER,
        metavar="ARGS",
        help="arguments passed to the Python file, optionally after --",
    )
    run_parser.set_defaults(action="run")

    repl_parser = commands.add_parser("repl", help="start the interactive SystemC Python playground")
    history = repl_parser.add_mutually_exclusive_group()
    history.add_argument("--history", type=Path, metavar="PATH", help="store prompt history at PATH")
    history.add_argument("--no-history", action="store_true", help="keep prompt history in memory only")
    repl_parser.set_defaults(action="repl")

    help_parser = commands.add_parser(
        "help",
        help="show help for systemc-py or a command",
        description="Show help for systemc-py or one of its commands.",
        usage="%(prog)s [OPTIONS] [<COMMAND>]",
    )
    help_parser.add_argument(
        "command_name",
        nargs="?",
        choices=("run", "repl", "help", "version"),
        metavar="<COMMAND>",
        help="Optional command (run, repl, help, version); omitted to show top-level help.",
    )
    help_parser.set_defaults(action="help")

    version_parser = commands.add_parser(
        "version",
        help="print the installed systemc-py version",
        description="Display the installed SystemC Python bindings version and exit.",
        usage="%(prog)s [OPTIONS]",
    )
    version_parser.set_defaults(action="version")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    """Run the lightweight systemc-py command dispatcher.

    Args:
        argv: Optional arguments, excluding the executable name. With no
            command, the top-level help is printed.

    Returns:
        Process status code. Argparse help and version actions exit with status
        zero directly.

    Raises:
        SystemExit: For malformed command-line arguments or argparse actions.

    The SystemC runtime is imported only after the selected user command
    requests it. ``run`` executes a script as ``__main__`` and exposes that
    script's arguments through ``sys.argv``. A script that initializes
    SystemC owns the process's singleton kernel; run independent simulations
    in separate processes.
    """
    parser = _build_parser()
    args = parser.parse_args(argv)
    if args.command is None:
        parser.print_help()
        return 0
    if args.action == "version":
        print_version(parser.prog, f"v{_version()}")
        return 0
    if args.action == "help":
        if args.command_name is None:
            parser.print_help()
            return 0
        parser.parse_args([args.command_name, "--help"])
        return 0
    if args.action == "run":
        if not Path(args.source).is_file():
            print(f"{parser.prog} run: Python file not found: {args.source}", file=sys.stderr)
            return 1
        from ._run import run_file

        script_args = args.script_args
        if script_args and script_args[0] == "--":
            script_args = script_args[1:]
        run_file(args.source, script_args)
        return 0
    if args.action == "repl":
        # Importing this module first imports amal.eda.systemc and initializes
        # the singleton native kernel, so keep it below command dispatch.
        from amal.eda.systemc.repl import Repl

        Repl(history_path=args.history, no_history=args.no_history).run()
        return 0
    parser.error("no command selected")
    return 2

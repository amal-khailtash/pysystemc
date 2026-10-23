"""Command-line entry point for the pysystemc Python playground."""

import argparse
from importlib.metadata import PackageNotFoundError, version
from pathlib import Path
from collections.abc import Sequence


def _version() -> str:
    """Read installed distribution metadata without importing native bindings."""
    try:
        return version("pysystemc")
    except PackageNotFoundError:
        return "unknown"


def _build_parser() -> argparse.ArgumentParser:
    """Build the lightweight CLI parser without importing SystemC or cppyy."""
    parser = argparse.ArgumentParser(
        prog="pysystemc",
        description="Explore native SystemC from an interactive Python session.",
    )
    parser.add_argument("--version", action="version", version=f"%(prog)s {_version()}")
    commands = parser.add_subparsers(dest="command")

    version_parser = commands.add_parser("version", help="print the installed pysystemc version")
    version_parser.set_defaults(action="version")

    repl_parser = commands.add_parser("repl", help="start the interactive SystemC Python playground")
    history = repl_parser.add_mutually_exclusive_group()
    history.add_argument("--history", type=Path, metavar="PATH", help="store prompt history at PATH")
    history.add_argument("--no-history", action="store_true", help="keep prompt history in memory only")
    repl_parser.set_defaults(action="repl")

    help_parser = commands.add_parser("help", help="show help for pysystemc or a command")
    help_parser.add_argument("command_name", nargs="?", choices=("repl", "version"), metavar="COMMAND")
    help_parser.set_defaults(action="help")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    """Run the lightweight pysystemc command dispatcher.

    Args:
        argv: Optional arguments, excluding the executable name. With no
            command, the top-level help is printed.

    Returns:
        Process status code. Argparse help and version actions exit with status
        zero directly.

    Raises:
        SystemExit: For malformed command-line arguments or argparse actions.

    The SystemC runtime is imported only after the user selects ``repl`` so
    version checks and help remain usable without loading the native kernel.
    """
    parser = _build_parser()
    args = parser.parse_args(argv)
    if args.command is None:
        parser.print_help()
        return 0
    if args.action == "version":
        print(f"pysystemc v{_version()}")
        return 0
    if args.action == "help":
        if args.command_name is None:
            parser.print_help()
            return 0
        parser.parse_args([args.command_name, "--help"])
        return 0
    if args.action == "repl":
        # Importing this module first imports amal.eda.systemc and initializes
        # the singleton native kernel, so keep it below command dispatch.
        from amal.eda.systemc.repl import Repl

        Repl(history_path=args.history, no_history=args.no_history).run()
        return 0
    parser.error("no command selected")
    return 2


if __name__ == "__main__":
    raise SystemExit(main())

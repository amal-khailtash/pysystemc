"""Lightweight command-line dispatcher for native C++ SystemC models."""

import argparse
from collections.abc import Sequence
from importlib.metadata import PackageNotFoundError, version
import sys


def _version() -> str:
    """Read the installed package version without importing native bindings."""
    try:
        return version("pysystemc")
    except PackageNotFoundError:
        return "unknown"


def _build_parser() -> argparse.ArgumentParser:
    """Build the C++ runner parser without loading SystemC or cppyy."""
    parser = argparse.ArgumentParser(
        prog="pysystemc-cpp",
        description="Compile native SystemC sources and invoke their entry in a fresh interpreter.",
    )
    parser.add_argument("--version", action="version", version=f"%(prog)s {_version()}")
    parser.add_argument("-I", "--include", action="append", default=[])
    parser.add_argument("source", nargs="+")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    """Parse C++ runner arguments and invoke native SystemC only when valid.

    Args:
        argv: Arguments without the executable name; defaults to sys.argv[1:].
            Arguments after -- are forwarded unchanged to native sc_main.

    Returns:
        The native sc_main status, or 1 after displaying runner errors.

    Raises:
        SystemExit: For help, version, or malformed command-line arguments.
    """
    values = list(sys.argv[1:] if argv is None else argv)
    separator = values.index("--") if "--" in values else len(values)
    parser = _build_parser()
    options = parser.parse_args(values[:separator])

    # Keep argument parsing, help, and version independent from native startup.
    from amal.eda.systemc._runner import run_sources

    try:
        return run_sources(options.source, values[separator + 1 :], options.include)
    except Exception as error:
        print(f"pysystemc-cpp: {error}", file=sys.stderr)
        return 1

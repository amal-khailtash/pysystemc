"""Compile native SystemC sources and invoke their entry in a fresh interpreter."""

from collections.abc import Sequence
from pathlib import Path
import sys

from ._runtime import cppyy
from .sc_core import sc_elab_and_sim

_invoked = False


def run_sources(sources: Sequence[str], arguments: Sequence[str] = (), includes: Sequence[str] = ()) -> int:
    """Compile sources in order and return native sc_main's integer status.

    Args:
        sources: UTF-8 C++ source filenames, each compiled in order.
        arguments: Arguments after native argv[0], which is the first source.
        includes: Additional include directories.

    Returns:
        The integer status returned by native sc_main.

    Raises:
        ValueError: No sources are supplied or an include path is not a directory.
        OSError: Source/include access fails.
        SyntaxError: Cling raises while parsing or compiling a source.
        RuntimeError: The runner was already invoked or the native entry fails.

    Invoke once per interpreter. Native models and kernel state remain alive
    until interpreter shutdown; unrelated runs require separate subprocesses.
    """
    global _invoked
    if _invoked:
        raise RuntimeError("native C++ runner requires a fresh interpreter for each invocation")
    if not sources:
        raise ValueError("at least one C++ source is required")
    for directory in includes:
        path = Path(directory).resolve(strict=True)
        if not path.is_dir():
            raise ValueError(f"include path is not a directory: {directory}")
        cppyy.add_include_path(str(path))
    # Even a failed compilation can leave declarations in Cling's global scope.
    _invoked = True
    cppyy.cppdef("#define sc_main pysystemc_cpp_main")
    try:
        for filename in sources:
            path = Path(filename).resolve(strict=True)
            cppyy.add_include_path(str(path.parent))
            print(f"\n[pysystemc:C++:info] Reading {filename}", file=sys.stderr)
            cppyy.cppdef(path.read_text(encoding="utf-8"))
    finally:
        cppyy.cppdef("#undef sc_main")
    cppyy.cppdef("pysystemc::native_model_entry = &pysystemc_cpp_main;")
    cppyy.gbl.sc_core.sc_elab_and_sim.__release_gil__ = True
    values = [sources[0], *arguments]
    try:
        return int(sc_elab_and_sim(len(values), values))
    finally:
        cppyy.cppdef("pysystemc::native_model_entry = nullptr;")


def main(argv: Sequence[str] | None = None) -> int:
    """Parse source/include options and '--' native arguments, returning exit status.

    Args:
        argv: CLI arguments without executable name; defaults to sys.argv[1:].

    Returns:
        Native status, or 1 after displaying file/compilation/runtime errors.

    Raises:
        SystemExit: For help, version, or malformed command-line arguments.
    """
    from amal.eda.cli.cpp import main as cli_main

    return cli_main(argv)


if __name__ == "__main__":
    sys.exit(main())

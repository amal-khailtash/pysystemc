"""Compile native SystemC sources and invoke their entry in a fresh interpreter."""

import argparse
from collections.abc import Sequence
from pathlib import Path
import sys

from . import __version__
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
        RuntimeError: Cling rejects a source or the native entry fails.

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
            print(f"[pysystemc:C++:info] Reading {filename}", file=sys.stderr)
            if not cppyy.cppdef(path.read_text(encoding="utf-8")):
                raise RuntimeError(f"C++ compilation failed: {filename}")
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
    """
    values = list(sys.argv[1:] if argv is None else argv)
    separator = values.index("--") if "--" in values else len(values)
    parser = argparse.ArgumentParser(prog="pysystemc-cpp", description=__doc__)
    parser.add_argument("--version", action="version", version=__version__)
    parser.add_argument("-I", "--include", action="append", default=[])
    parser.add_argument("source", nargs="+")
    options = parser.parse_args(values[:separator])
    try:
        return run_sources(options.source, values[separator + 1 :], options.include)
    except Exception as error:
        print(f"pysystemc-cpp: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())

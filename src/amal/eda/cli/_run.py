"""Run Python files through the lightweight systemc-py command."""

import os
from pathlib import Path
import runpy
import sys
from collections.abc import Sequence


def run_file(source: str | os.PathLike[str], script_args: Sequence[str] = ()) -> None:
    """Execute a Python file as ``__main__`` with its sibling directory importable.

    Args:
        source: Python file selected by ``systemc-py run``.
        script_args: Arguments exposed to the file after its ``sys.argv[0]``.

    Returns:
        None when the file finishes normally. A script's ``SystemExit`` status
        and other exceptions propagate to the caller.

    Raises:
        FileNotFoundError: If ``source`` is missing or is not a regular file.
        SystemExit: If the executed file exits explicitly.
        Exception: Any uncaught exception raised by the executed file.

    The current working directory is preserved. ``sys.argv`` and ``sys.path``
    are restored even when the script exits or raises. A script that imports
    SystemC can initialize its process-wide singleton kernel; launch separate
    processes for independent simulations rather than attempting to reset it.
    """
    source_argument = os.fspath(source)
    source_path = Path(source_argument)
    if not source_path.is_file():
        raise FileNotFoundError(f"Python file not found: {source_argument}")
    resolved_source = source_path.resolve()

    original_argv = sys.argv
    original_path = sys.path
    saved_path = original_path[:]
    try:
        sys.argv = [source_argument, *script_args]
        sys.path.insert(0, str(resolved_source.parent))
        runpy.run_path(source_argument, run_name="__main__")
    finally:
        sys.argv = original_argv
        original_path[:] = saved_path
        sys.path = original_path

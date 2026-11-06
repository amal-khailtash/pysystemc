"""Prepare native-first loader ordering during ordinary cppyy startup.

Single-threaded Linux startup creates and owns a private temporary copy of
glibc's ``ldd`` and changes ``PATH`` only for the cppyy import. Observable Python
thread concurrency uses ordinary imports unless explicitly enabled. Concurrent
native threads or threads starting during import can still observe that PATH.
"""

from __future__ import annotations

import importlib
import os
import re
import shutil
import sys
import tempfile
import threading
from pathlib import Path
from types import ModuleType


_RTLDLIST = re.compile(
    rb"(?m)^(?P<prefix>[ \t]*RTLDLIST[ \t]*=[ \t]*)"
    rb"(?P<quote>['\"])(?P<value>[^'\"\r\n]*)(?P=quote)"
    rb"(?P<tail>[ \t]*(?:\#[^\r\n]*)?\r?$)"
)
_SAFE_LOADER_TOKEN = re.compile(r"/[A-Za-z0-9_./+-]+\Z")


def _elf_class(path: str | os.PathLike[str]) -> int:
    """Read the ELF class byte for *path*.

    Raises:
        RuntimeError: If the file cannot be read or is not a supported ELF.
    """
    try:
        with open(path, "rb") as elf:
            header = elf.read(5)
    except OSError as error:
        raise RuntimeError(f"unsupported ldd adapter: cannot read ELF file {path}: {error}") from error
    if len(header) < 5 or header[:4] != b"\x7fELF" or header[4] not in (1, 2):
        raise RuntimeError(f"unsupported ldd adapter: {path} is not a supported ELF file")
    return header[4]


def _prepare_ldd(directory: str | os.PathLike[str]) -> Path:
    """Write a private ldd copy with loaders matching Python tried first.

    The source script is read only. Optional missing or unreadable loaders keep
    their relative order after loaders whose ELF class matches Python's.

    Args:
        directory: Temporary directory owned by the caller.

    Returns:
        The path to the prepared executable copy.

    Raises:
        OSError: If ldd cannot be read or the private copy cannot be written.
        RuntimeError: If ldd is unsupported or no reordering is needed.
    """
    original_path = shutil.which("ldd")
    if original_path is None:
        raise RuntimeError("unsupported ldd adapter: cannot resolve ldd from PATH")

    source = Path(original_path).read_bytes()
    matches = list(_RTLDLIST.finditer(source))
    if b"GNU C Library" not in source or len(matches) != 1:
        raise RuntimeError("unsupported ldd adapter: ldd has no simple quoted RTLDLIST assignment")
    match = matches[0]

    try:
        original_loaders = match.group("value").decode("ascii").split()
    except UnicodeDecodeError as error:
        raise RuntimeError("unsupported ldd adapter: RTLDLIST is not ASCII") from error
    if not original_loaders:
        raise RuntimeError("unsupported ldd adapter: ldd RTLDLIST is empty")
    if any(_SAFE_LOADER_TOKEN.fullmatch(loader) is None for loader in original_loaders):
        raise RuntimeError("unsupported ldd adapter: RTLDLIST contains an unsafe loader token")

    native_class = _elf_class(sys.executable)

    # Preserve missing compatibility loaders as fallbacks. Only readable ELF
    # loaders can be classified, and their relative order remains stable.
    loader_classes: dict[str, int | None] = {}
    for loader in dict.fromkeys(original_loaders):
        if not os.path.isfile(loader) or not os.access(loader, os.X_OK):
            loader_classes[loader] = None
            continue
        try:
            loader_classes[loader] = _elf_class(loader)
        except RuntimeError:
            loader_classes[loader] = None

    if not any(loader_classes[loader] == native_class for loader in original_loaders):
        raise RuntimeError("unsupported ldd adapter: RTLDLIST has no loader matching Python's ELF class")
    reordered_loaders = [loader for loader in original_loaders if loader_classes[loader] == native_class]
    reordered_loaders.extend(loader for loader in original_loaders if loader_classes[loader] != native_class)
    if reordered_loaders == original_loaders:
        raise RuntimeError("unsupported ldd adapter: RTLDLIST needs no loader reordering")

    rewritten = (
        source[: match.start("value")] + " ".join(reordered_loaders).encode("ascii") + source[match.end("value") :]
    )
    target = Path(directory) / "ldd"
    target.write_bytes(rewritten)
    target.chmod(0o755)
    return target


def _cleanup_temporary_directory(directory: tempfile.TemporaryDirectory[str]) -> None:
    """Attempt cleanup without masking a cppyy import exception."""
    try:
        directory.cleanup()
    except OSError:
        # The import result or exception is more useful than a cleanup error;
        # TemporaryDirectory also retries cleanup when its finalizer runs.
        pass


def import_cppyy() -> ModuleType:
    """Import and return cppyy, preferring Python-class ELF loaders at startup.

    Native-first loader ordering defaults to enabled on Linux when importing
    from the main Python thread with no other Python threads. Set
    ``PYSC_NATIVE_FIRST_LDD=0`` to disable it or ``1`` to enable it explicitly.
    Other explicit values use ordinary imports. It modifies this
    process's ``PATH`` only while importing cppyy, then restores the original
    presence and value exactly. Since ``PATH`` is process-wide, concurrent
    executable lookups can observe the temporary value during that import.

    The private ldd adapter supports only a simple, quoted glibc ``RTLDLIST``
    assignment with absolute shell-safe loader tokens. Unsupported layouts or
    preparation failures use an ordinary cppyy import. Errors raised by the
    cppyy import itself propagate unchanged.

    Returns:
        The imported cppyy module.
    """
    if "cppyy" in sys.modules:
        return importlib.import_module("cppyy")

    preference = os.environ.get("PYSC_NATIVE_FIRST_LDD")
    enabled = preference == "1" or (
        preference is None and threading.current_thread() is threading.main_thread() and threading.active_count() == 1
    )
    if not enabled or not sys.platform.startswith("linux"):
        return importlib.import_module("cppyy")

    try:
        temporary_directory = tempfile.TemporaryDirectory(prefix="pysystemc-ldd-")
    except OSError, RuntimeError:
        return importlib.import_module("cppyy")

    try:
        try:
            private_ldd = _prepare_ldd(temporary_directory.name)
        except OSError, RuntimeError:
            private_ldd = None

        if private_ldd is None:
            return importlib.import_module("cppyy")

        path_was_present = "PATH" in os.environ
        original_path = os.environ.get("PATH")
        try:
            if path_was_present:
                os.environ["PATH"] = f"{temporary_directory.name}{os.pathsep}{original_path}"
            else:
                os.environ["PATH"] = temporary_directory.name + os.pathsep + os.defpath
            return importlib.import_module("cppyy")
        finally:
            if path_was_present:
                # original_path is a string whenever PATH was present.
                os.environ["PATH"] = original_path if original_path is not None else ""
            else:
                os.environ.pop("PATH", None)
    finally:
        _cleanup_temporary_directory(temporary_directory)

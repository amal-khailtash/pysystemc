"""Verify bootstrap defaults, fallback, temporary resources and restoration."""

import importlib.util
import os
from pathlib import Path
from types import ModuleType, SimpleNamespace
import builtins

import pytest


@pytest.fixture
def bootstrap():
    """Load the bootstrap helper without initializing the native kernel."""
    path = Path(__file__).resolve().parents[1] / "src/amal/eda/systemc/_bootstrap.py"
    spec = importlib.util.spec_from_file_location("bootstrap_test", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.fixture
def loaders(bootstrap, tmp_path, monkeypatch):
    """Provide real ELF headers and a synthetic recognizable ldd script."""
    paths = [tmp_path / name for name in ("compat", "native", "missing")]
    for path, elf_class in zip(paths, (1, 2)):
        path.write_bytes(b"\x7fELF" + bytes([elf_class]))
        path.chmod(0o755)
    script = tmp_path / "system-ldd"
    script.write_text('#!/bin/sh\n# GNU C Library\nRTLDLIST="' + " ".join(map(str, paths)) + '"\n')
    monkeypatch.setattr(bootstrap.shutil, "which", lambda name: str(script))
    monkeypatch.setattr(bootstrap.sys, "executable", str(paths[1]))
    monkeypatch.setattr(bootstrap.sys, "platform", "linux")
    monkeypatch.setenv("PYSC_NATIVE_FIRST_LDD", "1")
    monkeypatch.delitem(bootstrap.sys.modules, "cppyy", raising=False)
    return script, paths


@pytest.mark.parametrize("original_path", [None, "", "/original/bin"])
@pytest.mark.parametrize("raises", [False, True])
@pytest.mark.parametrize("native_first_flag", [None, "1"])
def test_bootstrap_restores_path_and_resources(
    bootstrap, loaders, monkeypatch, original_path, raises, native_first_flag
):
    """Restore exact PATH and delete the private copy on success and import failure."""
    script, paths = loaders
    source = script.read_bytes()
    if native_first_flag is None:
        monkeypatch.delenv("PYSC_NATIVE_FIRST_LDD")
        # Model the safe default conditions independently of pytest's threads.
        main_thread = object()
        monkeypatch.setattr(
            bootstrap,
            "threading",
            SimpleNamespace(
                current_thread=lambda: main_thread,
                main_thread=lambda: main_thread,
                active_count=lambda: 1,
            ),
        )
    else:
        monkeypatch.setenv("PYSC_NATIVE_FIRST_LDD", native_first_flag)
        worker_thread = object()
        main_thread = object()
        monkeypatch.setattr(
            bootstrap,
            "threading",
            SimpleNamespace(
                current_thread=lambda: worker_thread,
                main_thread=lambda: main_thread,
                active_count=lambda: 2,
            ),
        )
    if original_path is None:
        monkeypatch.delenv("PATH", raising=False)
    else:
        monkeypatch.setenv("PATH", original_path)
    result = ModuleType("fake_cppyy")
    error = ImportError("native import failed")
    copies = []

    def importing(name):
        """Inspect the live adapter and simulate an import result or exception."""
        assert name == "cppyy"
        private = Path(os.environ["PATH"].split(os.pathsep)[0]) / "ldd"
        copies.append(private)
        assert private.read_text().split('RTLDLIST="')[1].split('"')[0].split() == list(
            map(str, (paths[1], paths[0], paths[2]))
        )
        assert os.access(private, os.X_OK)
        if raises:
            raise error
        return result

    monkeypatch.setattr(bootstrap.importlib, "import_module", importing)
    if raises:
        with pytest.raises(ImportError) as caught:
            bootstrap.import_cppyy()
        assert caught.value is error
    else:
        assert bootstrap.import_cppyy() is result
    assert os.environ.get("PATH") == original_path
    assert not copies[0].parent.exists()
    assert script.read_bytes() == source


@pytest.mark.parametrize("reason", ["disabled", "platform", "unrecognized", "unsafe", "preparation"])
def test_bootstrap_falls_back(bootstrap, loaders, monkeypatch, reason):
    """Unsupported settings and preparation failures retain ordinary cppyy imports."""
    script, _ = loaders
    before = os.environ.get("PATH")
    if reason == "disabled":
        monkeypatch.setenv("PYSC_NATIVE_FIRST_LDD", "0")
    elif reason == "platform":
        monkeypatch.setattr(bootstrap.sys, "platform", "darwin")
    elif reason == "unrecognized":
        script.write_text("#!/bin/sh\necho custom ldd\n")
    elif reason == "unsafe":
        script.write_text('# GNU C Library\nRTLDLIST="/lib/$(command)"\n')
    else:

        def failed_prepare(directory):
            """Simulate an unwritable temporary adapter."""
            raise OSError("unwritable")

        monkeypatch.setattr(bootstrap, "_prepare_ldd", failed_prepare)
    result = ModuleType("fake_cppyy")

    def importing(name):
        """Assert fallback never changes the import environment."""
        assert name == "cppyy" and os.environ.get("PATH") == before
        return result

    monkeypatch.setattr(bootstrap.importlib, "import_module", importing)
    assert bootstrap.import_cppyy() is result


@pytest.mark.parametrize("thread_state", ["worker", "multiple"])
def test_default_thread_conditions_fall_back(bootstrap, loaders, monkeypatch, thread_state):
    """Use an ordinary import by default outside the single main-thread case."""
    del loaders
    monkeypatch.delenv("PYSC_NATIVE_FIRST_LDD")
    main_thread = object()
    worker_thread = object()
    monkeypatch.setattr(
        bootstrap,
        "threading",
        SimpleNamespace(
            current_thread=lambda: worker_thread if thread_state == "worker" else main_thread,
            main_thread=lambda: main_thread,
            active_count=lambda: 1 if thread_state == "worker" else 2,
        ),
    )
    before = os.environ.get("PATH")
    result = ModuleType("fake_cppyy")

    def importing(name):
        """Confirm unsafe default conditions do not alter PATH."""
        assert name == "cppyy" and os.environ.get("PATH") == before
        return result

    monkeypatch.setattr(bootstrap.importlib, "import_module", importing)
    assert bootstrap.import_cppyy() is result


def test_already_imported_cppyy_bypasses_loader_preparation(bootstrap, loaders, monkeypatch):
    """Return an already-loaded cppyy module without preparing the ldd adapter."""
    del loaders
    result = ModuleType("fake_cppyy")
    monkeypatch.setitem(bootstrap.sys.modules, "cppyy", result)

    def unexpected_prepare(directory):
        """Fail if the already-loaded fast path attempts loader preparation."""
        raise AssertionError(f"unexpected ldd preparation in {directory}")

    monkeypatch.setattr(bootstrap, "_prepare_ldd", unexpected_prepare)
    assert bootstrap.import_cppyy() is result


def _fallback_importer(bootstrap, monkeypatch, original_path):
    """Install an ordinary fake cppyy import and return its identity sentinel."""
    result = ModuleType("fake_cppyy")

    def importing(name):
        """Assert fallback imports cppyy without changing PATH."""
        assert name == "cppyy"
        assert os.environ.get("PATH") == original_path
        return result

    monkeypatch.setattr(bootstrap.importlib, "import_module", importing)
    return result


def _track_temporary_directories(bootstrap, monkeypatch):
    """Record real adapter temp directories so fallback cleanup can be asserted."""
    original_factory = bootstrap.tempfile.TemporaryDirectory
    directories = []

    def create(*args, **kwargs):
        """Create and retain one owned temporary directory instance."""
        directory = original_factory(*args, **kwargs)
        directories.append(directory)
        return directory

    monkeypatch.setattr(bootstrap.tempfile, "TemporaryDirectory", create)
    return directories


@pytest.mark.parametrize(
    ("contents", "message", "cause"),
    [
        (b'#!/bin/sh\n# GNU C Library\nRTLDLIST="\xff"\n', "RTLDLIST is not ASCII", UnicodeDecodeError),
        (b'#!/bin/sh\n# GNU C Library\nRTLDLIST=""\n', "RTLDLIST is empty", None),
    ],
    ids=("non-ascii", "empty"),
)
def test_bad_rtld_lists_report_and_fall_back(bootstrap, loaders, monkeypatch, tmp_path, contents, message, cause):
    """Reject malformed native-loader lists, then import fake cppyy unchanged."""
    script, _ = loaders
    script.write_bytes(contents)
    source = script.read_bytes()
    with pytest.raises(RuntimeError, match=message) as caught:
        bootstrap._prepare_ldd(tmp_path)
    if cause is None:
        assert caught.value.__cause__ is None
    else:
        assert isinstance(caught.value.__cause__, cause)

    original_path = os.environ.get("PATH")
    directories = _track_temporary_directories(bootstrap, monkeypatch)
    result = _fallback_importer(bootstrap, monkeypatch, original_path)
    assert bootstrap.import_cppyy() is result
    assert os.environ.get("PATH") == original_path
    assert len(directories) == 1 and not Path(directories[0].name).exists()
    assert script.read_bytes() == source


def test_missing_ldd_reports_and_falls_back(bootstrap, loaders, monkeypatch, tmp_path):
    """Diagnose missing ldd and preserve PATH/source while ordinary import falls back."""
    script, _ = loaders
    source = script.read_bytes()
    monkeypatch.setattr(bootstrap.shutil, "which", lambda name: None)
    with pytest.raises(RuntimeError, match="cannot resolve ldd from PATH"):
        bootstrap._prepare_ldd(tmp_path)

    original_path = os.environ.get("PATH")
    directories = _track_temporary_directories(bootstrap, monkeypatch)
    result = _fallback_importer(bootstrap, monkeypatch, original_path)
    assert bootstrap.import_cppyy() is result
    assert os.environ.get("PATH") == original_path
    assert len(directories) == 1 and not Path(directories[0].name).exists()
    assert script.read_bytes() == source


@pytest.mark.parametrize(
    ("contents", "message"),
    [
        (b"\x7fELF", "not a supported ELF file"),
        (b"not an ELF file", "not a supported ELF file"),
        (b"\x7fELF\x03", "not a supported ELF file"),
    ],
    ids=("short-header", "non-elf", "unsupported-class"),
)
def test_elf_class_rejects_malformed_headers(bootstrap, tmp_path, contents, message):
    """Reject truncated, non-ELF and unsupported-class loader headers."""
    path = tmp_path / "malformed-loader"
    path.write_bytes(contents)
    with pytest.raises(RuntimeError, match=message) as caught:
        bootstrap._elf_class(path)
    assert caught.value.__cause__ is None


def test_elf_class_reports_unreadable_file(bootstrap, tmp_path, monkeypatch):
    """Preserve the permission error as the cause for an unreadable ELF file."""
    path = tmp_path / "unreadable-loader"
    path.write_bytes(b"\x7fELF\x02")
    original_open = builtins.open

    def unreadable(file, *args, **kwargs):
        """Deny only the selected loader while leaving test files readable."""
        if Path(file) == path:
            raise PermissionError("controlled unreadable ELF")
        return original_open(file, *args, **kwargs)

    monkeypatch.setattr(builtins, "open", unreadable)
    with pytest.raises(RuntimeError, match="cannot read ELF file") as caught:
        bootstrap._elf_class(path)
    assert isinstance(caught.value.__cause__, PermissionError)
    assert "controlled unreadable ELF" in str(caught.value.__cause__)


def test_unreadable_optional_loader_is_retained(bootstrap, loaders, monkeypatch, tmp_path):
    """Keep an unreadable optional loader after the readable Python-class loader."""
    script, paths = loaders
    source = script.read_bytes()
    original_open = builtins.open

    def unreadable(file, *args, **kwargs):
        """Simulate denied reads for one executable loader despite root access."""
        if Path(file) == paths[0]:
            raise PermissionError("controlled optional-loader read denial")
        return original_open(file, *args, **kwargs)

    monkeypatch.setattr(builtins, "open", unreadable)
    prepared = bootstrap._prepare_ldd(tmp_path)
    tokens = prepared.read_text().split('RTLDLIST="')[1].split('"')[0].split()
    assert tokens == [str(paths[1]), str(paths[0]), str(paths[2])]
    assert prepared.stat().st_mode & 0o111
    assert script.read_bytes() == source


@pytest.mark.parametrize("ordering", ["no_matching", "already_native"], ids=("no-matching", "already-native"))
def test_unusable_or_unneeded_ordering_falls_back(bootstrap, loaders, monkeypatch, tmp_path, ordering):
    """Use an ordinary import when no matching loader exists or order is already native."""
    script, paths = loaders
    if ordering == "no_matching":
        paths[1].write_bytes(b"\x7fELF\x01")
        python_elf = tmp_path / "python-elf"
        python_elf.write_bytes(b"\x7fELF\x02")
        python_elf.chmod(0o755)
        monkeypatch.setattr(bootstrap.sys, "executable", str(python_elf))
        message = "no loader matching Python's ELF class"
        ordered = paths
    else:
        ordered = [paths[1], paths[0], paths[2]]
        script.write_text(f'#!/bin/sh\n# GNU C Library\nRTLDLIST="{" ".join(map(str, ordered))}"\n')
        message = "needs no loader reordering"
    source = script.read_bytes()
    with pytest.raises(RuntimeError, match=message) as caught:
        bootstrap._prepare_ldd(tmp_path)
    assert caught.value.__cause__ is None

    original_path = os.environ.get("PATH")
    directories = _track_temporary_directories(bootstrap, monkeypatch)
    result = _fallback_importer(bootstrap, monkeypatch, original_path)
    assert bootstrap.import_cppyy() is result
    assert os.environ.get("PATH") == original_path
    assert len(directories) == 1 and not Path(directories[0].name).exists()
    assert script.read_bytes() == source


def test_temp_directory_creation_failure_falls_back(bootstrap, loaders, monkeypatch):
    """Ordinary import survives a failure before adapter resources are allocated."""
    del loaders
    original_path = os.environ.get("PATH")
    calls = []

    def unavailable(*args, **kwargs):
        """Record the native-first request then fail before creating resources."""
        calls.append(kwargs.get("prefix"))
        raise PermissionError("controlled temporary-directory denial")

    monkeypatch.setattr(bootstrap.tempfile, "TemporaryDirectory", unavailable)
    result = _fallback_importer(bootstrap, monkeypatch, original_path)
    assert bootstrap.import_cppyy() is result
    assert calls == ["pysystemc-ldd-"]
    assert os.environ.get("PATH") == original_path


def test_cleanup_oserror_does_not_mask_cppyy_import_error(bootstrap, loaders, monkeypatch):
    """Preserve the import exception and clean the owned directory after a failed cleanup."""
    del loaders
    original_factory = bootstrap.tempfile.TemporaryDirectory
    original_path = os.environ.get("PATH")
    import_error = ImportError("controlled cppyy import failure")
    directories = []

    class CleanupFailure:
        """Expose real temp storage while making the adapter cleanup fail once."""

        def __init__(self, *args, **kwargs):
            """Create a real owned directory and count cleanup attempts."""
            self.actual = original_factory(*args, **kwargs)
            self.name = self.actual.name
            self.cleanup_calls = 0

        def cleanup(self):
            """Raise the controlled cleanup error without hiding import failure."""
            self.cleanup_calls += 1
            raise OSError("controlled cleanup denial")

        def dispose(self):
            """Run the real cleanup after the production helper returns."""
            self.actual.cleanup()

    def create(*args, **kwargs):
        """Create and retain one proxy around real temporary storage."""
        directory = CleanupFailure(*args, **kwargs)
        directories.append(directory)
        return directory

    def importing(name):
        """Raise the exact import exception after inspecting native-first PATH."""
        assert name == "cppyy"
        assert os.environ["PATH"].startswith(directories[0].name + os.pathsep)
        raise import_error

    monkeypatch.setattr(bootstrap.tempfile, "TemporaryDirectory", create)
    monkeypatch.setattr(bootstrap.importlib, "import_module", importing)
    try:
        with pytest.raises(ImportError) as caught:
            bootstrap.import_cppyy()
        assert caught.value is import_error
        assert directories[0].cleanup_calls == 1
        assert Path(directories[0].name).exists()
        assert os.environ.get("PATH") == original_path
    finally:
        directories[0].dispose()
    assert not Path(directories[0].name).exists()

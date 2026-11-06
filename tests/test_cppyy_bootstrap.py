"""Verify bootstrap defaults, fallback, temporary resources and restoration."""

import importlib.util
import os
from pathlib import Path
from types import ModuleType, SimpleNamespace

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

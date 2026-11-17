"""Native-free tests for packaging the SystemC shared library safely."""

import base64
import csv
import hashlib
import importlib.util
import io
import sys
import tomllib
import types
import zipfile
from pathlib import Path
import subprocess
import sysconfig

import pytest


ROOT = Path(__file__).resolve().parents[1]
SONAME_MARKER = b"\nFAKE_SONAME="


def _load_backend(monkeypatch):
    """Load the PEP 517 backend with a native-free uv_build stub.

    Args:
        monkeypatch: Pytest fixture isolating the imported uv_build module.

    Returns:
        module: The loaded backend module.
    """
    uv_build = types.ModuleType("uv_build")
    for name in (
        "build_wheel",
        "build_sdist",
        "build_editable",
        "prepare_metadata_for_build_wheel",
        "get_requires_for_build_wheel",
        "get_requires_for_build_sdist",
        "get_requires_for_build_editable",
    ):
        setattr(uv_build, name, lambda *args, **kwargs: None)
    monkeypatch.setitem(sys.modules, "uv_build", uv_build)
    spec = importlib.util.spec_from_file_location("pysystemc_test_build_backend", ROOT / "build_backend/backend.py")
    assert spec is not None and spec.loader is not None
    backend = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(backend)
    return backend


def _stub_patchelf(monkeypatch, *, printed_soname="libsystemc.so", fail_set=False):
    """Simulate patchelf's two CLI operations on opaque test bytes.

    Args:
        monkeypatch: Fixture intercepting the backend's subprocess calls.
        printed_soname: SONAME returned by the fake ``--print-soname`` call.
        fail_set: If true, fail before modifying the temporary copy.

    Returns:
        list[Path]: Paths passed to fake ``patchelf`` commands.
    """
    calls = []

    def run(args, **kwargs):
        assert args[0] == "patchelf"
        copy = Path(args[-1])
        calls.append(copy)
        if args[1] == "--set-soname":
            if fail_set:
                raise subprocess.CalledProcessError(1, args, stderr="fake patchelf failure")
            copy.write_bytes(copy.read_bytes() + SONAME_MARKER + args[2].encode() + b"\n")
            return subprocess.CompletedProcess(args, 0)
        assert args[1] == "--print-soname"
        return subprocess.CompletedProcess(args, 0, stdout=printed_soname + "\n", stderr="")

    monkeypatch.setattr(subprocess, "run", run)
    return calls


def test_soname_rewrite_mutates_only_temporary_copy_and_cleans_it(monkeypatch, tmp_path):
    """Normalize copied bytes and leave both source and staging path untouched.

    Args:
        monkeypatch: Fixture isolating patchelf calls.
        tmp_path: Temporary directory containing an opaque fake shared library.

    Returns:
        None: Source bytes and temporary-copy lifetime are asserted.
    """
    backend = _load_backend(monkeypatch)
    calls = _stub_patchelf(monkeypatch)
    source = tmp_path / "libsystemc.so.2.3.4"
    source.write_bytes(b"fake library bytes")
    original = source.read_bytes()

    packaged = backend._systemc_wheel_payload(source)

    assert packaged == original + SONAME_MARKER + b"libsystemc.so\n"
    assert source.read_bytes() == original
    assert len(calls) == 2
    assert calls[0] == calls[1]
    assert calls[0] != source
    assert calls[0].name == "libsystemc.so"
    assert not calls[0].exists()


def test_soname_rewrite_cleans_temporary_copy_when_patchelf_fails(monkeypatch, tmp_path):
    """Propagate tool errors while preserving the source and cleaning staging.

    Args:
        monkeypatch: Fixture isolating patchelf calls.
        tmp_path: Temporary directory containing an opaque fake shared library.

    Returns:
        None: Failure propagation, source preservation, and cleanup are asserted.
    """
    backend = _load_backend(monkeypatch)
    calls = _stub_patchelf(monkeypatch, fail_set=True)
    source = tmp_path / "libsystemc.so.2.3.4"
    source.write_bytes(b"original library")

    with pytest.raises(subprocess.CalledProcessError):
        backend._systemc_wheel_payload(source)

    assert source.read_bytes() == b"original library"
    assert len(calls) == 1
    assert calls[0] != source
    assert not calls[0].exists()


def test_soname_rewrite_rejects_wrong_printed_soname(monkeypatch, tmp_path):
    """Reject an unexpected SONAME and clean up without changing the source.

    Args:
        monkeypatch: Fixture isolating patchelf calls.
        tmp_path: Temporary directory containing an opaque fake shared library.

    Returns:
        None: SONAME validation, source preservation, and temporary cleanup are asserted.
    """
    backend = _load_backend(monkeypatch)
    calls = _stub_patchelf(monkeypatch, printed_soname="libsystemc.so.2.3")
    source = tmp_path / "libsystemc.so.2.3.4"
    source.write_bytes(b"original library")

    with pytest.raises(RuntimeError, match="Expected packaged SystemC SONAME"):
        backend._systemc_wheel_payload(source)

    assert source.read_bytes() == b"original library"
    assert len(calls) == 2
    assert calls[0] == calls[1]
    assert not calls[0].exists()


def test_patchelf_is_required_for_builds_but_not_runtime():
    """Keep patchelf scoped to the PEP 517 build environment.

    Returns:
        None: Build-system and runtime dependency declarations are compared.
    """
    project = tomllib.loads((ROOT / "pyproject.toml").read_text())

    assert any(requirement.startswith("patchelf>=") for requirement in project["build-system"]["requires"])
    assert all(not requirement.startswith("patchelf") for requirement in project["project"]["dependencies"])


def test_wheel_has_one_normalized_systemc_payload_and_valid_record(monkeypatch, tmp_path):
    """Package one normalized payload and regenerate platform wheel metadata.

    The fake patchelf implementation edits opaque bytes only; this test does not
    model ELF structure, ABI acceptance, or successful dynamic loading.

    Args:
        monkeypatch: Fixture isolating native build and patchelf calls.
        tmp_path: Temporary fake native prefix and wheel output directory.

    Returns:
        None: Payload uniqueness, source immutability, tag, WHEEL and RECORD are asserted.
    """
    backend = _load_backend(monkeypatch)
    calls = _stub_patchelf(monkeypatch)
    native = tmp_path / "native"
    library_dir = native / "lib"
    library_dir.mkdir(parents=True)
    versioned = library_dir / "libsystemc.so.2.3.4"
    original = b"opaque fake SystemC shared library"
    versioned.write_bytes(original)
    (library_dir / "libsystemc.so.2.3").symlink_to(versioned.name)
    (library_dir / "libsystemc.so").symlink_to("libsystemc.so.2.3")
    (native / "include").mkdir()
    (native / "include/systemc").write_text("fake header\n")
    monkeypatch.setattr(backend, "NATIVE", native)
    monkeypatch.setattr(backend, "_native", lambda: None)

    def build_wheel(wheel_directory, config_settings=None, metadata_directory=None):
        """Write a minimal pure wheel for the packaging backend to transform."""
        name = "pysystemc-1.0.0-py3-none-any.whl"
        path = Path(wheel_directory) / name
        with zipfile.ZipFile(path, "w") as archive:
            archive.writestr(
                "pysystemc-1.0.0.dist-info/WHEEL",
                "Wheel-Version: 1.0\nGenerator: test\nRoot-Is-Purelib: true\nTag: py3-none-any\n\n",
            )
            archive.writestr("pysystemc-1.0.0.dist-info/RECORD", "")
        return name

    backend.uv_build.build_wheel = build_wheel
    output = tmp_path / "wheelhouse"
    output.mkdir()

    wheel_name = backend.build_wheel(output)

    platform_tag = sysconfig.get_platform().replace("-", "_").replace(".", "_")
    expected_tag = "py3-none-" + platform_tag
    wheel_path = output / wheel_name
    assert wheel_name.endswith(expected_tag + ".whl")
    assert versioned.read_bytes() == original
    assert len(calls) == 2 and calls[0] == calls[1]

    with zipfile.ZipFile(wheel_path) as archive:
        names = archive.namelist()
        systemc_names = [name for name in names if "_native/lib/libsystemc.so" in name]
        assert systemc_names == ["amal/eda/systemc/_native/lib/libsystemc.so"]
        payload = archive.read(systemc_names[0])
        assert payload == original + SONAME_MARKER + b"libsystemc.so\n"
        wheel_metadata = archive.read("pysystemc-1.0.0.dist-info/WHEEL").decode()
        assert "Root-Is-Purelib: false" in wheel_metadata
        assert "Tag: " + expected_tag in wheel_metadata
        record_name = "pysystemc-1.0.0.dist-info/RECORD"
        record = list(csv.reader(io.StringIO(archive.read(record_name).decode())))
        record_rows = {row[0]: row[1:] for row in record}
        assert set(record_rows) == set(names)
        assert record_rows[record_name] == ["", ""]
        expected_digest = base64.urlsafe_b64encode(hashlib.sha256(payload).digest()).rstrip(b"=").decode()
        assert record_rows[systemc_names[0]] == ["sha256=" + expected_digest, str(len(payload))]

"""Validate conditional socket getters in a separately built matching kernel."""

import os
from pathlib import Path
import shlex
import subprocess
import sys

from .support import ROOT


def _run_covariant(command, workdir, *, child_configuration=None, timeout=60):
    """Run a bounded native build or fresh kernel and expose failure output."""
    result = subprocess.run(
        command,
        cwd=workdir,
        env=dict(os.environ, **child_configuration) if child_configuration is not None else None,
        capture_output=True,
        text=True,
        timeout=timeout,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def test_covariant_base_socket_getters_match_native(tmp_path):
    """Check exact cv-qualified base getters, protocols and borrowed lifetimes.

    A dedicated native build is necessary because SystemC validates this macro
    against its compiled library. An existing matching test installation may be
    selected with PYSYSTEMC_COVARIANT_TEST_PREFIX to reuse a prepared kernel.
    Compiler flags and prefix selection are restricted to child processes.
    """
    prefix_override = os.environ.get("PYSYSTEMC_COVARIANT_TEST_PREFIX")
    prefix = Path(prefix_override) if prefix_override else tmp_path / "install"
    if not prefix_override:
        build = tmp_path / "build"
        _run_covariant(
            [
                "cmake",
                "-S",
                str(ROOT / "submodules/systemc"),
                "-B",
                str(build),
                f"-DCMAKE_INSTALL_PREFIX={prefix}",
                "-DCMAKE_INSTALL_LIBDIR=lib",
                "-DCMAKE_BUILD_TYPE=Release",
                "-DCMAKE_CXX_STANDARD=17",
                "-DCMAKE_CXX_FLAGS=-DSC_ENABLE_COVARIANT_VIRTUAL_BASE",
                "-DENABLE_PTHREADS=ON",
                "-DBUILD_SHARED_LIBS=ON",
                "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
            ],
            tmp_path,
        )
        _run_covariant(["cmake", "--build", str(build), "-j4"], tmp_path, timeout=300)
        _run_covariant(["cmake", "--install", str(build)], tmp_path)
    assert (prefix / "include/systemc").is_file()
    assert (prefix / "lib/libsystemc.so").is_file()
    executable = tmp_path / "native"
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    _run_covariant(
        [
            *compiler,
            "-std=c++17",
            "-DSC_ENABLE_COVARIANT_VIRTUAL_BASE",
            f"-I{prefix / 'include'}",
            f"-I{ROOT / 'tests/native'}",
            str(ROOT / "tests/native/tlm_covariant_socket_public.cpp"),
            f"-L{prefix / 'lib'}",
            f"-Wl,-rpath,{prefix / 'lib'}",
            "-lsystemc",
            "-pthread",
            "-o",
            str(executable),
        ],
        tmp_path,
    )
    native_output = _run_covariant([str(executable)], tmp_path)
    env = {}
    env["SYSTEMC_INSTALL_DIR"] = str(prefix)
    env["SYSTEMC_LIBRARY_DIR"] = str(prefix / "lib")
    env["EXTRA_CLING_ARGS"] = os.environ.get("EXTRA_CLING_ARGS", "-std=c++17") + " -DSC_ENABLE_COVARIANT_VIRTUAL_BASE"
    python_output = _run_covariant(
        [sys.executable, str(ROOT / "tests/models/tlm_covariant_socket_public.py")],
        tmp_path,
        child_configuration=env,
    )
    assert "TLM_COVARIANT_NATIVE_OK" in native_output
    assert "TLM_COVARIANT_PYTHON_OK" in python_output

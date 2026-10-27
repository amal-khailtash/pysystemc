"""Check guarded pre-import native pointers and foreign channel operations."""

import os
import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize(
    "family,mode",
    (
        ("cached", "normal"),
        ("cached", "reset"),
        ("foreign", "normal"),
        ("foreign", "reset"),
        ("foreign", "error"),
        ("copy_only", "normal"),
    ),
)
def test_native_blocking_calls(tmp_path, family, mode):
    """Compare completion/error/output and exactly-once reset/kill cleanup to native."""
    output = run_example(tmp_path, "tests/models/native_blocking_calls.py", (family, mode))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/native_blocking_calls.cpp"),
            "--",
            family,
            mode,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert [line for line in output.splitlines() if line.startswith("BLOCKING_")] == [
        line for line in native.stdout.splitlines() if line.startswith("BLOCKING_")
    ]
    assert f"NATIVE_BLOCKING_OK {family} {mode} {13 if family == 'cached' else 1}" in output


def test_raw_cached_wait_bypasses_guards(tmp_path):
    """Characterize cppyy's unsafe raw reset boundary in a bounded isolated child."""
    result = subprocess.run(
        [sys.executable, str(ROOT / "tests/models/native_blocking_calls.py"), "raw", "reset"],
        cwd=tmp_path,
        env=dict(os.environ),
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 86, result.stdout + result.stderr
    assert "RAW_CPPYY_UNWIND_BYPASS" in result.stdout
    assert "RAW_CPPYY_FATAL F539" in result.stdout


def test_native_blocking_return_contracts(tmp_path):
    """Reject borrowed and move-only results before native work changes state."""
    assert "NATIVE_BLOCKING_REJECTIONS_OK 4" in run_example(
        tmp_path, "tests/models/native_blocking_calls.py", ("rejections", "normal")
    )

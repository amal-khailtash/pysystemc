"""Validate SystemC runtime in isolated subprocesses."""

import os
import subprocess
import sys

from .support import ROOT


def test_banner_version_colors_and_disable(tmp_path):
    """Check version/banner ordering, explicit color and banner suppression."""
    from importlib.metadata import version

    command = [sys.executable, str(ROOT / "examples/helloworld/py/helloworld.py")]
    env = dict(os.environ, PYSYSTEMC_COLOR="always", NO_COLOR="1", PYSYSTEMC_LOG_LEVEL="DEBUG")
    env.pop("PYSC_COPYRIGHT_MESSAGE", None)
    result = subprocess.run(
        command, cwd=tmp_path, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60
    )
    assert result.returncode == 0, result.stdout
    banner = f"pysystemc {version('pysystemc')}"
    assert result.stdout.index(banner) < result.stdout.index("SystemC 2.3.4-Accellera")
    assert "\x1b[" in result.stdout
    assert "[pysystemc:C++:DEBUG]" in result.stdout
    env.update(PYSC_COPYRIGHT_MESSAGE="DISABLE", PYSYSTEMC_COLOR="never")
    result = subprocess.run(
        command, cwd=tmp_path, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60
    )
    assert result.returncode == 0, result.stdout
    assert banner not in result.stdout
    assert "SystemC 2.3.4-Accellera" in result.stdout
    assert "\x1b[" not in result.stdout


def test_runtime_missing_dependencies_report_actionable_errors(tmp_path):
    """Actual loader reports controlled missing fmt, SystemC header and library failures."""
    for scenario in ("fmt", "headers", "library", "abi"):
        result = subprocess.run(
            [sys.executable, str(ROOT / "tests/models/runtime_errors.py"), str(ROOT), scenario],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert result.returncode == 0, result.stdout + result.stderr
        assert "RUNTIME_ERRORS_OK" in result.stdout


def test_runtime_header_repair_and_override_preservation(tmp_path):
    """Copy cached headers, alias lowercase installs and diagnose repair failure."""
    for scenario in ("copy", "alias", "failure"):
        result = subprocess.run(
            [sys.executable, str(ROOT / "tests/models/runtime_repair.py"), str(ROOT), scenario],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert result.returncode == 0, result.stdout + result.stderr
        assert "RUNTIME_REPAIR_OK" in result.stdout

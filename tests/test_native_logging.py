"""Verify native binding diagnostics in a fresh bounded interpreter."""

import json
import subprocess
import sys

from .support import ROOT


def test_native_logging_bytes_color_controls_and_write_error():
    """Keep exact Unicode/NUL bytes, color precedence, debug gating and I/O errors."""
    result = subprocess.run(
        [sys.executable, str(ROOT / "tests/models/native_logging.py")],
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    captured = result.stderr.split("NATIVE_LOGGING_START\n", 1)[1].split("NATIVE_LOGGING_END\n", 1)[0]
    debug = "[pysystemc:C++:DEBUG] debug %s {} café\n"
    error = "[pysystemc:C++:ERROR] error %s {} café\n"
    colored = "\x1b[38;2;000;255;255m" + debug + "\x1b[0m" + "\x1b[38;2;255;000;000m" + error + "\x1b[0m"
    assert captured == error + debug + error + colored * 2 + (debug + error) * 2 + "[pysystemc:C++:ERROR] left\0right\n"
    record = next(
        json.loads(line.split("=", 1)[1])
        for line in result.stdout.splitlines()
        if line.startswith("NATIVE_LOGGING_RESULT=")
    )
    assert "cannot write to file" in record["write_error"]

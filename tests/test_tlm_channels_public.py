"""Compare public TLM 1 channel behavior with a native C++ reference."""

import subprocess
import sys

from .support import ROOT, run_example


def test_tlm_channels_match_cpp_reference(tmp_path):
    """Compare FIFO state, channel bindings, traffic order, and times."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python_output = run_example(python_dir, "tests/models/tlm_channels_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/tlm_channels_public.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("TLM_CHANNEL_EVENT ")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("TLM_CHANNEL_EVENT ")]
    assert len(python_records) == len(native_records) >= 12
    assert python_records == native_records
    expected_debug = ("empty", "size 2 - 0 used", "readable 0", "written/read 0/0")
    python_lines = python_output.splitlines()
    native_lines = native.stdout.splitlines()
    python_debug = tuple(
        line.strip() for line in python_lines[python_lines.index("empty") : python_lines.index("empty") + 4]
    )
    native_debug = tuple(
        line.strip() for line in native_lines[native_lines.index("empty") : native_lines.index("empty") + 4]
    )
    assert python_debug == native_debug == expected_debug
    assert "TLM_CHANNELS_PUBLIC_OK" in python_output
    assert "TLM_CHANNELS_PUBLIC_OK" in native.stdout

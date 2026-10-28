"""Compare Python sc_trace_file overload dispatch with a native recorder."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_trace_file_public(tmp_path):
    """Verify all abstract overloads, helper hooks and derived destruction."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    actual_output = run_example(python_dir, "tests/models/sc_core_trace_file_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_trace_file_public.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    actual = [line for line in actual_output.splitlines() if line.startswith("TRACE_FILE|")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("TRACE_FILE|")]
    assert len(actual) == 34
    assert actual == expected
    assert actual[:28] == [
        "TRACE_FILE|trace|event|event|event_sample",
        "TRACE_FILE|trace|time|time|7 ns",
        "TRACE_FILE|trace|bool|bool|1",
        "TRACE_FILE|trace|sc_bit|bit|1",
        "TRACE_FILE|trace|sc_logic|logic|1",
        "TRACE_FILE|trace|unsigned char|uchar|65|5",
        "TRACE_FILE|trace|unsigned short|ushort|501|9",
        "TRACE_FILE|trace|unsigned int|uint|1001|11",
        "TRACE_FILE|trace|unsigned long|ulong|2001|13",
        "TRACE_FILE|trace|char|char|c|6",
        "TRACE_FILE|trace|short|short|-301|10",
        "TRACE_FILE|trace|int|int|-1001|12",
        "TRACE_FILE|trace|long|long|-2001|14",
        "TRACE_FILE|trace|sc_dt::int64|int64|-3001|17",
        "TRACE_FILE|trace|sc_dt::uint64|uint64|4001|19",
        "TRACE_FILE|trace|float|float|1.25",
        "TRACE_FILE|trace|double|double|2.5",
        "TRACE_FILE|trace|sc_int_base|sc_int|-77",
        "TRACE_FILE|trace|sc_uint_base|sc_uint|211",
        "TRACE_FILE|trace|sc_signed|signed|-77",
        "TRACE_FILE|trace|sc_unsigned|unsigned|211",
        "TRACE_FILE|trace|sc_fxval|fxval|1.25",
        "TRACE_FILE|trace|sc_fxval_fast|fxval_fast|2.5",
        "TRACE_FILE|trace|sc_fxnum|fxnum|3.25",
        "TRACE_FILE|trace|sc_fxnum_fast|fxnum_fast|2.5",
        "TRACE_FILE|trace|sc_bv_base|bv|101",
        "TRACE_FILE|trace|sc_lv_base|lv|10Z",
        "TRACE_FILE|trace|enum|enum|1001|IDLE,BUSY",
    ]
    assert actual[28:] == [
        "TRACE_FILE|comment|native comment",
        "TRACE_FILE|delta|1",
        "TRACE_FILE|delta|0",
        "TRACE_FILE|unit|0.5|1",
        "TRACE_FILE|cycle|delta",
        "TRACE_FILE|cycle|timed",
    ]
    assert "TRACE_FILE_DESTRUCTIONS|1" in actual_output
    assert "TRACE_FILE_DESTRUCTIONS|1" in native.stdout

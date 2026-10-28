"""Compare inout/output port behavior, lifecycle and waveforms with SystemC."""

import os
from pathlib import Path
import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path, *arguments):
    """Build and run the standalone native port reference with a timeout.

    Args:
        tmp_path: Isolated directory for native compilation and trace output.
        arguments: Optional reference mode arguments, such as ``--errors``.

    Returns:
        Captured standard output from the native reference.

    Raises:
        AssertionError: If compilation or execution returns a nonzero status.
        subprocess.TimeoutExpired: If the reference exceeds sixty seconds.
    """
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_inout_ports_public.cpp"),
            *(("--", *arguments) if arguments else ()),
        ],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def read_trace(path):
    """Parse VCD declarations and value changes by signal name.

    Args:
        path: Path to one generated VCD file.

    Returns:
        A mapping from signal names to widths and timestamped values, plus all
        timestamps present in the trace.
    """
    declarations = {}
    signals = {}
    timestamps = set()
    current_time = 0
    for line in Path(path).read_text().splitlines():
        columns = line.split()
        if columns and columns[0] == "$var":
            width = int(columns[2])
            trace_id = columns[3]
            name = columns[4]
            declarations[trace_id] = name
            signals[name] = {"width": width, "values": []}
        elif line.startswith("#"):
            current_time = int(line[1:])
            timestamps.add(current_time)
        elif line.startswith(("0", "1", "x", "X", "z", "Z")) and len(line) > 1:
            trace_id = line[1:].strip()
            if trace_id in declarations:
                name = declarations[trace_id]
                signals[name]["values"].append((current_time, line[0].lower()))
        elif line.startswith("b"):
            value, trace_id = line.split()
            if trace_id in declarations:
                name = declarations[trace_id]
                signals[name]["values"].append((current_time, value[1:]))
    return signals, timestamps


def test_sc_inout_and_sc_out_public_families(tmp_path):
    """Check constructors, assignments, initialization, events and live traces."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/sc_core_inout_ports_public.py")
    native_output = run_native(native_dir)
    marker = "INOUT_PORT_"
    python_lines = [line for line in python_output.splitlines() if line.startswith(marker)]
    native_lines = [line for line in native_output.splitlines() if line.startswith(marker)]
    assert python_lines == native_lines, (python_lines, native_lines)
    event_rows = [line.split("|") for line in python_lines if line.startswith("INOUT_PORT_EVENT|")]
    assert len(event_rows) == 10
    assert sum(row[2] == "value" for row in event_rows) == 6
    assert sum(row[2] == "pos" for row in event_rows) == 2
    assert sum(row[2] == "neg" for row in event_rows) == 2
    for row in event_rows:
        assert row[5] == "1"
        if row[2] == "pos":
            assert row[6:] == ["1", "0"]
        elif row[2] == "neg":
            assert row[6:] == ["0", "1"]

    traces = [read_trace(directory / "sc_core_inout_ports_public.vcd") for directory in (python_dir, native_dir)]
    assert traces[0] == traces[1]
    signals, timestamps = traces[0]
    widths = {name: value["width"] for name, value in signals.items()}
    assert widths == {
        "wide_inout": 137,
        "wide_out": 137,
        "bool_inout": 1,
        "bool_out": 1,
        "logic_inout": 1,
        "logic_out": 1,
        "wide_member_add_trace": 137,
        "bool_member_add_trace": 1,
        "logic_member_add_trace": 1,
    }
    assert {1, 2}.issubset(timestamps)
    for name in ("wide_inout", "wide_out"):
        values = signals[name]["values"]
        assert any(time == 1 for time, _value in values)
        assert len({value for _time, value in values}) >= 2
    for name in ("bool_inout", "bool_out", "logic_inout", "logic_out"):
        values = signals[name]["values"]
        assert (0, "1") in values
        assert any(time == 1 and value == "0" for time, value in values)


def test_sc_inout_unbound_required_error(tmp_path):
    """Check an unbound required inout port fails consistently in fresh kernels."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/sc_core_inout_ports_public.py", ("--errors",))
    native_output = run_native(native_dir, "--errors")
    assert "INOUT_PORT_ERRORS|unbound-required" in python_output
    assert "INOUT_PORT_ERRORS|unbound-required" in native_output

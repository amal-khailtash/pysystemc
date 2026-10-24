"""Compare the complete ordinary sc_fifo public surface with native SystemC."""

import os
import subprocess
import sys

import pytest

from .support import ROOT, run_example


def run_native(tmp_path, *arguments):
    """Run the independent native FIFO model in its own bounded kernel process."""
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_fifo_public.cpp"),
            "--",
            *arguments,
        ],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def test_sc_core_fifo_public_matches_native_and_traces_values(tmp_path):
    """Check sizes, overloads, delta counts, FIFO text and meaningful VCD changes."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python_output = run_example(python_dir, "tests/models/sc_core_fifo_public.py")
    native_output = run_native(native_dir)
    python_reference = next(line for line in python_output.splitlines() if line.startswith("FIFO_REFERENCE "))
    native_reference = next(line for line in native_output.splitlines() if line.startswith("FIFO_REFERENCE "))
    assert python_reference == native_reference
    python_text = next(line for line in python_output.splitlines() if line.startswith("FIFO_TEXT "))
    native_text = next(line for line in native_output.splitlines() if line.startswith("FIFO_TEXT "))
    assert python_text == native_text
    python_stream = next(line for line in python_output.splitlines() if line.startswith("FIFO_STREAM "))
    native_stream = next(line for line in native_output.splitlines() if line.startswith("FIFO_STREAM "))
    assert python_stream == native_stream
    assert native_stream == "FIFO_STREAM 51|61"
    python_processes = next(line for line in python_output.splitlines() if line.startswith("FIFO_PROCESSES "))
    native_processes = next(line for line in native_output.splitlines() if line.startswith("FIFO_PROCESSES "))
    assert python_processes == native_processes
    python_assign = next(line for line in python_output.splitlines() if line.startswith("FIFO_ASSIGNMENT_CONVERSION "))
    native_assign = next(line for line in native_output.splitlines() if line.startswith("FIFO_ASSIGNMENT_CONVERSION "))
    assert python_assign == native_assign == "FIFO_ASSIGNMENT_CONVERSION 73 174224571863520493293247799005065324269"
    python_scalar = next(line for line in python_output.splitlines() if line.startswith("FIFO_SCALAR_CONVERSIONS "))
    native_scalar = next(line for line in native_output.splitlines() if line.startswith("FIFO_SCALAR_CONVERSIONS "))
    assert python_scalar == native_scalar == "FIFO_SCALAR_CONVERSIONS 2.75 True"
    python_destruction = next(line for line in python_output.splitlines() if line.startswith("FIFO_DESTRUCTION "))
    native_destruction = next(line for line in native_output.splitlines() if line.startswith("FIFO_DESTRUCTION "))
    assert python_destruction == native_destruction == "FIFO_DESTRUCTION 1"
    python_events = next(line for line in python_output.splitlines() if line.startswith("FIFO_EVENTS "))
    native_events = next(line for line in native_output.splitlines() if line.startswith("FIFO_EVENTS "))
    assert python_events == native_events

    vcd = (python_dir / "fifo_family.vcd").read_text()
    native_vcd = (native_dir / "fifo_family.vcd").read_text()
    declaration = next(line for line in vcd.splitlines() if "$var" in line and "wide_sample" in line)
    native_declaration = next(line for line in native_vcd.splitlines() if "$var" in line and "wide_sample" in line)
    assert declaration.split()[2] == native_declaration.split()[2] == "137"
    assert len([line for line in vcd.splitlines() if "$var" in line]) == 1
    assert len([line for line in native_vcd.splitlines() if "$var" in line]) == 1
    identifier = declaration.split()[3]
    transitions = [line for line in vcd.splitlines() if line.startswith("b") and line.endswith(f" {identifier}")]
    assert len(transitions) >= 2, vcd
    native_identifier = native_declaration.split()[3]
    native_transitions = [
        line for line in native_vcd.splitlines() if line.startswith("b") and line.endswith(f" {native_identifier}")
    ]
    assert len(native_transitions) >= 2, native_vcd
    expected_values = [
        174224571863520493293247799005065324265,
        174224571863520493293247799005065324271,
    ]

    def timed_transitions(trace, identifier):
        """Return traced integer values and VCD timestamps in native time units."""
        current_time = 0
        result = []
        for line in trace.splitlines():
            if line.startswith("#"):
                current_time = int(line[1:])
            elif line.startswith("b") and line.endswith(f" {identifier}"):
                result.append((int(line.split()[0][1:], 2), current_time))
        return result

    assert timed_transitions(vcd, identifier) == list(zip(expected_values, (0, 1)))
    assert timed_transitions(native_vcd, native_identifier) == list(zip(expected_values, (0, 1)))
    reference = python_reference.split()
    assert reference[1:] == [
        "16",
        "sc_fifo",
        "fifo_public.int_fifo",
        "23",
        "174224571863520493293247799005065324265",
        "174224571863520493293247799005065324267",
        "0",
        "1",
        "0",
        "2",
    ]


def test_sc_core_fifo_invalid_size_reports_native_error(tmp_path):
    """Compare zero-sized constructor failure to the native report identifier."""
    python_output = run_example(tmp_path, "tests/models/sc_core_fifo_public.py", ("invalid-size",))
    native_output = run_native(tmp_path, "invalid-size")
    error_text = "sc_fifo<T> must have a size of at least 1"
    assert "FIFO_INVALID_SIZE" in python_output and error_text in python_output
    assert "FIFO_INVALID_SIZE" in native_output and error_text in native_output


@pytest.mark.parametrize("direction", ("reader", "writer"))
def test_sc_core_fifo_rejects_duplicate_port_registration(tmp_path, direction):
    """Compare public reader/writer registration checks with native report output."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    mode = f"duplicate-{direction}"
    python_output = run_example(python_dir, "tests/models/sc_core_fifo_public.py", (mode,))
    native_output = run_native(native_dir, mode)
    error_id = 104 if direction == "reader" else 105
    detail = f"sc_fifo<T> cannot have more than one {direction}"
    assert "FIFO_REGISTER_ERROR" in python_output and f"E{error_id}" in python_output
    assert detail in python_output
    assert f"FIFO_REGISTER_ERROR {direction} {error_id}" in native_output
    assert detail in native_output

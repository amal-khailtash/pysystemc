"""Compare resolved vector output ports with an independent native simulation."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path, arguments=()):
    """Build and run the native reference in a fresh bounded process.

    Args:
        tmp_path: Isolated directory for native build products and VCD files.
        arguments: Optional native command-line arguments.

    Returns:
        Captured native standard output.

    Raises:
        AssertionError: If compilation or simulation returns a failure status.
        subprocess.TimeoutExpired: If native work exceeds sixty seconds.
    """
    environment = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    command = [
        sys.executable,
        str(ROOT / "examples/run_sysc_cpp.py"),
        str(ROOT / "tests/native/sc_core_resolved_vector_output_public.cpp"),
    ]
    if arguments:
        command.extend(("--", *arguments))
    result = subprocess.run(
        command,
        cwd=tmp_path,
        env=environment,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def trace_transitions(path):
    """Return each VCD signal's declared width and ordered transitions.

    Args:
        path: Native or Python VCD path from one isolated simulation.

    Returns:
        A mapping from signal name to ``(width, transitions)`` where a
        transition is a ``(timestamp, value)`` pair.
    """
    identifiers = {}
    signals = {}
    timestamp = 0
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("$var"):
            fields = line.split()
            width, identifier, name = int(fields[2]), fields[3], fields[4]
            identifiers[identifier] = (width, name)
            signals[name] = (width, [])
        elif line.startswith("#"):
            timestamp = int(line[1:])
        elif line.startswith("b"):
            value, identifier = line[1:].split()
            width, name = identifiers[identifier]
            padding = value[0] if value[0].lower() in "xz" else "0"
            signals[name][1].append((timestamp, value.lower().rjust(width, padding)))
        elif line and line[0].lower() in "01xz" and line[1:] in identifiers:
            _width, name = identifiers[line[1:]]
            signals[name][1].append((timestamp, line[0].lower()))
    return signals


def selected_records(output):
    """Keep stable test records while ignoring SystemC startup messages."""
    prefixes = (
        "RESOLVED_VECTOR_OUTPUT_CTORS ",
        "RESOLVED_VECTOR_OUTPUT_HOOKS ",
        "RESOLVED_VECTOR_OUTPUT_DESTRUCTION ",
        "RESOLVED_VECTOR_OUTPUT_ASSIGNMENTS ",
        "RESOLVED_VECTOR_OUTPUT_SAMPLES ",
        "RESOLVED_VECTOR_OUTPUT_INITIALIZE ",
        "RESOLVED_VECTOR_OUTPUT_OK",
    )
    return [line for line in output.splitlines() if line.startswith(prefixes)]


def assignment_bits(width, seed):
    """Build the expected four-state source vector for one assignment row."""
    symbols = "01XZ"
    result = [symbols[(bit + seed) % len(symbols)] for bit in range(width)]
    result[0] = "1" if seed % 2 else "0"
    result[-1] = "0" if seed % 2 else "1"
    return "".join(result).lower()


def test_sc_core_resolved_vector_output_public(tmp_path):
    """Check declarations, constructors, assignment, lifecycle, reports and VCD."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_errors_dir = tmp_path / "python_errors"
    native_errors_dir = tmp_path / "native_errors"
    for directory in (python_dir, native_dir, python_errors_dir, native_errors_dir):
        directory.mkdir()

    python_output = run_example(python_dir, "tests/models/sc_core_resolved_vector_output_public.py")
    native_output = run_native(native_dir)
    assert selected_records(python_output) == selected_records(native_output)
    assert "RESOLVED_VECTOR_OUTPUT_CTORS 8|8|8|8" in python_output
    assert "RESOLVED_VECTOR_OUTPUT_HOOKS 1|1|1|1" in python_output
    assert "RESOLVED_VECTOR_OUTPUT_DESTRUCTION 1|1|1|1" in python_output
    assert "RESOLVED_VECTOR_OUTPUT_ASSIGNMENTS 20" in python_output
    assert "RESOLVED_VECTOR_OUTPUT_OK" in python_output

    python_errors = run_example(
        python_errors_dir, "tests/models/sc_core_resolved_vector_output_public.py", ("--errors",)
    )
    native_errors = run_native(native_errors_dir, ("errors",))
    error_prefix = "RESOLVED_VECTOR_OUTPUT_ERRORS "
    python_error_records = [line for line in python_errors.splitlines() if line.startswith(error_prefix)]
    native_error_records = [line for line in native_errors.splitlines() if line.startswith(error_prefix)]
    assert python_error_records == native_error_records == ["RESOLVED_VECTOR_OUTPUT_ERRORS 117|117"]

    python_trace = trace_transitions(python_dir / "resolved_vector_output_public.vcd")
    native_trace = trace_transitions(native_dir / "native_resolved_vector_output_public.vcd")
    assert python_trace == native_trace
    for prefix, width in (("inout137", 137), ("out137", 137), ("inout5", 5), ("out5", 5)):
        for index in range(5):
            signal_width, transitions = python_trace[f"{prefix}_assign_{index}"]
            assert signal_width == width
            assert transitions == [(0, assignment_bits(width, index))]
        assert python_trace[f"{prefix}_resolved"][0] == width
        assert python_trace[f"{prefix}_initialized"][0] == width
    assert python_trace["inout137_resolved"][1] == [(0, "x" + "z" * 135 + "x"), (1, "z" * 137)]
    assert python_trace["inout5_resolved"][1] == [(0, "xzzzx"), (1, "zzzzz")]

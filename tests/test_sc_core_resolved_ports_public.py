"""Compare resolved scalar/vector input ports with an independent native run."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path, arguments=()):
    """Build and run the native reference in a fresh bounded process.

    Args:
        tmp_path: Isolated directory for native build products and traces.
        arguments: Optional arguments passed to the native simulation.

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
        str(ROOT / "tests/native/sc_core_resolved_ports_public.cpp"),
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
    """Return each named VCD signal's declared width and ordered changes.

    Args:
        path: Native or Python VCD path from one isolated simulation.

    Returns:
        A mapping from VCD signal name to ``(width, transitions)`` where each
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
    """Keep stable result records while ignoring SystemC startup messages."""
    prefixes = (
        "RESOLVED_PORT_CTORS ",
        "RESOLVED_PORT_HOOKS ",
        "RESOLVED_PORT_DESTRUCTION ",
        "RESOLVED_PORT_SAMPLE ",
        "RESOLVED_PORT_WIDE ",
        "RESOLVED_PORTS_OK",
    )
    return [line for line in output.splitlines() if line.startswith(prefixes)]


def test_sc_core_resolved_input_ports_public(tmp_path):
    """Verify 41 declarations through constructors, hooks, resolution and VCD."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_errors_dir = tmp_path / "python_errors"
    native_errors_dir = tmp_path / "native_errors"
    for directory in (python_dir, native_dir, python_errors_dir, native_errors_dir):
        directory.mkdir()

    python_output = run_example(python_dir, "tests/models/sc_core_resolved_ports_public.py")
    native_output = run_native(native_dir)
    assert selected_records(python_output) == selected_records(native_output)
    assert "RESOLVED_PORT_CTORS 10|10" in python_output
    assert "RESOLVED_PORT_HOOKS 1|1" in python_output
    assert "RESOLVED_PORT_DESTRUCTION 1|1" in python_output
    assert "RESOLVED_PORT_WIDE 137 " in python_output

    python_errors = run_example(python_errors_dir, "tests/models/sc_core_resolved_ports_public.py", ("--errors",))
    native_errors = run_native(native_errors_dir, ("errors",))
    error_prefix = "RESOLVED_PORT_ERRORS "
    python_error_records = [line for line in python_errors.splitlines() if line.startswith(error_prefix)]
    native_error_records = [line for line in native_errors.splitlines() if line.startswith(error_prefix)]
    assert python_error_records == native_error_records == ["RESOLVED_PORT_ERRORS 117|117"]

    python_trace = trace_transitions(python_dir / "resolved_ports_public.vcd")
    native_trace = trace_transitions(native_dir / "native_resolved_ports_public.vcd")
    assert set(python_trace) == set(native_trace) == {"scalar", "vector137"}
    assert python_trace == native_trace
    assert python_trace["scalar"] == (1, [(0, "x"), (1, "z")])
    assert python_trace["vector137"][0] == 137
    assert [time for time, _value in python_trace["vector137"][1]] == [0, 1]
    assert python_trace["vector137"][1][0][1].endswith("x")
    assert python_trace["vector137"][1][-1][1] == "z" * 137

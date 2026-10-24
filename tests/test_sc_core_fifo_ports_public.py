"""Compare public FIFO port overloads, errors and trace behavior with C++."""

import os
import re
import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path, arguments=()):
    """Run the independent native FIFO-port reference with a bounded timeout.

    Args:
        tmp_path: Isolated working directory for native build and VCD output.
        arguments: Optional arguments forwarded to native ``sc_main``.

    Returns:
        Captured standard output from the native reference.

    Raises:
        AssertionError: If native compilation or simulation fails.
        subprocess.TimeoutExpired: If the reference exceeds sixty seconds.
    """
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    command = [
        sys.executable,
        str(ROOT / "examples/run_sysc_cpp.py"),
        str(ROOT / "tests/native/sc_core_fifo_ports_public.cpp"),
    ]
    if arguments:
        command.extend(("--", *arguments))
    result = subprocess.run(
        command,
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def read_trace(path):
    """Read the named VCD signal's declaration and sampled transitions.

    Args:
        path: VCD file emitted by the Python or native model.

    Returns:
        A pair containing the declaration and (timestamp, value) transitions.
    """
    contents = path.read_text(encoding="utf-8")
    declarations = re.findall(r"\$var\s+\w+\s+\d+\s+(\S+)\s+activity\s+\$end", contents)
    assert len(declarations) == 1, contents
    code = declarations[0]
    transitions = []
    timestamp = None
    for line in contents.splitlines():
        line = line.strip()
        if line.startswith("#"):
            timestamp = line[1:]
        elif line in (f"0{code}", f"1{code}"):
            transitions.append((timestamp, line[0]))
    assert transitions, contents
    return declarations, transitions


def test_sc_core_fifo_input_output_port_families(tmp_path):
    """Verify all port constructors and compare values, reports and traces."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_error_dir = tmp_path / "python_errors"
    native_error_dir = tmp_path / "native_errors"
    python_dir.mkdir()
    native_dir.mkdir()
    python_error_dir.mkdir()
    native_error_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/sc_core_fifo_ports_public.py")
    native_output = run_native(native_dir)
    markers = (
        "FIFO_PORT_MATRIX",
        "FIFO_PORT_PROTECTED",
        "FIFO_PORT_LIFETIME",
        "FIFO_PORT_REFERENCE",
    )
    for marker in markers:
        python_lines = [line for line in python_output.splitlines() if line.startswith(marker)]
        native_lines = [line for line in native_output.splitlines() if line.startswith(marker)]
        assert len(python_lines) == 1, (marker, python_output)
        assert python_lines == native_lines, (marker, python_lines, native_lines)

    python_errors = run_example(python_error_dir, "tests/models/sc_core_fifo_ports_public.py", ("--errors",))
    native_errors = run_native(native_error_dir, ("errors",))
    assert [line for line in python_errors.splitlines() if line.startswith("FIFO_PORT_ERRORS")] == [
        "FIFO_PORT_ERRORS\t1\t1"
    ]
    assert [line for line in python_errors.splitlines() if line.startswith("FIFO_PORT_ERRORS")] == [
        line for line in native_errors.splitlines() if line.startswith("FIFO_PORT_ERRORS")
    ]

    python_declarations, python_transitions = read_trace(python_dir / "fifo_ports_public.vcd")
    native_declarations, native_transitions = read_trace(native_dir / "fifo_ports_public.vcd")
    assert len(python_declarations) == len(native_declarations) == 1
    assert [value for _, value in python_transitions] == ["0", "1", "0"]
    assert [value for _, value in python_transitions] == [value for _, value in native_transitions]
    assert [time for time, _ in python_transitions] == [time for time, _ in native_transitions]

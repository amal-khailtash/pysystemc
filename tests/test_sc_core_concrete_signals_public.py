"""Compare concrete SystemC signal families with native reference models."""

import subprocess
import sys

from .support import ROOT, run_example


def _trace_values(path):
    """Read VCD widths and value changes by declared trace name.

    Args:
        path: VCD file created in an isolated simulation directory.

    Returns:
        A mapping from trace name to its declared width and normalized values.
    """
    text = path.read_text()
    variables = {}
    for line in text.splitlines():
        if line.startswith("$var"):
            fields = line.split()
            variables[fields[4]] = (int(fields[2]), fields[3])

    values = {}
    for name, (width, identifier) in variables.items():
        if width == 1:
            seen = {
                line[0] for line in text.splitlines() if len(line) > 1 and line[0] in "01xz" and line[1:] == identifier
            }
        else:
            seen = {
                fields[0][1:].lower()
                for line in text.splitlines()
                if line.startswith("b")
                for fields in (line.split(),)
                if len(fields) == 2 and fields[1] == identifier
            }
        values[name] = (width, seen)
    return values


def _assert_trace(path):
    """Require useful scalar, 16-bit, and arbitrary-width VCD transitions."""
    trace = _trace_values(path)
    assert trace["buffer"] == (1, {"1"})
    assert trace["boolean"] == (1, {"0", "1"})
    assert trace["resolved"] == (1, {"0", "x"})
    assert trace["vector16"][0] == 16
    assert {"1xz", "10"} <= trace["vector16"][1]
    assert trace["vector137"][0] == 137
    assert len(trace["vector137"][1]) >= 2
    assert "#1" in path.read_text()


def _trace_transitions(path):
    """Map VCD names to ordered timestamp/value pairs, independent of identifiers.

    Args:
        path: Native or Python simulation trace file.

    Returns:
        Per-signal lists preserving transition order and native trace timestamps.
    """
    identifiers = {}
    transitions = {}
    timestamp = 0
    for line in path.read_text().splitlines():
        if line.startswith("$var"):
            fields = line.split()
            identifiers[fields[3]] = (fields[4], int(fields[2]))
            transitions[fields[4]] = []
        elif line.startswith("#"):
            timestamp = int(line[1:])
        elif line.startswith("b"):
            value, identifier = line[1:].split()
            name, width = identifiers[identifier]
            padding = value[0] if value[0].lower() in "xz" else "0"
            transitions[name].append((timestamp, value.lower().rjust(width, padding)))
        elif line and line[0] in "01xz" and line[1:] in identifiers:
            name, _width = identifiers[line[1:]]
            transitions[name].append((timestamp, line[0]))
    return transitions


def test_sc_core_concrete_signals_public(tmp_path):
    """Compare constructors, assignments, updates, multiwriters and VCD data."""
    output = run_example(tmp_path, "tests/models/sc_core_concrete_signals_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_concrete_signals_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    prefixes = (
        "CONCRETE_CTOR ",
        "CONCRETE_ASSIGN ",
        "CONCRETE_EVENTS ",
        "CONCRETE_PAYLOAD ",
        "CONCRETE_WIDE ",
        "CONCRETE_DELTA ",
        "CONCRETE_DESTRUCTION ",
    )
    actual = [line for line in output.splitlines() if line.startswith(prefixes)]
    expected = [line for line in native.stdout.splitlines() if line.startswith(prefixes)]
    assert actual == expected
    assert len([line for line in actual if line.startswith("CONCRETE_CTOR ")]) == 9
    assert len([line for line in actual if line.startswith("CONCRETE_ASSIGN ")]) == 3
    assert "CONCRETE_EVENTS 3|2|2|2|3|2|2" in actual
    assert any(line.startswith("CONCRETE_WIDE 137 ") for line in actual)
    assert "CONCRETE_SIGNALS_OK" in output

    _assert_trace(tmp_path / "concrete_signals.vcd")
    _assert_trace(tmp_path / "native_concrete_signals.vcd")
    assert _trace_transitions(tmp_path / "concrete_signals.vcd") == _trace_transitions(
        tmp_path / "native_concrete_signals.vcd"
    )

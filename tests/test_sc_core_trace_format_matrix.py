"""Compare typed VCD and WIF declarations and transitions to native SystemC."""

from __future__ import annotations

import re
import subprocess
import sys

import pytest

from .support import ROOT, run_example


VCD_NAMES = {
    "boolean",
    "bit",
    "logic",
    "schar",
    "uchar",
    "sshort",
    "ushort",
    "sint",
    "uint",
    "slong",
    "ulong",
    "int64",
    "uint64",
    "float",
    "double",
    "sc_int19",
    "sc_uint21",
    "sc_signed73",
    "sc_unsigned73",
    "sc_bv73",
    "sc_lv73",
    "fxval",
    "fxval_fast",
    "fxnum",
    "fxnum_fast",
    "enumeration",
    "time",
    "event",
}
WIF_NAMES = VCD_NAMES - {"time", "event"}
WIDTHS = {
    "boolean": 1,
    "bit": 1,
    "logic": 1,
    "schar": 8,
    "uchar": 8,
    "sshort": 13,
    "ushort": 13,
    "sint": 21,
    "uint": 21,
    "slong": 33,
    "ulong": 33,
    "int64": 47,
    "uint64": 57,
    "sc_int19": 19,
    "sc_uint21": 21,
    "sc_signed73": 73,
    "sc_unsigned73": 73,
    "sc_bv73": 73,
    "sc_lv73": 73,
    "enumeration": 2,
    "time": 64,
    "event": 1,
}
VCD_KINDS = {
    "boolean": "wire",
    "bit": "wire",
    "logic": "wire",
    "float": "real",
    "double": "real",
    "fxval": "real",
    "fxval_fast": "real",
    "fxnum": "wire",
    "fxnum_fast": "wire",
    "enumeration": "wire",
    "time": "time",
    "event": "event",
}
WIF_KINDS = {
    "boolean": "BIT",
    "bit": "BIT",
    "logic": "MVL",
    "float": "real",
    "double": "real",
    "sc_lv73": "MVL",
    "fxval": "real",
    "fxval_fast": "real",
    "fxnum": "BIT",
    "fxnum_fast": "BIT",
    "enumeration": '"enumeration__type__"',
}


def _parse_vcd(text: str):
    """Return declarations and timestamped semantic value changes from VCD."""
    declarations = {}
    code_to_name = {}
    for line in text.splitlines():
        if line.startswith("$var"):
            fields = line.split()
            _, kind, width, code, name = fields[:5]
            declarations[name] = (kind, int(width))
            code_to_name[code] = name

    transitions = []
    timestamp = 0
    in_values = True
    for line in text.splitlines():
        if line.startswith("$enddefinitions"):
            in_values = False
            continue
        if not in_values:
            if line == "$dumpvars":
                in_values = True
            continue
        if line.startswith("#"):
            timestamp = int(line[1:])
            continue
        if not line or line.startswith("$"):
            continue
        if line[0] in "bBrR":
            fields = line[1:].split()
            if len(fields) != 2 or fields[1] not in code_to_name:
                continue
            value, code = fields
            transitions.append((timestamp, code_to_name[code], value.lower()))
        elif line[0] in "01xXzZ":
            code = line[1:]
            if code in code_to_name:
                transitions.append((timestamp, code_to_name[code], line[0].lower()))
    return declarations, transitions


def _parse_wif(text: str):
    """Return WIF declarations and timestamped semantic assignments."""
    declarations = {}
    code_to_name = {}
    for line in text.splitlines():
        match = re.match(r'declare\s+(\S+)\s+"([^"]+)"\s+(\S+)(?:\s+(\d+)\s+(\d+))?', line)
        if match:
            code, name, kind, lower, upper = match.groups()
            if kind in {"BIT", "MVL"}:
                width = 1 if lower is None else int(upper) - int(lower) + 1
            else:
                # WIF real and enum declarations carry no packed bit width.
                width = None
            declarations[name] = (kind, width)
            code_to_name[code] = name

    transitions = []
    timestamp = 0
    for line in text.splitlines():
        delta = re.match(r"delta_time\s+(\d+)", line)
        if delta:
            timestamp += int(delta.group(1))
            continue
        match = re.match(r"assign\s+(\S+)\s+(.+?)\s*;", line)
        if match and match.group(1) in code_to_name:
            value = match.group(2).strip()
            transitions.append((timestamp, code_to_name[match.group(1)], value))
    return declarations, transitions


def _run_native(directory, fmt):
    """Run the ordinary C++ reference in a separate process and directory."""
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_trace_format_matrix.cpp"),
            "--",
            fmt,
        ],
        cwd=directory,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert f"TRACE_FORMAT_MATRIX_OK {fmt} 3 ns" in result.stdout
    return result.stdout


@pytest.mark.parametrize("fmt", ["vcd", "wif"])
def test_sc_core_trace_format_matrix(tmp_path, fmt):
    """Assert typed declarations, changing values, and Python/native parity."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    output = run_example(python_dir, "tests/models/sc_core_trace_format_matrix.py", (fmt,))
    native_output = _run_native(native_dir, fmt)
    assert f"TRACE_FORMAT_MATRIX_OK {fmt} 3 ns" in output

    if fmt == "vcd":
        actual_path = python_dir / "python_trace_matrix_vcd.vcd"
        native_path = native_dir / "native_trace_matrix_vcd.vcd"
        parser = _parse_vcd
    else:
        actual_path = python_dir / "python_trace_matrix_wif.awif"
        native_path = native_dir / "native_trace_matrix_wif.awif"
        parser = _parse_wif
    actual_text = actual_path.read_text()
    native_text = native_path.read_text()
    actual_declarations, actual_transitions = parser(actual_text)
    native_declarations, native_transitions = parser(native_text)

    names = VCD_NAMES if fmt == "vcd" else WIF_NAMES
    assert set(actual_declarations) == names
    assert set(native_declarations) == names
    assert actual_declarations == native_declarations
    for name in names:
        if name in WIDTHS and (fmt == "vcd" or actual_declarations[name][1] is not None):
            assert actual_declarations[name][1] == WIDTHS[name]
        observed = [(time, value) for time, signal, value in actual_transitions if signal == name]
        assert observed, f"{fmt} declared {name} without values"
        assert len({time for time, _ in observed}) >= 2, f"{fmt} {name} never changed across timestamps"
        if name != "event":
            assert len({value for _, value in observed}) >= 2, f"{fmt} {name} value did not change"
    expected_kinds = VCD_KINDS if fmt == "vcd" else WIF_KINDS
    for name, kind in expected_kinds.items():
        assert actual_declarations[name][0] == kind
    assert actual_transitions == native_transitions
    assert "ignored_pointer" not in actual_text
    assert "ignored_pointer" not in native_text
    if fmt == "vcd":
        assert {0, 1000, 2000} <= {time for time, _, _ in actual_transitions}
        assert "ignored_pointer" not in actual_declarations
    else:
        assert "time" not in actual_declarations and "event" not in actual_declarations
        assert "time" not in native_declarations and "event" not in native_declarations
    assert native_output

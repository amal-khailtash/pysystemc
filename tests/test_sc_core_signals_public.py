"""Compare public signal behavior, scheduler transitions and VCD output."""

import os
import subprocess
import sys

import pytest

from .support import ROOT, run_example


def run_native(workdir, *arguments):
    """Run one independent native SystemC model with a bounded timeout."""
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_signals_public.cpp"),
            "--",
            *arguments,
        ],
        cwd=workdir,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def vcd_records(path):
    """Return declaration widths and value/time pairs keyed by traced name."""
    declarations = {}
    transitions = {}
    identifiers = {}
    time = 0
    for line in path.read_text().splitlines():
        parts = line.split()
        if len(parts) >= 5 and parts[0] == "$var":
            width, identifier, name = int(parts[2]), parts[3], parts[4]
            declarations[name] = width
            identifiers[identifier] = name
        elif line.startswith("#"):
            time = int(line[1:])
        elif line.startswith("b"):
            value, identifier = parts[0][1:], parts[1]
            if identifier in identifiers:
                transitions.setdefault(identifiers[identifier], []).append((int(value, 2), time))
        elif line and line[0] in "01xXzZ" and len(line) > 1:
            identifier = line[1:]
            if identifier in identifiers:
                transitions.setdefault(identifiers[identifier], []).append((line[0], time))
    return declarations, transitions


def test_sc_core_signals_public_matches_native_and_traces(tmp_path):
    """Check constructors, seven typed policies, assignments, events and VCD values."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python_output = run_example(python_dir, "tests/models/sc_core_signals_public.py")
    native_output = run_native(native_dir)
    python_records = [line for line in python_output.splitlines() if line.startswith("SIGNAL")]
    native_records = [line for line in native_output.splitlines() if line.startswith("SIGNAL")]
    assert len(python_records) == len(native_records) == 43
    assert python_records == native_records
    assert python_records[-1] == "SIGNAL_TIME|1 ns|4"

    python_declarations, python_transitions = vcd_records(python_dir / "signals_public.vcd")
    native_declarations, native_transitions = vcd_records(native_dir / "signals_public.vcd")
    expected_names = {
        "int_one",
        "bool_one",
        "bool_many",
        "bool_unchecked",
        "logic_one",
        "logic_many",
        "logic_unchecked",
        "wide_sample",
    }
    assert python_declarations == native_declarations
    assert set(python_declarations) == expected_names
    assert python_declarations["wide_sample"] == 137
    assert python_transitions == native_transitions
    assert python_transitions["wide_sample"] == [
        (87112285931760246646623899502532662133001, 0),
        (87112285931760246646623899502532662133007, 1),
    ]
    for name in ("bool_one", "bool_many", "bool_unchecked", "logic_one", "logic_many", "logic_unchecked"):
        assert python_transitions[name] == [("1", 0), ("0", 1)]


@pytest.mark.parametrize(
    ("policy", "expected"),
    (
        ("one", "SIGNAL_PORT|one|duplicate-error|115"),
        ("many", "SIGNAL_PORT|many|multiple-outputs-allowed|0"),
        ("unchecked", "SIGNAL_PORT|unchecked|multiple-outputs-allowed|0"),
    ),
)
def test_sc_core_signal_writer_policy_port_registration(tmp_path, policy, expected):
    """Compare duplicate output registration behavior for all writer policies."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    python_output = run_example(python_dir, "tests/models/sc_core_signals_public.py", (f"port-{policy}",))
    native_output = run_native(native_dir, f"port-{policy}")
    assert [line for line in python_output.splitlines() if line.startswith("SIGNAL_PORT")] == [expected]
    assert [line for line in native_output.splitlines() if line.startswith("SIGNAL_PORT")] == [expected]

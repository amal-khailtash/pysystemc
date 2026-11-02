"""Compare free tracing overloads and delayed port waveforms with SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_trace_selector_errors_match_native(tmp_path):
    """Preserve cppyy selector errors and recover with an exact valid signature."""
    source = """
from amal.eda.systemc import sc_core as sc
import cppyy

native = cppyy.gbl.sc_core.sc_trace.__overload__
public = sc.sc_trace.__overload__
for arguments in ((), (None,), (123,), (("int",),)):
    errors = []
    for select in (native, public):
        try:
            select(*arguments)
        except (TypeError, SystemError, LookupError) as error:
            errors.append((type(error), str(error)))
        else:
            raise AssertionError(f"invalid selector accepted: {arguments!r}")
    assert errors[0] == errors[1], errors
signature = "sc_core::sc_trace_file*,const bool&,const std::string&"
assert callable(native(signature))
assert callable(public(signature))
tuple_signature = ("sc_core::sc_trace_file*", "const bool&", "const std::string&")
assert callable(native(tuple_signature))
assert callable(public(tuple_signature))
print("TRACE_SELECTOR_RECOVERY_OK")
"""
    result = subprocess.run([sys.executable, "-c", source], cwd=tmp_path, capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    assert "TRACE_SELECTOR_RECOVERY_OK" in result.stdout


def test_sc_core_free_trace_public(tmp_path):
    """Check exact dispatch, native widths, unbound registration and live values."""
    output = run_example(tmp_path, "tests/models/sc_core_free_trace_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_free_trace_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("FREE_TRACE|")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("FREE_TRACE|")]
    assert len(actual) == 83 and actual == expected
    assert "FREE_TRACE_IGNORED 1" in output and "FREE_TRACE_IGNORED 1" in native.stdout
    assert "FREE_TRACE_TIME 2000" in output and "FREE_TRACE_TIME 2000" in native.stdout
    traces = [(tmp_path / name).read_text().splitlines() for name in ("free_trace.vcd", "native_free_trace.vcd")]
    records = [[line for line in trace if line.startswith(("$var", "#", "b", "0", "1"))] for trace in traces]
    assert records[0] == records[1]
    widths = {row.split()[4]: int(row.split()[2]) for row in records[0] if row.startswith("$var")}
    assert widths == {
        "input_before_bind": 32,
        "inout_before_bind": 32,
        "wide_input_before_bind": 137,
        "wide_inout_before_bind": 137,
        "char_pointer_wave": 8,
        "char_array_wave": 8,
        "integer_width": 7,
        "wide_generic": 137,
    }
    assert "#1000" in records[0]
    assert any(line.startswith("b" + format((1 << 136) + 257, "b")) for line in records[0])
    identifiers = {
        row[4]: row[3]
        for row in (line.split() for line in records[0] if line.startswith("$var"))
        if row[4] in {"char_pointer_wave", "char_array_wave"}
    }
    assert set(identifiers) == {"char_pointer_wave", "char_array_wave"}
    for identifier in identifiers.values():
        transitions = [
            int(line.split()[0][1:], 2) for line in records[0] if line.startswith("b") and line.split()[1] == identifier
        ]
        assert 99 in transitions and 255 in transitions

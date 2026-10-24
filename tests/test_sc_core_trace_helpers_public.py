"""Compare public trace helpers with native waveforms and comments."""

import difflib
import re
import subprocess
import sys

from .support import ROOT, run_example


def _normalized(text):
    """Remove only native creation dates, preserving declarations and transitions."""
    text = re.sub(r"\$date.*?\$end", "$date <date> $end", text, flags=re.S)
    return re.sub(
        r'comment "ASCII WIF file produced on.*?"', 'comment "ASCII WIF file produced on <date>"', text, flags=re.S
    )


def test_sc_core_trace_helpers_public(tmp_path):
    """Verify matching comments, useful declarations, delta changes and final flush."""
    python_path = tmp_path / "python"
    native_path = tmp_path / "native"
    python_path.mkdir()
    native_path.mkdir()
    output = run_example(python_path, "tests/models/sc_core_trace_helpers_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_trace_helpers_public.cpp"),
        ],
        cwd=native_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "TRACE_HELPERS_REFERENCE 3 ns" in output and "TRACE_HELPERS_REFERENCE 3 ns" in native.stdout
    for suffix in ("vcd", "awif"):
        actual = (python_path / f"helpers.{suffix}").read_text()
        expected = (native_path / f"helpers.{suffix}").read_text()
        assert _normalized(actual) == _normalized(expected), "\n".join(
            difflib.unified_diff(
                _normalized(expected).splitlines(), _normalized(actual).splitlines(), fromfile="native", tofile="python"
            )
        )
        for comment in (
            "plain comment",
            "formatted word -17 29 1.25 %",
            "lengths 1099511627776 137 000000ff Q",
            "dynamic    1.250",
            "hexfloat 0x1.4p+0",
            "no arguments %",
            "position 17 word",
            "position dynamic    1.250",
            "counted",
            "count value 7",
            "long " + "a" * 5000,
        ):
            assert comment in actual
    vcd = (python_path / "helpers.vcd").read_text()
    assert re.search(r"\$var wire\s+1\s+(\w+)\s+sample", vcd)
    identifier = re.search(r"\$var wire\s+1\s+(\w+)\s+sample", vcd).group(1)
    assert "0" + identifier in vcd and "1" + identifier in vcd
    assert "#1000" in vcd and "#3000" in vcd
    assert "assign" in (python_path / "helpers.awif").read_text()

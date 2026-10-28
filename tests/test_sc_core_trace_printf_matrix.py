"""Compare complete formatted-comment conversion coverage against native printf."""

import re
import subprocess
import sys

from .support import ROOT, run_example


def _normalize(text):
    """Remove creation dates while preserving every comment and trace record."""
    text = re.sub(r"\$date.*?\$end", "$date <date> $end", text, flags=re.S)
    return re.sub(
        r'comment "ASCII WIF file produced on.*?"', 'comment "ASCII WIF file produced on <date>"', text, flags=re.S
    )


def test_sc_core_trace_printf_matrix(tmp_path):
    """Check 99 comments per format, eight count-pointer lengths and invalid formats."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    output = run_example(python_dir, "tests/models/sc_core_trace_printf_matrix.py")
    assert "PRINTF_MATRIX cases=99 formats=2 errors=33" in output
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_trace_printf_matrix.cpp"),
        ],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    for filename in ("printf_vcd.vcd", "printf_wif.awif"):
        actual = (python_dir / filename).read_text()
        expected = (native_dir / filename).read_text()
        assert _normalize(actual) == _normalize(expected)
        assert actual.count("PRINTF ") == 99
        for label, value in (
            ("signed_default_i", "-17"),
            ("unsigned_default_o", "0377"),
            ("float_default_E", "-1.2500E+00"),
            ("float_L_G", "-1.25"),
            ("positional_reuse", "17 word 00000017"),
        ):
            assert f"PRINTF {label} {value}" in actual
        assert "sample" in actual

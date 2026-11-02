"""Compare inherited hash constructor defaults with native SystemC values."""

import subprocess
import sys

from .support import ROOT, run_example


def test_hash_constructor_defaults(tmp_path):
    """Check native settings and record missing inherited trailing defaults."""
    output = run_example(tmp_path, "tests/models/hash_constructor_defaults.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/hash_constructor_defaults.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "HASH_INHERITED_TRAILING_DEFAULTS_UNSUPPORTED" in output
    actual = [row for row in output.splitlines() if row.startswith(("HASH_DEFAULTS ", "HASH_CONSTANTS "))]
    expected = [row for row in native.stdout.splitlines() if row.startswith(("HASH_DEFAULTS ", "HASH_CONSTANTS "))]
    assert (
        actual
        == expected
        == [
            "HASH_DEFAULTS omitted 11 5 2 1",
            "HASH_DEFAULTS explicit 11 5 2 1",
            "HASH_DEFAULTS custom 17 3 1.5 0",
            "HASH_CONSTANTS 11 5 2 1",
        ]
    )

"""Compare vector bit-proxy public families against independent native C++."""

import csv
from collections import Counter
import subprocess
import sys
from .support import ROOT, run_example


def test_vector_bitrefs_match_native(tmp_path):
    """Verify exact overloads, clone lifetimes, four-state values and reports."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as source:
        counts = Counter("::".join(row["name"].split("::")[:2]) for row in csv.DictReader(source))
    assert counts["sc_dt::sc_bitref_r"] == 44
    assert counts["sc_dt::sc_bitref"] == 78
    actual = run_example(tmp_path, "tests/models/sc_dt_vector_bitrefs_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_vector_bitrefs_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    p = [line for line in actual.splitlines() if line.startswith("VBR ")]
    n = [line for line in native.stdout.splitlines() if line.startswith("VBR ")]
    assert len(p) == len(n) == 954
    assert p == n
    assert "VECTOR_BITREFS_PUBLIC_OK" in actual
    assert "VECTOR_BITREFS_PUBLIC_OK" in native.stdout

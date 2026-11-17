"""Compare native VCD creation errors and successful scoped trace recovery."""

import subprocess
import sys

from .support import ROOT, run_example


def test_vcd_creation_errors_and_recovery(tmp_path):
    """Match native reports and inspect declarations and transitions after recovery."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()
    output = run_example(python_dir, "tests/models/vcd_creation_contract.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/vcd_creation_contract.cpp")],
        cwd=native_dir,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [row for row in output.splitlines() if row.startswith("VCD_CREATION_")]
    expected = [row for row in native.stdout.splitlines() if row.startswith("VCD_CREATION_")]
    assert len(actual) == 2 and actual == expected
    assert actual[0].startswith("VCD_CREATION_ERROR ")
    assert actual[1] == "VCD_CREATION_RECOVERY 2000"
    records = []
    for directory in (python_dir, native_dir):
        rows = (directory / "creation_recovery.vcd").read_text().splitlines()
        records.append([row for row in rows if row.startswith(("$var", "#", "b"))])
    assert records[0] == records[1]
    declarations = [row.split() for row in records[0] if row.startswith("$var")]
    assert len(declarations) == 1
    assert declarations[0][2] == "32" and declarations[0][4] == "signal"
    identifier = declarations[0][3]
    values = [int(row.split()[0][1:], 2) for row in records[0] if row.startswith("b") and row.split()[1] == identifier]
    assert values == [0, 7]
    assert "#1" in records[0]

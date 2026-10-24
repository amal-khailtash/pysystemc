"""Compare Python cleanup in native member waits with C++ RAII unwinding."""

import subprocess
import sys

from .support import ROOT, run_example


def test_member_wait_reset_kill_cleanup_matches_native(tmp_path):
    """Verify time and OR/AND member waits execute cleanup at reset and kill."""
    output = run_example(tmp_path, "tests/models/sc_core_member_unwind.py")
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/sc_core_member_unwind.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("MEMBER_UNWIND ")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("MEMBER_UNWIND ")]
    assert len(actual) == 24 and actual == expected
    actual_halt = [line for line in output.splitlines() if line.startswith("MEMBER_HALT ")]
    native_halt = [line for line in native.stdout.splitlines() if line.startswith("MEMBER_HALT ")]
    assert actual_halt == native_halt == ["MEMBER_HALT start 2000", "MEMBER_HALT cleanup 4000"]
    for kind in ("module", "channel"):
        for mode in range(3):
            assert [line.rsplit(" ", 2)[1:] for line in actual if line.startswith(f"MEMBER_UNWIND {kind}{mode} ")] == [
                ["start", "0"],
                ["cleanup", "2000"],
                ["start", "2000"],
                ["cleanup", "3000"],
            ]

"""Compare public process-monitor and join behavior with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_monitor_join_public(tmp_path):
    """Check monitor lifetime, join timing, unwind cleanup and rejected handles."""
    output = run_example(tmp_path, "tests/models/sc_core_monitor_join_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_monitor_join_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_monitor = [line for line in output.splitlines() if line.startswith("MONITOR_PUBLIC_REFERENCE")]
    native_monitor = [line for line in native.stdout.splitlines() if line.startswith("MONITOR_PUBLIC_REFERENCE")]
    assert python_monitor == ["MONITOR_PUBLIC_REFERENCE 0"]
    assert native_monitor == ["MONITOR_PUBLIC_REFERENCE 0 1"]
    python_join = [line for line in output.splitlines() if line.startswith("JOIN_PUBLIC_REFERENCE")]
    native_join = [line for line in native.stdout.splitlines() if line.startswith("JOIN_PUBLIC_REFERENCE")]
    assert (
        python_join
        == native_join
        == [
            "JOIN_PUBLIC_REFERENCE dynamic_start=2@0 clocked_start=2@0 late_add=1@2000 "
            "dynamic_done=0@3000 clocked_done=0@6000",
        ]
    )
    python_cleanup = [line for line in output.splitlines() if line.startswith("JOIN_CLEANUP_REFERENCE")]
    native_cleanup = [line for line in native.stdout.splitlines() if line.startswith("JOIN_CLEANUP_REFERENCE")]
    assert (
        python_cleanup
        == native_cleanup
        == [
            "JOIN_CLEANUP_REFERENCE dynamic_start@0 clocked_start@0 dynamic_cleanup@500 dynamic_start@500 "
            "clocked_cleanup@500 clocked_start@500 dynamic_cleanup@1000 clocked_cleanup@2500",
        ]
    )
    python_rejections = [line for line in output.splitlines() if line.startswith("JOIN_REJECTION_REFERENCE")]
    native_rejections = [line for line in native.stdout.splitlines() if line.startswith("JOIN_REJECTION_REFERENCE")]
    rejection_type = "Attempt to register method process with sc_join object"
    assert (
        python_rejections
        == native_rejections
        == [
            f"JOIN_REJECTION_REFERENCE\tmethod\t561\t2\t{rejection_type}\t\t0",
            f"JOIN_REJECTION_REFERENCE\tempty\t561\t2\t{rejection_type}\t\t0",
        ]
    )

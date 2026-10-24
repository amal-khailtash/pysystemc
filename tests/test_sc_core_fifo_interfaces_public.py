"""Compare public FIFO interface behavior against native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_fifo_interfaces_public(tmp_path):
    """Verify six interface families, native event timing and FIFO lifetimes."""
    output = run_example(tmp_path, "tests/models/sc_core_fifo_interfaces_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_fifo_interfaces_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual_values = [line for line in output.splitlines() if line.startswith("FIFO_IFACE_VALUES")]
    expected_values = [line for line in native.stdout.splitlines() if line.startswith("FIFO_IFACE_VALUES")]
    actual_events = [line for line in output.splitlines() if line.startswith("FIFO_IFACE_EVENT")]
    expected_events = [line for line in native.stdout.splitlines() if line.startswith("FIFO_IFACE_EVENT")]
    actual_wide = [line for line in output.splitlines() if line.startswith("FIFO_IFACE_WIDE")]
    expected_wide = [line for line in native.stdout.splitlines() if line.startswith("FIFO_IFACE_WIDE")]
    actual_lifetime = [line for line in output.splitlines() if line.startswith("FIFO_IFACE_LIFETIME")]
    expected_lifetime = [line for line in native.stdout.splitlines() if line.startswith("FIFO_IFACE_LIFETIME")]
    actual_destruction = [line for line in output.splitlines() if line.startswith("FIFO_IFACE_DESTRUCTION")]
    expected_destruction = [line for line in native.stdout.splitlines() if line.startswith("FIFO_IFACE_DESTRUCTION")]
    assert actual_values == expected_values
    assert actual_events == expected_events
    assert actual_wide == expected_wide
    assert actual_lifetime == expected_lifetime == ["FIFO_IFACE_LIFETIME 1"]
    assert actual_destruction == expected_destruction == ["FIFO_IFACE_DESTRUCTION 1 1 1 1 1 1"]
    assert actual_values == ["FIFO_IFACE_VALUES 0 2 0 91 1 0 0 0 2 17 23 0 0 2"]
    assert actual_events == ["FIFO_IFACE_EVENT W 0 0", "FIFO_IFACE_EVENT R 1000 1"]
    assert actual_wide == [
        "FIFO_IFACE_WIDE 0 1 0 0 3 "
        "174224571863520493293247799005065324265 "
        "174224571863520493293247799005065324267 "
        "174224571863520493293247799005065324271 0 0 3"
    ]
    assert native.stdout.index("FIFO_IFACE_LIFETIME") < native.stdout.index("FIFO_IFACE_DESTRUCTION")

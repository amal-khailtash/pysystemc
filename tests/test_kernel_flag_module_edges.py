"""Check public kernel flag module behavior in a fresh interpreter."""

from .support import run_example


def test_kernel_flag_module_edges(tmp_path):
    """Verify ordinary module attributes, live native flags, and import snapshots."""
    output = run_example(tmp_path, "tests/models/kernel_flag_module_edges.py")
    records = [line for line in output.splitlines() if line.startswith("KERNEL_FLAG_MODULE_EDGE ")]
    assert records == [
        "KERNEL_FLAG_MODULE_EDGE ordinary=identity,removed native_unchanged=1 assignments=4 raw=2 "
        "invalid=8 snapshots=independent"
    ]

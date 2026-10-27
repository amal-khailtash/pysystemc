"""Run native pointer-vector index boundary checks in an isolated process."""

from .support import run_example


def test_sc_core_pvector_index_limits(tmp_path):
    """Reject wrapping unsigned indices while preserving existing pointers."""
    output = run_example(tmp_path, "tests/models/sc_core_pvector_index_limits.py")
    assert "PVECTOR_INDEX_LIMITS_OK rejected=10 unchanged=1 null_growth=1" in output

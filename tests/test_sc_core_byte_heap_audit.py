"""Verify byte heap zero-length and initialization boundary semantics."""

from .support import run_example


def test_sc_core_byte_heap_audit(tmp_path):
    """Check empty-view pointer parity and safe rejection of an uninitialized heap."""
    output = run_example(tmp_path, "tests/models/sc_core_byte_heap_audit.py")
    assert "BYTE_HEAP_ZERO_CAPACITY True 0" in output
    assert "BYTE_HEAP_ZERO_CAPACITY_BOUNDS True" in output
    assert "BYTE_HEAP_UNINITIALIZED_ZERO True" in output

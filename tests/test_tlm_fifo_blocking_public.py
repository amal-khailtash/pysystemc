"""Run guarded TLM FIFO blocking behavior in an isolated SystemC process."""

from .support import run_example


def test_tlm_fifo_blocking_operations_and_cleanup(tmp_path):
    """Check blocking overloads, wait resumption, value copies, and cleanup."""
    output = run_example(tmp_path, "tests/models/tlm_fifo_blocking_public.py")
    assert "TLM_FIFO_BLOCKING_PUBLIC_OK" in output

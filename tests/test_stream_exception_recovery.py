"""Verify native stream exception masks and datatype recovery behavior."""

from .support import run_example


def test_stream_exception_recovery_and_chaining(tmp_path):
    """Check exhausted extraction and failed insertion recover on same streams."""
    output = run_example(tmp_path, "tests/models/stream_exception_recovery.py")
    assert "STREAM_EXCEPTION_INPUT_RECOVERED fail|eof eof width=137" in output
    assert "STREAM_EXCEPTION_OUTPUT_RECOVERED bad|fail good" in output
    assert "STREAM_EXCEPTION_CHAINED_SAME_OWNER" in output

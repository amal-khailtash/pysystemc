"""Verify native kill unwind through the public process-handle throw_it API."""

from .support import run_example


def test_throw_it_unwinds_killed_caller_after_target_yields(tmp_path):
    """Check exception payload, callback cleanup order, timestamps and termination."""
    output = run_example(tmp_path, "tests/models/throw_it_unwind.py")
    records = [line for line in output.splitlines() if line.startswith("THROW_IT_UNWIND ")]
    assert records == [
        "THROW_IT_UNWIND target_enter 0",
        "THROW_IT_UNWIND injector_before 1000",
        "THROW_IT_UNWIND target_caught 1000",
        "THROW_IT_UNWIND injector_cleanup 1000",
        "THROW_IT_UNWIND after_kill 1000",
        "THROW_IT_UNWIND target_done 2000",
        "THROW_IT_UNWIND target_cleanup 2000",
    ]
    assert "THROW_IT_UNWIND_OK" in output

"""Cover scheduler unwind when Python callbacks suppress or replace cleanup."""

from .support import run_example


def test_section1_lifecycle_edges(tmp_path):
    """Verify reset/kill survive suppressed unwind and raising finally blocks."""
    output = run_example(tmp_path, "tests/models/section1_lifecycle_edges.py")
    assert "SECTION1_LIFECYCLE reset_ns=1000 kill_ns=2000 suppressed=2 raised=2" in output
    assert "SECTION1_LIFECYCLE_RELEASE callbacks=2 cleanup_errors=2" in output

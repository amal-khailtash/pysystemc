"""Check process-control context errors and report callback ownership."""

from .support import run_example


def test_process_control_rejects_paused_and_stopped_top_level_calls(tmp_path):
    """Ensure invalid reset/kill calls leave a live paused process untouched."""
    output = run_example(tmp_path, "tests/models/sc_core_control_error_lifetime.py", ("controls",))
    assert "CONTROL_ERROR_LIFETIME_OK paused=2 stopped=2" in output


def test_report_handler_exception_and_callback_release(tmp_path):
    """Check report callback failures propagate once and restore releases owners."""
    output = run_example(tmp_path, "tests/models/sc_core_control_error_lifetime.py", ("report",))
    assert "REPORT_CALLBACK_LIFETIME_OK calls=2 replaced=1 released=2 reused=1" in output

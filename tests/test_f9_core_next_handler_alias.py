"""Record missing direct handler callback typedef lookup independently."""

from .support import run_example


def test_f9_core_next_handler_alias(tmp_path):
    """Compile the exact typedef signature and confirm both namespace lookups fail."""
    output = run_example(tmp_path, "tests/models/f9_core_next_handler_alias.py")
    assert "F9_CORE_NEXT_HANDLER_ALIAS 1 0 0" in output

"""Exercise uncovered public export specialization and borrower paths."""

from .support import run_example


def test_sc_core_export_audit(tmp_path):
    """Keep string specialization and borrowed arrow/report semantics native."""
    output = run_example(tmp_path, "tests/models/sc_core_export_audit.py")
    assert "EXPORT_AUDIT_OK type=string arrow=borrowed duplicate=126 value=47" in output

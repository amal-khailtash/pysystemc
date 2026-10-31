"""Exercise public namespace discovery in one isolated Python process."""

from .support import run_example


def test_namespace_presentation_filters_bridge_names_and_keeps_public_api(tmp_path):
    """Compare public help/import/discovery behavior without kernel advancement."""
    output = run_example(tmp_path, "tests/models/namespace_presentation.py")
    assert "NAMESPACE_PRESENTATION_OK" in output

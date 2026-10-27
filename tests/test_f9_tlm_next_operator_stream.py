"""Validate the direct Python alias for TLM phase stream insertion."""

from .support import run_example


def test_direct_tlm_operator_stream_alias(tmp_path):
    """Call tlm.operator<< directly and compare it with native stream syntax."""
    output = run_example(tmp_path, "tests/models/f9_tlm_next_operator_stream.py")
    assert "TLM_NEXT_OPERATOR_STREAM|BEGIN_REQ|1" in output

"""Cover public constructor-adapter reuse and native rejection edges."""

from .support import run_example


def test_section1_hash_adapter_edges(tmp_path):
    """Exercise native predicate reuse and preserve inherited-call errors."""
    output = run_example(tmp_path, "tests/models/section1_hash_adapter_edges.py")
    assert "HASH_ADAPTER_EDGES predicate_reuse=ProbeWithTag signature_rejection=sc_phash preserved_errors=4" in output
    assert "HASH_ADAPTER_RETRY_DIAGNOSTICS unknown,overlap,invalid_growth" in output

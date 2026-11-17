"""Verify cached boolean-bit concatenation preserves native views and lifetime."""

from .support import run_example


def test_concat_dispatch_cache_matches_native(tmp_path):
    """Compare eight routes with native overloads and live parent mutations."""
    output = run_example(tmp_path, "tests/models/concat_dispatch_cache.py")
    assert "BOOL_BIT_CONCAT_MODEL_OK parents=bv137/lv257 routes=mutable/readonly,head/tail" in output

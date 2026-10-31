"""Check safe recovery and ownership of public legacy string adapters."""

from .support import run_example


def test_legacy_string_recovery_and_source_ownership(tmp_path):
    """Verify conversion failures recover without corrupting native strings."""
    output = run_example(tmp_path, "tests/models/legacy_string_recovery.py")
    assert "LEGACY_STRING_FMT_REJECTED_RECOVERED value=0007 alias=owner" in output
    assert "LEGACY_STRING_BYTES_REJECTED_RECOVERED native" in output
    assert "LEGACY_STRING_MUTATION_REJECTED_RECOVERED aZc" in output
    assert "LEGACY_STRING_FORMAT_COPY native:17 Native:17 native:17" in output
    assert "LEGACY_STRING_CONCAT_SOURCE_RELEASED pre-suffix" in output

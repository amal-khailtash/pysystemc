"""Validate public sc_core callback alias types in a fresh cppyy process."""

from .support import run_example


def test_f9_core_alias_types(tmp_path):
    """Compile exact native type checks and exercise Python callback conversion."""
    output = run_example(tmp_path, "tests/models/f9_core_alias_types.py")
    assert "F9_CORE_ALIAS_TYPES 1 1 1 CFT 0 0 MAP_FN 0 0" in output

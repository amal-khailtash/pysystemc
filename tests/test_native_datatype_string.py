"""Keep SystemC datatype str and f-string output on native stream insertion."""

from .support import run_example


def test_native_datatype_string_matches_stream_insertion(tmp_path):
    """Compare arbitrary-width and fixed-point strings in an isolated process."""
    output = run_example(tmp_path, "tests/models/native_datatype_string.py")
    assert "NATIVE_DATATYPE_STRING_OK" in output
    values = [line for line in output.splitlines() if line.startswith("NDS\tVALUE\t")]
    assert len(values) == 16
    assert "NDS\tVALUE\tbiguint257\t987654321" in values
    assert "NDS\tVALUE\tbigint257\t-987654321" in values
    assert "NDS\tSHIFT\t123456789" in output
    assert "NDS\tCONCAT\t76397\t87405" in output

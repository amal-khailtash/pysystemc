"""Verify scalar bit/logic operators against native expressions and reports."""

import json

from .support import run_example


def test_scalar_bit_operators_match_native_and_recover_from_reports(tmp_path):
    """Check current values, result ownership, cached dispatch and native reports.

    Args:
        tmp_path: Isolated working directory for a fresh native child.
    Raises:
        AssertionError: Native parity, ownership or report recovery differs.
    """
    output = run_example(tmp_path, "tests/models/scalar_bit_operators.py")
    marker = "SCALAR_BIT_RESULT="
    records = [json.loads(line[len(marker):]) for line in output.splitlines() if line.startswith(marker)]
    assert len(records) == 1, output
    record = records[0]
    assert record["paths_checked"] == 160
    assert record["invalid_operands_checked"] == 12
    assert record["public_report_count"] == record["native_report_count"] == 2
    assert record["public_report_messages"] == record["native_report_messages"]
    assert all(record[key] for key in (
        "package_imported_before_cppyy", "native_parity", "cache_present",
        "cache_callable_stable", "report_recovery_parity",
    ))

"""Regress repeated numeric reports after cached binary dispatch lookup."""

import json

from .support import run_example


MARKER = "NUMERIC_BINARY_REPORT_REUSE="


def test_numeric_binary_report_reuse_throws_each_time_and_reuses_current_value(tmp_path):
    """Check repeated public/native zero-divisor reports and same-key recovery.

    Args:
        tmp_path: Isolated working directory for the fresh SystemC child.
    Raises:
        AssertionError: If reports, cache behavior, ownership or native parity differ.
    """
    output = run_example(tmp_path, "tests/models/numeric_binary_report_reuse.py")
    records = [line[len(MARKER) :] for line in output.splitlines() if line.startswith(MARKER)]
    assert len(records) == 1, output
    record = json.loads(records[0])

    assert record["public_report_count"] == record["native_report_count"] == 2
    assert record["messages_match_native"]
    assert record["messages_report_division_by_zero"]
    assert all("division by zero" in message.lower() for message in record["public_messages"])
    assert all("division by zero" in message.lower() for message in record["native_messages"])
    assert record["denominator_after_same_owner_mutation"] == "2"
    assert record["public_result_value"] == record["native_result_value"] == "1"
    assert record["native_parity"]
    assert record["results_owned"]
    assert record["result_addresses_independent"]
    assert record["sources_unchanged"]
    assert record["source_addresses_unchanged"]
    if record["cache_present"]:
        assert not record["cache_key_present_before"]
        assert record["cache_key_present_after_failure"]
        assert record["cache_callable_stable_through_success"]
        assert record["cache_size_before"] == 0
        assert record["cache_size_after"] == 1
        assert record["cache_reused_without_storing_exception"]

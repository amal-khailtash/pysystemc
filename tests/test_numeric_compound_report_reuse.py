"""Regress repeated numeric reports after compound dispatch lookup."""

import json

from .support import run_example


MARKER = "NUMERIC_COMPOUND_REPORT_REUSE="


def test_numeric_compound_report_reuse_matches_native_failure_and_recovery(tmp_path):
    """Check repeated public/native reports, mutation parity and recovery.

    Args:
        tmp_path: Isolated working directory for the fresh SystemC child.
    Raises:
        AssertionError: If report messages, mutation, ownership or parity differ.
    """
    output = run_example(tmp_path, "tests/models/numeric_compound_report_reuse.py")
    records = [line[len(MARKER) :] for line in output.splitlines() if line.startswith(MARKER)]
    assert len(records) == 1, output
    record = json.loads(records[0])

    assert record["public_report_count"] == record["native_report_count"] == 2
    assert record["messages_match_native"]
    assert record["messages_report_division_by_zero"]
    assert all("division by zero" in message.lower() for message in record["public_messages"])
    assert all("division by zero" in message.lower() for message in record["native_messages"])
    assert record["public_failure_states"] == record["native_failure_states"]
    assert record["failure_states_match_native"]
    assert record["public_owner_value_before_failures"] == "7"
    assert record["native_owner_value_before_failures"] == "7"
    assert record["public_return_is_owner"]
    assert record["native_helper_return_is_void"]
    assert record["public_result_value"] == record["native_result_value"] == "1"
    assert record["native_parity"]
    assert record["public_owner_still_owned"]
    assert record["native_owner_still_owned"]
    assert record["public_denominator_still_owned"]
    assert record["native_denominator_still_owned"]
    assert record["ownership_unchanged"]
    assert record["owner_addresses_distinct"]
    assert record["denominator_addresses_distinct"]
    assert record["sources_unchanged"]
    assert record["addresses_unchanged"]
    assert record["cache_present"]
    assert not record["cache_key_present_before"]
    assert record["cache_key_present_after_failure"]
    assert record["cache_callable_stable_through_success"]
    assert record["cache_reused_without_storing_exception"]
    assert record["cache_size_before"] == 0
    assert record["cache_size_after"] == 1

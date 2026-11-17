"""Check member next_trigger cache isolation and event-expression lifetimes."""

import json

from .support import run_example


def test_sc_core_member_trigger_lifetime(tmp_path):
    """Match two interleaved Python module owners to independent native methods."""
    output = run_example(tmp_path, "tests/models/sc_core_member_trigger_lifetime.py")
    marker = "MEMBER_TRIGGER_LIFETIME_OK "
    records = [line[len(marker) :] for line in output.splitlines() if line.startswith(marker)]
    assert len(records) == 1
    result = json.loads(records[0])
    python_rows = {
        label: [[row["step"], row["ticks"]] for row in rows] for label, rows in result["python_rows"].items()
    }
    assert python_rows == result["native_rows"]
    assert result["owner_methods_valid_after_simulation"]
    assert result["driver_terminated"]
    assert len(result["expression_weakref_unsupported"]) + result["expression_weakrefs_collected"] == 4

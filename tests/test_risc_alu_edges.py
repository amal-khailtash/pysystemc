"""Validate source ALU operations within defined native integer ranges."""

import json

from .support import run_example


def test_risc_alu_defined_operations_flags_and_timing_match_native(tmp_path):
    """Compare 17 operations, retained results, destinations, flags and cycle times."""
    rows = []
    for variant in ("python", "native"):
        output = run_example(tmp_path, "tests/models/risc_alu_edges.py", (variant,))
        rows.append(json.loads(next(line.removeprefix("ALU_EDGES=") for line in output.splitlines()
                                    if line.startswith("ALU_EDGES="))))
    assert rows[0] == rows[1]
    expected = [0, 579, -19, 0, -3, 2147385345, -4, -4, -9, 8, 14, 6, -13, 1073741824, 64, -1, -1]
    assert [row["result"] for row in rows[0]] == expected
    assert [row["dest"] for row in rows[0]] == list(range(17))
    assert [row["Z"] for row in rows[0]] == [value == 0 for value in expected]
    assert all(not row["C"] and not row["V"] for row in rows[0])
    assert all(b["ns"] > a["ns"] for a, b in zip(rows[0], rows[0][1:]))

"""Audit all 800 sc_dt named arithmetic declarations with exact native parity."""

from collections import Counter
import csv
import hashlib
import subprocess
import sys

import pytest

from .support import ROOT, run_example

OPERATIONS = ("add", "sub", "mult", "div")
OUTPUTS = ("sc_dt::sc_fxval", "sc_dt::sc_fxval_fast", "sc_dt::sc_fxnum", "sc_dt::sc_fxnum_fast")
INVENTORY_SHA256 = "872ec81a450298c26254f42feec6dc95964cf4f131c8147b87d8f71f533ef6bd"
OUTPUT_ROW_COUNTS = {
    "sc_dt::sc_fxval": 66,
    "sc_dt::sc_fxval_fast": 68,
    "sc_dt::sc_fxnum": 33,
    "sc_dt::sc_fxnum_fast": 33,
}
OUTPUT_SIGNATURE_COUNTS = {
    "sc_dt::sc_fxval": 64,
    "sc_dt::sc_fxval_fast": 64,
    "sc_dt::sc_fxnum": 33,
    "sc_dt::sc_fxnum_fast": 33,
}
SCALAR_TYPES = {
    "int",
    "unsigned int",
    "long",
    "unsigned long",
    "float",
    "double",
    "const char *",
    "sc_dt::int64",
    "sc_dt::uint64",
}


def inventory_rows():
    """Return all inventory entries for native sc_dt add/sub/mult/div."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as source:
        return [
            row for row in csv.DictReader(source) if row["name"] in {f"sc_dt::{operation}" for operation in OPERATIONS}
        ]


def arguments(signature):
    """Split one exact native function declaration into its argument types."""
    return tuple(part.strip() for part in signature[signature.index("(") + 1 : signature.rindex(")")].split(", "))


def normalized(spelling):
    """Normalize spacing without erasing pointer, reference, or cv qualifiers."""
    return " ".join(spelling.split())


def signature_matrix(rows, output):
    """Build a reusable operation-to-exact-operand-signatures table for one output."""
    matrix = {operation: {} for operation in OPERATIONS}
    for row in rows:
        operation = row["name"].split("::")[-1]
        params = arguments(row["signature"])
        if normalized(params[0]) == f"{output} &":
            matrix[operation][tuple(normalized(param) for param in params[1:])] = params
    return matrix


def matrix_records(output, prefix):
    """Select deterministic native-parity records from one captured subprocess."""
    return [line for line in output.splitlines() if line.startswith(prefix + "\t")]


def test_sc_dt_named_arithmetic_inventory():
    """Prove all 800 declarations, duplicate rows, outputs and ordered operands."""
    rows = inventory_rows()
    assert len(rows) == 800
    assert Counter(row["name"] for row in rows) == Counter({f"sc_dt::{name}": 200 for name in OPERATIONS})
    assert all(row["kind"] == "FunctionDecl" for row in rows)
    canonical = "".join("\t".join(row[key] for key in ("name", "kind", "source", "signature")) + "\n" for row in rows)
    assert hashlib.sha256(canonical.encode()).hexdigest() == INVENTORY_SHA256

    for operation in OPERATIONS:
        selected = [row for row in rows if row["name"] == f"sc_dt::{operation}"]
        row_counts = Counter(arguments(row["signature"])[0] for row in selected)
        assert row_counts == Counter({f"{output} &": OUTPUT_ROW_COUNTS[output] for output in OUTPUTS})
        for output in OUTPUTS:
            matrix = signature_matrix(selected, output)
            pairs = matrix[operation]
            assert len(pairs) == OUTPUT_SIGNATURE_COUNTS[output]
            for left, right in pairs:
                assert left not in SCALAR_TYPES or right not in SCALAR_TYPES, (output, left, right)
            assert any(left in SCALAR_TYPES and right not in SCALAR_TYPES for left, right in pairs)
            assert any(left not in SCALAR_TYPES and right in SCALAR_TYPES for left, right in pairs)


@pytest.mark.parametrize("output_index,output", tuple(enumerate(OUTPUTS)))
@pytest.mark.parametrize("mode", (0, 1), ids=("lower_half", "upper_half"))
def test_sc_dt_named_arithmetic_native_parity(tmp_path, output_index, output, mode):
    """Compare public and exact-selector calls with all native function pointers."""
    python_output = run_example(
        tmp_path,
        "tests/models/sc_dt_named_arithmetic_public.py",
        (str(output_index), str(mode)),
    )
    selector = tmp_path / "named_arithmetic_select.cpp"
    selector.write_text(
        f"/** @file Select one bounded exact named arithmetic matrix half. */\n"
        f"#define NAMED_ARITH_OUTPUT {output_index}\n#define NAMED_ARITH_MODE {mode}\n",
        encoding="utf-8",
    )
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(selector),
            str(ROOT / "tests/native/sc_dt_named_arithmetic_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = matrix_records(python_output, "NAR")
    native_records = matrix_records(native.stdout, "NAR")
    python_alias = matrix_records(python_output, "NARA")
    native_alias = matrix_records(native.stdout, "NARA")
    python_alias_limits = matrix_records(python_output, "NAR_ALIAS_UNSUPPORTED")
    native_alias_limits = matrix_records(native.stdout, "NAR_ALIAS_UNSUPPORTED")
    rows = inventory_rows()
    matrix = signature_matrix(rows, output)
    ordered = sorted(matrix["add"])
    midpoint = (len(ordered) + 1) // 2
    selected = ordered[:midpoint] if mode == 0 else ordered[midpoint:]
    assert len(python_records) == len(native_records) == 4 * len(selected)
    assert python_records == native_records
    alias_count = 0 if mode != 0 or output == "sc_dt::sc_fxnum" else 8
    assert len(python_alias) == len(native_alias) == alias_count
    assert python_alias == native_alias
    expected_alias_limit = (
        ["NAR_ALIAS_UNSUPPORTED\tsc_dt::sc_fxnum\tDEFN_BIN_FNC_OUTPUT_MUST_NOT_ALIAS_INPUT"]
        if mode == 0 and output == "sc_dt::sc_fxnum"
        else []
    )
    assert python_alias_limits == native_alias_limits == expected_alias_limit
    assert f"SC_DT_NAMED_ARITHMETIC_PUBLIC_OK {output_index} {mode}" in python_output
    if mode == 0 and output in ("sc_dt::sc_fxnum", "sc_dt::sc_fxnum_fast"):
        assert f"NAMED_ARITH_LIFETIME\t{output}\t1\t" in python_output

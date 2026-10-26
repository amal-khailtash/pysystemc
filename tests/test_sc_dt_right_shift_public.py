"""Audit native shift and stream-extraction declarations against C++ references."""

from collections import Counter
import csv
import hashlib
import subprocess
import sys

from .support import ROOT, run_example

INVENTORY_SHA256 = "6f3f56ddd66aeedd7e1071c06079bdd4e5f42672e6b8385f50ac37827d3c3b9a"
STREAM_TARGETS = {
    "signed",
    "unsigned",
    "sc_int_base",
    "sc_uint_base",
    "sc_bv_proxy",
    "sc_lv_proxy",
    "sc_bit",
    "sc_logic",
    "sc_fxval",
    "sc_fxval_fast",
    "sc_fxnum",
    "sc_fxnum_fast",
    "sc_string_old",
    "sc_signed_bitref",
    "sc_unsigned_bitref",
    "sc_int_bitref",
    "sc_uint_bitref",
    "sc_bitref_template",
    "sc_logic_bitref",
    "sc_signed_subref",
    "sc_unsigned_subref",
    "sc_int_subref",
    "sc_uint_subref",
    "sc_subref_template",
    "sc_logic_subref",
    "sc_fxnum_bitref",
    "sc_fxnum_fast_bitref",
    "sc_fxnum_subref",
    "sc_fxnum_fast_subref",
    "sc_concref",
    "sc_concatref",
}
NATIVE_SHIFT_COUNTS = {
    "signed": {
        "signed_signed",
        "signed_unsigned",
        "signed_int64",
        "signed_uint64",
        "signed_long",
        "signed_ulong",
        "signed_int",
        "signed_unsigned_int",
        "signed_sc_int",
        "signed_sc_uint",
    },
    "unsigned": {
        "unsigned_signed",
        "unsigned_unsigned",
        "unsigned_int64",
        "unsigned_uint64",
        "unsigned_long",
        "unsigned_ulong",
        "unsigned_int",
        "unsigned_unsigned_int",
        "unsigned_sc_int",
        "unsigned_sc_uint",
    },
    "concat": {
        "concat_int",
        "concat_unsigned_int",
        "concat_long",
        "concat_unsigned_long",
        "concat_int64",
        "concat_uint64",
    },
}


def records(output, prefix):
    """Return tab-separated behavior rows for one record family."""
    return [line for line in output.splitlines() if line.startswith(prefix + "\t")]


def test_sc_dt_right_shift_inventory_and_native_parity(tmp_path):
    """Cover all 68 declarations, exact typed counts, 31 streams and wide results."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as source:
        rows = [row for row in csv.DictReader(source) if row["name"] == "sc_dt::operator>>"]
    assert len(rows) == 68
    canonical = "".join(
        "\t".join(row[field] for field in ("kind", "signature", "source", "line")) + "\n" for row in rows
    )
    assert hashlib.sha256(canonical.encode()).hexdigest() == INVENTORY_SHA256
    assert Counter(row["kind"] for row in rows) == Counter({"FunctionDecl": 63, "FunctionTemplateDecl": 5})
    numeric_rows = [row for row in rows if row["signature"] and not row["signature"].startswith("::std::istream")]
    stream_rows = [row for row in rows if row["signature"].startswith("::std::istream")]
    assert len(numeric_rows) == 32 and len(stream_rows) == 31
    python_output = run_example(tmp_path, "tests/models/sc_dt_right_shift_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_right_shift_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    python_shifts = records(python_output, "RS")
    native_shifts = records(native.stdout, "RS")
    python_shift_by_label = {line.split("\t")[1]: line for line in python_shifts}
    native_shift_by_label = {line.split("\t")[1]: line for line in native_shifts}
    assert len(python_shift_by_label) == len(python_shifts) == 56
    assert len(native_shift_by_label) == len(native_shifts) == 56
    assert python_shift_by_label == native_shift_by_label
    parsed_shifts = [line.split("\t") for line in native_shifts]
    assert len(parsed_shifts) == 56
    shift_labels = {row[1] for row in parsed_shifts}
    assert shift_labels == {
        *NATIVE_SHIFT_COUNTS["signed"],
        *NATIVE_SHIFT_COUNTS["unsigned"],
        *NATIVE_SHIFT_COUNTS["concat"],
        "signed_zero",
        "signed_negative",
        "signed_large",
        "unsigned_zero",
        "unsigned_negative",
        "unsigned_large",
        "concat_zero",
        "concat_large",
        "fxval_int",
        "fxval_zero",
        "fxval_negative",
        "fxval_large",
        "fxval_fast_int",
        "fxval_fast_zero",
        "fxval_fast_negative",
        "fxval_fast_large",
        "fxnum_int",
        "fxnum_zero",
        "fxnum_negative",
        "fxnum_large",
        "fxnum_fast_int",
        "fxnum_fast_zero",
        "fxnum_fast_negative",
        "fxnum_fast_large",
        *(f"bigint_{width}" for width in (137, 191, 257)),
        *(f"biguint_{width}" for width in (137, 191, 257)),
    }
    assert Counter(row[1].split("_")[0] for row in parsed_shifts) == Counter(
        {
            "signed": 13,
            "unsigned": 13,
            "concat": 8,
            "fxval": 8,
            "fxnum": 8,
            "bigint": 3,
            "biguint": 3,
        }
    )
    shift_by_label = {row[1]: row[2:] for row in parsed_shifts}
    assert shift_by_label["signed_negative"] == shift_by_label["signed_zero"]
    assert shift_by_label["unsigned_negative"] == shift_by_label["unsigned_zero"]
    assert {label for label in shift_labels if label.startswith(("bigint_", "biguint_"))} == {
        f"{family}_{width}" for family in ("bigint", "biguint") for width in (137, 191, 257)
    }

    python_streams = records(python_output, "SE")
    native_streams = records(native.stdout, "SE")
    assert python_streams == native_streams
    parsed_streams = [line.split("\t") for line in native_streams]
    assert len(parsed_streams) == len(STREAM_TARGETS) == 31
    assert {row[1] for row in parsed_streams} == STREAM_TARGETS
    assert all(row[4:] == ["1", "1", "0", "0", "4"] for row in parsed_streams)

    python_parents = records(python_output, "PAR")
    native_parents = records(native.stdout, "PAR")
    assert python_parents == native_parents
    assert {line.split("\t")[1] for line in native_parents} == {
        "sc_signed_bitref",
        "sc_unsigned_bitref",
        "sc_int_bitref",
        "sc_uint_bitref",
        "sc_bitref_template",
        "sc_logic_bitref",
        "sc_signed_subref",
        "sc_unsigned_subref",
        "sc_int_subref",
        "sc_uint_subref",
        "sc_subref_template",
        "sc_logic_subref",
        "sc_fxnum_bitref",
        "sc_fxnum_fast_bitref",
        "sc_fxnum_subref",
        "sc_fxnum_fast_subref",
        "sc_concref_left",
        "sc_concref_right",
        "sc_concatref_left",
        "sc_concatref_right",
    }
    assert all(row[2] != row[3] for row in (line.split("\t") for line in native_parents))

    python_failures = records(python_output, "FAIL")
    native_failures = records(native.stdout, "FAIL")
    assert len(python_failures) == len(native_failures) == 3
    assert python_failures[0] == native_failures[0] == "FAIL\tsigned_empty_conversion\t1\t37\t6\tconversion failed"
    assert python_failures[2] == native_failures[2] == "FAIL\tstring_empty\t1\t\t6\t0"
    python_masked = python_failures[1].split("\t")
    native_masked = native_failures[1].split("\t")
    assert python_masked[:5] == native_masked[:5] == ["FAIL", "unsigned_masked_empty", "1", "37", "6"]
    # cppyy exposes this libstdc++ I/O failure through the std::exception base.
    assert python_masked[5] == "std::exception"
    assert native_masked[5].endswith("iostream error")
    assert records(python_output, "LIFE") == records(native.stdout, "LIFE")
    assert "SC_DT_RIGHT_SHIFT_PUBLIC_OK" in python_output
    assert "SC_DT_RIGHT_SHIFT_PUBLIC_OK" in native.stdout

"""Check exact vector-owner inventory coverage and independent native parity."""

import csv
from collections import Counter
import hashlib
from pathlib import PurePosixPath
import subprocess
import sys

from .support import ROOT, run_example

FAMILY_ROW_COUNTS = {
    "sc_bv": 37,
    "sc_lv": 38,
    "sc_bv_base": 55,
    "sc_lv_base": 54,
}
INVENTORY_SHA256 = "3f765d0d43ea448612f10ed0d24d09d6a9a3c2b78fab2c4da463c4286059cc2c"
FIXED_BV_CONSTRUCTORS = {
    "default",
    "bool",
    "char",
    "char_pointer",
    "bool_pointer",
    "logic_pointer",
    "sc_unsigned",
    "sc_signed",
    "sc_uint_base",
    "sc_int_base",
    "unsigned_long",
    "long",
    "unsigned_int",
    "int",
    "uint64",
    "int64",
    "proxy_bv",
    "proxy_lv",
    "copy",
}
FIXED_LV_CONSTRUCTORS = FIXED_BV_CONSTRUCTORS | {"logic"}
BASE_BV_CONSTRUCTORS = {
    "int_length",
    "bool_length",
    "char_pointer",
    "char_pointer_length",
    "copy",
    "proxy_bv",
    "proxy_lv",
    "proxy_template",
    "sc_signed",
    "sc_unsigned",
    "sc_uint_base",
    "sc_int_base",
}
BASE_LV_CONSTRUCTORS = {
    "int_length",
    "logic_length",
    "char_pointer",
    "char_pointer_length",
    "copy",
    "proxy_bv",
    "proxy_lv",
    "proxy_template",
    "sc_signed",
    "sc_unsigned",
    "sc_uint_base",
    "sc_int_base",
}
FIXED_ASSIGNMENTS = {
    "proxy_template",
    "copy",
    "char_pointer",
    "bool_pointer",
    "logic_pointer",
    "sc_unsigned",
    "sc_signed",
    "sc_uint_base",
    "sc_int_base",
    "unsigned_long",
    "long",
    "unsigned_int",
    "int",
    "uint64",
    "int64",
}
BASE_ASSIGNMENTS = FIXED_ASSIGNMENTS | {"proxy_bv", "proxy_lv"}


def inventory_rows():
    """Return only the four audited vector families, including class rows."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as inventory_file:
        rows = csv.DictReader(inventory_file)
        return [
            (
                row["name"],
                row["kind"],
                PurePosixPath(row["source"]).as_posix(),
                row["signature"],
            )
            for row in rows
            if row["name"] in {"sc_dt::sc_bv", "sc_dt::sc_lv"}
            or row["name"].startswith(("sc_dt::sc_bv::", "sc_dt::sc_lv::", "sc_dt::sc_bv_base", "sc_dt::sc_lv_base"))
        ]


def output_records(output):
    """Return emitted parity rows, excluding documented adapter metadata."""
    return [
        line
        for line in output.splitlines()
        if line.startswith("SDV\t") and line.split("\t", 2)[1] not in {"gap", "compat"}
    ]


def family_records(records, record_family, owner_name):
    """Select one emitted constructor or assignment label set."""
    return {
        line.split("\t")[3]
        for line in records
        if line.split("\t")[1] == record_family and line.split("\t")[2] == owner_name
    }


def test_sc_dt_vector_owners_inventory_and_native_parity(tmp_path):
    """Cover all 184 inventory rows and compare public Python results with C++."""
    actual_inventory = inventory_rows()
    assert len(actual_inventory) == 184
    assert Counter(name.removeprefix("sc_dt::").split("::", 1)[0] for name, _, _, _ in actual_inventory) == Counter(
        FAMILY_ROW_COUNTS
    )
    canonical_inventory = "".join("\t".join(row) + "\n" for row in actual_inventory)
    assert hashlib.sha256(canonical_inventory.encode()).hexdigest() == INVENTORY_SHA256

    python_output = run_example(tmp_path, "tests/models/sc_dt_vectors_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_vectors_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    python_records = output_records(python_output)
    native_records = output_records(native.stdout)
    assert python_records == native_records
    assert len(python_records) == 142

    expected_ctors = {
        "sc_bv37": FIXED_BV_CONSTRUCTORS,
        "sc_lv38": FIXED_LV_CONSTRUCTORS,
        "sc_bv_base55": BASE_BV_CONSTRUCTORS,
        "sc_lv_base54": BASE_LV_CONSTRUCTORS,
    }
    expected_assignments = {
        "sc_bv37": FIXED_ASSIGNMENTS,
        "sc_lv38": FIXED_ASSIGNMENTS,
        "sc_bv_base55": BASE_ASSIGNMENTS,
        "sc_lv_base54": BASE_ASSIGNMENTS,
    }
    for owner_name, labels in expected_ctors.items():
        assert family_records(python_records, "ctor", owner_name) == labels
    for owner_name, labels in expected_assignments.items():
        assert family_records(python_records, "assign", owner_name) == labels

    kinds = Counter(line.split("\t")[1] for line in python_records)
    assert kinds == Counter(
        {
            "ctor": 63,
            "assign": 64,
            "storage": 2,
            "storage_after": 2,
            "protected": 2,
            "wide": 4,
            "wide_proxy": 1,
            "lifetime": 2,
            "traits": 1,
            "error": 1,
        }
    )
    assert (
        "SDV\tcompat\tsc_bv_base55\tsc_signed\tSC_DT_DEPRECATED constructor uses length+assignment compatibility path"
        in python_output
    )
    assert (
        "SDV\tcompat\tsc_lv_base54\tsc_int_base\tSC_DT_DEPRECATED constructor uses length+assignment compatibility path"
        in python_output
    )
    compatibility = [line for line in python_output.splitlines() if line.startswith("SDV\tcompat\t")]
    assert len(compatibility) == 16
    assert Counter(line.split("\t")[2] for line in compatibility) == Counter(
        {"sc_bv37": 2, "sc_lv38": 2, "sc_bv_base55": 6, "sc_lv_base54": 6}
    )

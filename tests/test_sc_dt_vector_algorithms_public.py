"""Compare public sc_dt vector algorithms with an independent native model."""

import csv
from collections import Counter
from pathlib import PurePosixPath
import subprocess
import sys

from .support import ROOT, run_example

FAMILY_ROW_COUNTS = {
    "lrotate": 4,
    "rrotate": 4,
    "reverse": 4,
    "lshift": 6,
    "rshift": 6,
    "b_not": 6,
    "and_reduce": 7,
    "nand_reduce": 7,
    "nor_reduce": 7,
    "or_reduce": 7,
    "xnor_reduce": 7,
    "xor_reduce": 7,
    "equal": 7,
    "not_equal": 7,
}


def inventory_family_rows():
    """Read assigned inventory rows with exact source and declaration kinds."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as inventory_file:
        rows = csv.DictReader(inventory_file)
        return [
            (row["name"].removeprefix("sc_dt::"), row["kind"], PurePosixPath(row["source"]).name, row["signature"])
            for row in rows
            if row["name"].startswith("sc_dt::") and row["name"].removeprefix("sc_dt::") in FAMILY_ROW_COUNTS
        ]


def expected_inventory_sources():
    """Return precise overload and template duplicate counts by source file."""
    expected = Counter()
    for family in ("lrotate", "rrotate", "reverse"):
        for source in ("sc_proxy.h", "sc_lv_base.h"):
            expected[(family, "FunctionDecl", source)] += 1
            expected[(family, "FunctionTemplateDecl", source)] += 1
    for family in ("lshift", "rshift"):
        expected[(family, "FunctionDecl", "sc_fxval.h")] += 2
        expected[(family, "FunctionDecl", "sc_fxnum.h")] += 4
    expected[("b_not", "FunctionDecl", "sc_bit.h")] = 2
    expected[("b_not", "FunctionDecl", "sc_fix.h")] = 2
    expected[("b_not", "FunctionDecl", "sc_ufix.h")] = 2
    for family in ("and_reduce", "nand_reduce", "nor_reduce", "or_reduce", "xnor_reduce", "xor_reduce"):
        expected[(family, "FunctionDecl", "sc_proxy.h")] = 1
        expected[(family, "FunctionTemplateDecl", "sc_proxy.h")] = 1
        expected[(family, "FunctionDecl", "sc_concatref.h")] = 1
        expected[(family, "FunctionDecl", "sc_int_base.h")] = 2
        expected[(family, "FunctionDecl", "sc_uint_base.h")] = 2
    expected[("equal", "FunctionDecl", "sc_bit.h")] = 7
    expected[("not_equal", "FunctionDecl", "sc_bit.h")] = 7
    return expected


def expected_inventory_rows():
    """Return every assigned declaration with its exact signature and source."""
    expected = Counter()

    def add(name, kind, source, signature):
        expected[(name, kind, source, signature)] += 1

    rotate_signatures = {
        "lrotate": "const sc_dt::sc_lv_base (const sc_proxy<X> &, int)",
        "rrotate": "const sc_dt::sc_lv_base (const sc_proxy<X> &, int)",
        "reverse": "const sc_dt::sc_lv_base (const sc_proxy<X> &)",
    }
    for name, signature in rotate_signatures.items():
        for source in ("sc_proxy.h", "sc_lv_base.h"):
            add(name, "FunctionDecl", source, signature)
            add(name, "FunctionTemplateDecl", source, "")

    shift_signatures = {
        "sc_fxval.h": (
            "void (sc_dt::sc_fxval &, const sc_dt::sc_fxval &, int)",
            "void (sc_dt::sc_fxval_fast &, const sc_dt::sc_fxval_fast &, int)",
        ),
        "sc_fxnum.h": (
            "void (sc_dt::sc_fxval &, const sc_dt::sc_fxnum &, int)",
            "void (sc_dt::sc_fxnum &, const sc_dt::sc_fxnum &, int)",
            "void (sc_dt::sc_fxval_fast &, const sc_dt::sc_fxnum_fast &, int)",
            "void (sc_dt::sc_fxnum_fast &, const sc_dt::sc_fxnum_fast &, int)",
        ),
    }
    for name in ("lshift", "rshift"):
        for source, signatures in shift_signatures.items():
            for signature in signatures:
                add(name, "FunctionDecl", source, signature)

    for source, signature in (
        ("sc_bit.h", "const sc_dt::sc_bit (const sc_dt::sc_bit &)"),
        ("sc_bit.h", "void (sc_dt::sc_bit &, const sc_dt::sc_bit &)"),
        ("sc_fix.h", "void (sc_dt::sc_fix &, const sc_dt::sc_fix &)"),
        ("sc_fix.h", "void (sc_dt::sc_fix_fast &, const sc_dt::sc_fix_fast &)"),
        ("sc_ufix.h", "void (sc_dt::sc_ufix &, const sc_dt::sc_ufix &)"),
        ("sc_ufix.h", "void (sc_dt::sc_ufix_fast &, const sc_dt::sc_ufix_fast &)"),
    ):
        add("b_not", "FunctionDecl", source, signature)

    reduce_signatures = (
        ("sc_proxy.h", "FunctionDecl", "typename sc_proxy<X>::value_type (const sc_proxy<X> &)"),
        ("sc_proxy.h", "FunctionTemplateDecl", ""),
        ("sc_concatref.h", "FunctionDecl", "bool (const sc_dt::sc_concatref &)"),
        ("sc_int_base.h", "FunctionDecl", "bool (const sc_dt::sc_int_base &)"),
        ("sc_int_base.h", "FunctionDecl", "bool (const sc_dt::sc_int_subref_r &)"),
        ("sc_uint_base.h", "FunctionDecl", "bool (const sc_dt::sc_uint_base &)"),
        ("sc_uint_base.h", "FunctionDecl", "bool (const sc_dt::sc_uint_subref_r &)"),
    )
    for name in ("and_reduce", "nand_reduce", "nor_reduce", "or_reduce", "xnor_reduce", "xor_reduce"):
        for source, kind, signature in reduce_signatures:
            add(name, kind, source, signature)

    comparison_signatures = (
        "bool (const sc_dt::sc_bit &, const sc_dt::sc_bit &)",
        "bool (const sc_dt::sc_bit &, bool)",
        "bool (const sc_dt::sc_bit &, char)",
        "bool (const sc_dt::sc_bit &, int)",
        "bool (bool, const sc_dt::sc_bit &)",
        "bool (char, const sc_dt::sc_bit &)",
        "bool (int, const sc_dt::sc_bit &)",
    )
    for name in ("equal", "not_equal"):
        for signature in comparison_signatures:
            add(name, "FunctionDecl", "sc_bit.h", signature)
    return expected


def test_sc_dt_vector_algorithm_inventory_and_native_parity(tmp_path):
    """Cover all 86 inventory rows and compare public Python results to C++."""
    inventory_rows = inventory_family_rows()
    assert Counter(name for name, _, _, _ in inventory_rows) == Counter(FAMILY_ROW_COUNTS)
    assert len(inventory_rows) == 86
    actual_sources = Counter((name, kind, source) for name, kind, source, _ in inventory_rows)
    assert actual_sources == expected_inventory_sources()
    assert Counter(inventory_rows) == expected_inventory_rows()

    python_output = run_example(tmp_path, "tests/models/sc_dt_vector_algorithms_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_vector_algorithms_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    prefix = "SCA\t"
    python_records = [line for line in python_output.splitlines() if line.startswith(prefix)]
    native_records = [line for line in native.stdout.splitlines() if line.startswith(prefix)]
    assert python_records == native_records
    assert len(python_records) == 189
    assert Counter(line.split("\t")[1] for line in python_records) == Counter(
        {"rotate": 12, "expression": 1, "reduce": 42, "b_not": 6, "shift": 12, "compare": 56, "operator": 60}
    )

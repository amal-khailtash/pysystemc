"""Verify fixed-point part-reference declarations and native behavior parity."""

import csv
from collections import Counter
import subprocess
import sys

from .support import ROOT, run_example


FAMILY_SHAPES = {
    "sc_dt::sc_fxnum_subref_r": "read_only",
    "sc_dt::sc_fxnum_fast_subref_r": "read_only",
    "sc_dt::sc_fxnum_subref": "mutable",
    "sc_dt::sc_fxnum_fast_subref": "mutable",
}
EXPECTED_COUNTS = {
    "sc_dt::sc_fxnum_subref_r": 24,
    "sc_dt::sc_fxnum_fast_subref_r": 24,
    "sc_dt::sc_fxnum_subref": 32,
    "sc_dt::sc_fxnum_fast_subref": 32,
}
SOURCE = "submodules/systemc/src/sysc/datatypes/fx/sc_fxnum.h"


def expected_rows(family, shape):
    """Build exact inventory name/kind/signature rows for one proxy family.

    Args:
        family: Fully qualified native proxy family name.
        shape: ``read_only`` or ``mutable`` declaration shape.

    Returns:
        Counter containing every expected inventory declaration.
    """
    short_name = family.split("::")[-1]
    expected = Counter({(family, "CXXRecordDecl", ""): 1})
    expected[(f"{family}::{short_name}", "CXXConstructorDecl", f"void (const {family} &)")] = 1

    def add(method, kind, signature, count=1):
        """Add one exact method declaration to the expected set."""
        expected[(f"{family}::{method}", kind, signature)] += count

    if shape == "read_only":
        add("~" + short_name, "CXXDestructorDecl", "void () noexcept")
        add("operator sc_bv_base", "CXXConversionDecl", "sc_dt::sc_bv_base () const")
        add("and_reduce", "CXXMethodDecl", "bool () const")
        add("nand_reduce", "CXXMethodDecl", "bool () const")
        add("or_reduce", "CXXMethodDecl", "bool () const")
        add("nor_reduce", "CXXMethodDecl", "bool () const")
        add("xor_reduce", "CXXMethodDecl", "bool () const")
        add("xnor_reduce", "CXXMethodDecl", "bool () const")
        add("dump", "CXXMethodDecl", "void (::std::ostream &) const")
        add("print", "CXXMethodDecl", "void (::std::ostream &) const")
        add("length", "CXXMethodDecl", "int () const")
        for method, signature in (
            ("to_int", "int () const"),
            ("to_uint", "unsigned int () const"),
            ("to_long", "long () const"),
            ("to_ulong", "unsigned long () const"),
            ("to_int64", "sc_dt::int64 () const"),
            ("to_uint64", "sc_dt::uint64 () const"),
            ("to_signed", "int () const"),
            ("to_unsigned", "unsigned int () const"),
            ("to_string", "const std::string () const"),
            ("to_string", "const std::string (sc_dt::sc_numrep) const"),
            ("to_string", "const std::string (sc_dt::sc_numrep, bool) const"),
        ):
            add(method, "CXXMethodDecl", signature)
    else:
        add("scan", "CXXMethodDecl", "void (::std::istream &)")
        for method in ("operator&=", "operator|=", "operator^="):
            for rhs in (
                "const sc_dt::sc_bv_base &",
                "const sc_dt::sc_lv_base &",
                "const sc_dt::sc_fxnum_subref &"
                if short_name == "sc_fxnum_subref"
                else "const sc_dt::sc_fxnum_fast_subref &",
                "const sc_dt::sc_fxnum_fast_subref &"
                if short_name == "sc_fxnum_subref"
                else "const sc_dt::sc_fxnum_subref &",
            ):
                add(method, "CXXMethodDecl", f"{family} &({rhs})")
        for rhs in (
            "char",
            "const bool *",
            "const char *",
            "const sc_dt::sc_bv_base &",
            "const sc_dt::sc_lv_base &",
            "const sc_dt::sc_int_base &",
            "const sc_dt::sc_uint_base &",
            "const sc_dt::sc_signed &",
            "const sc_dt::sc_unsigned &",
            "int",
            "unsigned int",
            "long",
            "unsigned long",
            "sc_dt::int64",
            "sc_dt::uint64",
            "const sc_dt::sc_fxnum_subref &"
            if short_name == "sc_fxnum_subref"
            else "const sc_dt::sc_fxnum_fast_subref &",
            "const sc_dt::sc_fxnum_fast_subref &"
            if short_name == "sc_fxnum_subref"
            else "const sc_dt::sc_fxnum_subref &",
        ):
            add("operator=", "CXXMethodDecl", f"{family} &({rhs})")
    return expected


def read_inventory():
    """Read the four fixed-point proxy declaration groups from the API inventory.

    Returns:
        Mapping from native proxy family name to its inventory rows.
    """
    inventory = ROOT / "docs" / "_static" / "public-api-inventory.csv"
    families = set(EXPECTED_COUNTS)
    rows = {family: [] for family in families}
    with inventory.open(newline="", encoding="utf-8") as source:
        for row in csv.DictReader(source):
            parts = row["name"].split("::")
            if len(parts) > 1 and "::".join(parts[:2]) in families:
                rows["::".join(parts[:2])].append(row)
    return rows


def run_native(tmp_path, *arguments):
    """Run the native model in a bounded independent process.

    Args:
        tmp_path: Pytest temporary directory used as the working directory.
        *arguments: Optional native report mode.

    Returns:
        Completed native process result.
    """
    command = [
        sys.executable,
        str(ROOT / "examples" / "run_sysc_cpp.py"),
        str(ROOT / "tests" / "native" / "sc_dt_fixed_subrefs_public.cpp"),
    ]
    if arguments:
        command.extend(("--", *arguments))
    return subprocess.run(
        command,
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )


def test_sc_dt_fixed_subrefs_match_native_reference(tmp_path):
    """Verify all 112 declarations, native parity and range-conversion adapters."""
    declarations = read_inventory()
    assert {family: len(rows) for family, rows in declarations.items()} == EXPECTED_COUNTS
    assert sum(EXPECTED_COUNTS.values()) == 112
    for family, rows in declarations.items():
        assert {row["source"] for row in rows} == {SOURCE}
        actual = Counter((row["name"], row["kind"], row["signature"]) for row in rows)
        assert actual == expected_rows(family, FAMILY_SHAPES[family])

    python_output = run_example(tmp_path, "tests/models/sc_dt_fixed_subrefs_public.py")
    native = run_native(tmp_path)
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = [line for line in python_output.splitlines() if line.startswith("FXSUB\t")]
    native_records = [line for line in native.stdout.splitlines() if line.startswith("FXSUB\t")]
    behavioral_records = python_records
    if behavioral_records != native_records:
        for index, (python_record, native_record) in enumerate(zip(behavioral_records, native_records)):
            if python_record != native_record:
                raise AssertionError(f"record {index} differs:\nPython: {python_record}\nNative: {native_record}")
        raise AssertionError(f"record counts differ: Python={len(behavioral_records)} native={len(native_records)}")

    python_fields = [record.split("\t") for record in python_records]
    assert len([row for row in python_fields if row[1] == "view"]) == 18
    assert len([row for row in python_fields if row[1] == "assign"]) == 34
    assert len([row for row in python_fields if row[1] == "compound"]) == 24
    assert len([row for row in python_fields if row[1] == "copy"]) == 6
    assert len([row for row in python_fields if row[1] == "access"]) == 12
    signed_compat_rows = [row for row in python_fields if row[1] == "signed_compat"]
    assert len(signed_compat_rows) == 18
    assert all(row[-2:] != ["unavailable", "unavailable"] for row in signed_compat_rows)
    assert [row for row in python_fields if row[1] == "deprecated_api"] == [["FXSUB", "deprecated_api", "available"]]
    assert len([row for row in python_fields if row[1] == "vector_copy"]) == 2
    macro_rows = [row for row in python_fields if row[1] == "deprecated_macro"]
    assert len(macro_rows) == 1 and macro_rows[0][-1] in {"0", "1"}

    for mode in ("negative-fixed", "upper-fast"):
        python_report = run_example(
            tmp_path,
            "tests/models/sc_dt_fixed_subrefs_public.py",
            (mode,),
        )
        native_report = run_native(tmp_path, mode)
        assert native_report.returncode == 0, native_report.stdout + native_report.stderr
        assert [line for line in python_report.splitlines() if line.startswith("FXSUB\t")] == [
            line for line in native_report.stdout.splitlines() if line.startswith("FXSUB\t")
        ]

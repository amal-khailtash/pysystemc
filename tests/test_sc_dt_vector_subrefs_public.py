"""Compare public sc_dt subreference bindings with exact inventory and C++."""

import csv
from collections import Counter
from pathlib import PurePosixPath
import subprocess
import sys

from .support import ROOT, run_example

FAMILY_ROW_COUNTS = {
    "sc_concref_r": 20,
    "sc_concref": 25,
    "sc_subref_r": 22,
    "sc_subref": 30,
}

EXPECTED_TARGET_ROWS = """sc_dt::sc_concref	ClassTemplateDecl	sc_bit_proxies.h\t
sc_dt::sc_concref	ClassTemplateDecl	sc_proxy.h\t
sc_dt::sc_concref	CXXRecordDecl	sc_bit_proxies.h\t
sc_dt::sc_concref	CXXRecordDecl	sc_proxy.h\t
sc_dt::sc_concref::base_type	TypedefDecl	sc_bit_proxies.h	sc_concref_r<X, Y>
sc_dt::sc_concref::clone	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> *() const
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(int)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(const sc_concref<X, Y> &)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(const bool *)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(const sc_dt::sc_int_base &)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(unsigned int)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(sc_dt::int64)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(const sc_dt::sc_uint_base &)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(const sc_dt::sc_logic *)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(sc_dt::uint64)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(const sc_dt::sc_unsigned &)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(const sc_proxy<Z> &)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(long)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(const sc_dt::sc_signed &)
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(unsigned long)
sc_dt::sc_concref::operator=	FunctionTemplateDecl	sc_bit_proxies.h\t
sc_dt::sc_concref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_concref<X, Y> &(const char *)
sc_dt::sc_concref::sc_concref<X, Y>	CXXConstructorDecl	sc_bit_proxies.h	void (const sc_concref<X, Y> &)
sc_dt::sc_concref::sc_concref<X, Y>	CXXConstructorDecl	sc_bit_proxies.h	void (X &, Y &, int)
sc_dt::sc_concref::scan	CXXMethodDecl	sc_bit_proxies.h	void (::std::istream &)
sc_dt::sc_concref_r	CXXRecordDecl	sc_bit_proxies.h\t
sc_dt::sc_concref_r	ClassTemplateDecl	sc_bit_proxies.h\t
sc_dt::sc_concref_r	ClassTemplateDecl	sc_proxy.h\t
sc_dt::sc_concref_r	CXXRecordDecl	sc_proxy.h\t
sc_dt::sc_concref_r::clean_tail	CXXMethodDecl	sc_bit_proxies.h	void ()
sc_dt::sc_concref_r::clone	CXXMethodDecl	sc_bit_proxies.h	sc_concref_r<X, Y> *() const
sc_dt::sc_concref_r::get_bit	CXXMethodDecl	sc_bit_proxies.h	sc_dt::sc_concref_r::value_type (int) const
sc_dt::sc_concref_r::get_bit	CXXMethodDecl	sc_bit_proxies.h	typename sc_concref_r<X, Y>::value_type (int) const
sc_dt::sc_concref_r::get_cword	CXXMethodDecl	sc_bit_proxies.h	sc_dt::sc_digit (int) const
sc_dt::sc_concref_r::get_word	CXXMethodDecl	sc_bit_proxies.h	sc_dt::sc_digit (int) const
sc_dt::sc_concref_r::is_01	CXXMethodDecl	sc_bit_proxies.h	bool () const
sc_dt::sc_concref_r::length	CXXMethodDecl	sc_bit_proxies.h	int () const
sc_dt::sc_concref_r::sc_concref_r<X, Y>	CXXConstructorDecl	sc_bit_proxies.h	void (const sc_concref_r<X, Y> &)
sc_dt::sc_concref_r::sc_concref_r<X, Y>	CXXConstructorDecl	sc_bit_proxies.h	void (const X &, const Y &, int)
sc_dt::sc_concref_r::set_bit	CXXMethodDecl	sc_bit_proxies.h	void (int, sc_dt::sc_concref_r::value_type)
sc_dt::sc_concref_r::set_cword	CXXMethodDecl	sc_bit_proxies.h	void (int, sc_dt::sc_digit)
sc_dt::sc_concref_r::set_word	CXXMethodDecl	sc_bit_proxies.h	void (int, sc_dt::sc_digit)
sc_dt::sc_concref_r::size	CXXMethodDecl	sc_bit_proxies.h	int () const
sc_dt::sc_concref_r::value_type	TypedefDecl	sc_bit_proxies.h	typename sc_proxy<sc_concref_r<X, Y>>::value_type
sc_dt::sc_concref_r::~sc_concref_r<X, Y>	CXXDestructorDecl	sc_bit_proxies.h	void ()
sc_dt::sc_subref	CXXRecordDecl	sc_proxy.h\t
sc_dt::sc_subref	ClassTemplateDecl	sc_bit_proxies.h\t
sc_dt::sc_subref	ClassTemplateDecl	sc_proxy.h\t
sc_dt::sc_subref	ClassTemplateSpecializationDecl	sc_proxy.h\t
sc_dt::sc_subref	ClassTemplateSpecializationDecl	sc_proxy.h\t
sc_dt::sc_subref	CXXRecordDecl	sc_bit_proxies.h\t
sc_dt::sc_subref::base_type	TypedefDecl	sc_bit_proxies.h	sc_subref_r<X>
sc_dt::sc_subref::clone	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> *() const
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(const sc_dt::sc_int_base &)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(int)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(sc_dt::int64)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(const sc_dt::sc_signed &)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_lv_base.h	sc_subref<X> &(const sc_subref_r<X> &)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(const bool *)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_lv_base.h	sc_subref<X> &(const sc_subref<X> &)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(sc_dt::uint64)
sc_dt::sc_subref::operator=	FunctionTemplateDecl	sc_bit_proxies.h\t
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(const sc_proxy<Y> &)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(const sc_dt::sc_logic *)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(const sc_subref_r<X> &)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(const sc_dt::sc_unsigned &)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(long)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(unsigned long)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(const sc_subref<X> &)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(unsigned int)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(const char *)
sc_dt::sc_subref::operator=	CXXMethodDecl	sc_bit_proxies.h	sc_subref<X> &(const sc_dt::sc_uint_base &)
sc_dt::sc_subref::sc_subref<X>	CXXConstructorDecl	sc_bit_proxies.h	void (X &, int, int)
sc_dt::sc_subref::sc_subref<X>	CXXConstructorDecl	sc_bit_proxies.h	void (const sc_subref<X> &)
sc_dt::sc_subref::scan	CXXMethodDecl	sc_bit_proxies.h	void (::std::istream &)
sc_dt::sc_subref_r	CXXRecordDecl	sc_proxy.h\t
sc_dt::sc_subref_r	ClassTemplateSpecializationDecl	sc_proxy.h\t
sc_dt::sc_subref_r	CXXRecordDecl	sc_bit_proxies.h\t
sc_dt::sc_subref_r	ClassTemplateDecl	sc_bit_proxies.h\t
sc_dt::sc_subref_r	ClassTemplateSpecializationDecl	sc_proxy.h\t
sc_dt::sc_subref_r	ClassTemplateDecl	sc_proxy.h\t
sc_dt::sc_subref_r::clean_tail	CXXMethodDecl	sc_bit_proxies.h	void ()
sc_dt::sc_subref_r::clone	CXXMethodDecl	sc_bit_proxies.h	sc_subref_r<X> *() const
sc_dt::sc_subref_r::get_bit	CXXMethodDecl	sc_bit_proxies.h	typename sc_subref_r<X>::value_type (int) const
sc_dt::sc_subref_r::get_bit	CXXMethodDecl	sc_bit_proxies.h	sc_dt::sc_subref_r::value_type (int) const
sc_dt::sc_subref_r::get_cword	CXXMethodDecl	sc_bit_proxies.h	sc_dt::sc_digit (int) const
sc_dt::sc_subref_r::get_word	CXXMethodDecl	sc_bit_proxies.h	sc_dt::sc_digit (int) const
sc_dt::sc_subref_r::is_01	CXXMethodDecl	sc_bit_proxies.h	bool () const
sc_dt::sc_subref_r::length	CXXMethodDecl	sc_bit_proxies.h	int () const
sc_dt::sc_subref_r::reversed	CXXMethodDecl	sc_bit_proxies.h	bool () const
sc_dt::sc_subref_r::sc_subref_r<X>	CXXConstructorDecl	sc_bit_proxies.h	void (const sc_subref_r<X> &)
sc_dt::sc_subref_r::sc_subref_r<X>	CXXConstructorDecl	sc_bit_proxies.h	void (const X &, int, int)
sc_dt::sc_subref_r::set_bit	CXXMethodDecl	sc_bit_proxies.h	void (int, sc_dt::sc_subref_r::value_type)
sc_dt::sc_subref_r::set_cword	CXXMethodDecl	sc_bit_proxies.h	void (int, sc_dt::sc_digit)
sc_dt::sc_subref_r::set_word	CXXMethodDecl	sc_bit_proxies.h	void (int, sc_dt::sc_digit)
sc_dt::sc_subref_r::size	CXXMethodDecl	sc_bit_proxies.h	int () const
sc_dt::sc_subref_r::value_type	TypedefDecl	sc_bit_proxies.h	typename sc_proxy<sc_subref_r<X>>::value_type"""


def inventory_target_rows():
    """Read the four audited proxy families with exact kind, source, signature."""
    family_markers = tuple(FAMILY_ROW_COUNTS)
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as inventory_file:
        rows = csv.DictReader(inventory_file)
        return [
            (
                row["name"],
                row["kind"],
                PurePosixPath(row["source"]).name,
                row["signature"],
            )
            for row in rows
            if row["name"].startswith("sc_dt::") and any(f"::{family}" in row["name"] for family in family_markers)
        ]


def expected_target_rows():
    """Return the frozen exact public inventory declarations under audit."""
    return [tuple(line.split("\t")) for line in EXPECTED_TARGET_ROWS.splitlines() if line]


def family_for_name(name):
    """Select the longest audited family token occurring in a qualified name."""
    return next(family for family in FAMILY_ROW_COUNTS if f"::{family}" in name)


def test_sc_dt_vector_subrefs_inventory_and_native_parity(tmp_path):
    """Cover all 97 proxy inventory rows and compare fresh-process C++ results."""
    actual_inventory = inventory_target_rows()
    expected_inventory = expected_target_rows()
    assert len(actual_inventory) == 97
    assert len(expected_inventory) == 97
    assert Counter(family_for_name(name) for name, _, _, _ in actual_inventory) == Counter(FAMILY_ROW_COUNTS)
    assert Counter(actual_inventory) == Counter(expected_inventory)

    python_output = run_example(tmp_path, "tests/models/sc_dt_vector_subrefs_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_vector_subrefs_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    prefix = "SDR\t"
    python_records = [line for line in python_output.splitlines() if line.startswith(prefix)]
    native_records = [line for line in native.stdout.splitlines() if line.startswith(prefix)]
    assert python_records == native_records
    assert len(python_records) == 153
    assert Counter(line.split("\t")[1] for line in python_records) == Counter(
        {
            "construct": 11,
            "borrowed": 1,
            "range": 8,
            "state": 10,
            "traits": 10,
            "print": 10,
            "convert": 2,
            "error": 1,
            "concat": 6,
            "assign": 2,
            "compound": 6,
            "scan": 3,
            "assignment": 47,
            "storage": 30,
            "clone": 6,
        }
    )
    assignment_labels = Counter(
        (fields[2], fields[3])
        for line in python_records
        if line.startswith("SDR\tassignment\t")
        for fields in (line.split("\t"),)
    )
    common_assignments = {
        "proxy_template",
        "bool_pointer",
        "char_pointer",
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
    assert assignment_labels == Counter(
        {("bv_subref", label): 1 for label in common_assignments | {"subref_readonly_copy", "subref_mutable_copy"}}
        | {("lv_subref", label): 1 for label in common_assignments | {"subref_readonly_copy", "subref_mutable_copy"}}
        | {("concat", label): 1 for label in common_assignments | {"concref_copy"}}
    )
    storage_labels = Counter(
        (fields[2], fields[3], fields[4])
        for line in python_records
        if line.startswith("SDR\tstorage\t")
        for fields in (line.split("\t"),)
    )
    assert storage_labels == Counter(
        {
            (family, kind, operation): 1
            for family, kind in (
                ("sc_subref", "bv"),
                ("sc_subref", "lv"),
                ("sc_subref_r", "bv"),
                ("sc_subref_r", "lv"),
                ("sc_concref", "mixed"),
                ("sc_concref_r", "mixed"),
            )
            for operation in ("get_bit", "set_bit", "set_word", "set_cword", "clean_tail")
        }
    )
    clone_labels = Counter(line.split("\t")[2] for line in python_records if line.startswith("SDR\tclone\t"))
    assert clone_labels == Counter(
        {
            "sc_subref": 2,
            "sc_subref_r": 2,
            "sc_concref": 1,
            "sc_concref_r": 1,
        }
    )

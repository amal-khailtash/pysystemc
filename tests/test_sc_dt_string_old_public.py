"""Audit all public sc_string_old declarations and compare to native behavior."""

import csv
from collections import Counter
import subprocess
import sys

from .support import ROOT


SOURCE = "submodules/systemc/src/sysc/utils/sc_string.h"
STRING = "sc_dt::sc_string_old"


def expected_declarations():
    """Return every legacy string declaration and related public free operator.

    Returns:
        Counter of exact native name, declaration kind, and signature tuples.
    """
    expected = Counter({(STRING, "CXXRecordDecl", ""): 1})

    def add_member(name, kind, signature, count=1):
        """Add one expected class-scope declaration with its exact spelling."""
        expected[(f"{STRING}::{name}", kind, signature)] += count

    for signature in (
        "void (int)",
        "void (const char *)",
        "void (const char *, int)",
        f"void (const {STRING} &)",
    ):
        add_member("sc_string_old", "CXXConstructorDecl", signature)
    add_member("~sc_string_old", "CXXDestructorDecl", "void () noexcept")
    add_member("c_str", "CXXMethodDecl", "const char *() const")
    add_member("operator const char *", "CXXConversionDecl", "const char *() const")
    add_member("cmp", "CXXMethodDecl", f"int (const {STRING} &) const")
    add_member("cmp", "CXXMethodDecl", "int (const char *) const")
    add_member("contains", "CXXMethodDecl", "bool (char) const")
    add_member("fmt", "CXXMethodDecl", f"{STRING} &(const T &)")
    add_member("fmt", "FunctionTemplateDecl", "")
    add_member("fmt", "CXXMethodDecl", f"{STRING} &(const {STRING} &)")
    add_member("insert", "CXXMethodDecl", f"{STRING} &(const {STRING} &, unsigned int)")
    add_member("is_delimiter", "CXXMethodDecl", f"bool (const {STRING} &, unsigned int) const")
    add_member("length", "CXXMethodDecl", "int () const")
    add_member("lowercase", "CXXMethodDecl", f"{STRING} () const")
    add_member("make_str", "CXXMethodDecl", f"{STRING} (long)")
    add_member("pos", "CXXMethodDecl", f"int (const {STRING} &) const")
    add_member("print", "CXXMethodDecl", "void (::std::ostream &) const")
    add_member("remove", "CXXMethodDecl", f"{STRING} &(unsigned int, unsigned int)")
    add_member("set", "CXXMethodDecl", "void (int, char)")
    add_member("substr", "CXXMethodDecl", f"{STRING} (int, int) const")
    add_member("to_string", "CXXMethodDecl", f"{STRING} (const char *, ...)")
    add_member("uppercase", "CXXMethodDecl", f"{STRING} () const")
    for operator in ("==", "!=", "<", "<=", ">", ">="):
        add_member(f"operator{operator}", "CXXMethodDecl", "bool (const char *) const")
        add_member(f"operator{operator}", "CXXMethodDecl", f"bool (const {STRING} &) const")
    add_member("operator+", "CXXMethodDecl", f"{STRING} (const char *) const")
    add_member("operator+", "CXXMethodDecl", f"{STRING} (char) const")
    add_member("operator+", "CXXMethodDecl", f"{STRING} (const {STRING} &) const")
    add_member("operator+=", "CXXMethodDecl", f"{STRING} &(const char *)")
    add_member("operator+=", "CXXMethodDecl", f"{STRING} &(char)")
    add_member("operator+=", "CXXMethodDecl", f"{STRING} &(const {STRING} &)")
    add_member("operator=", "CXXMethodDecl", f"{STRING} &(const char *)")
    add_member("operator=", "CXXMethodDecl", f"{STRING} &(const {STRING} &)")
    add_member("operator[]", "CXXMethodDecl", "char (int) const")
    add_member("operator[]", "CXXMethodDecl", "char &(int)")

    expected[("sc_dt::operator+", "FunctionDecl", f"{STRING} (const char *, const {STRING} &)")] = 1
    expected[("sc_dt::operator<<", "FunctionDecl", f"::std::ostream &(::std::ostream &, const {STRING} &)")] = 1
    expected[("sc_dt::operator>>", "FunctionDecl", f"::std::istream &(::std::istream &, {STRING} &)")] = 1
    return expected


def read_declarations():
    """Read all class and related free-function rows from the public inventory.

    Returns:
        List of inventory rows scoped to sc_string_old.
    """
    inventory = ROOT / "docs" / "_static" / "public-api-inventory.csv"
    rows = []
    with inventory.open(newline="", encoding="utf-8") as source:
        for row in csv.DictReader(source):
            if row["name"] == STRING or row["name"].startswith(f"{STRING}::"):
                rows.append(row)
            elif (
                row["name"] in {"sc_dt::operator+", "sc_dt::operator<<", "sc_dt::operator>>"}
                and STRING in row["signature"]
            ):
                rows.append(row)
    return rows


def run_native(tmp_path, *arguments):
    """Run the independent native reference in a bounded subprocess.

    Args:
        tmp_path: Isolated per-test working directory.
        *arguments: Optional native report mode.

    Returns:
        Native process result with captured output and errors.
    """
    command = [
        sys.executable,
        str(ROOT / "examples" / "run_sysc_cpp.py"),
        str(ROOT / "tests" / "native" / "sc_dt_string_old_public.cpp"),
    ]
    if arguments:
        command.extend(("--", *arguments))
    return subprocess.run(command, cwd=tmp_path, capture_output=True, text=True, timeout=60)


def test_sc_string_old_exact_inventory_and_native_parity(tmp_path):
    """Verify all 48 class rows, all 3 friend operators and public semantics."""
    rows = read_declarations()
    actual = Counter((row["name"], row["kind"], row["signature"]) for row in rows)
    assert actual == expected_declarations()
    assert len([row for row in rows if row["name"] == STRING or row["name"].startswith(f"{STRING}::")]) == 48
    assert len(rows) == 51
    assert {row["source"] for row in rows} == {SOURCE}

    python_result = subprocess.run(
        [sys.executable, str(ROOT / "tests" / "models" / "sc_dt_string_old_public.py")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert python_result.returncode == 0, python_result.stdout + python_result.stderr
    native = run_native(tmp_path)
    assert native.returncode == 0, native.stdout + native.stderr
    python_rows = [line for line in python_result.stdout.splitlines() if line.startswith("SO\t")]
    native_rows = [line for line in native.stdout.splitlines() if line.startswith("SO\t")]
    expected_operations = Counter(
        {
            "type": 1,
            "construct": 5,
            "visibility": 1,
            "cow": 2,
            "assign": 2,
            "append": 3,
            "concat": 4,
            "compare": 12,
            "cmp": 2,
            "index": 4,
            "substr": 4,
            "insert": 2,
            "remove": 1,
            "search": 1,
            "length": 1,
            "conversion": 1,
            "case": 1,
            "make_str": 1,
            "fmt": 4,
            "format_varargs": 1,
            "format_dynamic": 1,
            "format_truncated": 1,
            "print": 1,
            "ostream": 1,
            "istream": 1,
            "lifetime": 1,
        }
    )
    assert len(python_rows) == len(native_rows) == sum(expected_operations.values()) == 59
    assert Counter(row.split("\t", 2)[1] for row in python_rows) == expected_operations
    behavioral = [line for line in python_rows if "\tbinding_gap\t" not in line]
    if behavioral != native_rows:
        for index, (py_row, native_row) in enumerate(zip(behavioral, native_rows)):
            if py_row != native_row:
                raise AssertionError(f"record {index} differs:\nPython: {py_row}\nNative: {native_row}")
        raise AssertionError(f"record counts differ: Python={len(behavioral)} native={len(native_rows)}")

    gaps = [line.split("\t") for line in python_rows if "\tbinding_gap\t" in line]
    assert gaps == []
    assert "(W801) string is too long: truncated" in python_result.stdout + python_result.stderr
    assert "(W801) string is too long: truncated" in native.stdout + native.stderr

    for mode in ("insert-upper", "remove-negative", "delimiter-upper"):
        python_report = subprocess.run(
            [sys.executable, str(ROOT / "tests" / "models" / "sc_dt_string_old_public.py"), mode],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        native_report = run_native(tmp_path, mode)
        assert python_report.returncode == 0, python_report.stdout + python_report.stderr
        assert native_report.returncode == 0, native_report.stdout + native_report.stderr
        python_report_rows = [line for line in python_report.stdout.splitlines() if line.startswith("SO\t")]
        native_report_rows = [line for line in native_report.stdout.splitlines() if line.startswith("SO\t")]
        assert len(python_report_rows) == len(native_report_rows) == 1
        assert python_report_rows[0].split("\t")[:3] == ["SO", "report", mode]
        assert all(python_report_rows[0].split("\t")[3:])
        assert python_report_rows == native_report_rows

"""Audit 1,412 configurable fixed-point owner declarations with native parity."""

from collections import Counter
import csv
import hashlib
import subprocess
import sys

import pytest

from .support import ROOT, run_example

FAMILIES = ("sc_fix", "sc_ufix", "sc_fix_fast", "sc_ufix_fast")
INVENTORY_SHA256 = "a7f922345f025b0840b148c6cb20ed863a1a39d5a9aebd1647e0bcfcca667eb4"
SOURCE_SIGNATURES = {
    "int": "int",
    "unsigned_int": "unsigned int",
    "long": "long",
    "unsigned_long": "unsigned long",
    "float": "float",
    "double": "double",
    "text": "const char*",
    "fxval": "const sc_dt::sc_fxval&",
    "fxval_fast": "const sc_dt::sc_fxval_fast&",
    "fxnum": "const sc_dt::sc_fxnum&",
    "fxnum_fast": "const sc_dt::sc_fxnum_fast&",
    "int64": "sc_dt::int64",
    "uint64": "sc_dt::uint64",
    "int_base": "const sc_dt::sc_int_base&",
    "uint_base": "const sc_dt::sc_uint_base&",
    "signed": "const sc_dt::sc_signed&",
    "unsigned": "const sc_dt::sc_unsigned&",
}
SHAPES = {
    "observer": (),
    "width": ("int", "int"),
    "modes": ("sc_dt::sc_q_mode", "sc_dt::sc_o_mode"),
    "modes_n": ("sc_dt::sc_q_mode", "sc_dt::sc_o_mode", "int"),
    "width_modes": ("int", "int", "sc_dt::sc_q_mode", "sc_dt::sc_o_mode"),
    "width_modes_n": ("int", "int", "sc_dt::sc_q_mode", "sc_dt::sc_o_mode", "int"),
    "cast": ("const sc_dt::sc_fxcast_switch&",),
    "width_cast": ("int", "int", "const sc_dt::sc_fxcast_switch&"),
    "modes_cast": ("sc_dt::sc_q_mode", "sc_dt::sc_o_mode", "const sc_dt::sc_fxcast_switch&"),
    "modes_n_cast": ("sc_dt::sc_q_mode", "sc_dt::sc_o_mode", "int", "const sc_dt::sc_fxcast_switch&"),
    "width_modes_cast": ("int", "int", "sc_dt::sc_q_mode", "sc_dt::sc_o_mode", "const sc_dt::sc_fxcast_switch&"),
    "width_modes_n_cast": (
        "int",
        "int",
        "sc_dt::sc_q_mode",
        "sc_dt::sc_o_mode",
        "int",
        "const sc_dt::sc_fxcast_switch&",
    ),
    "params": ("const sc_dt::sc_fxtype_params&",),
    "params_cast": ("const sc_dt::sc_fxtype_params&", "const sc_dt::sc_fxcast_switch&"),
}
POLICIES = {"wide137", "wide191", "round_saturate", "truncate_wrap", "negative_iwl", "integer_iwl"}


def inventory_rows():
    """Return frozen names, declaration kinds, sources and exact signatures."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="") as source:
        return [
            (row["name"], row["kind"], row["source"], row["signature"])
            for row in csv.DictReader(source)
            if row["name"].removeprefix("sc_dt::").split("::", 1)[0] in FAMILIES
        ]


def constructor_signatures(family):
    """Derive the complete 253-signature schema independently of CSV row order."""
    observer = "sc_dt::sc_fxnum_fast_observer*" if family.endswith("fast") else "sc_dt::sc_fxnum_observer*"
    signatures = {f"void ({', '.join((*shape, observer))})" for shape in SHAPES.values()}
    signatures.update(
        f"void ({', '.join((source, *shape, observer))})"
        for source in SOURCE_SIGNATURES.values()
        for shape in SHAPES.values()
    )
    signatures.add(f"void (const sc_dt::{family}&)")
    return {signature.replace(" ", "") for signature in signatures}


def records(output):
    """Select deterministic constructor, mutation and ownership observations."""
    return [line for line in output.splitlines() if line.startswith("FXO\t")]


def test_sc_dt_fix_owner_inventory():
    """Prove all 1,412 declarations and every constructor suffix/source are present."""
    rows = inventory_rows()
    assert len(rows) == 1412
    assert Counter(name.removeprefix("sc_dt::").split("::", 1)[0] for name, _, _, _ in rows) == Counter(
        dict.fromkeys(FAMILIES, 353)
    )
    canonical = "".join("\t".join(row) + "\n" for row in rows)
    assert hashlib.sha256(canonical.encode()).hexdigest() == INVENTORY_SHA256
    for family in FAMILIES:
        actual = {
            signature.replace(" ", "")
            for name, kind, _, signature in rows
            if name == f"sc_dt::{family}::{family}" and kind == "CXXConstructorDecl"
        }
        assert len(actual) == 253
        assert actual == constructor_signatures(family)


@pytest.mark.parametrize("family", FAMILIES)
@pytest.mark.parametrize("mode", (0, 1, 2), ids=("constructors_a", "constructors_b", "mutations"))
def test_sc_dt_fix_owner_native_parity(tmp_path, family, mode):
    """Compare one isolated family/mode with exact native types and semantic values."""
    python_output = run_example(tmp_path, "tests/models/sc_dt_fix_owners_public.py", (family, str(mode)))
    selector = tmp_path / "fix_owners_select.cpp"
    selector.write_text(
        f"/** @file Select one deterministic native owner/mode matrix. */\n#define FIX_OWNERS_FAMILY {FAMILIES.index(family)}\n#define FIX_OWNERS_MODE {mode}\n",
        encoding="utf-8",
    )
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(selector),
            str(ROOT / "tests/native/sc_dt_fix_owners_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records, native_records = records(python_output), records(native.stdout)
    mismatches = [
        (python, reference) for python, reference in zip(python_records, native_records) if python != reference
    ]
    assert len(python_records) == len(native_records)
    assert not mismatches, mismatches[:8]
    split = [line.split("\t") for line in python_records]
    assert {row[2] for row in split} == {family}
    if mode < 2:
        selected_sources = tuple(SOURCE_SIGNATURES)[:9] if mode == 0 else tuple(SOURCE_SIGNATURES)[9:]
        labels = {source + "/" + shape for source in selected_sources for shape in SHAPES}
        if mode == 0:
            labels |= {"none/" + shape for shape in SHAPES} | {"copy/copy"}
        for operation in ("ctor", "ctor_public"):
            assert {row[3] for row in split if row[1] == operation} == labels
        assert len(python_records) == (285 if mode == 0 else 227)
        assert f"FXO\tctor_lifetime\t{family}\tnumbers_gone\t1" in python_records
        assert f"FXO\tctor_lifetime\t{family}\tobserver_gone\t1" in python_records
    else:
        assert len(python_records) == 134
        assert Counter(row[1] for row in split) == Counter(
            {
                "traits": 1,
                "observer_binding": 17,
                "assign": 18,
                "add": 17,
                "subtract": 17,
                "multiply": 17,
                "divide": 17,
                "and": 2,
                "or": 2,
                "xor": 2,
                "left": 1,
                "right": 1,
                "preinc": 1,
                "postinc": 1,
                "predec": 1,
                "postdec": 1,
                "complement": 1,
                "observer_lifetime": 2,
                "lifetime": 1,
                "error": 1,
                "policy_ctor": 6,
                "policy_assign": 6,
                "python_integer": 1,
            }
        )
        for operation in ("observer_binding", "add", "subtract", "multiply", "divide"):
            assert {row[3] for row in split if row[1] == operation} == set(SOURCE_SIGNATURES)
        assert {row[3] for row in split if row[1] == "assign"} == set(SOURCE_SIGNATURES) | {"copy"}
        assert {row[3] for row in split if row[1] == "policy_ctor"} == POLICIES
        assert f"FXO\terror\t{family}\ttext\t1\t1" in python_records
        assert f"FXO\tobserver_lifetime\t{family}\tnumbers_gone\t1" in python_records
        assert f"FXO\tobserver_lifetime\t{family}\tobserver_gone\t1" in python_records
    assert f"SC_DT_FIX_OWNERS_PUBLIC_OK {family} {mode}" in python_output

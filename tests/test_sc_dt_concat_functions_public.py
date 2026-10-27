"""Audit public sc_dt concat/comma declarations and live vector results."""

from collections import Counter
import csv
import hashlib
import re
import subprocess
import sys

from .support import ROOT, run_example


FAMILIES = {
    "sc_dt::concat": {
        "digest": "02894f5d8ca8db58c155f4b83c1afb86d3ce5f4bb044a6c892a02b2e6bf89b0b",
        "parents": {
            ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "281"),
            ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "291"),
            ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "301"),
            ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "1285"),
            ("submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h", "870"),
            ("submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h", "1379"),
        },
    },
    "sc_dt::operator,": {
        "digest": "a76f81168a7e2575e240aca8eafd21491c3eb4326992b76423bbe01f43c6c507",
        "parents": {
            ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "230"),
            ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "240"),
            ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "250"),
            ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "1234"),
            ("submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h", "815"),
            ("submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h", "1324"),
        },
    },
}
EXPECTED_TEMPLATE_CHILDREN = {
    ("sc_dt::concat", "sc_bit_proxies.h", "281"): {
        "sc_concref_r<sc_bitref_r<T1>, sc_bitref_r<T2>> (sc_bitref_r<T1>, sc_bitref_r<T2>)",
    },
    ("sc_dt::concat", "sc_bit_proxies.h", "291"): {
        "sc_concref_r<sc_bitref_r<T1>, sc_concref_r<T2, T3>> (sc_bitref_r<T1>, sc_concref_r<T2, T3>)",
    },
    ("sc_dt::concat", "sc_bit_proxies.h", "301"): {
        "sc_concref_r<sc_bitref_r<T>, sc_dt::sc_lv_base> (sc_bitref_r<T>, const char *)",
    },
    ("sc_dt::concat", "sc_bit_proxies.h", "1285"): {
        "sc_concref_r<sc_concref_r<T1, T2>, sc_concref_r<T3, T4>> (sc_concref_r<T1, T2>, sc_concref_r<T3, T4>)",
    },
    ("sc_dt::concat", "sc_lv_base.h", "870"): {
        "sc_concref_r<sc_bitref_r<T>, sc_dt::sc_lv_base> (sc_bitref_r<T>, const char *)",
    },
    ("sc_dt::concat", "sc_lv_base.h", "1379"): {
        "sc_concref_r<sc_concref_r<T1, T2>, sc_dt::sc_lv_base> (sc_concref_r<T1, T2>, const char *)",
    },
    ("sc_dt::operator,", "sc_bit_proxies.h", "230"): {
        "sc_concref_r<sc_bitref_r<T1>, sc_bitref_r<T2>> (sc_bitref_r<T1>, sc_bitref_r<T2>)",
    },
    ("sc_dt::operator,", "sc_bit_proxies.h", "240"): {
        "sc_concref_r<sc_bitref_r<T1>, sc_concref_r<T2, T3>> (sc_bitref_r<T1>, sc_concref_r<T2, T3>)",
    },
    ("sc_dt::operator,", "sc_bit_proxies.h", "250"): {
        "sc_concref_r<sc_bitref_r<T>, sc_dt::sc_lv_base> (sc_bitref_r<T>, const char *)",
    },
    ("sc_dt::operator,", "sc_bit_proxies.h", "1234"): {
        "sc_concref_r<sc_concref_r<T1, T2>, sc_concref_r<T3, T4>> (sc_concref_r<T1, T2>, sc_concref_r<T3, T4>)",
    },
    ("sc_dt::operator,", "sc_lv_base.h", "815"): {
        "sc_concref_r<sc_bitref_r<T>, sc_dt::sc_lv_base> (sc_bitref_r<T>, const char *)",
    },
    ("sc_dt::operator,", "sc_lv_base.h", "1324"): {
        "sc_concref_r<sc_concref_r<T1, T2>, sc_dt::sc_lv_base> (sc_concref_r<T1, T2>, const char *)",
    },
}
EXPECTED_RECORDS = {
    "mutable",
    "mutable_write",
    "comma_mutable",
    "readonly",
    "comma_readonly",
    "mutable_parts",
    "mutable_parts_write",
    "readonly_parts",
    "nested",
    "nested_readonly",
    "string_leaf",
    "logic_leaf",
    "bool_leaf",
    "bool_head",
    "custom",
    "integer_concat",
    "integer_comma",
    "integer_bool",
    "parent_lifetime",
}
EXPECTED_TEMPLATE_PARENT_RECORDS = {
    "parent_bit_pair",
    "parent_bit_concat",
    "parent_bit_text_bv",
    "parent_bit_text_lv",
    "parent_nested_pair",
    "parent_concat_text",
}


def inventory_rows():
    """Load the exact two free-function families from the generated inventory."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="", encoding="utf-8") as source:
        return [row for row in csv.DictReader(source) if row["name"] in FAMILIES]


def canonical_digest(rows):
    """Hash declaration identity fields in their generated inventory order."""
    fields = ("name", "kind", "signature", "source", "line")
    canonical = "".join("\t".join(row[field] for field in fields) + "\n" for row in rows)
    return hashlib.sha256(canonical.encode()).hexdigest()


def normalized(signature):
    """Collapse formatting whitespace while preserving all C++ type tokens."""
    return re.sub(r"\s+", " ", signature).strip()


def records(output):
    """Select model observations without diagnostic or banner output."""
    return [line for line in output.splitlines() if line.startswith("SCF\t")]


def test_sc_dt_concat_and_comma_inventory_is_exact():
    """Freeze all 340 rows, 240 normalized declarations and six template parents."""
    rows = inventory_rows()
    assert len(rows) == 340
    for family, expected in FAMILIES.items():
        family_rows = [row for row in rows if row["name"] == family]
        assert len(family_rows) == 170
        assert Counter(row["kind"] for row in family_rows) == Counter({"FunctionDecl": 164, "FunctionTemplateDecl": 6})
        assert canonical_digest(family_rows) == expected["digest"]
        declarations = [row for row in family_rows if row["kind"] == "FunctionDecl"]
        assert len({normalized(row["signature"]) for row in declarations}) == 120
        parents = {(row["source"], row["line"]) for row in family_rows if row["kind"] == "FunctionTemplateDecl"}
        assert parents == expected["parents"]
        for source, line in parents:
            child_signatures = {
                row["signature"]
                for row in family_rows
                if row["kind"] == "FunctionDecl" and row["source"] == source and row["line"] == line
            }
            assert child_signatures == EXPECTED_TEMPLATE_CHILDREN[(family, source.rsplit("/", 1)[-1], line)]


def test_sc_dt_concat_and_comma_match_fresh_native_processes(tmp_path):
    """Compare public dt.concat calls with native concat and comma results."""
    python_output = run_example(tmp_path, "tests/models/sc_dt_concat_functions_public.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_concat_functions_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert Counter(records(python_output)) == Counter(records(native.stdout))
    parsed = [record.split("\t") for record in records(python_output)]
    expected_labels = set(EXPECTED_RECORDS)
    expected_labels.update(f"concat_{index:03d}" for index in range(120))
    expected_labels.update(f"comma_{index:03d}" for index in range(120))
    for label in EXPECTED_TEMPLATE_PARENT_RECORDS:
        expected_labels.add(label)
        expected_labels.add(label.replace("parent_", "comma_parent_", 1))
    assert len(parsed) == 271
    assert Counter(row[1] for row in parsed) == Counter({label: 1 for label in expected_labels})

    results = {row[1]: row[2:] for row in parsed}
    assert results["mutable"][0] == "328"
    assert results["readonly"][0] == "328"
    assert results["mutable_parts"][0] == results["readonly_parts"][0] == "14"
    assert results["nested"][0] == results["nested_readonly"][0] == "329"
    assert results["string_leaf"][0] == "143"
    assert results["bool_leaf"][0] == "138"
    assert results["logic_leaf"][0] == "192"
    assert results["custom"][0] == "328"
    assert results["integer_concat"][0] == results["integer_comma"][0] == "328"

    # Python comma syntax creates a tuple. The model calls dt.concat for those
    # result-family comparisons, while this independent C++ phase calls the
    # native operator, comma overloads directly.
    # The comma expression runs after writable was assigned, so it observes
    # the updated backing vectors rather than the initial mutable record.
    assert results["comma_mutable"][1] == results["mutable_write"][2]
    assert results["comma_readonly"] == results["readonly"]
    assert results["integer_concat"] == results["integer_comma"]

    # Exact overload selectors retain the writable and read-only return types;
    # cppyy's pretty-printed type name is compared with the native demangled
    # type name in every matrix row above.
    assert results["concat_003"][2] == "sc_dt::sc_concref<sc_dt::sc_bv_base,sc_dt::sc_lv_base>"
    assert results["concat_025"][2] == "sc_dt::sc_concref_r<sc_dt::sc_bv_base,sc_dt::sc_lv_base>"

"""Audit the public arbitrary-width and four-state binary operators."""

from collections import Counter
import csv
import hashlib
import re
import subprocess
import sys

from .support import ROOT, run_example


INVENTORY_DIGESTS = {
    "sc_dt::operator%": "7a9cb23ef293a8a2e06e14387f7748a909ba212301f55dc699d90283cfb17b8e",
    "sc_dt::operator&": "b23161c148f07a6fd67771d35a8f4b672b15c56da769e911294db5866b5fcea5",
    "sc_dt::operator|": "ee2eeb09b13863f68c44422c1599e49e7913120135a9fc64131cad93de1dd9d7",
    "sc_dt::operator^": "cba1473c25ba954ca2d6350c3636e178ae3658d7d023e5571050aeb953c61ce5",
}
EXPECTED_ROWS = {"sc_dt::operator%": 69, "sc_dt::operator&": 115, "sc_dt::operator|": 115, "sc_dt::operator^": 115}
EXPECTED_KINDS = {
    "sc_dt::operator%": Counter({"FunctionDecl": 69}),
    "sc_dt::operator&": Counter({"FunctionDecl": 111, "FunctionTemplateDecl": 4}),
    "sc_dt::operator|": Counter({"FunctionDecl": 111, "FunctionTemplateDecl": 4}),
    "sc_dt::operator^": Counter({"FunctionDecl": 111, "FunctionTemplateDecl": 4}),
}
TEMPLATE_PARENT_CHILDREN = {
    "sc_dt::operator&": {
        ("submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h", "418"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "193"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h", "375"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_proxy.h", "552"),
    },
    "sc_dt::operator|": {
        ("submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h", "477"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "201"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h", "520"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_proxy.h", "589"),
    },
    "sc_dt::operator^": {
        ("submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h", "579"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "209"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h", "635"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_proxy.h", "626"),
    },
}
TEMPLATE_CHILD_SIGNATURES = {
    ("sc_dt::operator&", "sc_lv_base.h", "418"): {
        "const sc_dt::sc_lv_base (const char *, const sc_proxy<X> &)",
    },
    ("sc_dt::operator&", "sc_bit_proxies.h", "193"): {
        "sc_dt::sc_logic (const sc_bitref_r<T1> &, const sc_bitref_r<T2> &)",
    },
    ("sc_dt::operator&", "sc_lv_base.h", "375"): {
        "const sc_dt::sc_lv_base (const sc_proxy<X> &, const sc_proxy<Y> &)",
        "const sc_dt::sc_lv_base (const sc_proxy<sc_dt::sc_bv_base> &, const sc_proxy<sc_dt::sc_bv_base> &)",
        "const sc_dt::sc_lv_base (const sc_proxy<sc_dt::sc_bv_base> &, const sc_proxy<sc_dt::sc_lv_base> &)",
    },
    ("sc_dt::operator&", "sc_proxy.h", "552"): {
        "const sc_dt::sc_lv_base (const sc_proxy<X> &, const sc_proxy<Y> &)",
    },
    ("sc_dt::operator|", "sc_lv_base.h", "477"): {
        "const sc_dt::sc_lv_base (const sc_proxy<X> &, const sc_proxy<Y> &)",
        "const sc_dt::sc_lv_base (const sc_proxy<sc_dt::sc_bv_base> &, const sc_proxy<sc_dt::sc_bv_base> &)",
        "const sc_dt::sc_lv_base (const sc_proxy<sc_dt::sc_bv_base> &, const sc_proxy<sc_dt::sc_lv_base> &)",
    },
    ("sc_dt::operator|", "sc_bit_proxies.h", "201"): {
        "sc_dt::sc_logic (const sc_bitref_r<T1> &, const sc_bitref_r<T2> &)",
    },
    ("sc_dt::operator|", "sc_lv_base.h", "520"): {
        "const sc_dt::sc_lv_base (const char *, const sc_proxy<X> &)",
    },
    ("sc_dt::operator|", "sc_proxy.h", "589"): {
        "const sc_dt::sc_lv_base (const sc_proxy<X> &, const sc_proxy<Y> &)",
    },
    ("sc_dt::operator^", "sc_lv_base.h", "579"): {
        "const sc_dt::sc_lv_base (const sc_proxy<X> &, const sc_proxy<Y> &)",
        "const sc_dt::sc_lv_base (const sc_proxy<sc_dt::sc_bv_base> &, const sc_proxy<sc_dt::sc_bv_base> &)",
        "const sc_dt::sc_lv_base (const sc_proxy<sc_dt::sc_bv_base> &, const sc_proxy<sc_dt::sc_lv_base> &)",
    },
    ("sc_dt::operator^", "sc_bit_proxies.h", "209"): {
        "sc_dt::sc_logic (const sc_bitref_r<T1> &, const sc_bitref_r<T2> &)",
    },
    ("sc_dt::operator^", "sc_lv_base.h", "635"): {
        "const sc_dt::sc_lv_base (const char *, const sc_proxy<X> &)",
    },
    ("sc_dt::operator^", "sc_proxy.h", "626"): {
        "const sc_dt::sc_lv_base (const sc_proxy<X> &, const sc_proxy<Y> &)",
    },
}


def inventory_rows():
    """Load the generated public inventory without widening family matches."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


def stable_digest(rows):
    """Hash declaration identity fields while ignoring mutable review status."""
    canonical = "".join(
        "\t".join(row[field] for field in ("name", "kind", "signature", "source", "line")) + "\n" for row in rows
    )
    return hashlib.sha256(canonical.encode()).hexdigest()


def normalized_signature(row):
    """Return a stable signature key, retaining all concrete argument types."""
    return re.sub(r"\s+", " ", row["signature"]).strip()


def concrete_probe_signature(signature):
    """Resolve dependent aliases to the concrete types used by C++ probes."""
    concrete = (
        signature.replace("sc_proxy<X>", "sc_dt::sc_proxy<sc_dt::sc_bv_base>")
        .replace("sc_proxy<Y>", "sc_dt::sc_proxy<sc_dt::sc_lv_base>")
        .replace("sc_bitref_r<T1>", "sc_dt::sc_bitref_r<sc_dt::sc_bv_base>")
        .replace("sc_bitref_r<T2>", "sc_dt::sc_bitref_r<sc_dt::sc_lv_base>")
    )
    return re.sub(r"(?<!sc_dt::)sc_proxy<", "sc_dt::sc_proxy<", concrete)


def test_integer_binary_operator_inventory_is_exact_and_probeable():
    """Retain duplicate source identities and every distinct native signature."""
    rows = inventory_rows()
    probe_text = (ROOT / "tests/native/sc_dt_integer_binary_probe.h").read_text(encoding="utf-8")
    compact_probe = re.sub(r"[\s\"]+", "", probe_text)
    for family, digest in INVENTORY_DIGESTS.items():
        family_rows = [row for row in rows if row["name"] == family]
        assert len(family_rows) == EXPECTED_ROWS[family]
        assert Counter(row["kind"] for row in family_rows) == EXPECTED_KINDS[family]
        assert stable_digest(family_rows) == digest
        if family == "sc_dt::operator%":
            unique_signatures = {normalized_signature(row) for row in family_rows}
            assert len(unique_signatures) == 59
        else:
            declarations = [row for row in family_rows if row["kind"] == "FunctionDecl"]
            unique_signatures = {normalized_signature(row) for row in declarations}
            assert len(unique_signatures) == 100
        for signature in unique_signatures:
            needle = f"exactoverload{family}{concrete_probe_signature(signature)}"
            assert re.sub(r"\s+", "", needle) in compact_probe, (family, signature)

    for family, parents in TEMPLATE_PARENT_CHILDREN.items():
        family_rows = [row for row in rows if row["name"] == family]
        actual_parents = {(row["source"], row["line"]) for row in family_rows if row["kind"] == "FunctionTemplateDecl"}
        assert actual_parents == parents
        for source, line in parents:
            # The template parent is an AST wrapper. Its same-location function
            # declaration is kept as the exact signature identity to probe.
            children = [
                row
                for row in family_rows
                if row["kind"] == "FunctionDecl" and row["source"] == source and row["line"] == line
            ]
            expected_signatures = TEMPLATE_CHILD_SIGNATURES[(family, source.rsplit("/", 1)[-1], line)]
            assert {row["signature"] for row in children} == expected_signatures, (family, source, line)
            assert all("sc_proxy<" in signature or "sc_bitref_r<" in signature for signature in expected_signatures)


def test_integer_binary_operators_match_independent_native_phases(tmp_path):
    """Compare each typed Python case with its exact native function-pointer call."""

    def run_native(phase):
        """Run one independent native phase and require a clean result."""
        native = subprocess.run(
            [
                sys.executable,
                str(ROOT / "examples/run_sysc_cpp.py"),
                str(ROOT / "tests/native/sc_dt_integer_binary_public.cpp"),
                "--",
                phase,
            ],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert native.returncode == 0, native.stdout + native.stderr
        assert "SC_DT_INTEGER_BINARY_PUBLIC_OK" in native.stdout
        assert "BIN\tprobe_count\t359" in native.stdout
        return native.stdout

    representative_labels = {
        "integer": {
            "remainder_signed_signed",
            "remainder_signed_int",
            "remainder_int_signed",
            "remainder_unsigned_fixed",
            "and_signed_unsigned",
            "or_unsigned_signed",
            "xor_fixed_signed",
            "and_int_unsigned",
            "or_unsigned_int",
            "xor_int_signed",
            "remainder_integer_concat",
            "and_integer_concat",
            "or_concat_integer",
            "xor_concat_unsigned",
            "lifetime_owned_result",
        },
        "vector": {
            "and_bv_lv",
            "or_lv_bv",
            "xor_bv_int",
            "or_int_bv",
            "xor_bitrefs",
            "lifetime_owned_vector",
            "and_bool_pointer_left",
            "and_bool_pointer_right",
            "or_char_pointer_left",
            "or_char_pointer_right",
            "xor_logic_pointer_left",
            "xor_logic_pointer_right",
            "and_logic_bool",
            "or_logic_char",
            "xor_bit_int",
        },
    }
    for phase, labels in representative_labels.items():
        python_output = run_example(tmp_path, "tests/models/sc_dt_integer_binary_public.py", (phase,))
        native_output = run_native(phase)
        python_records = {
            tuple(line.split("\t", 2)[1:]): line for line in python_output.splitlines() if line.startswith("BIN\t")
        }
        native_records = {
            tuple(line.split("\t", 2)[1:]): line for line in native_output.splitlines() if line.startswith("BIN\t")
        }
        assert {label for label, _value in python_records} == labels | {"probe_count"}
        assert python_records == native_records, phase
        assert python_records[("probe_count", "359")] == "BIN\tprobe_count\t359"
        assert "SC_DT_INTEGER_BINARY_PUBLIC_OK" in python_output

    for phase, family in {
        "mod": "sc_dt::operator%",
        "and": "sc_dt::operator&",
        "or": "sc_dt::operator|",
        "xor": "sc_dt::operator^",
    }.items():
        signatures = sorted(
            {row["signature"] for row in inventory_rows() if row["name"] == family and row["kind"] == "FunctionDecl"}
        )
        python_output = run_example(tmp_path, "tests/models/sc_dt_integer_binary_public.py", (phase,))
        native_output = run_native(phase)
        python_records = {}
        for line in python_output.splitlines():
            if not line.startswith("MATRIX\t"):
                continue
            fields = line.split("\t", 5)
            case_index = int(fields[2])
            assert 0 <= case_index < len(signatures), line
            assert fields[1] == phase, line
            assert fields[3] == "PASS", f"{family} {signatures[case_index]}: {line}"
            python_records[signatures[case_index]] = fields[4]
        native_records = {}
        for line in native_output.splitlines():
            if not line.startswith("MATRIX\t"):
                continue
            fields = line.split("\t", 3)
            case_index = int(fields[2])
            assert 0 <= case_index < len(signatures), line
            assert fields[1] == phase, line
            native_records[signatures[case_index]] = fields[3]
        assert python_records.keys() == native_records.keys() == set(signatures)
        assert python_records == native_records, family
        assert "SC_DT_INTEGER_BINARY_PUBLIC_OK" in python_output

    native_errors = run_native("errors")
    assert "BIN\tremainder_by_zero\tCAUGHT:" in native_errors
    assert "division by zero" in native_errors.lower()

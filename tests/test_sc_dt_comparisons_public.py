"""Audit the exact public comparison overload inventory against native calls."""

import csv
import hashlib
import re
import subprocess
import sys

import pytest

from .support import ROOT, run_example


FAMILIES = {
    "lt": "sc_dt::operator<",
    "le": "sc_dt::operator<=",
    "gt": "sc_dt::operator>",
    "ge": "sc_dt::operator>=",
    "eq": "sc_dt::operator==",
    "ne": "sc_dt::operator!=",
}
DIGESTS = {
    "lt": "c1aad737f54b2c8996797bcc0f6ca97f09ede225107369c41584bec6155df79f",
    "le": "e48c8d6b54f2126e04bbd5da066e359b8c01ec3a487ea625abbeb11f55e384f0",
    "gt": "c054c029e268337ad7619e0fd33e847ef42ec1015a313c89790090f15b84029a",
    "ge": "66d413a1681d9af56ca02262e3136ae8f7823316c660be8d66144694ffeccf17",
    "eq": "43966b4c73696182145634b10e32b30ad20a4d849c0743921c0761e0ced7636f",
    "ne": "de61c4a72fae374a4998405f0b32522e1345ae7608200653eb8a3de418271fd1",
}
ROW_COUNTS = {"lt": 185, "le": 185, "gt": 185, "ge": 185, "eq": 268, "ne": 291}
UNIQUE_COUNTS = {"lt": 181, "le": 181, "gt": 181, "ge": 181, "eq": 261, "ne": 284}
TEMPLATE_PARENTS = {
    "eq": {
        ("submodules/systemc/src/sysc/datatypes/bit/sc_proxy.h", "681"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_lv_base.h", "657"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "217"),
    },
    "ne": {
        ("submodules/systemc/src/sysc/datatypes/bit/sc_bit_proxies.h", "222"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_proxy.h", "681"),
        ("submodules/systemc/src/sysc/datatypes/bit/sc_proxy.h", "662"),
    },
}


def inventory_rows():
    """Read declaration identities from the committed public inventory."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


def stable_digest(rows):
    """Hash stable declaration fields while excluding mutable review metadata."""
    canonical = "".join(
        "\t".join(row[key] for key in ("name", "kind", "signature", "source", "line")) + "\n" for row in rows
    )
    return hashlib.sha256(canonical.encode()).hexdigest()


def test_comparison_inventory_has_exact_native_probes():
    """Retain declaration rows and assert every distinct signature has a probe."""
    rows = inventory_rows()
    probe_text = (ROOT / "tests/native/sc_dt_comparisons_probe.h").read_text(encoding="utf-8")
    compact = re.sub(r"[\s\"]+", "", probe_text)
    for phase, family in FAMILIES.items():
        family_rows = [row for row in rows if row["name"] == family]
        assert len(family_rows) == ROW_COUNTS[phase]
        assert stable_digest(family_rows) == DIGESTS[phase]
        signatures = {
            re.sub(r"\s+", " ", row["signature"]).strip() for row in family_rows if row["kind"] == "FunctionDecl"
        }
        assert len(signatures) == UNIQUE_COUNTS[phase]
        for signature in signatures:
            expected = f"exactoverload{family}{signature}"
            assert re.sub(r"\s+", "", expected) in compact, (family, signature)
        parents = {(row["source"], row["line"]) for row in family_rows if row["kind"] == "FunctionTemplateDecl"}
        assert parents == TEMPLATE_PARENTS.get(phase, set())
    assert sum(UNIQUE_COUNTS.values()) == 1269


@pytest.mark.parametrize(("phase", "family"), FAMILIES.items(), ids=FAMILIES.keys())
def test_python_comparisons_match_independent_native_overloads(tmp_path, phase, family):
    """Compare one public Python operator with every exact native overload."""
    python_output = run_example(tmp_path, "tests/models/sc_dt_comparisons_public.py", (phase,))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_comparisons_public.cpp"),
            "--",
            phase,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "SC_DT_COMPARISONS_PUBLIC_OK" in python_output
    assert "SC_DT_COMPARISONS_PUBLIC_OK" in native.stdout
    count = UNIQUE_COUNTS[phase]
    assert f"CMP\tprobe_count\t{count}" in python_output
    assert f"CMP\tprobe_count\t{count}" in native.stdout
    python_records = {}
    for line in python_output.splitlines():
        if line.startswith("CMP\t") and "\tPASS\t" in line:
            _, record_phase, index, _status, value = line.split("\t", 4)
            key = (record_phase, int(index))
            assert key not in python_records
            python_records[key] = value
        elif line.startswith("CMP\t") and "\tERROR:" in line:
            raise AssertionError(f"Python comparison failed: {line}")
    native_records = {}
    for line in native.stdout.splitlines():
        if line.startswith("CMP\t") and "\tprobe_count\t" not in line:
            _, record_phase, index, value = line.split("\t", 3)
            native_records[(record_phase, int(index))] = value
    expected = {(phase, index) for index in range(count)}
    assert python_records.keys() == native_records.keys() == expected
    assert python_records == native_records, family
    if phase == "eq":
        python_extra = {
            line.split("\t", 2)[1]: line.split("\t")[2:]
            for line in python_output.splitlines()
            if line.startswith("EXTRA\t")
        }
        native_extra = {
            line.split("\t", 2)[1]: line.split("\t")[2:]
            for line in native.stdout.splitlines()
            if line.startswith("EXTRA\t")
        }
        assert set(python_extra) == set(native_extra) == {"precision", "vector", "lifetime", "invalid"}
        for name in ("precision", "vector", "lifetime"):
            assert python_extra[name] == native_extra[name], name
        assert python_extra["precision"] == ["1", "0", "1"]
        assert python_extra["vector"][0:2] == ["1", "0"]
        assert python_extra["invalid"] == ["1", "1", "1"]
        assert native_extra["invalid"][0].startswith("204|value is not valid|")
        assert "?" in native_extra["invalid"][0]
        assert native_extra["invalid"][0].endswith("|1")

"""Audit exact sc_dt arithmetic operator declarations and runtime behavior."""

from collections import Counter
import csv
import hashlib
import re
import subprocess
import sys

import pytest

from .support import ROOT, run_example

FAMILIES = ("sc_dt::operator+", "sc_dt::operator-", "sc_dt::operator*", "sc_dt::operator/")
INVENTORY_DIGESTS = {
    "sc_dt::operator+": "071638071bc0899c3cea2d5be91e619ba58b4dc05e7c21d9b05da4269fde54c8",
    "sc_dt::operator-": "738549430ce35c9b0674ddd39ee27a83bcbf489bc136a96d74b86b074cc7290e",
    "sc_dt::operator*": "07469529dac21d9e7b7fdfaefdac8a13a8d9601884514cc16c27a23cddec5883",
    "sc_dt::operator/": "668c80f1300476fdcbd2961b89c802d9eda4ab3e7ce216c72db84f32f7965a92",
}
EXPECTED_ROWS = (194, 203, 191, 191)
EXPECTED_SIGNATURES = (182, 181, 179, 179)
CHUNK_SIZE = 32


def inventory_rows():
    """Read every declaration row for the four arithmetic operator names."""
    with (ROOT / "docs/_static/public-api-inventory.csv").open(newline="", encoding="utf-8") as source:
        return [row for row in csv.DictReader(source) if row["name"] in FAMILIES]


def stable_digest(rows):
    """Hash declaration identity and location while excluding tracking status."""
    canonical = "".join(
        "\t".join(row[field] for field in ("name", "kind", "source", "signature", "line")) + "\n" for row in rows
    )
    return hashlib.sha256(canonical.encode()).hexdigest()


def split_arguments(signature):
    """Split a C++ parameter list without splitting nested template commas."""
    text = signature[signature.index("(") + 1 : signature.rindex(")")]
    result = []
    depth = 0
    start = 0
    for index, character in enumerate(text):
        if character == "<":
            depth += 1
        elif character == ">":
            depth -= 1
        elif character == "," and depth == 0:
            result.append(text[start:index].strip())
            start = index + 1
    if text.strip():
        result.append(text[start:].strip())
    return result


def exact_signatures(rows, family):
    """Return unique exact function signatures in deterministic runtime order."""
    return sorted({row["signature"] for row in rows if row["name"] == family and row["kind"] == "FunctionDecl"})


def test_sc_dt_arithmetic_operator_inventory_is_exact_and_probeable():
    """Freeze all 779 rows, duplicate identities, unary signatures and native probes."""
    rows = inventory_rows()
    assert len(rows) == sum(EXPECTED_ROWS) == 779
    probe = (ROOT / "tests/native/sc_dt_arithmetic_operators_probe.h").read_text(encoding="utf-8")
    compact_probe = re.sub(r"[\s\"]+", "", probe)
    for index, family in enumerate(FAMILIES):
        selected = [row for row in rows if row["name"] == family]
        signatures = exact_signatures(rows, family)
        assert len(selected) == EXPECTED_ROWS[index]
        assert Counter(row["kind"] for row in selected) == Counter({"FunctionDecl": EXPECTED_ROWS[index]})
        assert stable_digest(selected) == INVENTORY_DIGESTS[family]
        assert len(signatures) == EXPECTED_SIGNATURES[index]
        arities = Counter(len(split_arguments(signature)) for signature in signatures)
        expected_arities = {1: 2, 2: 180} if index == 0 else ({1: 2, 2: 179} if index == 1 else {2: 179})
        assert arities == expected_arities, (family, arities)
        for signature in signatures:
            exact_probe = re.sub(r"\s+", "", f"exact overload {family} {signature}")
            assert exact_probe in compact_probe, (family, signature)


@pytest.mark.parametrize("family_index,family", tuple(enumerate(FAMILIES)))
def test_sc_dt_arithmetic_operator_public_protocol_matches_exact_native_functions(tmp_path, family_index, family):
    """Compare every exact selector and Python operator result with native pointers in small chunks."""
    signatures = exact_signatures(inventory_rows(), family)
    dispatch_gaps = []
    lifetime_types = set()
    wide_case_indices = {
        index
        for index, signature in enumerate(signatures)
        if split_arguments(signature)
        in (
            ["const sc_dt::sc_signed &", "const sc_dt::sc_signed &"],
            ["const sc_dt::sc_unsigned &", "const sc_dt::sc_unsigned &"],
        )
    }
    for start in range(0, len(signatures), CHUNK_SIZE):
        end = min(start + CHUNK_SIZE, len(signatures))
        arguments = (str(family_index), str(start), str(end))
        python_output = run_example(tmp_path, "tests/models/sc_dt_arithmetic_operators_public.py", arguments)
        selector = tmp_path / "arithmetic_operators_select.cpp"
        selector.write_text(
            "/** @file Select one bounded exact arithmetic operator matrix chunk. */\n"
            f"#define SC_DT_ARITHMETIC_FAMILY {family_index}\n"
            f"#define SC_DT_ARITHMETIC_START {start}\n"
            f"#define SC_DT_ARITHMETIC_END {end}\n",
            encoding="utf-8",
        )
        native = subprocess.run(
            [
                sys.executable,
                str(ROOT / "examples/run_sysc_cpp.py"),
                str(selector),
                str(ROOT / "tests/native/sc_dt_arithmetic_operators_public.cpp"),
            ],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert native.returncode == 0, native.stdout + native.stderr
        assert f"SC_DT_ARITHMETIC_OPERATORS_PUBLIC_OK {family_index} {start} {end}" in native.stdout

        expected = {
            int(fields[2]): "\t".join(fields[3:])
            for line in native.stdout.splitlines()
            if line.startswith("NATIVE\t")
            for fields in (line.split("\t"),)
        }
        exact = {
            int(fields[2]): "\t".join(fields[3:])
            for line in python_output.splitlines()
            if line.startswith("MATRIX\t")
            for fields in (line.split("\t"),)
        }
        public = {
            int(fields[2]): "\t".join(fields[3:])
            for line in python_output.splitlines()
            if line.startswith("PUBLIC\t")
            for fields in (line.split("\t"),)
        }
        owned = {
            int(fields[2]): fields[3]
            for line in python_output.splitlines()
            if line.startswith("OWNED\t")
            for fields in (line.split("\t"),)
        }
        lifetime_records = [line.split("\t") for line in python_output.splitlines() if line.startswith("LIFETIME\t")]
        lifetime_types.update(fields[3] for fields in lifetime_records)
        selector_errors = [line for line in python_output.splitlines() if line.startswith("MATRIX_ERROR\t")]
        assert not selector_errors, "\n".join(selector_errors[:8])
        assert set(expected) == set(exact) == set(range(start, end))
        assert exact == expected, (
            f"exact Python selector differs from native function pointers for {family} [{start}:{end}]"
        )
        public_errors = [line for line in python_output.splitlines() if line.startswith("PUBLIC_ERROR\t")]
        dispatch_gaps.extend(public_errors)
        if set(public) != set(range(start, end)):
            dispatch_gaps.extend(
                f"PUBLIC_MISSING\t{family_index}\t{index}\t{signatures[index]}"
                for index in sorted(set(range(start, end)) - set(public))
            )
        dispatch_gaps.extend(
            f"PUBLIC_MISMATCH\t{family_index}\t{index}\t{signatures[index]}\t{public[index]} != {expected[index]}"
            for index in sorted(set(public) & set(expected))
            if public[index] != expected[index]
        )
        assert set(owned) == set(public)
        assert all(value == "1" for value in owned.values()), "operator result did not own independent native storage"
        for index in sorted(wide_case_indices & set(range(start, end))):
            result_type, payload = expected[index].split("|", 1)
            width = int(payload.rsplit(":", 1)[1])
            assert result_type in ("sc_dt::sc_signed", "sc_dt::sc_unsigned")
            if family_index == 1:
                assert width == 32, f"same-source subtraction did not retain the native zero width: {expected[index]}"
            else:
                assert width >= 191, f"wide exact result lost its 191-bit source width: {expected[index]}"

    assert not dispatch_gaps, "Python operator dispatcher gaps:\n" + "\n".join(dispatch_gaps[:32])
    expected_lifetime_types = {signature.partition(" (")[0].removeprefix("const ") for signature in signatures}
    assert lifetime_types == expected_lifetime_types

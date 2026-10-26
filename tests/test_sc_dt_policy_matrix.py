"""Compare fixed-point quantization and overflow policies with native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example

FAMILIES = ("sc_fix", "sc_ufix", "sc_fix_fast", "sc_ufix_fast")
QUANTIZATION_LABELS = (
    "tie_pos_even",
    "tie_pos_odd",
    "tie_neg_even",
    "tie_neg_odd",
    "below_pos",
    "above_pos",
    "below_neg",
    "above_neg",
)
OVERFLOW_LABELS = ("overflow_positive", "overflow_negative")


def records(output):
    """Return fixed-point policy observations from one runner's stdout."""
    return [line for line in output.splitlines() if line.startswith("F2\t")]


def test_fixed_point_policy_matrix_matches_native_and_distinguishes_modes(tmp_path):
    """Cover every q/o pair, cast switch, overflow width and legal family."""
    python_output = run_example(tmp_path, "tests/models/sc_dt_policy_matrix.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_dt_policy_matrix.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = records(python_output)
    native_records = records(native.stdout)
    assert python_records == native_records

    rows = [line.split("\t") for line in python_records]
    value_rows = [row for row in rows if row[1] == "value"]
    reject_rows = [row for row in rows if row[1] == "reject"]
    q_modes = {row[3] for row in value_rows}
    o_modes = {row[4] for row in value_rows}
    assert len(q_modes) == 7
    assert len(o_modes) == 5
    assert len(value_rows) == 2 * 518 + 2 * 406

    # SC_WRAP_SM is defined for signed encodings only. The Python model and
    # native oracle both catch and record SystemC's explicit construction error.
    assert len(reject_rows) == 2 * 7
    assert {(row[2], row[3], row[4], row[5], row[6]) for row in reject_rows} == {
        (family, q_mode, "4", "2", "unsigned_wrap_sm") for family in ("sc_ufix", "sc_ufix_fast") for q_mode in q_modes
    }
    assert all(row[-1] == "1" for row in reject_rows)

    for family in FAMILIES:
        # Positive and negative ties plus adjacent values distinguish all
        # seven quantizers in each signed family.
        if family in ("sc_fix", "sc_fix_fast"):
            for o_mode in o_modes - {"4"}:  # Keep overflow saturation fixed.
                quantized = {
                    row_mode: tuple(
                        next(
                            (row[8], row[15], row[16])
                            for row in value_rows
                            if row[2] == family and row[3] == row_mode and row[4] == o_mode and row[7] == label
                        )
                        for label in QUANTIZATION_LABELS
                    )
                    for row_mode in q_modes
                }
                assert len(set(quantized.values())) == 7

        # Both ends of the representable range must exercise every legal
        # overflow policy. Flags and exact decimal values are in each row.
        legal_overflows = {mode for mode in o_modes if not (mode == "4" and family.startswith("sc_ufix"))}
        assert legal_overflows == ({"0", "1", "2", "3", "4"} if family.startswith("sc_fix") else {"0", "1", "2", "3"})
        assert {(row[3], row[4]) for row in value_rows if row[2] == family} == {
            (q_mode, o_mode) for q_mode in q_modes for o_mode in legal_overflows
        }
        for mode in legal_overflows:
            labels = {
                row[7] for row in value_rows if row[2] == family and row[3] == "6" and row[4] == mode and row[6] == "1"
            }
            assert set(OVERFLOW_LABELS) <= labels

        overflow_vectors = {}
        for mode in legal_overflows:
            vector = [
                next(
                    (row[8], row[15], row[16])
                    for row in value_rows
                    if row[2] == family and row[3] == "6" and row[4] == mode and row[7] == label and row[6] == "1"
                )
                for label in OVERFLOW_LABELS
            ]
            if mode in ("3", "4"):
                vector.append(
                    next(
                        (row[8], row[15], row[16])
                        for row in value_rows
                        if row[2] == family
                        and row[3] == "6"
                        and row[4] == mode
                        and row[5] == "0"
                        and row[7] == "n_bits_probe"
                        and row[6] == "1"
                    )
                )
            overflow_vectors[mode] = tuple(vector)
        if family.startswith("sc_fix"):
            assert len(set(overflow_vectors.values())) == 5
        else:
            # Unsigned saturation and symmetric saturation have the same
            # endpoint behavior; the other legal overflow policies differ.
            assert overflow_vectors["0"] == overflow_vectors["2"]
            assert len(set(overflow_vectors.values())) == 3

        for mode in legal_overflows:
            on_off = {
                (row[6], row[7]): (row[8], row[15], row[16])
                for row in value_rows
                if row[2] == family
                and row[3] == "0"
                and row[4] == mode
                and row[7] in ("lossy_positive", "lossy_negative")
            }
            for label in ("lossy_positive", "lossy_negative"):
                assert on_off[("1", label)] != on_off[("0", label)]

        # n_bits changes the overflow result for both wrap policies. Other
        # policies retain the configured value but do not use it semantically.
        for mode in ("3", "4"):
            widths = {
                row[5]: (row[8], row[15], row[16])
                for row in value_rows
                if row[2] == family and row[3] == "6" and row[4] == mode and row[6] == "1" and row[7] == "n_bits_probe"
            }
            if mode == "3" or family.startswith("sc_fix"):
                high_n = "3" if mode == "4" else "2"
                assert set(widths) == {"0", high_n}
                assert widths["0"] != widths[high_n]

    assert "SC_DT_POLICY_MATRIX_OK" in python_output
    assert "SC_DT_POLICY_MATRIX_OK" in native.stdout

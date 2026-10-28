"""Compare native writer-policy contention, ownership and update behavior."""

import subprocess
import sys

from .support import ROOT, run_example


def records(output):
    """Extract exact writer-policy records from captured process output."""
    return [line for line in output.splitlines() if line.startswith("WPM\t")]


def test_writer_policies_report_contention_and_sample_delta_updates(tmp_path):
    """Compare equal and changed writes under every writer policy."""
    python_output = run_example(tmp_path, "tests/models/sc_core_writer_policy_matrix.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_writer_policy_matrix.cpp"),
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

    report_rows = [row.split("\t") for row in python_records if row.startswith("WPM\tREPORT\t")]
    sample_rows = [row.split("\t") for row in python_records if row.startswith("WPM\tSAMPLE\t")]
    final_rows = [row.split("\t") for row in python_records if row.startswith("WPM\tFINAL\t")]
    assert len(report_rows) == 3
    assert [tuple(row[2:9]) for row in report_rows] == [
        ("one", "equal_same_delta", "115", "2", "sc_signal<T> cannot have more than one driver", "0 s", "0"),
        ("many", "equal_same_delta", "115", "2", "sc_signal<T> cannot have more than one driver", "0 s", "0"),
        ("one", "later_owner", "115", "2", "sc_signal<T> cannot have more than one driver", "2 ns", "4"),
    ]
    first_contention, many_contention, later_contention = (row[9] for row in report_rows)
    assert "signal `top.signal_one'" in first_contention
    assert "first driver `top.first'" in first_contention
    assert "second driver `top.second'" in first_contention
    assert "signal `top.signal_many'" in many_contention
    assert "conflicting write in delta cycle 0" in many_contention
    assert "signal `top.signal_one'" in later_contention
    assert "first driver `top.second'" in later_contention
    assert "second driver `top.first'" in later_contention
    assert all("signal `top.signal_unchecked'" not in row[9] for row in report_rows)

    assert len(sample_rows) == 9
    assert [(row[2], row[3], row[4], row[5]) for row in sample_rows] == [
        (policy, value, time, delta)
        for time, value, delta in (("0 s", "1", "1"), ("0 s", "2", "2"), ("2 ns", "3", "5"))
        for policy in ("one", "many", "unchecked")
    ]
    assert [(row[2], row[3], row[4], row[5]) for row in final_rows] == [
        ("one", "3", "3 ns", "6"),
        ("many", "3", "3 ns", "6"),
        ("unchecked", "3", "3 ns", "6"),
    ]
    assert "SC_CORE_WRITER_POLICY_MATRIX_OK" in python_output
    assert "SC_CORE_WRITER_POLICY_MATRIX_OK" in native.stdout

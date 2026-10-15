"""Validate SystemC upstream in isolated subprocesses."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def test_upstream_forkjoin_matches_native(tmp_path):
    """Compare native dynamic process order and timestamps with the Python port."""
    import re

    output = run_example(tmp_path, "examples/sysc/2.1/forkjoin/forkjoin.py")
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "submodules/systemc/examples/sysc/2.1/forkjoin/forkjoin.cpp"),
        ],
        cwd=tmp_path,
        env=dict(os.environ),
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    pattern = r"^(Round robin thread .*|Thread \d+ ending\.|Returned int is \d+)"
    assert re.findall(pattern, output, re.M) == re.findall(pattern, result.stdout, re.M)
    assert len(re.findall(pattern, output, re.M)) == 25


def test_upstream_report_rules_match_native(tmp_path):
    """Compare all report action masks, fields and times against native C++."""
    import re

    output = run_example(tmp_path, "examples/sysc/2.1/sc_report/main.py")
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "submodules/systemc/examples/sysc/2.1/sc_report/main.cpp"),
        ],
        cwd=tmp_path,
        env=dict(os.environ),
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    pattern = r"^report: ID[1-4].*\n msg=.*"
    assert re.findall(pattern, output, re.M) == re.findall(pattern, result.stdout, re.M)
    assert len(re.findall(pattern, output, re.M)) == 40


def test_upstream_reset_matches_native(tmp_path):
    """Compare all synchronous reset/value records with the upstream native port."""
    import re

    output = run_example(tmp_path, "examples/sysc/2.1/reset_signal_is/reset_signal_is.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "submodules/systemc/examples/sysc/2.1/reset_signal_is/reset_signal_is.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    pattern = r"^\d+ (?:s|ns): (?:reset|\d+)$"
    records = re.findall(pattern, output, re.M)
    assert len(records) == 101
    assert records == re.findall(pattern, native.stdout, re.M)


def test_upstream_barrier_matches_native(tmp_path):
    """Compare release order and timestamps for the Python event barrier."""
    import re

    output = run_example(tmp_path, "examples/sysc/2.1/scx_barrier/main.py")
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "submodules/systemc/examples/sysc/2.1/scx_barrier/main.cpp"),
        ],
        cwd=tmp_path,
        env=dict(os.environ),
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    pattern = r"^\d+\.\d+ - [abc]$"
    assert re.findall(pattern, output, re.M) == re.findall(pattern, result.stdout, re.M)
    assert len(re.findall(pattern, output, re.M)) == 3


def test_upstream_exports_match_native(tmp_path):
    """Compare hierarchical export dispatch times against the C++ reference."""
    import re

    output = run_example(tmp_path, "examples/sysc/2.1/sc_export/main.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "submodules/systemc/examples/sysc/2.1/sc_export/main.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    pattern = r"^\d+ ns In Channel run\(\)"
    records = re.findall(pattern, output, re.M)
    assert records == re.findall(pattern, native.stdout, re.M)
    assert records == ["10 ns In Channel run()", "17 ns In Channel run()", "20 ns In Channel run()"]

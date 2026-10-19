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
    pattern = r"^(Round robin thread .*|Thread \d+ ending\.|Returned int is \d+|(?:Test_function|test_function|void_function|ref_function) sees .*|Done\.)"
    records = re.findall(pattern, output, re.M)
    native_records = re.findall(pattern, result.stdout, re.M)
    assert [record.lower() for record in records] == [record.lower() for record in native_records]
    assert len(records) == 29
    assert records[:12] == [f"Round robin thread {index % 4 + 1} at time {100 + index * 10} ns" for index in range(12)]


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
    sections = r"^(?:We got .*|Default settings.*|Specific settings.*|temporarily .*)$"
    assert re.findall(sections, output, re.M) == re.findall(sections, result.stdout, re.M)
    assert len(re.findall(sections, output, re.M)) == 6


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
    assert records[0] == "0 s: reset"
    assert sum(record.endswith(": reset") for record in records) == 6
    assert "Done" in output and "Done" in native.stdout


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
    protocol_output = run_example(tmp_path, "examples/sysc/2.1/sc_export/main_protocol.py")
    assert re.findall(pattern, protocol_output, re.M) == records


def test_upstream_delay_pipe_matches_native(tmp_path):
    """Compare all four-stage121bit exported-pipeline edge observations."""
    import re

    output = run_example(tmp_path, "examples/sysc/2.1/dpipe/main.py")
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "amal.eda.systemc._runner",
            str(ROOT / "submodules/systemc/examples/sysc/2.1/dpipe/main.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    pattern = r"^(\d+): (\d+)$"
    assert re.findall(pattern, output, re.M) == re.findall(pattern, result.stdout, re.M)
    assert len(re.findall(pattern, output, re.M)) == 10


def test_upstream_mutex_policy_matches_native(tmp_path):
    """Compare FIFO policy ownership order and all three acquisition times."""
    import re

    output = run_example(tmp_path, "examples/sysc/2.1/scx_mutex_w_policy/scx_mutex_w_policy.py")
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "amal.eda.systemc._runner",
            str(ROOT / "submodules/systemc/examples/sysc/2.1/scx_mutex_w_policy/scx_mutex_w_policy.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    pattern = r"^t\d got mutex at .*"
    assert re.findall(pattern, output, re.M) == re.findall(pattern, result.stdout, re.M)
    assert len(re.findall(pattern, output, re.M)) == 3


def test_upstream_async_host_notification_matches_native(tmp_path):
    """Compare separate-host-thread event time and lifecycle with native source."""
    import re

    output = run_example(tmp_path, "examples/sysc/2.3/simple_async/main.py")
    result = subprocess.run(
        [
            sys.executable,
            "-m",
            "amal.eda.systemc._runner",
            str(ROOT / "submodules/systemc/examples/sysc/2.3/simple_async/main.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    pattern = r"^(?:Start SystemC.*|I'm busy!|Asked to stop.*|The dog barks.*|Program completed)$"
    assert re.findall(pattern, output, re.M) == re.findall(pattern, result.stdout, re.M)
    assert len(re.findall(pattern, output, re.M)) == 5


def test_upstream_specialized_signals_matches_native(tmp_path):
    """Compare all concatenation outputs and assert26 native arithmetic checks."""
    import re

    output = run_example(tmp_path, "examples/sysc/2.1/specialized_signals/main.py")
    directory = ROOT / "submodules/systemc/examples/sysc/2.1/specialized_signals"
    sources = [
        directory / (stem + ".cpp")
        for stem in ("scx_signal_int", "scx_signal_uint", "scx_signal_signed", "scx_signal_unsigned", "main")
    ]
    result = subprocess.run(
        [sys.executable, "-m", "amal.eda.systemc._runner", *map(str, sources)],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    pattern = r"^(?:\( m_in_|m_signal_|m_inout_).*"
    assert re.findall(pattern, output, re.M) == re.findall(pattern, result.stdout, re.M)
    assert len(re.findall(pattern, output, re.M)) == 13
    assert " != " not in result.stdout

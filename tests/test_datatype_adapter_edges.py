"""Compare selected datatype adapters with independent native references."""

import os
import subprocess
import sys

from .support import ROOT, run_example


def run_native(tmp_path):
    """Compile and execute the native datatype reference in a fresh process.

    Args:
        tmp_path: Isolated directory for native compilation and execution.
    Returns:
        Captured output from the bounded native process.
    Raises:
        AssertionError: Compilation or native execution returns nonzero.
        subprocess.TimeoutExpired: The process exceeds sixty seconds.
    """
    environment = dict(os.environ)
    environment.setdefault("SYSTEMC_HOME", str(ROOT / "submodules/systemc"))
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/datatype_adapter_edges.cpp"),
        ],
        cwd=tmp_path,
        env=environment,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def selected_records(output):
    """Keep deterministic observations and omit native kernel banners."""
    return [line for line in output.splitlines() if line.startswith("DAE\t")]


def test_datatype_adapter_edges_match_native(tmp_path):
    """Check text concat, fixed-range copies, exact assignment and streams."""
    python_dir = tmp_path / "python"
    native_dir = tmp_path / "native"
    python_dir.mkdir()
    native_dir.mkdir()

    python_output = run_example(python_dir, "tests/models/datatype_adapter_edges.py")
    native_output = run_native(native_dir)
    python_records = selected_records(python_output)
    native_records = selected_records(native_output)
    python_parity = [line for line in python_records if not line.startswith("DAE\tERROR\t")]

    assert python_parity == native_records
    assert [record.split("\t")[1:3] for record in native_records] == [
        ["CONCAT", "head"],
        ["CONCAT", "head"],
        ["CONCAT", "tail"],
        ["CONCAT", "tail"],
        ["CONCAT", "vector_head"],
        ["CONCAT", "vector_head"],
        ["CONCAT", "vector_tail"],
        ["CONCAT", "vector_tail"],
        ["RANGE", "slow"],
        ["RANGE", "slow"],
        ["RANGE", "slow"],
        ["RANGE", "fast"],
        ["RANGE", "fast"],
        ["RANGE", "fast"],
        ["RANGE", "mutable_slow"],
        ["RANGE", "mutable_slow"],
        ["RANGE", "mutable_slow"],
        ["RANGE", "mutable_fast"],
        ["RANGE", "mutable_fast"],
        ["RANGE", "mutable_fast"],
        ["ASSIGN", "wide_integer"],
        ["STREAM", "ordinary_string"],
        ["STREAM", "mutable_ctypes_int"],
    ]
    assert "DAE\tERROR\tunsupported_operator\tTypeError" in python_records
    immutable_rejections = [
        record for record in python_records if record.startswith("DAE\tERROR\timmutable_stream_target\t")
    ]
    assert [record.split("\t")[3] for record in immutable_rejections] == ["bool", "int", "float", "str", "cppyy_int"]
    assert all(
        "TypeError\tstream extraction requires mutable native target\t41" in record for record in immutable_rejections
    )
    assert len(native_records) == 23

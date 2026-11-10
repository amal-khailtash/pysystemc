"""Compare upstream TLM memory and DMI behavior with the isolated Python model."""

import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

from .support import ROOT, run_example


def _compile_native_reference(tmp_path):
    """Compile the native probe with the unmodified example implementation."""
    probe = subprocess.run(
        [
            sys.executable,
            "-c",
            "import json; from amal.eda.systemc import _runtime as r; "
            "print(json.dumps([str(r._headers), str(r._library)]))",
        ],
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert probe.returncode == 0, probe.stdout + probe.stderr
    headers, library = json.loads(probe.stdout.splitlines()[-1])
    common = ROOT / "submodules/systemc/examples/tlm/common"
    executable = tmp_path / "native_example_tlm_memory_edges"
    sources = (
        ROOT / "tests/native/example_tlm_memory_edges.cpp",
        common / "src/memory.cpp",
        common / "src/report.cpp",
        common / "src/lt_dmi_target.cpp",
    )
    command = [
        *shlex.split(os.environ.get("CXX", "c++")),
        "-std=c++17",
        "-I",
        headers,
        "-I",
        str(common / "include"),
        *map(str, sources),
        library,
        f"-Wl,-rpath,{Path(library).parent}",
        "-pthread",
        "-o",
        str(executable),
    ]
    compiled = subprocess.run(command, capture_output=True, text=True, timeout=60)
    assert compiled.returncode == 0, compiled.stdout + compiled.stderr
    return executable


def _tagged_rows(output, prefix):
    """Parse structured probe lines while keeping scenario labels as keys."""
    rows = {}
    for line in output.splitlines():
        if line.startswith(prefix + "|"):
            fields = line.split("|")
            rows[fields[1]] = fields[2:]
    return rows


def test_example_tlm_memory_edges_match_native_source_contract(tmp_path):
    """Preserve native status precedence, length transfers, delays and DMI bounds."""
    executable = _compile_native_reference(tmp_path)
    native = subprocess.run([str(executable)], cwd=tmp_path, capture_output=True, text=True, timeout=60)
    assert native.returncode == 0, native.stdout + native.stderr
    native_memory = _tagged_rows(native.stdout, "MEM")
    expected_status_delay = {
        "write4": ("TLM_OK_RESPONSE", "9 ns"),
        "read4": ("TLM_OK_RESPONSE", "5 ns"),
        "byteenable_write": ("TLM_OK_RESPONSE", "9 ns"),
        "stream_mismatch_read": ("TLM_OK_RESPONSE", "5 ns"),
        "length1_read": ("TLM_OK_RESPONSE", "5 ns"),
        "length5_write": ("TLM_OK_RESPONSE", "9 ns"),
        "length0_read": ("TLM_OK_RESPONSE", "5 ns"),
        "exact_end": ("TLM_ADDRESS_ERROR_RESPONSE", "2 ns"),
        "cross_end": ("TLM_ADDRESS_ERROR_RESPONSE", "2 ns"),
        "out_of_range": ("TLM_ADDRESS_ERROR_RESPONSE", "2 ns"),
        "bad_address_byteenable": ("TLM_ADDRESS_ERROR_RESPONSE", "2 ns"),
        "ignore_first": ("TLM_OK_RESPONSE", "0 s"),
        "ignore_second": ("TLM_OK_RESPONSE", "2 ns"),
        "bad_address_ignore": ("TLM_ADDRESS_ERROR_RESPONSE", "2 ns"),
    }
    assert set(native_memory) == set(expected_status_delay)
    assert {name: tuple(row[:2]) for name, row in native_memory.items()} == expected_status_delay
    assert native_memory["byteenable_write"][3].split(",")[4:8] == ["51", "62", "73", "84"]
    assert native_memory["stream_mismatch_read"][2].split(",")[:4] == ["51", "62", "73", "84"]
    assert native_memory["length1_read"][2].split(",")[:4] == ["11", "C1", "C2", "C3"]
    assert native_memory["length5_write"][3].split(",")[8:13] == ["91", "82", "73", "64", "55"]
    assert native_memory["exact_end"][3] == native_memory["cross_end"][3]
    assert native.stdout.count("Unsupported Command Extension") == 1

    native_dmi = [line.split("|")[1:] for line in native.stdout.splitlines() if line.startswith("DMI|")]
    assert native_dmi == [
        ["enabled", "0", "1", "0", "4096", "1", "3 ns", "7 ns"],
        ["enabled", "4096", "1", "0", "4096", "1", "3 ns", "7 ns"],
        ["enabled", "4097", "0"],
        ["disabled", "0", "0"],
        ["disabled", "4096", "0"],
        ["disabled", "4097", "0"],
    ]
    invalidations = [line for line in native.stdout.splitlines() if line.startswith("INVALIDATE|")]
    assert invalidations == ["INVALIDATE|0|4096"]

    python = run_example(tmp_path, "tests/models/example_tlm_memory_edges.py")
    python_memory = _tagged_rows(python, "PY")
    assert set(python_memory) == set(expected_status_delay)
    assert python_memory == native_memory

    python_dmi = [line.split("|")[1:] for line in python.splitlines() if line.startswith("PYDMI|")]
    assert python_dmi == [
        ["enabled", "0", "1", "0", "4096", "1", "3 ns", "7 ns"],
        ["enabled", "4096", "1", "0", "4096", "1", "3 ns", "7 ns"],
        ["enabled", "4097", "0", "", "", "", "", ""],
        ["disabled", "0", "0"],
        ["disabled", "4096", "0"],
        ["disabled", "4097", "0"],
    ]

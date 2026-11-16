"""Check bounded back-pressure and reset edges against upstream channel ports."""

import re
import subprocess
import sys
from pathlib import Path

import pytest

from .support import ROOT, run_example

CASES = [
    ("rvd", "stalled_consumer"),
    ("ttd", "stalled_consumer"),
    ("rvd", "delayed_ack"),
    ("ttd", "delayed_ack"),
    ("rvd", "reset_pending"),
    ("ttd", "reset_pending"),
]
RECORD = re.compile(r"^EDGE (\S+) value=(-?\d+) ns=(\d+)$", re.M)


def _records(output: str) -> list[tuple[str, int, int]]:
    """Parse only role, value, and integer simulation-time probe records."""
    return [(tag, int(value), int(time)) for tag, value, time in RECORD.findall(output)]


@pytest.mark.parametrize(("protocol", "scenario"), CASES)
def test_handshake_edges_match_upstream_ports(tmp_path: Path, protocol: str, scenario: str) -> None:
    """Compare one isolated stalled/delayed/reset scenario to native channels.

    Args:
        tmp_path: Fresh working directory for independent SystemC subprocesses.
        protocol: Select the native ready-valid or toggle-toggle channel.
        scenario: Select stalled consumer, delayed acknowledgement, or reset.
    """
    python_output = run_example(tmp_path, "tests/models/example_handshake_edges.py", (protocol, scenario))
    native_source = ROOT / "tests/native/example_handshake_edges.cpp"
    include_dir = ROOT / "submodules/systemc/examples/sysc/2.3/include"
    native = subprocess.run(
        [
            sys.executable,
            "-m",
            "amal.eda.systemc._runner",
            "-I",
            str(include_dir),
            str(native_source),
            "--",
            protocol,
            scenario,
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr

    python_records = _records(python_output)
    native_records = _records(native.stdout)
    assert python_records, python_output
    assert python_records == native_records, (protocol, scenario, python_records, native_records)

    tags = [tag for tag, _, _ in python_records]
    received = [value for tag, value, _ in python_records if tag == "RECV"]
    sent_done = [value for tag, value, _ in python_records if tag == "SEND_DONE"]

    if scenario == "stalled_consumer":
        assert received == [41]
        assert sent_done == [41]
        begin_time = next(time for tag, value, time in python_records if (tag, value) == ("SEND_BEGIN", 41))
        receive_time = next(time for tag, value, time in python_records if (tag, value) == ("RECV", 41))
        assert receive_time >= begin_time + 30
        assert tags.index("SEND_BEGIN") < tags.index("RECV")
    elif scenario == "delayed_ack":
        assert received == [11, 22]
        assert sent_done == [11, 22]
        begin_second = next(i for i, (tag, value, _) in enumerate(python_records) if (tag, value) == ("SEND_BEGIN", 22))
        done_first = next(i for i, (tag, value, _) in enumerate(python_records) if (tag, value) == ("SEND_DONE", 11))
        receive_first = next(i for i, (tag, value, _) in enumerate(python_records) if (tag, value) == ("RECV", 11))
        done_second = next(i for i, (tag, value, _) in enumerate(python_records) if (tag, value) == ("SEND_DONE", 22))
        receive_second = next(i for i, (tag, value, _) in enumerate(python_records) if (tag, value) == ("RECV", 22))
        assert receive_first < done_second < receive_second
        if protocol == "rvd":
            assert done_first < begin_second
        else:
            assert begin_second < receive_first
    else:
        assert [(tag, value) for tag, value, _ in python_records if tag.startswith("RESET_")] == [
            ("RESET_ASSERT", -1),
            ("RESET_RELEASE", -1),
        ]
        assert received == ([42] if protocol == "rvd" else [43])
        if protocol == "rvd":
            assert received[0] == 42
            assert received[0] != 41
        else:
            assert received[0] == 43
            assert received[0] not in (41, 42)
        reset_assert = next(i for i, (tag, _, _) in enumerate(python_records) if tag == "RESET_ASSERT")
        reset_release = next(i for i, (tag, _, _) in enumerate(python_records) if tag == "RESET_RELEASE")
        first_begin = next(i for i, (tag, value, _) in enumerate(python_records) if (tag, value) == ("SEND_BEGIN", 41))
        assert first_begin < reset_assert < reset_release
        assert reset_release < next(
            i for i, (tag, value, _) in enumerate(python_records) if (tag, value) == ("SEND_BEGIN", received[0])
        )
        if protocol == "rvd":
            assert not any(tag == "SEND_DONE" and value == 41 for tag, value, _ in python_records)
        else:
            assert any(tag == "SEND_DONE" and value == 41 for tag, value, _ in python_records)
            assert any(tag == "SEND_BEGIN" and value == 42 for tag, value, _ in python_records)
            assert not any(tag == "SEND_DONE" and value == 42 for tag, value, _ in python_records)

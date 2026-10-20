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
    assert "Program completed" in output and "Program completed" in result.stdout


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


def test_upstream_ready_valid_matches_native(tmp_path):
    """Compare every ready/valid producer, DUT and consumer value/time record."""
    _compare_handshake(tmp_path, "sc_rvd")


def test_upstream_toggle_toggle_matches_native(tmp_path):
    """Compare every toggle handshake producer, DUT and consumer value/time record."""
    _compare_handshake(tmp_path, "sc_ttd")


def _compare_handshake(tmp_path, group):
    """Run independent kernels and assert role, value, time, order and completion."""
    import re

    output = run_example(tmp_path, f"examples/sysc/2.3/{group}/main.py")
    native = subprocess.run(
        [
            sys.executable,
            "-m",
            "amal.eda.systemc._runner",
            "-I",
            str(ROOT / "submodules/systemc/examples/sysc/2.3/include"),
            str(ROOT / f"submodules/systemc/examples/sysc/2.3/{group}/main.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    pattern = r"^( +)(\d+) +(\d+ (?:s|ns|ps))$"
    records = re.findall(pattern, output, re.M)
    assert records == re.findall(pattern, native.stdout, re.M)
    consumers = [(int(value), time) for indent, value, time in records if len(indent) >= 15]
    assert [value for value, _ in consumers] == list(range(40))
    assert len(records) == (131 if group == "sc_rvd" else 132)
    assert consumers[-1][1] == ("254 ns" if group == "sc_rvd" else "298 ns")
    assert "Program completed" in output and "Program completed" in native.stdout


def test_upstream_pipe_matches_native(tmp_path):
    """Compare all fifty double pipeline results with the native multistage model."""
    import re

    output = run_example(tmp_path, "examples/sysc/pipe/main.py")
    directory = ROOT / "submodules/systemc/examples/sysc/pipe"
    native = subprocess.run(
        [
            sys.executable,
            "-m",
            "amal.eda.systemc._runner",
            *[str(directory / f"{stem}.cpp") for stem in ("numgen", "stage1", "stage2", "stage3", "display", "main")],
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    pattern = r"^Result = (.*)$"
    records = re.findall(pattern, output, re.M)
    assert len(records) == 50
    assert records == re.findall(pattern, native.stdout, re.M)


def test_upstream_fir_variants_match_native(tmp_path):
    """Compare behavioral and RTL FIR stimulus/results/times independently."""
    import re

    directory = ROOT / "submodules/systemc/examples/sysc/fir"
    for rtl in (False, True):
        stem = "main_rtl" if rtl else "main"
        output = run_example(tmp_path, f"examples/sysc/fir/{stem}.py")
        sources = ("fir_fsm", "fir_data") if rtl else ("fir",)
        native = subprocess.run(
            [str(_compile_upstream(tmp_path, directory, (*sources, "stimulus", "display", stem)))],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert native.returncode == 0, native.stdout + native.stderr
        pattern = r"^(?:Stimuli : .*|Display : .*|Information : Reset state|Simulation of .*finished.*)$"
        records = re.findall(pattern, output, re.M)
        assert records == re.findall(pattern, native.stdout, re.M)
        assert sum(record.startswith("Display") for record in records) == 24


def test_upstream_fft_float_variants_match_native(tmp_path):
    """Compare output files for default and all four upstream float input pairs."""
    _compare_fft(tmp_path, "fft_flpt")


def test_upstream_fft_fixed_variants_match_native(tmp_path):
    """Compare exact fixed-point outputs for all five upstream input pairs."""
    _compare_fft(tmp_path, "fft_fxpt")


def _compare_fft(tmp_path, group):
    """Copy inputs into isolated runs and compare every output and stage heading."""
    import re
    import shutil

    directory = ROOT / f"submodules/systemc/examples/sysc/fft/{group}"
    executable = _compile_upstream(tmp_path, directory, ("fft", "source", "sink", "main"))
    for suffix in ("", ".1", ".2", ".3", ".4"):
        work = tmp_path / ("default" if not suffix else suffix[1:])
        work.mkdir()
        for component in ("real", "imag"):
            shutil.copyfile(directory / f"in_{component}{suffix}", work / f"in_{component}")
        output = run_example(work, f"examples/sysc/fft/{group}/main.py")
        python_files = [(work / name).read_text() for name in ("out_real", "out_imag")]
        native = subprocess.run(
            [str(executable)],
            cwd=work,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert native.returncode == 0, native.stdout + native.stderr
        pattern = r"^(?:Reading in the samples.*|Computing.*|Writing the transform.*|Done.*|End of Input Stream.*)$"
        assert re.findall(pattern, output, re.M) == re.findall(pattern, native.stdout, re.M)
        for name, expected in zip(("out_real", "out_imag"), python_files):
            actual = (work / name).read_text()
            assert len(expected.split()) >= 16
            assert expected == actual, (group, suffix, name, expected, actual)


def _compile_upstream(tmp_path, directory, stems):
    """Compile unmodified sources as separate translation units with native SystemC.

    Unguarded upstream headers require normal separate C++ translation units.
    Discovery follows the active binding runtime in a separate interpreter.
    """
    import json
    import shlex

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
    from pathlib import Path

    executable = tmp_path / "native_example"
    command = [
        *shlex.split(os.environ.get("CXX", "c++")),
        "-std=c++17",
        "-I",
        headers,
        *[str(directory / f"{stem}.cpp") for stem in stems],
        library,
        f"-Wl,-rpath,{Path(library).parent}",
        "-pthread",
        "-o",
        str(executable),
    ]
    result = subprocess.run(command, capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    return executable


def test_upstream_custom_fifo_matches_native(tmp_path):
    """Compare every byte and immediate occupancy marker against upstream FIFO."""
    import re

    output = run_example(tmp_path, "examples/sysc/simple_fifo/simple_fifo.py")
    native = subprocess.run(
        [
            sys.executable,
            "-m",
            "amal.eda.systemc._runner",
            str(ROOT / "submodules/systemc/examples/sysc/simple_fifo/simple_fifo.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    pattern = r"V(?:<9>)?isit.*today!"
    records = re.findall(pattern, output)
    assert len(records) == 1
    assert records == re.findall(pattern, native.stdout)
    assert "<1>" in records[0] and "<9>" in records[0]


def test_upstream_rsa_seed_matches_native(tmp_path):
    """Compare all keys/messages for deterministic Linux source random streams."""
    import re

    executable = _compile_upstream(tmp_path, ROOT / "submodules/systemc/examples/sysc/rsa", ("rsa",))
    for seed in (1, 7):
        python = subprocess.run(
            [sys.executable, str(ROOT / "examples/sysc/rsa/rsa.py"), str(seed)],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert python.returncode == 0, python.stdout + python.stderr
        native = subprocess.run([str(executable), str(seed)], cwd=tmp_path, capture_output=True, text=True, timeout=60)
        assert native.returncode == 0, native.stdout + native.stderr
        pattern = r"^(?:Random number.*|RSA .*|[end] = .*|\d+|Message.*|Ciphered.*|Deciphered.*|Note that.*|showing.*)$"
        records = re.findall(pattern, python.stdout, re.M)
        assert records == re.findall(pattern, native.stdout, re.M)
        assert len(records) == 15


def test_upstream_packet_switch_seed_matches_native(tmp_path):
    """Compare all sent/received packets and drop counters with fixed source time."""
    import re
    import shutil

    directory = ROOT / "submodules/systemc/examples/sysc/pkt_switch"
    source = tmp_path / "source"
    shutil.copytree(directory, source)
    sender = source / "sender.cpp"
    # Change only the test copy's wall-time seed, preserving native routing/stimulus.
    sender.write_text(sender.read_text().replace("srand((unsigned)time(NULL));", "srand(7);"))
    executable = _compile_upstream(tmp_path, source, ("fifo", "sender", "receiver", "switch_clk", "switch", "main"))
    python = subprocess.run(
        [sys.executable, str(ROOT / "examples/sysc/pkt_switch/main.py")],
        cwd=tmp_path,
        env=dict(os.environ, PYSYSTEMC_PACKET_SEED="7"),
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert python.returncode == 0, python.stdout + python.stderr
    native = subprocess.run([str(executable)], cwd=tmp_path, capture_output=True, text=True, timeout=60)
    assert native.returncode == 0, native.stdout + native.stderr
    pattern = r"^ *(?:New Packet.*|Destination Addresses:.*|Packet Value:.*|Sender ID:.*|Receiver ID:.*|Total number.*|Percentage packets.*)$"
    records = [line.strip() for line in re.findall(pattern, python.stdout, re.M)]
    assert len(records) > 1000
    assert records == [line.strip() for line in re.findall(pattern, native.stdout, re.M)]


def test_upstream_simple_bus_matches_native(tmp_path):
    """Compare one hundred direct monitor records across competing bus masters."""
    import re

    directory = ROOT / "submodules/systemc/examples/sysc/simple_bus"
    stems = (
        "simple_bus",
        "simple_bus_arbiter",
        "simple_bus_master_blocking",
        "simple_bus_master_non_blocking",
        "simple_bus_master_direct",
        "simple_bus_tools",
        "simple_bus_types",
        "simple_bus_main",
    )
    executable = _compile_upstream(tmp_path, directory, stems)
    output = run_example(tmp_path, "examples/sysc/simple_bus/simple_bus_main.py")
    native = subprocess.run([str(executable)], cwd=tmp_path, capture_output=True, text=True, timeout=60)
    assert native.returncode == 0, native.stdout + native.stderr
    pattern = r"^.*top.master_d : mem.*$"
    records = re.findall(pattern, output, re.M)
    assert len(records) == 100
    assert records == re.findall(pattern, native.stdout, re.M)


def test_upstream_risc_python_top_matches_native(tmp_path):
    """Compare all Python CPU components against the independent native CPU."""
    import re
    import shutil

    directory = ROOT / "submodules/systemc/examples/sysc/risc_cpu"
    for image in directory.glob("*.img"):
        shutil.copyfile(image, tmp_path / image.name)
    stems = ("bios", "paging", "icache", "fetch", "decode", "exec", "mmxu", "floating", "dcache", "pic", "main")
    executable = _compile_upstream(tmp_path, directory, stems)
    output = run_example(tmp_path, "examples/sysc/risc_cpu/main.py")
    native = subprocess.run([str(executable)], cwd=tmp_path, capture_output=True, text=True, timeout=60)
    assert native.returncode == 0, native.stdout + native.stderr
    pattern = r"^.*(?:IFU :|ID:|ALU :|FPU ?:|MMX :|R\s*\d+\().*$"
    records = re.findall(pattern, output, re.M)
    assert len(records) > 150
    assert records == re.findall(pattern, native.stdout, re.M)
    assert "SHUTDOWN - at CSIM 274 ns" in output

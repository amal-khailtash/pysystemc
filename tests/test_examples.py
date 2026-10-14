"""Run native singleton simulations in fresh interpreter processes."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def run_example(tmp_path, path):
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run([sys.executable, str(ROOT / path)], cwd=tmp_path,
                            env=env, capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def test_hello_callback_and_lifecycle(tmp_path):
    output = run_example(tmp_path, "examples/helloworld/py/helloworld.py")
    assert output.count("- hello2()") == 1
    for event in ("Before End of Elaboration", "Elaboration finished",
                  "Simulation started", "Simulation finished"):
        assert event in output


def test_counter_sensitivity_and_trace(tmp_path):
    output = run_example(tmp_path, "examples/counter/py/counter_tb.py")
    assert output.count("Incremented Counter") == 20
    assert "@74 ns :: Incremented Counter 0" in output
    trace = (tmp_path / "counter.vcd").read_text()
    for name in ("clock", "reset", "enable", "count"):
        assert any(name in line for line in trace.splitlines() if line.startswith("$var"))
    assert "b1111 aaaad" in trace
    assert "b0 aaaad" in trace


def test_fifo_threads_with_pthreads(tmp_path):
    """Validate blocking Python FIFO processes with the default native build."""
    output = run_example(tmp_path, "examples/simple_fifo/py/simple_fifo.py")
    assert "Visit www.accellera.org and see what SystemC can do for you today!" in output


def test_upstream_forkjoin_matches_native(tmp_path):
    """Compare native dynamic process order and timestamps with the Python port."""
    import re
    output = run_example(tmp_path, "examples/sysc/2.1/forkjoin/forkjoin.py")
    result = subprocess.run([sys.executable, str(ROOT / "examples/run_sysc_cpp.py"),
        str(ROOT / "submodules/systemc/examples/sysc/2.1/forkjoin/forkjoin.cpp")],
        cwd=tmp_path, env=dict(os.environ), capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    pattern = r"^(Round robin thread .*|Thread \d+ ending\.|Returned int is \d+)"
    assert re.findall(pattern, output, re.M) == re.findall(pattern, result.stdout, re.M)
    assert len(re.findall(pattern, output, re.M)) == 25


def test_upstream_report_rules_match_native(tmp_path):
    """Compare all report action masks, fields and times against native C++."""
    import re
    output = run_example(tmp_path, "examples/sysc/2.1/sc_report/main.py")
    result = subprocess.run([sys.executable, str(ROOT / "examples/run_sysc_cpp.py"),
        str(ROOT / "submodules/systemc/examples/sysc/2.1/sc_report/main.cpp")],
        cwd=tmp_path, env=dict(os.environ), capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    pattern = r"^report:.*|^ msg=.*"
    assert re.findall(pattern, output, re.M) == re.findall(pattern, result.stdout, re.M)
    assert len(re.findall(pattern, output, re.M)) == 80


def test_reset_rejected_before_native_unwind(tmp_path):
    """Unsupported reset must produce a clear failure rather than native abort."""
    result = subprocess.run([sys.executable,
        str(ROOT / "examples/sysc/2.1/reset_signal_is/reset_signal_is.py")], cwd=tmp_path,
        env=dict(os.environ), capture_output=True, text=True, timeout=60)
    assert result.returncode != 0
    assert "Python process reset is not supported" in result.stderr
    assert "F539" not in result.stderr
    assert "segmentation" not in result.stderr


def test_upstream_barrier_matches_native(tmp_path):
    """Compare release order and timestamps for the Python event barrier."""
    import re
    output = run_example(tmp_path, "examples/sysc/2.1/scx_barrier/main.py")
    result = subprocess.run([sys.executable, str(ROOT / "examples/run_sysc_cpp.py"),
        str(ROOT / "submodules/systemc/examples/sysc/2.1/scx_barrier/main.cpp")],
        cwd=tmp_path, env=dict(os.environ), capture_output=True, text=True, timeout=60)
    assert result.returncode == 0, result.stdout + result.stderr
    pattern = r"^\d+\.\d+ - [abc]$"
    assert re.findall(pattern, output, re.M) == re.findall(pattern, result.stdout, re.M)
    assert len(re.findall(pattern, output, re.M)) == 3


def test_arbitrary_width_datatypes(tmp_path):
    """Verify runtime template instantiation, wide signals and bit/range access."""
    assert "DATATYPES_OK" in run_example(tmp_path, "tests/models/datatypes.py")


def test_banner_version_colors_and_disable(tmp_path):
    """Check version/banner ordering, explicit color and banner suppression."""
    from importlib.metadata import version
    command = [sys.executable, str(ROOT / "examples/helloworld/py/helloworld.py")]
    env = dict(os.environ, PYSYSTEMC_COLOR="always", NO_COLOR="1",
               PYSYSTEMC_LOG_LEVEL="DEBUG")
    env.pop("PYSC_COPYRIGHT_MESSAGE", None)
    result = subprocess.run(command, cwd=tmp_path, env=env,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60)
    assert result.returncode == 0, result.stdout
    banner = f"pysystemc {version('pysystemc')}"
    assert result.stdout.index(banner) < result.stdout.index("SystemC 2.3.4-Accellera")
    assert "\x1b[" in result.stdout
    assert "[pysystemc:C++:DEBUG]" in result.stdout
    env.update(PYSC_COPYRIGHT_MESSAGE="DISABLE", PYSYSTEMC_COLOR="never")
    result = subprocess.run(command, cwd=tmp_path, env=env,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60)
    assert result.returncode == 0, result.stdout
    assert banner not in result.stdout
    assert "SystemC 2.3.4-Accellera" in result.stdout
    assert "\x1b[" not in result.stdout


def test_cpp_runner_kernel_banner(tmp_path):
    """Native startup prints the kernel banner and honors its suppression flag."""
    command = [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"),
               str(ROOT / "examples/helloworld/cpp/helloworld.cpp")]
    env = dict(os.environ)
    env.pop("SYSTEMC_DISABLE_COPYRIGHT_MESSAGE", None)
    env.pop("SC_COPYRIGHT_MESSAGE", None)
    result = subprocess.run(command, cwd=tmp_path, env=env,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60)
    assert result.returncode == 0, result.stdout
    assert "[pysystemc:C++:info]" in result.stdout
    assert "Reading " in result.stdout
    assert result.stdout.index("SystemC 2.3.4-Accellera") < result.stdout.index("Hello world using approach 1")
    env["SC_COPYRIGHT_MESSAGE"] = "DISABLE"
    result = subprocess.run(command, cwd=tmp_path, env=env,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60)
    assert result.returncode == 0, result.stdout
    assert "SystemC 2.3.4-Accellera" not in result.stdout
    assert "Hello world using approach 2" in result.stdout

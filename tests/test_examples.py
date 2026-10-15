"""Run native singleton simulations in fresh interpreter processes."""

import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def run_example(tmp_path, path):
    """Run a finite simulation in a fresh process and return captured stdout.

    Args:
        tmp_path: Isolated working directory for simulation outputs.
        path: Repository-relative example or semantic test script.

    Raises:
        AssertionError: If native/Python execution returns a failure status.
        subprocess.TimeoutExpired: If the simulation exceeds 60 seconds.
    """
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [sys.executable, str(ROOT / path)], cwd=tmp_path, env=env, capture_output=True, text=True, timeout=60
    )
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def test_hello_callback_and_lifecycle(tmp_path):
    """Verify method dispatch and all elaboration/simulation lifecycle hooks."""
    output = run_example(tmp_path, "examples/helloworld/py/helloworld.py")
    assert output.count("- hello2()") == 1
    for event in ("Before End of Elaboration", "Elaboration finished", "Simulation started", "Simulation finished"):
        assert event in output


def test_counter_sensitivity_and_trace(tmp_path):
    """Check edge sensitivity, modulo counter values and actual VCD transitions."""
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
    pattern = r"^report:.*|^ msg=.*"
    assert re.findall(pattern, output, re.M) == re.findall(pattern, result.stdout, re.M)
    assert len(re.findall(pattern, output, re.M)) == 80


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


def test_arbitrary_width_datatypes(tmp_path):
    """Verify runtime template instantiation, wide signals and bit/range access."""
    assert "DATATYPES_OK" in run_example(tmp_path, "tests/models/datatypes.py")


def test_banner_version_colors_and_disable(tmp_path):
    """Check version/banner ordering, explicit color and banner suppression."""
    from importlib.metadata import version

    command = [sys.executable, str(ROOT / "examples/helloworld/py/helloworld.py")]
    env = dict(os.environ, PYSYSTEMC_COLOR="always", NO_COLOR="1", PYSYSTEMC_LOG_LEVEL="DEBUG")
    env.pop("PYSC_COPYRIGHT_MESSAGE", None)
    result = subprocess.run(
        command, cwd=tmp_path, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60
    )
    assert result.returncode == 0, result.stdout
    banner = f"pysystemc {version('pysystemc')}"
    assert result.stdout.index(banner) < result.stdout.index("SystemC 2.3.4-Accellera")
    assert "\x1b[" in result.stdout
    assert "[pysystemc:C++:DEBUG]" in result.stdout
    env.update(PYSC_COPYRIGHT_MESSAGE="DISABLE", PYSYSTEMC_COLOR="never")
    result = subprocess.run(
        command, cwd=tmp_path, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60
    )
    assert result.returncode == 0, result.stdout
    assert banner not in result.stdout
    assert "SystemC 2.3.4-Accellera" in result.stdout
    assert "\x1b[" not in result.stdout


def test_cpp_runner_kernel_banner(tmp_path):
    """Native startup prints the kernel banner and honors its suppression flag."""
    command = [
        sys.executable,
        str(ROOT / "examples/run_sysc_cpp.py"),
        str(ROOT / "examples/helloworld/cpp/helloworld.cpp"),
    ]
    env = dict(os.environ)
    env.pop("SYSTEMC_DISABLE_COPYRIGHT_MESSAGE", None)
    env.pop("SC_COPYRIGHT_MESSAGE", None)
    result = subprocess.run(
        command, cwd=tmp_path, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60
    )
    assert result.returncode == 0, result.stdout
    assert "[pysystemc:C++:info]" in result.stdout
    assert "Reading " in result.stdout
    assert result.stdout.index("SystemC 2.3.4-Accellera") < result.stdout.index("Hello world using approach 1")
    env["SC_COPYRIGHT_MESSAGE"] = "DISABLE"
    result = subprocess.run(
        command, cwd=tmp_path, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60
    )
    assert result.returncode == 0, result.stdout
    assert "SystemC 2.3.4-Accellera" not in result.stdout
    assert "Hello world using approach 2" in result.stdout


def test_native_namespaces_fixed_point_and_tlm(tmp_path):
    """Check exports plus native quantization and transaction payload semantics."""
    output = run_example(tmp_path, "tests/models/namespaces.py")
    assert "NAMESPACES_OK" in output


def test_toy_event_cancellation(tmp_path):
    """A cancelled timed notification never invokes the method observer."""
    assert "EVENT_CANCEL_OK" in run_example(tmp_path, "examples/toy/event_cancel.py")


def test_toy_mixed_native_gate(tmp_path):
    """Native ports connected to Python-owned channels preserve gate behavior."""
    assert "MIXED_GATE_OK" in run_example(tmp_path, "examples/toy/mixed_gate.py")


def test_toy_trace_closes_after_exception(tmp_path):
    """A context-managed trace flushes transitions even when its body fails."""
    output = run_example(tmp_path, "examples/toy/trace_cleanup.py")
    assert "TRACE_CLEANUP_OK" in output
    assert (tmp_path / "cleanup.vcd").is_file()


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


def test_toy_adder_width_and_overflow(tmp_path):
    """Check combinational port propagation and native width truncation."""
    assert "ADDER_OK" in run_example(tmp_path, "examples/toy/adder.py")


def test_toy_up_counter_reset_enable_wrap(tmp_path):
    """Check positive-edge synchronous reset, enable hold and eight-bit wrap."""
    assert "UP_COUNTER_OK" in run_example(tmp_path, "examples/toy/up_counter.py")


def test_documented_api_errors_and_cleanup(tmp_path):
    """Invalid callbacks fail clearly and repeated cleanup remains safe."""
    assert "API_ERRORS_OK" in run_example(tmp_path, "tests/models/api_errors.py")


def test_main_missing_callable_reports_error(tmp_path):
    """Native entry failure propagates a nonzero status and useful diagnostic."""
    result = subprocess.run(
        [sys.executable, "-c", "from amal.eda.systemc import main; main()"],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode != 0
    assert "sc_main" in result.stderr and "Python sc_main failed" in result.stderr


def test_spawn_options_and_static_wait(tmp_path):
    """Configured dynamic sensitivity and zero-argument waits execute correctly."""
    assert "PROCESS_OPTIONS_OK" in run_example(tmp_path, "tests/models/process_options.py")


def test_module_identity_lifecycle_and_registration(tmp_path):
    """Native namespace stays intact and independent hosts dispatch in order."""
    assert "MODULE_ISOLATION_OK" in run_example(tmp_path, "tests/models/module_isolation.py")


def test_method_exception_reports_failure(tmp_path):
    """A Python method failure exits with diagnostics instead of hanging."""
    code = """
from amal.eda.systemc import main, sc_core as sc
class Broken(sc.sc_module):
    def __init__(self, name):
        super().__init__(name)
        self.sc_method("fail", self.fail)
    def fail(self):
        raise ValueError("method_failure_marker")
def sc_main(args):
    model = Broken("broken")
    sc.sc_start(1, sc.SC_NS)
    return 0
main()
"""
    result = subprocess.run([sys.executable, "-c", code], cwd=tmp_path, capture_output=True, text=True, timeout=60)
    assert result.returncode != 0
    assert "method_failure_marker" in result.stderr


def test_entry_status_arguments_and_reference_cleanup(tmp_path):
    """Reject overflowing statuses and release callables on success and errors."""
    assert "ENTRY_ERRORS_OK" in run_example(tmp_path, "tests/models/entry_errors.py")


def test_method_static_sensitivity_and_initialization(tmp_path):
    """Compare interface, port, event and edge sensitivity with native C++ after GC."""
    output = run_example(tmp_path, "tests/models/method_sensitivity.py")
    assert "METHOD_SENSITIVITY_OK" in output
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/method_sensitivity.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    python_records = sorted(line for line in output.splitlines() if line.startswith("RECORD "))
    native_records = sorted(line for line in native.stdout.splitlines() if line.startswith("RECORD "))
    assert len(python_records) == 7
    assert python_records == native_records


def test_registration_failure_releases_callbacks(tmp_path):
    """Failed conversions retain no callback and do not add native processes."""
    assert "REGISTRATION_FAILURE_OK" in run_example(tmp_path, "tests/models/registration_failure.py")


def test_callback_retained_through_kernel_stop(tmp_path):
    """Module-owned callable survives temporary reference GC and kernel stop."""
    assert "CALLBACK_LIFETIME_OK" in run_example(tmp_path, "tests/models/callback_lifetime.py")


def test_lifecycle_exceptions_report_useful_failures(tmp_path):
    """All four lifecycle exceptions leave entry without native abort or hang."""
    for hook in (
        "before_end_of_elaboration",
        "end_of_elaboration",
        "start_of_simulation",
        "end_of_simulation",
    ):
        result = subprocess.run(
            [sys.executable, str(ROOT / "tests/models/lifecycle_failure.py"), hook],
            cwd=tmp_path,
            capture_output=True,
            text=True,
            timeout=60,
        )
        assert result.returncode > 0, result.stdout + result.stderr
        assert f"lifecycle_failure_marker:{hook}" in result.stderr
        assert "segmentation violation" not in result.stderr
        assert "F539" not in result.stderr


def test_shutdown_releases_module_and_callback_owners(tmp_path):
    """Normal subprocess exit destroys module cycles and standalone callbacks."""
    marker = tmp_path / "released.txt"
    result = subprocess.run(
        [sys.executable, str(ROOT / "tests/models/shutdown_release.py"), str(marker)],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert "SHUTDOWN_READY" in result.stdout
    assert "Exception ignored" not in result.stderr
    records = marker.read_text().splitlines()
    assert records.count("callback") == 16, records
    assert records.count("module") == 16, records


def test_thread_event_lists_clocked_wait_and_pause(tmp_path):
    """Compare thread wake times, clock cycle waits and pause/resume with native C++."""
    output = run_example(tmp_path, "tests/models/thread_semantics.py")
    assert "THREAD_SEMANTICS_OK" in output
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/thread_semantics.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    records = sorted(line for line in output.splitlines() if line.startswith("THREAD_RECORD "))
    assert len(records) == 6
    assert records == sorted(line for line in native.stdout.splitlines() if line.startswith("THREAD_RECORD "))


def test_experimental_reset_kill_unwind_feasibility(tmp_path):
    """Preserved unwind restarts at 3 ns and terminates from a native controller."""
    assert "UNWIND_PROBE_OK" in run_example(tmp_path, "tests/models/unwind_feasibility.py")


def test_reset_kill_wait_families_and_cleanup(tmp_path):
    """Repeated resets and kill preserve Python finally blocks across wait families."""
    assert "RESET_WAITS_OK" in run_example(tmp_path, "tests/models/reset_waits.py")


def test_self_reset_kill_executes_cleanup(tmp_path):
    """Self control restores scheduler unwind and executes Python finally blocks."""
    assert "SELF_CONTROL_OK" in run_example(tmp_path, "tests/models/self_control.py")


def test_entry_arguments_and_simulation_lifecycle(tmp_path):
    """Entry preserves argv while bounded starts and terminal hooks follow native state."""
    result = subprocess.run(
        [sys.executable, str(ROOT / "tests/models/entry_lifecycle.py"), "café", "--value=17"],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert "ENTRY_LIFECYCLE_OK" in result.stdout


def test_method_dynamic_event_and_timed_sensitivity(tmp_path):
    """Method next_trigger handles event expressions and timed reactivation."""
    assert "DYNAMIC_METHOD_OK" in run_example(tmp_path, "tests/models/dynamic_method.py")


def test_modeling_datatype_and_vcd_matrix(tmp_path):
    """Check native channel updates, ports, datatype assignment and trace transitions."""
    output = run_example(tmp_path, "tests/models/modeling_matrix.py")
    assert "MODELING_MATRIX_OK" in output
    native = subprocess.run(
        [sys.executable, str(ROOT / "examples/run_sysc_cpp.py"), str(ROOT / "tests/native/modeling_matrix.cpp")],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert [line for line in output.splitlines() if line.startswith("MATRIX_")] == [
        line for line in native.stdout.splitlines() if line.startswith("MATRIX_")
    ]
    python_trace = (tmp_path / "matrix.vcd").read_text()
    native_trace = (tmp_path / "native_matrix.vcd").read_text()
    assert python_trace[python_trace.index("$timescale") :] == native_trace[native_trace.index("$timescale") :]


def test_required_port_binding_diagnostic(tmp_path):
    """Unbound required input reports its hierarchical name without native abort."""
    assert "BINDING_ERROR_OK" in run_example(tmp_path, "tests/models/binding_error.py")


def test_datatype_mutation_and_typed_ports(tmp_path):
    """Verify overflow, bit/range writes, conversion and arbitrary typed port copies."""
    assert "DATATYPE_OPERATIONS_OK" in run_example(tmp_path, "tests/models/datatype_operations.py")


def test_trace_setup_and_output_failure_cleanup(tmp_path):
    """Trace setup/output errors preserve diagnostics and permit a subsequent valid trace."""
    assert "TRACE_FAILURE_OK" in run_example(tmp_path, "tests/models/trace_failure.py")


def test_hierarchy_children_and_status_helpers(tmp_path):
    """Native hierarchy paths and helper output preserve child module ownership."""
    assert "HIERARCHY_CHECKS_OK" in run_example(tmp_path, "tests/models/hierarchy_checks.py")


def test_version_time_registry_and_introspection_apis(tmp_path):
    """Check post-2.3.1 time, registry, RTTI, never-event and initialized-signal behavior."""
    assert "VERSION_APIS_OK" in run_example(tmp_path, "tests/models/version_apis.py")

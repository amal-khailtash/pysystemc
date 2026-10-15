"""Validate SystemC lifecycle in isolated subprocesses."""

import subprocess
import sys

from .support import ROOT, run_example


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


def test_source_event_wait_and_termination_corner_cases(tmp_path):
    """Verify current-source empty-list/count errors and posttermination events."""
    assert "SOURCE_CORNER_CASES_OK" in run_example(tmp_path, "tests/models/source_corner_cases.py")

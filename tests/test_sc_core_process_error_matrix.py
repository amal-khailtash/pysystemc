"""Assert process-control restriction reports match native SystemC."""

import subprocess
import sys

from .support import ROOT, run_example


def test_sc_core_process_error_matrix(tmp_path):
    """Compare native and Python report identities, messages and severity."""
    output = run_example(tmp_path, "tests/models/sc_core_process_error_matrix.py")
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_process_error_matrix.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    actual = [line for line in output.splitlines() if line.startswith("PROCESS_ERROR")]
    expected = [line for line in native.stdout.splitlines() if line.startswith("PROCESS_ERROR")]
    assert actual[:-1] == expected
    assert actual[-1] == (
        "PROCESS_ERROR python_exception_type TypeError | throw_it requires a native C++ exception value"
    )
    assert [line.split()[1] for line in expected] == [
        "throw_before_simulation",
        "throw_while_paused",
        "empty_throw",
        "method_throw",
        "throw_terminated_thread",
        "suspend_reset_method",
        "resume_disabled_suspended_method",
        "throw_after_stop",
    ]

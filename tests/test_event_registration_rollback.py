"""Check event-list registration rollback against native SystemC controls."""

import os
import subprocess
import sys

import pytest

from .support import ROOT, run_example


CASES = [
    (process_kind, expression_kind, timed, "full")
    for process_kind, expressions in (
        ("thread", ("OR", "AND")),
        ("cthread", ("OR", "AND")),
        ("method", ("OR", "AND")),
    )
    for expression_kind in expressions
    for timed in ("untimed", "timed")
]
CASES.extend(
    [
        ("cthread", "AND", "timed", "partial"),
        ("method", "OR", "timed", "partial"),
    ]
)


@pytest.mark.parametrize(
    ("process_kind", "expression_kind", "timed_arg", "state_arg"),
    CASES,
    ids=[f"{process}-{expression.lower()}-{timed}-{state}" for process, expression, timed, state in CASES],
)
def test_event_registration_rollback(tmp_path, process_kind, expression_kind, timed_arg, state_arg):
    """Require synthetic rollback cleanup and normal native winner controls.

    Each model and native reference runs in its own child process because the
    SystemC kernel is a process-wide singleton. A native CTHREAD rejection is
    reported as a failing reference case instead of being silently skipped.
    Two additional cases remove one event member after full registration, then
    inject failure to check cleanup from a synthetic partial state. They are
    not allocator-failure simulations.
    """
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/event_registration_rollback.cpp"),
            "--",
            process_kind,
            expression_kind,
            timed_arg,
        ],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, (
        f"native successful {process_kind}/{expression_kind}/{timed_arg} control failed or rejected the case:\n"
        f"{native.stdout}{native.stderr}"
    )
    output = run_example(
        tmp_path,
        "tests/models/event_registration_rollback.py",
        (process_kind, expression_kind, timed_arg, state_arg),
    )

    python_prefix = "EVENT_REGISTRATION_ROLLBACK "
    native_prefix = "EVENT_REGISTRATION_ROLLBACK_NATIVE "
    python_records = [line for line in output.splitlines() if line.startswith(python_prefix)]
    native_records = [line for line in native.stdout.splitlines() if line.startswith(native_prefix)]
    assert len(python_records) == 1, output
    assert len(native_records) == 1, native.stdout
    event_ns = 1 if expression_kind == "OR" else 2
    timeout_ns = 3
    event_time = f"{event_ns} ns"
    expected_native = (
        f"{native_prefix}{process_kind} {expression_kind} {timed_arg} event={event_time} timeout={timeout_ns} ns"
    )
    assert native_records == [expected_native], native.stdout + native.stderr

    recovery_ns = 5 if process_kind == "cthread" else 4
    quiet_ns = 8 if process_kind == "cthread" else 7
    expected_python = (
        f"{python_prefix}{process_kind} {expression_kind} {timed_arg} {state_arg} "
        f"recovery={recovery_ns} ns quiet={quiet_ns} ns deletions=1 "
        f"injected=1 timeouts_staged={int(timed_arg == 'timed')} "
        "staged_timeout_callbacks=0 legitimate_timeout_callbacks=1"
    )
    assert python_records == [expected_python], output

"""Check lifecycle exception diagnostics, ordering and native state per hook."""

import os
import subprocess
import sys

import pytest

from .support import ROOT


@pytest.mark.parametrize(
    ("hook", "expected"),
    (
        (
            "before_end_of_elaboration",
            "hooks=before_end_of_elaboration process=none error=ValueError "
            "diagnostic=lifecycle_metadata_marker:before_end_of_elaboration after=2/0 final=2/0",
        ),
        (
            "end_of_elaboration",
            "hooks=before_end_of_elaboration,end_of_elaboration process=none error=ValueError "
            "diagnostic=lifecycle_metadata_marker:end_of_elaboration after=4/0 final=4/0",
        ),
        (
            "start_of_simulation",
            "hooks=before_end_of_elaboration,end_of_elaboration,start_of_simulation process=none error=ValueError "
            "diagnostic=lifecycle_metadata_marker:start_of_simulation after=8/0 final=8/0",
        ),
        (
            "end_of_simulation",
            "hooks=before_end_of_elaboration,end_of_elaboration,start_of_simulation,end_of_simulation "
            "process=0/0 error=ValueError diagnostic=lifecycle_metadata_marker:end_of_simulation "
            "after=32/1000 final=128/1000 stop_repeat_reports=0",
        ),
    ),
)
def test_lifecycle_failure_metadata_and_process_order(tmp_path, hook, expected):
    """Pin each failing hook's native status, time, process order and diagnostic."""
    env = dict(os.environ)
    env.setdefault("SYSTEMC_HOME", str(ROOT / "submodules/systemc"))
    result = subprocess.run(
        [sys.executable, str(ROOT / "tests/models/sc_core_lifecycle_error_metadata.py"), hook],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert f"LIFECYCLE_ERROR_METADATA {hook} {expected}" in result.stdout
    assert "W545" not in result.stdout + result.stderr
    assert "Exception ignored" not in result.stderr

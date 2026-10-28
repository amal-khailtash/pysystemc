"""Compare Python creator failure and ownership with native vector rollback."""

import subprocess
import sys

import pytest

from .support import ROOT, run_example


@pytest.mark.parametrize("scenario", ("failure", "invalid"))
def test_sc_core_vector_creator_public(tmp_path, scenario):
    """Assert partial cleanup, retry, borrowed proxy lifetime and final destruction."""
    actual = run_example(tmp_path, "tests/models/sc_core_vector_creator_public.py", (scenario,))
    native = subprocess.run(
        [
            sys.executable,
            str(ROOT / "examples/run_sysc_cpp.py"),
            str(ROOT / "tests/native/sc_core_vector_creator_public.cpp"),
        ],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
    )
    assert native.returncode == 0, native.stdout + native.stderr
    assert "VECTOR_CREATOR_OK 0 3 6" in actual and "VECTOR_CREATOR_OK 0 3 6" in native.stdout

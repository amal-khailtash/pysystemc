"""Check tracing rejects or warns on unsupported values without sampling them."""

import os
import subprocess
import sys

from .support import ROOT


def test_trace_rejects_generic_readable_without_sampling(tmp_path):
    """Unsupported readers stay untouched; an empty FIFO trace returns promptly."""
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [sys.executable, str(ROOT / "tests/models/f8_core_trace_read_side_effect.py")],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )
    output = result.stdout + result.stderr
    assert result.returncode == 0, output
    assert "TRACE_READ_SIDE_EFFECT 0 FILLED_FIFO 1 23 EMPTY_FIFO_EMPTY" in output
    assert output.count("(W710)") == 2, output

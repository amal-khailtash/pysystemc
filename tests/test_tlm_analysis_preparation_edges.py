"""Exercise the native late-preparation edge for analysis FIFOs."""

import os
import subprocess
import sys

from .support import ROOT


def test_raw_fifo_instantiation_preserves_facade_failure_and_other_payload_recovers(tmp_path):
    """Retain cppyy's native failure cause and verify a fresh payload still simulates."""
    env = dict(os.environ, SYSTEMC_HOME=os.environ.get("SYSTEMC_HOME", str(ROOT / "submodules/systemc")))
    result = subprocess.run(
        [sys.executable, str(ROOT / "tests/models/tlm_analysis_preparation_edges.py")],
        cwd=tmp_path,
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
    )

    assert result.returncode == 0, result.stdout + result.stderr
    assert "TLM_ANALYSIS_LATE_FAILURE SyntaxError True" in result.stdout
    assert "TLM_ANALYSIS_RECOVERY 6 ns 1 57 1 88" in result.stdout
    assert "TLM_ANALYSIS_PREPARATION_EDGES_OK" in result.stdout
    assert "explicit qualification required to use member 'nb_put'" in result.stderr

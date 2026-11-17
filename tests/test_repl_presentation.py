"""Fresh-process validation for public-name REPL completion."""

import json
import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]


def test_repl_completion_uses_public_namespace_policy(tmp_path: Path) -> None:
    """Check real REPL completion, aliases, hidden bridge names, and templates."""
    env = dict(os.environ)
    env.setdefault("SYSTEMC_HOME", str(ROOT / "submodules/systemc"))
    env["COVERAGE_FILE"] = str(tmp_path.resolve() / "coverage.repl_presentation")
    result = subprocess.run(
        [sys.executable, str(ROOT / "tests/models/repl_presentation.py")],
        cwd=ROOT,
        capture_output=True,
        text=True,
        timeout=90,
        env=env,
        check=False,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    report_line = next(line for line in result.stdout.splitlines() if line.startswith("REPL_PRESENTATION_OK "))
    report = json.loads(report_line.removeprefix("REPL_PRESENTATION_OK "))
    assert report["public_paths"] == [
        "sc_core.sc_time",
        "sc_dt.sc_context",
        "tlm.tlm_analysis_fifo",
        "tlm_utils.simple_target_socket",
    ]
    assert report["hidden_bridge_names"] == [
        "python_wait",
        "python_member_wait",
        "PythonInterface",
        "guarded_wait",
        "python_member_wait_call",
        "python_member_next_trigger_call",
    ]
    assert report["template_cpp_names"] == ["sc_dt::sc_uint<73>", "sc_dt::sc_biguint<73>"]

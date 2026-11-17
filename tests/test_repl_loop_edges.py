"""Verify interactive REPL recovery and exit paths in a fresh process."""

import os
from pathlib import Path
import subprocess
import sys


REPO_ROOT = Path(__file__).resolve().parents[1]


def test_repl_loop_edges_recover_and_exit_cleanly(tmp_path: Path) -> None:
    """Run real Repl loops and assert recovery, output, and SystemExit behavior."""
    model = REPO_ROOT / "tests/models/repl_loop_edges.py"
    history_root = tmp_path / "xdg-state"
    environment = dict(os.environ)
    command = [sys.executable, str(model), str(history_root)]
    result = subprocess.run(
        command,
        cwd=REPO_ROOT,
        env=environment,
        capture_output=True,
        text=True,
        timeout=60,
    )

    assert result.returncode == 0, result.stdout + result.stderr
    assert "RECOVERED_AFTER_SYNTAX 41" in result.stdout
    assert "RECOVERED_AFTER_INTERRUPT 41" in result.stdout
    assert "MAIN_ENTRY_REACHED" in result.stdout
    assert "AFTER_SYSTEM_EXIT" not in result.stdout
    assert result.stdout.count("KeyboardInterrupt") == 1
    assert "SyntaxError" in result.stderr
    assert "KeyboardInterrupt" in result.stderr
    assert (history_root / "pysystemc").is_dir()
    assert (history_root / "history-parent-is-a-file").read_text(encoding="utf-8") == "keep this temporary test file"

"""Verify conservative CI selection, including renames out of runtime code."""

import runpy
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
SELECT = runpy.run_path(str(ROOT / "tools/select_ci.py"))["select_mode"]


@pytest.mark.parametrize(
    ("paths", "expected"),
    [
        (["docs/guide.rst", "README.md"], "docs"),
        (["CHANGELOG.md"], "docs"),
        ([], "full"),
        (["docs/guide.rst", "src/runtime.py"], "full"),
        (["docs/guide.rst", "uv.lock"], "full"),
        ([".github/workflows/ci.yml"], "full"),
        (["tools/select_ci.py"], "full"),
        (["tests/test_api.py"], "full"),
    ],
)
def test_selection(paths: list[str], expected: str) -> None:
    """Only a nonempty set of documentation paths may skip full validation."""
    assert SELECT(paths) == expected


def test_runtime_rename_requires_full(tmp_path: Path) -> None:
    """Moving runtime code into docs must retain full CI despite rename detection."""

    def git(*args: str) -> str:
        """Execute Git within the disposable test repository."""
        return subprocess.check_output(["git", "-C", str(tmp_path), *args], text=True).strip()

    git("init", "-q")
    git("config", "user.name", "CI test")
    git("config", "user.email", "ci@example.invalid")
    (tmp_path / "src").mkdir()
    (tmp_path / "src/runtime.py").write_text("print('runtime')\n")
    git("add", ".")
    git("commit", "-qm", "base")
    base = git("rev-parse", "HEAD")
    (tmp_path / "docs").mkdir()
    git("mv", "src/runtime.py", "docs/runtime.py")
    git("commit", "-qm", "rename")
    result = subprocess.check_output(
        [sys.executable, str(ROOT / "tools/select_ci.py"), "--base", base, "--head", "HEAD"], cwd=tmp_path, text=True
    ).strip()
    assert result == "full"

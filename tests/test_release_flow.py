"""Tests for release metadata validation against temporary Git histories."""

import json
import runpy
import subprocess
from pathlib import Path
from collections.abc import Callable

import pytest


_CHECKER = Path(__file__).resolve().parents[1] / "tools" / "check_release_metadata.py"
_VALIDATE_RELEASE: Callable[[str, str], str] = runpy.run_path(str(_CHECKER))["validate_release"]


def _git(repo: Path, *args: str) -> str:
    """Run a Git command in a test repository and return its output."""
    return subprocess.check_output(["git", "-C", str(repo), *args], text=True).strip()


def _release_repo(repo: Path, head_changes: dict[str, str]) -> tuple[str, str]:
    """Create base and proposed release commits with optional head file edits."""
    repo.mkdir()
    _git(repo, "init", "-q")
    _git(repo, "config", "user.name", "Release test")
    _git(repo, "config", "user.email", "release-test@example.invalid")

    files = {
        "pyproject.toml": ('[project]\nname = "pysystemc"\nversion = "0.1.0"\ndependencies = ["cppyy>=3"]\n'),
        "uv.lock": (
            'version = 1\nrevision = 3\nrequires-python = ">=3.14"\n\n'
            '[[package]]\nname = "cppyy"\nversion = "3.1.2"\n'
            'source = { registry = "https://pypi.org/simple" }\n\n'
            '[[package]]\nname = "pysystemc"\nversion = "0.1.0"\n'
            'source = { editable = "." }\n'
        ),
        ".release-please-manifest.json": json.dumps({".": "0.1.0"}) + "\n",
        "CHANGELOG.md": "## [0.1.0]\n\nInitial release.\n",
        "src/pysystemc.py": '"""Minimal source fixture."""\n',
    }
    for name, contents in files.items():
        target = repo / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(contents, encoding="utf-8")
    _git(repo, "add", ".")
    _git(repo, "commit", "-qm", "base release")
    base = _git(repo, "rev-parse", "HEAD")

    for name, contents in head_changes.items():
        target = repo / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(contents, encoding="utf-8")
    _git(repo, "add", ".")
    _git(repo, "commit", "-qm", "proposed release")
    head = _git(repo, "rev-parse", "HEAD")
    return base, head


def _valid_changes() -> dict[str, str]:
    """Return the expected metadata-only version bump to 0.1.1."""
    return {
        "pyproject.toml": ('[project]\nname = "pysystemc"\nversion = "0.1.1"\ndependencies = ["cppyy>=3"]\n'),
        "uv.lock": (
            'version = 1\nrevision = 3\nrequires-python = ">=3.14"\n\n'
            '[[package]]\nname = "cppyy"\nversion = "3.1.2"\n'
            'source = { registry = "https://pypi.org/simple" }\n\n'
            '[[package]]\nname = "pysystemc"\nversion = "0.1.1"\n'
            'source = { editable = "." }\n'
        ),
        ".release-please-manifest.json": json.dumps({".": "0.1.1"}) + "\n",
        "CHANGELOG.md": "## [0.1.1]\n\nRelease notes.\n",
    }


def test_valid_metadata_only_version_bump_passes(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    """Accept a version bump when project, lock, manifest and changelog agree."""
    base, head = _release_repo(tmp_path / "repo", _valid_changes())
    monkeypatch.chdir(tmp_path / "repo")

    assert _VALIDATE_RELEASE(base, head) == "0.1.1"


def test_stale_local_project_lock_version_fails(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    """Reject a bump whose editable project version was not updated in uv.lock."""
    changes = _valid_changes()
    changes["uv.lock"] = changes["uv.lock"].replace(
        'name = "pysystemc"\nversion = "0.1.1"',
        'name = "pysystemc"\nversion = "0.1.0"',
    )
    base, head = _release_repo(tmp_path / "repo", changes)
    monkeypatch.chdir(tmp_path / "repo")

    with pytest.raises(ValueError, match="lockfile"):
        _VALIDATE_RELEASE(base, head)


@pytest.mark.parametrize("changed_file", ["pyproject.toml", "uv.lock"])
def test_dependency_changes_fail(changed_file: str, tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    """Reject dependency changes in project metadata and the resolved lock."""
    changes = _valid_changes()
    if changed_file == "pyproject.toml":
        changes[changed_file] = changes[changed_file].replace("cppyy>=3", "cppyy>=4")
    else:
        changes[changed_file] = changes[changed_file].replace(
            'name = "cppyy"\nversion = "3.1.2"',
            'name = "cppyy"\nversion = "3.1.3"',
        )
    base, head = _release_repo(tmp_path / "repo", changes)
    monkeypatch.chdir(tmp_path / "repo")

    with pytest.raises(ValueError, match="changes project settings|lockfile"):
        _VALIDATE_RELEASE(base, head)


def test_source_file_changes_fail(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> None:
    """Reject a release commit that includes source changes."""
    changes = _valid_changes()
    changes["src/pysystemc.py"] = '"""Changed source fixture."""\n'
    base, head = _release_repo(tmp_path / "repo", changes)
    monkeypatch.chdir(tmp_path / "repo")

    with pytest.raises(ValueError, match="unexpected changes"):
        _VALIDATE_RELEASE(base, head)


@pytest.mark.parametrize("inconsistency", ["manifest", "changelog"])
def test_manifest_or_changelog_inconsistency_fails(
    inconsistency: str, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Reject release metadata when the manifest or changelog misses the new version."""
    changes = _valid_changes()
    if inconsistency == "manifest":
        changes[".release-please-manifest.json"] = json.dumps({".": "0.1.0"}) + "\n"
    else:
        changes["CHANGELOG.md"] = "## [0.1.0]\n\nNo release entry.\n"
    base, head = _release_repo(tmp_path / "repo", changes)
    monkeypatch.chdir(tmp_path / "repo")

    with pytest.raises(ValueError, match="manifest|Changelog"):
        _VALIDATE_RELEASE(base, head)

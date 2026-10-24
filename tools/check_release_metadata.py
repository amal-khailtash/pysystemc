"""Validate that a release PR changes only version metadata and its changelog."""

import argparse
import copy
import json
import re
import subprocess
import tomllib


ALLOWED = {"pyproject.toml", "uv.lock", ".release-please-manifest.json", "CHANGELOG.md"}


def read_revision(revision: str, path: str) -> bytes:
    """Read a tracked file at a Git revision without changing the checkout."""
    return subprocess.check_output(["git", "show", f"{revision}:{path}"])


def normalized_lock(document: dict, name: str) -> tuple[dict, str]:
    """Remove only the local project's version, returning its prior value."""
    result = copy.deepcopy(document)
    packages = [package for package in result["package"] if package["name"] == name]
    if len(packages) != 1 or packages[0].get("source") != {"editable": "."}:
        raise ValueError("Lockfile must contain exactly one editable project")
    return result, packages[0].pop("version")


def validate_release(base: str, head: str) -> str:
    """Return the release version after checking both Git trees.

    Args:
        base: Base revision of the release PR.
        head: Proposed release revision.

    Returns:
        The new version shared by project metadata, lockfile and manifest.

    Raises:
        ValueError: Code, dependencies or inconsistent release metadata changed.
        subprocess.CalledProcessError: A revision or required file is missing.
    """
    changed = set(subprocess.check_output(["git", "diff", "--name-only", base, head], text=True).splitlines())
    if not changed <= ALLOWED or "pyproject.toml" not in changed:
        raise ValueError(f"Release PR contains unexpected changes: {sorted(changed)}")
    old = tomllib.loads(read_revision(base, "pyproject.toml").decode())
    new = tomllib.loads(read_revision(head, "pyproject.toml").decode())
    name = new["project"]["name"]
    old_version = old["project"].pop("version")
    version = new["project"].pop("version")
    if old != new:
        raise ValueError("Release PR changes project settings or dependencies")
    pattern = r"\d+\.\d+\.\d+(?:(?:a|b|rc)\d+)?"
    if not re.fullmatch(pattern, version) or version == old_version:
        raise ValueError("Release PR must select a new semantic package version")
    locks = [normalized_lock(tomllib.loads(read_revision(ref, "uv.lock").decode()), name) for ref in (base, head)]
    if locks[0][0] != locks[1][0] or [lock[1] for lock in locks] != [old_version, version]:
        raise ValueError("Release lockfile changes dependencies or has an inconsistent version")
    manifests = [json.loads(read_revision(ref, ".release-please-manifest.json")) for ref in (base, head)]
    if manifests != [{".": old_version}, {".": version}]:
        raise ValueError("Release manifest does not match the project version")
    changelog = read_revision(head, "CHANGELOG.md").decode()
    if f"## [{version}]" not in changelog:
        raise ValueError("Changelog has no heading for the release")
    return version


def main() -> None:
    """Validate revisions supplied by CI; fail before any publication occurs."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", required=True)
    args = parser.parse_args()
    print(f"Release metadata validated: {validate_release(args.base, args.head)}")


if __name__ == "__main__":
    main()

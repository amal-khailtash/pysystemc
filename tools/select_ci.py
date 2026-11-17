"""Select docs-only validation conservatively from complete Git tree differences."""

import argparse
import subprocess


DOC_FILES = {"README.md", "RELEASING.md", "CHANGELOG.md", "LICENSE", ".readthedocs.yaml", ".readthedocs.yml"}


def select_mode(paths: list[str]) -> str:
    """Return docs only for a nonempty set containing exclusively documentation paths."""
    return "docs" if paths and all(path.startswith("docs/") or path in DOC_FILES for path in paths) else "full"


def main() -> None:
    """Classify both sides of renames; missing or invalid revisions fail CI."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", required=True)
    parser.add_argument("--head", required=True)
    args = parser.parse_args()
    paths = subprocess.check_output(["git", "diff", "--no-renames", "--name-only", "-z", args.base, args.head])
    print(select_mode(paths.decode().rstrip("\0").split("\0") if paths else []))


if __name__ == "__main__":
    main()

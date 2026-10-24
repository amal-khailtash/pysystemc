"""Verify that the package index exposes every release artifact with its exact hash."""

import argparse
import hashlib
import json
from pathlib import Path
import time
from urllib.error import HTTPError, URLError
from urllib.parse import quote
from urllib.request import urlopen


def verify_files(files: list[Path], published: list[dict]) -> None:
    """Check artifact names, SHA-256 hashes and availability against index metadata.

    Args:
        files: Locally built distributions that were uploaded.
        published: File records returned by the package index JSON API.

    Raises:
        ValueError: An artifact is missing, has different bytes, or is yanked.
    """
    records = {record["filename"]: record for record in published}
    if not files:
        raise ValueError("No release distributions found")
    for path in files:
        record = records.get(path.name)
        if record is None:
            raise ValueError(f"Package index does not expose {path.name}")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if record.get("yanked") or record.get("digests", {}).get("sha256") != digest:
            raise ValueError(f"Package index artifact differs or is yanked: {path.name}")


def main() -> None:
    """Verify an upload with bounded retries for package-index propagation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--repository", choices=("pypi", "testpypi"), default="pypi")
    parser.add_argument("--dist", type=Path, default=Path("dist"))
    args = parser.parse_args()
    host = "test.pypi.org" if args.repository == "testpypi" else "pypi.org"
    url = f"https://{host}/pypi/{quote(args.project, safe='')}/{quote(args.version, safe='')}/json"
    files = sorted([*args.dist.glob("*.whl"), *args.dist.glob("*.tar.gz")])
    for attempt in range(12):
        try:
            with urlopen(url, timeout=20) as response:
                metadata = json.load(response)
            if metadata["info"]["version"] != args.version:
                raise ValueError("Index version does not match release")
            verify_files(files, metadata["urls"])
            print(f"Verified {len(files)} published artifacts for {args.project} {args.version}")
            return
        except HTTPError as error:
            if error.code != 404 and error.code < 500:
                raise
            failure = error
        except (URLError, TimeoutError, ValueError) as error:
            failure = error
        if attempt == 11:
            raise SystemExit(f"Publication could not be verified: {failure}")
        time.sleep(5)


if __name__ == "__main__":
    main()

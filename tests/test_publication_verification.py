"""Check release artifact verification against package index records."""

import hashlib
from pathlib import Path
import runpy

import pytest

ROOT = Path(__file__).resolve().parents[1]
VERIFY_FILES = runpy.run_path(str(ROOT / "tools/verify_publication.py"))["verify_files"]


def _published_record(path: Path, *, yanked: bool = False) -> dict:
    """Build package index metadata matching a local artifact."""
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    return {
        "filename": path.name,
        "digests": {"sha256": digest},
        "yanked": yanked,
    }


def test_verify_files_accepts_multiple_artifacts_with_matching_hashes(tmp_path):
    """Accept a release when every wheel and source archive matches its index hash."""
    wheel = tmp_path / "pysystemc-1.2.3-py3-none-any.whl"
    source = tmp_path / "pysystemc-1.2.3.tar.gz"
    wheel.write_bytes(b"wheel artifact bytes")
    source.write_bytes(b"source artifact bytes")

    VERIFY_FILES([wheel, source], [_published_record(wheel), _published_record(source)])


def test_verify_files_rejects_missing_artifact(tmp_path):
    """Reject a local distribution absent from package index metadata."""
    artifact = tmp_path / "pysystemc-1.2.3.whl"
    artifact.write_bytes(b"wheel artifact bytes")

    with pytest.raises(ValueError, match="does not expose"):
        VERIFY_FILES([artifact], [])


def test_verify_files_rejects_wrong_hash(tmp_path):
    """Reject an index digest that does not match the local artifact bytes."""
    artifact = tmp_path / "pysystemc-1.2.3.whl"
    artifact.write_bytes(b"wheel artifact bytes")
    record = _published_record(artifact)
    record["digests"]["sha256"] = "0" * 64

    with pytest.raises(ValueError, match="differs or is yanked"):
        VERIFY_FILES([artifact], [record])


def test_verify_files_rejects_yanked_artifact(tmp_path):
    """Reject an artifact marked yanked even when its digest matches."""
    artifact = tmp_path / "pysystemc-1.2.3.whl"
    artifact.write_bytes(b"wheel artifact bytes")

    with pytest.raises(ValueError, match="differs or is yanked"):
        VERIFY_FILES([artifact], [_published_record(artifact, yanked=True)])


def test_verify_files_rejects_empty_release(tmp_path):
    """Reject publication verification when the local release has no artifacts."""
    with pytest.raises(ValueError, match="No release distributions found"):
        VERIFY_FILES([], [])

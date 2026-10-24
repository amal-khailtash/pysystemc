"""Tests for moving pending changelog details into generated releases."""

import re
import runpy
from collections.abc import Callable
from pathlib import Path

import pytest


_ROLLOVER: Callable[[str, str], str] = runpy.run_path(
    str(Path(__file__).resolve().parents[1] / "tools" / "prepare_release_notes.py")
)["roll_over"]


def test_pending_details_move_into_release_and_keep_history() -> None:
    """Move pending details while retaining generated notes and historical bytes."""
    preamble = "# Changelog\n\nRelease notes follow.\n\n"
    generated_heading = "## [1.2.3](https://example.invalid/compare/v1.2.2...v1.2.3)"
    generated_summary = "### Summary\n- Generated release summary."
    pending_details = "### Added\n- Pending feature."
    older_release = "## [1.2.2]\n\n### Fixed\n- Older fix.\n\n<!-- historical marker -->\n"
    changelog = (
        f"{preamble}## [Unreleased]\n\n{pending_details}\n\n"
        f"{generated_heading}\n\n{generated_summary}\n\n{older_release}"
    )

    result = _ROLLOVER(changelog, "1.2.3")
    headings = re.findall(r"^## \[[^\]]+\].*$", result, flags=re.MULTILINE)

    assert headings[0] == "## [Unreleased]"
    assert headings.count("## [Unreleased]") == 1
    assert result.startswith(preamble)
    assert generated_heading in result
    assert generated_summary in result
    assert pending_details in result
    assert result.index(generated_summary) < result.index(pending_details)
    assert result.count("- Pending feature.") == 1
    assert result.endswith(older_release)

    assert _ROLLOVER(result, "1.2.3") == result


@pytest.mark.parametrize(
    "changelog",
    [
        "## [1.2.2]\n\nOlder release.\n",
        "## [Unreleased]\n\nFirst pending section.\n\n"
        "## [1.2.3]\n\nGenerated release.\n\n"
        "## [Unreleased]\n\nSecond pending section.\n",
    ],
    ids=["missing-release-heading", "duplicate-unreleased"],
)
def test_invalid_release_sections_are_rejected(changelog: str) -> None:
    """Reject a missing requested release or multiple Unreleased sections."""
    with pytest.raises(ValueError):
        _ROLLOVER(changelog, "1.2.3")


def test_changelog_without_pending_section_gets_empty_unreleased() -> None:
    """Add an empty top Unreleased section when there are no pending details."""
    changelog = "# Changelog\n\n## [1.2.3]\n\nGenerated release summary.\n"

    result = _ROLLOVER(changelog, "1.2.3")
    headings = re.findall(r"^## \[[^\]]+\].*$", result, flags=re.MULTILINE)

    assert headings[0] == "## [Unreleased]"
    assert headings.count("## [Unreleased]") == 1
    assert "Generated release summary." in result
    assert result.count("Generated release summary.") == 1

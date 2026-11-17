"""Preserve friendship source-site evidence independently of declaration identity."""

from copy import deepcopy
from pathlib import Path
import runpy

_RECONCILE = runpy.run_path(str(Path(__file__).resolve().parents[1] / "tools/reconcile_api_inventory.py"))["reconcile"]


def _snapshot(configuration):
    """Return a minimal declaration inventory without friendship collection."""
    return {
        "schema_version": 4,
        "systemc_revision": "pinned",
        "configuration": [configuration],
        "headers": [],
        "macro_declarations": [],
        "declarations": [
            {
                "id": "target-id",
                "name": "ns::target",
                "source": "include/friend.hpp",
                "line": 5,
                "access": "public",
                "classification": "public",
                "status": "untested",
            }
        ],
    }


def _relationship(owner, targets):
    """Build one portable site with a configurable owner and resolution payload."""
    return {
        "occurrence_id": "site-id",
        "source_occurrence": {"source": "include/friend.hpp", "line": 5, "offset": 42},
        "owner": {"name": owner, "declaration_ids": ["owner-id"]},
        "target": {"kind": "FunctionDecl", "name": "ns::target"},
        "target_declaration_ids": targets,
        "resolution": "child_declaration" if targets else "target_filtered_or_outside_inventory",
    }


def test_friend_variants_preserve_sites_configurations_and_declarations():
    """Retain conflicting site payloads and missing collection without support changes."""
    snapshots = [_snapshot(name) for name in ["old", "empty", "modeling", "compatibility", "alternate"]]
    snapshots[1]["friend_relationships"] = []
    snapshots[2]["friend_relationships"] = [_relationship("ns::Host", ["target-id"])]
    snapshots[3]["friend_relationships"] = deepcopy(snapshots[2]["friend_relationships"]) * 2
    snapshots[4]["friend_relationships"] = [_relationship("ns::Other", [])]
    before = deepcopy(snapshots)
    union = _RECONCILE(snapshots)
    legacy = _RECONCILE(
        [{key: value for key, value in item.items() if key != "friend_relationships"} for item in snapshots]
    )
    assert {key: value for key, value in union.items() if not key.startswith("friend_relationship")} == legacy
    assert "friend_relationships" not in legacy
    assert union["friend_relationship_coverage"] == {
        "collected": [["empty"], ["modeling"], ["compatibility"], ["alternate"]],
        "not_collected": [["old"]],
    }
    sites = union["friend_relationships"]
    assert len(sites) == 1 and sites[0]["occurrence_id"] == "site-id"
    variants = sites[0]["semantic_variants"]
    assert len(variants) == 2
    assert variants[0]["configurations"] == [["modeling"], ["compatibility"]]
    assert variants[0]["target_declaration_ids"] == ["target-id"]
    assert variants[1]["configurations"] == [["alternate"]]
    assert variants[1]["target_declaration_ids"] == []
    assert snapshots == before
    variants[0]["owner"]["declaration_ids"].append("mutation")
    assert snapshots == before


def test_distinct_friend_sites_do_not_merge_by_shared_target():
    """Keep two class grants as separate occurrences even when their target agrees."""
    snapshot = _snapshot("modeling")
    first = _relationship("ns::Host", ["target-id"])
    second = _relationship("ns::Other", ["target-id"])
    second["occurrence_id"] = "second-site"
    second["source_occurrence"]["offset"] = 99
    snapshot["friend_relationships"] = [first, second]
    union = _RECONCILE([snapshot])
    assert len(union["friend_relationships"]) == 2
    assert len(union["declarations"]) == 1
    assert union["declarations"][0]["status"] == "untested"

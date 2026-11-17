"""Keep condition provenance separate from declaration and configuration identity."""

from copy import deepcopy
from pathlib import Path
import runpy

_RECONCILE = runpy.run_path(str(Path(__file__).resolve().parents[1] / "tools/reconcile_api_inventory.py"))["reconcile"]


def _snapshot(name):
    """Create one source inventory with no collected condition metadata."""
    return {
        "schema_version": 4,
        "systemc_revision": "pinned",
        "configuration": [name],
        "headers": [],
        "macro_declarations": [],
        "declarations": [
            {
                "id": "decl",
                "name": "ns::value",
                "source": "include/a.h",
                "line": 7,
                "access": "public",
                "classification": "public_candidate",
                "status": "untested",
            }
        ],
    }


def _condition(identifier, offset, context):
    """Create a source occurrence with a configurable textual branch history."""
    return {
        "occurrence_id": identifier,
        "declaration_id": "decl",
        "source_occurrence": {"source": "include/a.h", "line": 7, "offset": offset},
        "conditional_context": context,
        "location_kind": "written_source",
    }


def test_condition_variants_preserve_declarations_and_collection_coverage():
    """Keep branch differences, duplicate observations and older absence distinct."""
    snapshots = [_snapshot(n) for n in ["old", "empty", "modeling", "compatibility", "alternate"]]
    snapshots[1]["declaration_conditions"] = []
    first = _condition("site", 42, [["#ifndef GUARD"], ["#if FEATURE"]])
    second = _condition("site", 42, [["#ifndef GUARD"], ["#if FEATURE", "#else"]])
    snapshots[2]["declaration_conditions"] = [first]
    snapshots[3]["declaration_conditions"] = [deepcopy(first), deepcopy(first)]
    snapshots[4]["declaration_conditions"] = [second, _condition("other-offset", 99, [])]
    before = deepcopy(snapshots)
    union = _RECONCILE(snapshots)
    legacy = _RECONCILE([{k: v for k, v in s.items() if k != "declaration_conditions"} for s in snapshots])
    assert {k: v for k, v in union.items() if not k.startswith("declaration_condition")} == legacy
    assert union["declaration_condition_coverage"] == {
        "collected": [["empty"], ["modeling"], ["compatibility"], ["alternate"]],
        "not_collected": [["old"]],
    }
    sites = {s["occurrence_id"]: s for s in union["declaration_conditions"]}
    assert len(sites) == 2
    variants = sites["site"]["semantic_variants"]
    assert len(variants) == 2
    assert variants[0]["configurations"] == [["modeling"], ["compatibility"]]
    assert variants[1]["configurations"] == [["alternate"]]
    assert variants[1]["conditional_context"][1] == ["#if FEATURE", "#else"]
    assert snapshots == before
    variants[0]["conditional_context"][0].append("mutation")
    assert snapshots == before


def test_unlocated_condition_records_merge_only_identical_payloads():
    """Retain unavailable contexts without assigning a fabricated source identity."""
    first = _condition(None, None, None)
    first["unavailable_reason"] = "source_offset_unavailable"
    second = deepcopy(first)
    second["declaration_id"] = "different-decl"
    snapshots = [_snapshot("first"), _snapshot("second")]
    snapshots[0]["declaration_conditions"] = [first, second]
    snapshots[1]["declaration_conditions"] = [deepcopy(first)]
    sites = _RECONCILE(snapshots)["declaration_conditions"]
    assert len(sites) == 2 and all(s["occurrence_id"] is None for s in sites)
    first_site = next(s for s in sites if s["semantic_variants"][0]["declaration_id"] == "decl")
    assert first_site["semantic_variants"][0]["configurations"] == [["first"], ["second"]]
    assert first_site["semantic_variants"][0]["conditional_context"] is None

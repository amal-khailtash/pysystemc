"""Preserve configuration-specific declaration semantics during inventory union."""

import hashlib
import json
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[1]
_RECONCILE = runpy.run_path(str(ROOT / "tools/reconcile_api_inventory.py"))["reconcile"]


def _declaration(**overrides):
    """Build a schema-4 declaration with the semantic metadata extracted by Clang."""
    entry = {
        "id": "stable-id",
        "name": "ns::Owner::method",
        "kind": "CXXMethodDecl",
        "signature": "void (int)",
        "source": "include/owner.h",
        "line": 2,
        "offset": 12,
        "access": "protected",
        "static": False,
        "virtual": False,
        "pure": False,
        "bases": [],
        "parameters": [{"name": "value", "type": "int", "has_default": False}],
        "template_parameters": [{"name": "T", "kind": "TemplateTypeParmDecl", "type": None}],
        "template_arguments": [],
        "template_context": [{"name": "ns::Owner", "arguments": []}],
        "classification": "public_candidate",
        "status": "untested",
        "mapping_status": "unreviewed",
    }
    entry.update(overrides)
    return entry


def _snapshot(configuration, declarations):
    """Build a valid minimal schema-4 snapshot for one extraction configuration."""
    return {
        "schema_version": 4,
        "systemc_revision": "pinned-systemc",
        "configuration": configuration,
        "headers": [{"source": "include/owner.h"}],
        "macro_declarations": [],
        "declarations": declarations,
    }


def test_reconcile_retains_config_scoped_semantic_variants_and_isolates_sources():
    """Keep differing flags/defaults/access while coalescing equal configurations."""
    modeling = _declaration()
    modeling_duplicate = _declaration(line=5)
    baseline = _declaration(line=8)
    compatibility = _declaration(
        access="private",
        static=True,
        virtual=True,
        pure=True,
        bases=[{"type": {"qualType": "ns::Base"}, "access": "public", "isVirtual": False}],
        parameters=[{"name": "value", "type": "int", "has_default": True}],
        status="supported",
    )
    snapshots = [
        _snapshot(["modeling"], [modeling, modeling_duplicate]),
        _snapshot(["baseline"], [baseline]),
        _snapshot(["compatibility"], [compatibility]),
    ]
    original_json = json.dumps(snapshots, sort_keys=True)
    union = _RECONCILE(snapshots)
    row = union["declarations"][0]

    assert union["schema_version"] == 4
    assert row["id"] == modeling["id"]
    assert union["fingerprint"] == hashlib.sha256(b"stable-id").hexdigest()
    assert row["signature"] == compatibility["signature"]
    assert row["static"] is False and row["access"] == "protected"
    assert row["status"] == "untested" and row["classification"] == "public_candidate"
    assert "status" not in row["semantic_variants"][0]
    assert row["configurations"] == [["modeling"], ["baseline"], ["compatibility"]]
    assert union["families"] == {"ns::Owner": ["stable-id"]}
    assert union["headers"] == [{"source": "include/owner.h"}]
    assert union["macro_declarations"] == []
    assert {item["line"] for item in row["occurrences"]} == {2, 5, 8}
    assert len(row["semantic_variants"]) == 2

    first, second = row["semantic_variants"]
    assert first["configurations"] == [["modeling"], ["baseline"]]
    assert first["static"] is False and first["virtual"] is False and first["pure"] is False
    assert first["access"] == "protected" and first["bases"] == []
    assert first["parameters"][0]["has_default"] is False
    assert second["configurations"] == [["compatibility"]]
    assert second["static"] is True and second["virtual"] is True and second["pure"] is True
    assert second["access"] == "private" and second["bases"][0]["type"]["qualType"] == "ns::Base"
    assert second["parameters"][0]["has_default"] is True
    assert first["template_parameters"] == modeling["template_parameters"]
    assert first["template_arguments"] == modeling["template_arguments"]
    assert first["template_context"] == modeling["template_context"]
    assert {
        (item["source"], item["line"], item["access"], tuple(item["configuration"])) for item in first["occurrences"]
    } == {
        ("include/owner.h", 2, "protected", ("modeling",)),
        ("include/owner.h", 5, "protected", ("modeling",)),
        ("include/owner.h", 8, "protected", ("baseline",)),
    }
    assert second["occurrences"] == [
        {
            "source": "include/owner.h",
            "line": 2,
            "access": "private",
            "configuration": ["compatibility"],
        }
    ]

    serialized = json.dumps(union, sort_keys=True)
    assert serialized == json.dumps(_RECONCILE(snapshots), sort_keys=True)
    assert original_json == json.dumps(snapshots, sort_keys=True)
    assert first["parameters"] is not modeling["parameters"]
    assert first["template_context"] is not modeling["template_context"]
    modeling["parameters"][0]["has_default"] = True
    modeling["bases"].append({"type": {"qualType": "mutated"}})
    modeling["template_context"][0]["arguments"].append({"type": {"qualType": "float"}})
    snapshots[0]["configuration"].append("mutated")
    assert first["parameters"][0]["has_default"] is False
    assert first["bases"] == []
    assert first["template_context"][0]["arguments"] == []
    assert first["occurrences"][0]["configuration"] == ["modeling"]
    assert row["configurations"][0] == ["modeling"]
    assert row["parameters"][0]["has_default"] is False


def test_reconcile_distinguishes_missing_metadata_from_explicit_defaults():
    """Do not collapse a missing extracted field into false or an empty default."""
    omitted = _declaration()
    omitted.pop("static")
    explicit = _declaration(line=9, static=False)
    union = _RECONCILE([_snapshot(["omitted"], [omitted]), _snapshot(["explicit"], [explicit])])
    variants = union["declarations"][0]["semantic_variants"]

    assert len(variants) == 2
    missing = next(item for item in variants if "static" not in item)
    present = next(item for item in variants if "static" in item)
    assert missing["configurations"] == [["omitted"]]
    assert present["configurations"] == [["explicit"]]
    assert present["static"] is False


def test_reconcile_traces_same_configuration_declaration_definition_variants():
    """Keep same-signature declaration metadata differences tied to each source site."""
    declaration = _declaration(
        parameters=[{"name": None, "type": "int", "has_default": False}],
    )
    definition = _declaration(
        source="src/owner.cpp",
        line=50,
        parameters=[{"name": "value", "type": "int", "has_default": False}],
    )
    union = _RECONCILE([_snapshot(["modeling"], [declaration, definition])])
    row = union["declarations"][0]

    assert declaration["signature"] == definition["signature"] == row["signature"]
    assert len(row["semantic_variants"]) == 2
    assert row["configurations"] == [["modeling"]]
    assert [variant["configurations"] for variant in row["semantic_variants"]] == [
        [["modeling"]],
        [["modeling"]],
    ]
    assert {
        (
            variant["parameters"][0]["name"],
            occurrence["source"],
            occurrence["line"],
            tuple(occurrence["configuration"]),
        )
        for variant in row["semantic_variants"]
        for occurrence in variant["occurrences"]
    } == {
        (None, "include/owner.h", 2, ("modeling",)),
        ("value", "src/owner.cpp", 50, ("modeling",)),
    }


def test_remaining_report_keeps_metadata_variants_in_family_work(tmp_path):
    """Expose source metadata reconciliation even for otherwise supported families."""
    write_remaining = runpy.run_path(str(ROOT / "tools/reconcile_api_inventory.py"))["write_remaining"]
    entry = {
        "id": "variant-id",
        "name": "ns::Owner::method",
        "source": "include/owner.h",
        "classification": "public",
        "status": "supported",
        "python": "ns.Owner.method",
        "ownership": "borrowed",
        "gil": "held",
        "errors": "native exceptions",
        "evidence": ["tests/test_inventory_semantic_variants.py"],
        "semantic_variants": [{}, {}],
    }
    inventory = {
        "systemc_revision": "pinned",
        "fingerprint": "stable",
        "scope": "source review",
        "declarations": [entry],
        "families": {"ns::Owner": [entry["id"]]},
    }
    output = tmp_path / "remaining.md"
    write_remaining(inventory, output)
    report = output.read_text()

    assert "Reconcile declaration/definition or configuration metadata variants" in report
    assert "not behavioral contradictions or support evidence" in report
    assert "do not promote a support status" in report

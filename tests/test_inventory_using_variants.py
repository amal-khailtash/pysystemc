"""Keep using-declaration relationships scoped to source configurations."""

from copy import deepcopy
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[1]
_RECONCILE = runpy.run_path(str(ROOT / "tools/reconcile_api_inventory.py"))["reconcile"]


def _snapshot(configuration, targets=None):
    """Build an inventory with optional target metadata for one using declaration."""
    row = {
        "id": "using-id",
        "name": "ns::Derived::Base::run",
        "source": "include/derived.hpp",
        "line": 12,
        "access": "public",
        "classification": "public_candidate",
        "status": "untested",
    }
    if targets is not None:
        row["using_targets"] = targets
    return {
        "schema_version": 4,
        "systemc_revision": "pinned",
        "configuration": [configuration],
        "headers": [],
        "macro_declarations": [],
        "declarations": [row],
    }


def test_using_target_variants_preserve_configuration_and_input_isolation():
    """Keep different overload targets and absent metadata without changing support."""
    first_targets = [{"declaration_id": "int-overload", "signature": "void (int)"}]
    second_targets = [
        {"declaration_id": "int-overload", "signature": "void (int)"},
        {"declaration_id": "double-overload", "signature": "void (double)"},
    ]
    snapshots = [
        _snapshot("historical"),
        _snapshot("baseline", first_targets),
        _snapshot("modeling", second_targets),
        _snapshot("compatibility", deepcopy(second_targets)),
    ]
    before = deepcopy(snapshots)
    union = _RECONCILE(snapshots)
    row = union["declarations"][0]
    variants = row["semantic_variants"]
    assert row["id"] == "using-id" and row["status"] == "untested"
    assert "using_targets" not in row
    assert len(variants) == 3
    assert "using_targets" not in variants[0]
    assert variants[0]["configurations"] == [["historical"]]
    assert variants[1]["using_targets"] == first_targets
    assert variants[1]["configurations"] == [["baseline"]]
    assert variants[2]["using_targets"] == second_targets
    assert variants[2]["configurations"] == [["modeling"], ["compatibility"]]
    assert [entry["configuration"] for entry in variants[2]["occurrences"]] == [
        ["modeling"],
        ["compatibility"],
    ]
    assert snapshots == before
    variants[2]["using_targets"][0]["signature"] = "modified"
    assert snapshots == before

"""Keep extracted base-class metadata portable without losing its semantics."""

from copy import deepcopy
import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[1]
_INVENTORY = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))


def _extract_entries(source, tmp_path, monkeypatch):
    """Compile a small C++17 fixture and return its reconciled source declarations."""
    header = tmp_path / "submodules/systemc/src/bases.hpp"
    header.parent.mkdir(parents=True, exist_ok=True)
    header.write_text(source)
    monkeypatch.setitem(_INVENTORY["declarations"].__globals__, "ROOT", tmp_path)
    _INVENTORY["source_text"].cache_clear()
    result = subprocess.run(
        ["clang++", "-std=c++17", "-Xclang", "-ast-dump=json", "-fsyntax-only", str(header)],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=True,
        timeout=30,
    )
    ast = json.loads(result.stdout)
    _INVENTORY["restore_source_files"]([ast])
    original_ast = deepcopy(ast)
    entries = list(_INVENTORY["declarations"](ast))
    assert ast == original_ast
    entries = _INVENTORY["reconcile_declarations"](entries)
    return ast, entries


def _nested_values(value):
    """Yield every nested value in JSON-shaped metadata."""
    if isinstance(value, dict):
        yield value
        for child in value.values():
            yield from _nested_values(child)
    elif isinstance(value, list):
        for child in value:
            yield from _nested_values(child)


def test_base_metadata_is_portable_and_semantic(tmp_path, monkeypatch):
    """Retain written base types and inheritance properties across extractions."""
    source = """namespace fixture {
struct Root {};
using Alias = Root;
class ProtectedVirtual : protected virtual Alias {};
class PrivateBase : private Alias {};
template<class T> struct Dependent : T {};
template<class... Bases> struct Pack : Bases... {};
}
"""
    ast, entries = _extract_entries(source, tmp_path, monkeypatch)
    second_ast, second_entries = _extract_entries(source, tmp_path, monkeypatch)

    def by_name(rows):
        """Index emitted base-bearing declarations by fully qualified name."""
        return {entry["name"]: entry for entry in rows if entry["name"].startswith("fixture::") and entry.get("bases")}

    extracted = by_name(entries)
    repeated = by_name(second_entries)
    assert extracted.keys() == repeated.keys()
    assert {name: row["bases"] for name, row in extracted.items()} == {
        name: row["bases"] for name, row in repeated.items()
    }
    assert {name: _INVENTORY["declaration_id"](row) for name, row in extracted.items()} == {
        name: _INVENTORY["declaration_id"](row) for name, row in repeated.items()
    }

    aliased = extracted["fixture::ProtectedVirtual"]["bases"][0]
    assert aliased["access"] == "protected"
    assert aliased["writtenAccess"] == "protected"
    assert aliased["isVirtual"] is True
    assert aliased["type"]["qualType"] == "fixture::Alias"
    assert aliased["type"]["desugaredQualType"] == "fixture::Root"
    assert "typeAliasDeclId" not in aliased["type"]

    private = extracted["fixture::PrivateBase"]["bases"][0]
    assert private["access"] == "private"
    assert private["writtenAccess"] == "private"
    dependent = extracted["fixture::Dependent"]["bases"][0]
    assert dependent["type"]["qualType"] == "T"
    pack = extracted["fixture::Pack"]["bases"][0]
    assert pack["type"]["qualType"] == "Bases"
    assert pack["isPackExpansion"] is True
    assert all("typeAliasDeclId" not in value for row in extracted.values() for value in _nested_values(row["bases"]))

    def alias_decl(root):
        return next(
            node
            for node in _nested_values(root)
            if isinstance(node, dict) and node.get("kind") == "TypeAliasDecl" and node.get("name") == "Alias"
        )

    first_alias = alias_decl(ast)
    second_alias = alias_decl(second_ast)
    assert first_alias["id"]
    assert second_alias["id"]
    raw_aliased_record = next(
        node
        for node in _nested_values(ast)
        if isinstance(node, dict)
        and node.get("kind") == "CXXRecordDecl"
        and node.get("name") == "ProtectedVirtual"
        and node.get("bases")
    )
    assert raw_aliased_record["bases"][0]["type"]["typeAliasDeclId"] == first_alias["id"]


def test_base_normalizer_removes_only_alias_ids_and_copies_input():
    """Strip alias pointer IDs recursively while preserving unknown metadata."""
    bases = [
        {
            "access": "public",
            "isFutureClangProperty": {"typeAliasDeclId": "nested-transient", "value": [1, 2]},
            "type": {
                "qualType": "Alias",
                "desugaredQualType": "Root",
                "typeAliasDeclId": "0x1234",
                "unknownTypeMetadata": {"declarationId": "semantic-id", "width": 17},
            },
        },
        None,
        "malformed-but-preserved",
    ]
    original = deepcopy(bases)

    normalized = _INVENTORY["_portable_base_metadata"](bases)

    assert bases == original
    assert normalized == [
        {
            "access": "public",
            "isFutureClangProperty": {"value": [1, 2]},
            "type": {
                "qualType": "Alias",
                "desugaredQualType": "Root",
                "unknownTypeMetadata": {"declarationId": "semantic-id", "width": 17},
            },
        },
        None,
        "malformed-but-preserved",
    ]
    normalized[0]["type"]["qualType"] = "changed"
    assert bases == original

"""Preserve exact Clang targets for emitted using declarations."""

from copy import deepcopy
import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[1]
_INVENTORY = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))


def _compile_fixture(source, tmp_path, monkeypatch):
    """Compile a small C++17 fixture and return its restored Clang AST."""
    header = tmp_path / "submodules/systemc/src/using_targets.hpp"
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
    return ast


def _extract(ast):
    """Run the production extraction and target-reconciliation steps on an AST."""
    original_ast = deepcopy(ast)
    entries = list(_INVENTORY["declarations"](ast))
    assert ast == original_ast
    entries = _INVENTORY["reconcile_declarations"](entries)
    assert ast == original_ast
    for entry in entries:
        entry["id"] = _INVENTORY["declaration_id"](entry)
    return entries


def _values(value):
    """Yield nested JSON values, including scalar leaves, for leakage checks."""
    yield value
    if isinstance(value, dict):
        for child in value.values():
            yield from _values(child)
    elif isinstance(value, list):
        for child in value:
            yield from _values(child)


def test_using_targets_reconcile_exact_overloads_and_members(tmp_path, monkeypatch):
    """Resolve namespace, class, overload, and alias targets without AST IDs."""
    source = """namespace fixture {
namespace targets {
void select(int);
void select(double);
struct Base { void method(); using Alias = int; };
}
using targets::select;
using targets::Base;
struct Derived : targets::Base {
    using Base::method;
    using Base::Alias;
};
template<class T> struct Dependent : T { using T::member; };
}
"""
    ast = _compile_fixture(source, tmp_path, monkeypatch)
    first = _extract(ast)
    second = _extract(_compile_fixture(source, tmp_path, monkeypatch))

    def using_rows(entries):
        """Return source rows for concrete Clang UsingDecl nodes."""
        return [entry for entry in entries if entry["kind"] == "UsingDecl"]

    first_using = using_rows(first)
    second_using = using_rows(second)
    assert [(entry["name"], entry["id"]) for entry in first_using] == [
        (entry["name"], entry["id"]) for entry in second_using
    ]
    assert [entry["using_targets"] for entry in first_using] == [entry["using_targets"] for entry in second_using]

    namespace_overload = next(entry for entry in first_using if entry["name"] == "fixture::targets::select")
    overload_targets = namespace_overload["using_targets"]
    assert len(overload_targets) == 2
    assert {target["signature"] for target in overload_targets} == {"void (int)", "void (double)"}
    assert all(target["kind"] == "FunctionDecl" for target in overload_targets)
    assert all(target["name"] == "fixture::targets::select" for target in overload_targets)
    assert all(target["source"] == "submodules/systemc/src/using_targets.hpp" for target in overload_targets)

    namespace_class = next(entry for entry in first_using if entry["name"] == "fixture::targets::Base")
    assert len(namespace_class["using_targets"]) == 1
    assert namespace_class["using_targets"][0]["kind"] == "CXXRecordDecl"
    assert namespace_class["using_targets"][0]["name"] == "fixture::targets::Base"

    method_using = next(entry for entry in first_using if entry["name"] == "fixture::Derived::Base::method")
    assert method_using["using_targets"][0]["kind"] == "CXXMethodDecl"
    assert method_using["using_targets"][0]["name"] == "fixture::targets::Base::method"
    alias_using = next(entry for entry in first_using if entry["name"] == "fixture::Derived::Base::Alias")
    assert alias_using["using_targets"][0]["kind"] == "TypeAliasDecl"
    assert alias_using["using_targets"][0]["name"] == "fixture::targets::Base::Alias"
    assert alias_using["using_targets"][0]["signature"] == "int"

    indexed_ids = {entry["id"] for entry in first}
    assert all(target["declaration_id"] in indexed_ids for entry in first_using for target in entry["using_targets"])
    assert all("unresolved_reason" not in target for entry in first_using for target in entry["using_targets"])
    assert all(
        not {"ast_id", "previous_decl", "parent_context", "_inventory_ast_id", "_using_target_refs"}.intersection(value)
        for entry in first
        for value in _values(entry)
        if isinstance(value, dict)
    )
    assert all(
        not any(isinstance(value, str) and value.startswith("0x") for value in _values(entry)) for entry in first
    )

    identity_variant = deepcopy(namespace_overload)
    identity_variant["using_targets"] = [{"declaration_id": "different", "kind": "TypedefDecl"}]
    assert _INVENTORY["declaration_id"](identity_variant) == namespace_overload["id"]

    unresolved_using_nodes = [
        node
        for node in _values(ast)
        if isinstance(node, dict) and node.get("kind") == "UnresolvedUsingValueDecl" and node.get("name") == "member"
    ]
    assert unresolved_using_nodes
    assert not any(entry["name"].endswith("::member") for entry in first)


def test_filtered_using_target_stays_unresolved_despite_matching_spelling(tmp_path, monkeypatch):
    """Leave filtered targets unresolved instead of guessing by member spelling."""
    source = """namespace fixture {
namespace targets { void outside(int); }
namespace decoy { void outside(int); }
namespace importer { using targets::outside; }
}
"""
    ast = _compile_fixture(source, tmp_path, monkeypatch)
    original_ast = deepcopy(ast)
    namespaces = [
        node
        for node in _values(ast)
        if isinstance(node, dict) and node.get("kind") == "NamespaceDecl" and node.get("name") in {"decoy", "importer"}
    ]
    entries = []
    for namespace in namespaces:
        entries.extend(_INVENTORY["declarations"](namespace))
    assert ast == original_ast
    entries = _INVENTORY["reconcile_declarations"](entries)
    assert ast == original_ast

    using = next(entry for entry in entries if entry["kind"] == "UsingDecl")
    target = using["using_targets"][0]
    assert target["kind"] == "FunctionDecl"
    assert target["name"] == "outside"
    assert target["signature"] == "void (int)"
    assert target["source"] is None
    assert target["unresolved_reason"] == "target_filtered_or_outside_inventory"
    assert "declaration_id" not in target
    assert not any(
        entry["kind"] == "FunctionDecl" and entry["name"] == "fixture::targets::outside" for entry in entries
    )


def test_separate_compiler_scopes_do_not_resolve_colliding_ast_ids():
    """Keep same-valued AST pointers isolated across independent extraction scopes."""
    using_entry = {
        "ast_id": "0xusing",
        "previous_decl": None,
        "parent_context": None,
        "name": "fixture::importer::outside",
        "kind": "UsingDecl",
        "signature": None,
        "static": False,
        "template_context": [],
        "template_arguments": [],
        "template_parameters": [],
        "source": "submodules/systemc/src/importer.hpp",
        "line": 1,
        "offset": 0,
        "access": "public",
        "virtual": False,
        "pure": False,
        "bases": [],
        "parameters": [],
        "python_candidate": "amal.eda.systemc.fixture.importer.outside",
        "classification": "unreviewed",
        "status": "untested",
        "evidence": [],
        "_using_target_refs": [
            {
                "_target_ast_id": "0xcolliding-target",
                "kind": "FunctionDecl",
                "name": "outside",
                "signature": "void (int)",
                "source": None,
            }
        ],
    }
    target_entry = {
        "ast_id": "0xcolliding-target",
        "previous_decl": None,
        "parent_context": None,
        "name": "fixture::decoy::outside",
        "kind": "FunctionDecl",
        "signature": "void (int)",
        "static": False,
        "template_context": [],
        "template_arguments": [],
        "template_parameters": [],
        "source": "submodules/systemc/src/decoy.hpp",
        "line": 1,
        "offset": 0,
        "access": "public",
        "virtual": False,
        "pure": False,
        "bases": [],
        "parameters": [],
        "python_candidate": "amal.eda.systemc.fixture.decoy.outside",
        "classification": "unreviewed",
        "status": "untested",
        "evidence": [],
    }

    importer_rows = _INVENTORY["reconcile_declarations"]([using_entry])
    decoy_rows = _INVENTORY["reconcile_declarations"]([target_entry])

    unresolved = importer_rows[0]["using_targets"][0]
    assert unresolved["unresolved_reason"] == "target_filtered_or_outside_inventory"
    assert "declaration_id" not in unresolved
    assert not {"ast_id", "_inventory_ast_id", "_using_target_refs"}.intersection(importer_rows[0])
    assert decoy_rows[0]["name"] == "fixture::decoy::outside"
    assert not {"ast_id", "_inventory_ast_id"}.intersection(decoy_rows[0])


def test_constructor_using_shadow_remains_explicitly_unresolved(tmp_path, monkeypatch):
    """Report Clang's specialized constructor shadow as an unsupported link."""
    source = """namespace fixture {
struct Base { Base(int); };
struct Derived : Base { using Base::Base; };
}
"""
    ast = _compile_fixture(source, tmp_path, monkeypatch)
    shadow_kinds = {
        node["kind"]
        for node in _values(ast)
        if isinstance(node, dict) and node.get("kind", "").endswith("UsingShadowDecl")
    }
    assert "ConstructorUsingShadowDecl" in shadow_kinds
    assert "UsingShadowDecl" not in shadow_kinds

    entries = _extract(ast)
    using = next(entry for entry in entries if entry["kind"] == "UsingDecl")
    assert using["using_targets"] == [
        {
            "kind": None,
            "name": None,
            "signature": None,
            "source": None,
            "unresolved_reason": "using_shadow_not_emitted",
        }
    ]

"""Preserve lexical friendship as portable edges without changing declaration rows."""

from copy import deepcopy
import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[1]
_INVENTORY = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))


def _compile_fixture(source, tmp_path, monkeypatch):
    """Compile C++17 once, restoring omitted filenames before extraction."""
    header = tmp_path / "submodules/systemc/src/friend_relationships.hpp"
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
    """Reconcile one invocation while proving opt-in sidecars leave rows unchanged."""
    original = deepcopy(ast)
    baseline = list(_INVENTORY["declarations"](ast))
    friends = []
    entries = list(_INVENTORY["declarations"](ast, friend_relationships=friends))
    assert entries == baseline
    assert ast == original
    baseline = _INVENTORY["reconcile_declarations"](baseline)
    entries = _INVENTORY["reconcile_declarations"](entries, friend_relationships=friends)
    assert entries == baseline
    assert ast == original
    for entry in entries:
        entry["id"] = _INVENTORY["declaration_id"](entry)
    return entries, friends


def _values(value):
    """Yield all nested values to check for invocation-local pointer leakage."""
    yield value
    if isinstance(value, dict):
        for child in value.values():
            yield from _values(child)
    elif isinstance(value, list):
        for child in value:
            yield from _values(child)


def test_friend_children_and_type_only_forms_preserve_existing_declarations(tmp_path, monkeypatch):
    """Resolve direct free/template friends and keep qualified/dependent types unresolved."""
    source = """namespace fixture {
struct Host;
struct Existing {};
template<class T> struct Box {};
template<class T> struct Peer;
void hidden(Host const&);
template<class T> T free_template(Host const&, T);
struct Host {
  friend void hidden(Host const&);
  template<class T> friend T free_template(Host const&, T);
  friend void hidden_only(Host const&);
  template<class T> friend T fresh_friend(Host const&, T);
  friend class Existing;
  friend class Box<int>;
};
void hidden_only(Host const&) {}
template<class T> T fresh_friend(Host const&, T x) { return x; }
struct Qualified { friend class fixture::Existing; };
template<class T> struct DepHost { friend T; };
template<class T> struct TemplateHost { template<class U> friend class Peer; };
template<class T> struct Mixed { template<class U> friend U fresh_mixed(U); };
namespace imported { using fixture::hidden; }
}
"""
    ast = _compile_fixture(source, tmp_path, monkeypatch)
    entries, friends = _extract(ast)
    assert len(friends) == 10
    indexed = {entry["id"]: entry for entry in entries}
    assert len({friend["occurrence_id"] for friend in friends}) == len(friends)
    assert all(friend["owner"]["declaration_ids"] for friend in friends)
    assert all(identifier in indexed for friend in friends for identifier in friend["owner"]["declaration_ids"])

    for name in ("hidden", "hidden_only", "free_template", "fresh_friend", "Peer", "fresh_mixed"):
        friend = next(friend for friend in friends if friend["target"].get("name") == f"fixture::{name}")
        assert friend["resolution"] == "child_declaration"
        assert "unresolved_reason" not in friend
        target_rows = [indexed[identifier] for identifier in friend["target_declaration_ids"]]
        assert all(row["name"] == f"fixture::{name}" for row in target_rows)
        assert all(row["access"] == "public" for row in target_rows)
        if name in {"free_template", "fresh_friend", "fresh_mixed"}:
            assert {row["kind"] for row in target_rows} == {"FunctionTemplateDecl", "FunctionDecl"}
        elif name == "Peer":
            assert {row["kind"] for row in target_rows} == {"ClassTemplateDecl", "CXXRecordDecl"}
        else:
            assert {row["kind"] for row in target_rows} == {"FunctionDecl"}

    unresolved = [friend for friend in friends if friend["resolution"] == "type_only_unresolved"]
    assert len(unresolved) == 4
    assert {friend["target"]["written_type"] for friend in unresolved} == {
        "class Existing",
        "class fixture::Existing",
        "class Box<int>",
        "T",
    }
    assert all(friend["target_declaration_ids"] == [] for friend in unresolved)
    assert all(
        friend["unresolved_reason"] == "type_only_friend_has_no_exact_declaration_reference" for friend in unresolved
    )
    assert any(friend["target"].get("desugared_type") for friend in unresolved)

    mixed = next(friend for friend in friends if friend["target"].get("name") == "fixture::fresh_mixed")
    assert mixed["owner"]["name"] == "fixture::Mixed"
    assert len(mixed["owner"]["declaration_ids"]) == 2
    context = mixed["owner"]["template_context"]
    assert context[0]["name"] == "fixture::Mixed"
    assert [parameter["name"] for parameter in context[0]["template_parameters"]] == ["T"]
    assert [parameter["name"] for parameter in mixed["target"]["template_parameters"]] == ["U"]
    assert all(indexed[identifier]["template_context"] == [] for identifier in mixed["target_declaration_ids"])
    assert all(indexed[identifier]["template_context"] == [] for identifier in mixed["owner"]["declaration_ids"])
    assert not any(isinstance(value, str) and value.startswith("0x") for friend in friends for value in _values(friend))
    assert not any(
        isinstance(value, dict) and any(key.startswith("_") for key in value)
        for friend in friends
        for value in _values(friend)
    )


def test_filtered_friend_children_do_not_guess_by_name(tmp_path, monkeypatch):
    """Keep absent direct AST targets unresolved despite represented namesakes."""
    source = """namespace fixture {
struct Host { template<class T> friend T select(T); };
namespace decoy { template<class T> T select(T); }
}
"""
    ast = _compile_fixture(source, tmp_path, monkeypatch)
    friends = []
    entries = list(_INVENTORY["declarations"](ast, friend_relationships=friends))
    references = set(friends[0]["_target_ast_ids"])
    entries = [entry for entry in entries if entry["ast_id"] not in references]
    _INVENTORY["reconcile_declarations"](entries, friend_relationships=friends)
    assert any(entry["name"] == "fixture::decoy::select" for entry in entries)
    assert friends[0]["target_declaration_ids"] == []
    assert friends[0]["resolution"] == "target_filtered_or_outside_inventory"
    assert friends[0]["unresolved_reason"] == "target_filtered_or_outside_inventory"


def test_friend_template_partial_and_owner_unavailable_are_explicit(tmp_path, monkeypatch):
    """Retain exact represented template patterns while reporting missing endpoints."""
    source = """namespace fixture {
struct Host { template<class T> friend T select(T); };
}
"""
    ast = _compile_fixture(source, tmp_path, monkeypatch)
    friends = []
    entries = list(_INVENTORY["declarations"](ast, friend_relationships=friends))
    excluded = {friends[0]["_target_ast_ids"][0], *friends[0]["owner"]["_ast_ids"]}
    entries = [entry for entry in entries if entry["ast_id"] not in excluded]
    _INVENTORY["reconcile_declarations"](entries, friend_relationships=friends)
    assert len(friends[0]["target_declaration_ids"]) == 1
    assert friends[0]["resolution"] == "child_declaration_partial"
    assert friends[0]["unresolved_reason"] == "some_target_declarations_filtered_or_outside_inventory"
    assert friends[0]["owner"]["declaration_ids"] == []
    assert friends[0]["owner"]["unresolved_reason"] == "owner_filtered_or_outside_inventory"


def test_friend_partial_and_ambiguous_owner_and_target_references(tmp_path, monkeypatch):
    """Expose ambiguous and missing references while retaining independently proven IDs."""
    source = """namespace fixture {
template<class T> struct Host { template<class U> friend U select(U); };
}
"""
    ast = _compile_fixture(source, tmp_path, monkeypatch)
    initial_friends = []
    initial_rows = list(_INVENTORY["declarations"](ast, friend_relationships=initial_friends))
    owner_template = initial_friends[0]["owner"]["_ast_ids"][0]
    target_template = initial_friends[0]["_target_ast_ids"][0]

    partial_friends = deepcopy(initial_friends)
    partial_rows = [deepcopy(entry) for entry in initial_rows if entry["ast_id"] != owner_template]
    _INVENTORY["reconcile_declarations"](partial_rows, friend_relationships=partial_friends)
    assert len(partial_friends[0]["owner"]["declaration_ids"]) == 1
    assert partial_friends[0]["owner"]["unresolved_reason"] == "some_owner_declarations_filtered_or_outside_inventory"

    ambiguous_friends = deepcopy(initial_friends)
    ambiguous_rows = deepcopy(initial_rows)
    for reference in (owner_template, target_template):
        decoy = deepcopy(next(entry for entry in initial_rows if entry["ast_id"] == reference))
        decoy["name"] = "unrelated::same_pointer"
        ambiguous_rows.append(decoy)
    _INVENTORY["reconcile_declarations"](ambiguous_rows, friend_relationships=ambiguous_friends)
    friend = ambiguous_friends[0]
    assert len(friend["owner"]["declaration_ids"]) == 1
    assert friend["owner"]["unresolved_reason"] == "owner_declaration_mapping_ambiguous"
    assert len(friend["target_declaration_ids"]) == 1
    assert friend["resolution"] == "target_declaration_mapping_ambiguous"
    assert friend["unresolved_reason"] == "target_declaration_mapping_ambiguous"


def test_friend_occurrence_identity_retains_macro_provenance(tmp_path, monkeypatch):
    """Distinguish repeated macro grants and keep IDs independent of endpoint payload."""
    source = """namespace fixture {
#define GRANT friend void grant(int);
struct First { GRANT };
struct Second { GRANT };
}
"""
    ast = _compile_fixture(source, tmp_path, monkeypatch)
    entries, friends = _extract(ast)
    assert len(friends) == 2
    assert friends[0]["occurrence_id"] != friends[1]["occurrence_id"]
    assert (
        friends[0]["source_occurrence"]["location"]["spellingLoc"]
        == friends[1]["source_occurrence"]["location"]["spellingLoc"]
    )
    assert (
        friends[0]["source_occurrence"]["location"]["expansionLoc"]
        != friends[1]["source_occurrence"]["location"]["expansionLoc"]
    )
    assert all(friend["source_occurrence"]["location_basis"] == "expansion" for friend in friends)
    assert all("source_range" in friend["source_occurrence"] for friend in friends)
    altered = deepcopy(ast)
    for value in _values(altered):
        if isinstance(value, dict) and value.get("kind") == "FunctionDecl" and value.get("name") == "grant":
            value["name"] = "different_payload"
    _, variants = _extract(altered)
    assert [friend["occurrence_id"] for friend in variants] == [friend["occurrence_id"] for friend in friends]
    assert all(friend["target"].get("name") == "fixture::different_payload" for friend in variants)
    assert all(entry["source"] == "submodules/systemc/src/friend_relationships.hpp" for entry in entries)


def test_friend_pointer_collision_stays_inside_compiler_invocation(tmp_path, monkeypatch):
    """Do not resolve a removed endpoint against a same-valued pointer in another scope."""
    source = """namespace fixture { struct Host { friend void target(); }; }
"""
    ast = _compile_fixture(source, tmp_path, monkeypatch)
    friends = []
    rows = list(_INVENTORY["declarations"](ast, friend_relationships=friends))
    reference = friends[0]["_target_ast_ids"][0]
    target = next(entry for entry in rows if entry["ast_id"] == reference)
    first_scope = [entry for entry in rows if entry["ast_id"] != reference]
    second_scope = deepcopy(target)
    second_scope["name"] = "other::target"
    first_scope = _INVENTORY["reconcile_declarations"](first_scope, friend_relationships=friends)
    second_scope = _INVENTORY["reconcile_declarations"]([second_scope])
    combined = [*first_scope, *second_scope]
    assert any(entry["name"] == "other::target" for entry in combined)
    assert friends[0]["target_declaration_ids"] == []
    assert friends[0]["resolution"] == "target_filtered_or_outside_inventory"
    assert not any(isinstance(value, str) and value.startswith("0x") for value in _values(friends))


def test_friend_audit_summary_distinguishes_absence_empty_and_unresolved(tmp_path):
    """Report optional source coverage and preserve the legacy report when absent."""
    inventory = {"headers": [], "declarations": [], "systemc_revision": "fixture"}
    report = tmp_path / "audit.md"
    _INVENTORY["write_audit_report"](inventory, report)
    legacy = report.read_text()
    assert "Friend source relationships" not in legacy
    inventory["friend_relationships"] = []
    _INVENTORY["write_audit_report"](inventory, report)
    assert "Friend source sites: 0; relationship records: 0." in report.read_text()
    inventory["friend_relationships"] = [
        {"occurrence_id": "one", "resolution": "child_declaration"},
        {"occurrence_id": "one", "resolution": "child_declaration_partial"},
        {"occurrence_id": "two", "resolution": "type_only_unresolved"},
    ]
    _INVENTORY["write_audit_report"](inventory, report)
    updated = report.read_text()
    assert "Friend source sites: 2; relationship records: 3." in updated
    assert "Exact target records: 1; unresolved or partial target records: 2." in updated
    assert "do not establish Python binding or ADL support" in updated
    del inventory["friend_relationships"]
    _INVENTORY["write_audit_report"](inventory, report)
    assert report.read_text() == legacy

"""Verify written and effective static/virtual flags from source AST relations."""

import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[1]
_INVENTORY = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))


def _compile_entries(source, tmp_path, monkeypatch):
    """Compile and reconcile one C++17 fixture in an isolated source tree."""
    header = tmp_path / "submodules/systemc/src/effective_flags.hpp"
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
    return list(_INVENTORY["declarations"](ast))


def test_redeclaration_links_preserve_written_flags_and_resolve_effective_flags(tmp_path, monkeypatch):
    """Carry static/virtual through linked definitions, including a three-row chain."""
    source = """namespace fixture {
static void linked();
void linked();
void linked() {}
struct Local {
  static int value;
  static void utility();
  virtual void callback();
  void ordinary();
};
int Local::value = 1;
void Local::utility() {}
void Local::callback() {}
struct Base { virtual void marked(); virtual void implicit(); virtual ~Base(); };
struct Derived : Base { Derived(); ~Derived(); void marked() override; void implicit(); };
}
"""
    entries = _INVENTORY["reconcile_declarations"](_compile_entries(source, tmp_path, monkeypatch))
    by_name = {}
    for entry in entries:
        by_name.setdefault(entry["name"], []).append(entry)

    linked = sorted(by_name["fixture::linked"], key=lambda row: row["line"])
    assert [row["written_static"] for row in linked] == [True, False, False]
    assert [row["static"] for row in linked] == [True, True, True]
    assert linked[0]["effective_static_status"] == "resolved_from_self"
    assert linked[1]["effective_static_status"] == linked[2]["effective_static_status"] == "resolved_from_previous_decl"

    value_decl, value_def = sorted(by_name["fixture::Local::value"], key=lambda row: row["line"])
    assert (value_decl["written_static"], value_decl["static"]) == (True, True)
    assert (value_def["written_static"], value_def["static"]) == (False, True)
    assert value_def["effective_static_status"] == "resolved_from_previous_decl"

    static_decl, static_def = sorted(by_name["fixture::Local::utility"], key=lambda row: row["line"])
    assert (static_decl["written_static"], static_decl["static"]) == (True, True)
    assert (static_def["written_static"], static_def["static"]) == (False, True)
    assert static_def["virtual"] is False
    assert static_def["effective_static_status"] == "resolved_from_previous_decl"

    virtual_decl, virtual_def = sorted(by_name["fixture::Local::callback"], key=lambda row: row["line"])
    assert (virtual_decl["written_virtual"], virtual_decl["virtual"]) == (True, True)
    assert (virtual_def["written_virtual"], virtual_def["virtual"]) == (False, True)
    assert virtual_def["effective_virtual_status"] == "resolved_from_previous_decl_virtual_specifier"

    ordinary = by_name["fixture::Local::ordinary"][0]
    assert ordinary["written_virtual"] is False
    assert ordinary["virtual"] is False
    assert ordinary["effective_virtual_status"] == "resolved_nonvirtual_no_base_record"

    marked = by_name["fixture::Derived::marked"][0]
    assert marked["written_virtual"] is False
    assert marked["virtual"] is True
    assert marked["effective_virtual_status"] == "resolved_from_override_attribute"
    implicit = by_name["fixture::Derived::implicit"][0]
    assert implicit["written_virtual"] is False
    assert implicit["virtual"] is None
    assert implicit["effective_virtual_status"] == "unresolved_inherited_override_not_exposed"

    constructor = by_name["fixture::Derived::Derived"][0]
    assert constructor["kind"] == "CXXConstructorDecl"
    assert constructor["written_virtual"] is False
    assert constructor["virtual"] is False
    assert constructor["effective_virtual_status"] == "resolved_nonvirtual_constructor"

    destructor = by_name["fixture::Derived::~Derived"][0]
    assert destructor["kind"] == "CXXDestructorDecl"
    assert destructor["written_virtual"] is False
    assert destructor["virtual"] is None
    assert destructor["effective_virtual_status"] == "unresolved_inherited_override_not_exposed"


def test_missing_previous_declaration_leaves_effective_flags_unknown(tmp_path, monkeypatch):
    """Never infer static/virtual from an unrepresented redeclaration target."""
    source = """namespace fixture {
struct Owner { static int value; virtual void callback(); };
int Owner::value = 1;
void Owner::callback() {}
}
    """
    raw_entries = _compile_entries(source, tmp_path, monkeypatch)
    callback_declaration_id = next(
        entry["ast_id"] for entry in raw_entries
        if entry["name"] == "fixture::Owner::callback" and not entry.get("previous_decl")
    )
    value_declaration_id = next(
        entry["ast_id"] for entry in raw_entries
        if entry["name"] == "fixture::Owner::value" and not entry.get("previous_decl")
    )
    callback_definition = next(
        entry for entry in raw_entries
        if entry.get("previous_decl") == callback_declaration_id
    )
    value_definition = next(
        entry for entry in raw_entries
        if entry.get("previous_decl") == value_declaration_id
    )
    removed_ids = {callback_definition["previous_decl"], value_definition["previous_decl"]}
    retained = [entry for entry in raw_entries if entry.get("ast_id") not in removed_ids]
    entries = _INVENTORY["reconcile_declarations"](retained)

    callback = next(entry for entry in entries if entry["name"] == "fixture::Owner::callback")
    assert callback["written_virtual"] is False
    assert callback["virtual"] is None
    assert callback["effective_virtual_status"] == "unresolved_previous_declaration_not_in_snapshot"

    value = next(entry for entry in entries if entry["name"] == "fixture::Owner::value")
    assert value["written_static"] is False
    assert value["static"] is None
    assert value["effective_static_status"] == "unresolved_previous_declaration_not_in_snapshot"

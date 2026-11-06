"""Preserve written function and template defaults from Clang source metadata."""

from copy import deepcopy
import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[1]
_INVENTORY = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))


def _ast_entries(source, tmp_path, monkeypatch):
    """Compile one small C++17 fixture and extract declarations with the production walker."""
    fixture_root = tmp_path
    header = fixture_root / "submodules/systemc/src/defaults.hpp"
    header.parent.mkdir(parents=True)
    header.write_text(source)
    monkeypatch.setitem(_INVENTORY["declarations"].__globals__, "ROOT", fixture_root)
    _INVENTORY["source_text"].cache_clear()
    result = subprocess.run(
        ["clang++", "-std=c++17", "-Xclang", "-ast-dump=json", "-fsyntax-only", str(header)],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=True,
        timeout=30,
    )
    roots = [json.loads(result.stdout)]
    _INVENTORY["restore_source_files"](roots)
    entries = list(_INVENTORY["declarations"](roots[0]))
    return _INVENTORY["reconcile_declarations"](entries)


def test_function_and_template_default_source_metadata(tmp_path, monkeypatch):
    """Recover authored defaults, inherited origins, and portable source byte spans."""
    source = """// π keeps offsets byte-based before the declarations.
#define OBJECT_DEFAULT (41 + 1)
#define FUNCTION_DEFAULT(value) ((value) + 1)
#define DECLARE_WITH_DEFAULT(name) void name(int value=7)
#define DECLARE_METHOD_DEFAULT(name) void name(int value=9)
namespace fixture {
enum Mode { enabled = 1 };
template<class> struct Box {};
int arguments(int required, int literal = 7, Mode selected = Mode::enabled,
              int object_macro = OBJECT_DEFAULT,
              int function_macro = FUNCTION_DEFAULT /* keep comment */ (2),
              int multiline = (1 +
                  2));
void inherited(int value = 11);
void inherited(int value);
void macro_inherited(int value = FUNCTION_DEFAULT(3));
void macro_inherited(int value);
DECLARE_WITH_DEFAULT(macro_generated);
template<class T> struct Owner { DECLARE_METHOD_DEFAULT(run); };
template<class T> void Owner<T>::run(int value) {}
template<class U, typename T = int, int N = sizeof(T),
         template<class> class C = Box> struct Holder {};
}
"""
    entries = _ast_entries(source, tmp_path, monkeypatch)
    assert all(not {"ast_id", "previous_decl", "parent_context"}.intersection(entry) for entry in entries)
    arguments = next(entry for entry in entries if entry["name"] == "fixture::arguments")
    parameters = {parameter["name"]: parameter for parameter in arguments["parameters"]}

    assert parameters["required"]["has_default"] is False
    assert parameters["required"]["default_argument"] is None
    assert parameters["literal"]["default_argument"]["expression"] == "7"
    assert parameters["selected"]["default_argument"]["expression"] == "Mode::enabled"
    assert parameters["object_macro"]["default_argument"]["expression"] == "OBJECT_DEFAULT"
    function_macro = parameters["function_macro"]["default_argument"]
    assert function_macro["expression"] == "FUNCTION_DEFAULT /* keep comment */ (2)"
    assert function_macro["argument_origin"] == "written"
    assert function_macro["macro_provenance"]
    macro_site = function_macro["macro_provenance"][0]
    assert set(macro_site) == {"expansion_token", "spelling_tokens"}
    assert macro_site["expansion_token"]["source"].endswith("defaults.hpp")
    token_start, token_stop = macro_site["expansion_token"]["byte_range"]
    assert source.encode()[token_start:token_stop].decode() == "FUNCTION_DEFAULT"
    assert [token["source"] for token in macro_site["spelling_tokens"]]
    assert parameters["multiline"]["default_argument"]["expression"] == "(1 +\n                  2)"
    multiline_source = parameters["multiline"]["default_argument"]["expression_source"]
    start, stop = multiline_source["byte_range"]
    assert source.encode()[start:stop].decode() == "(1 +\n                  2)"
    assert multiline_source["line_range"][1] == multiline_source["line_range"][0] + 1
    assert start > source.index("int arguments")  # π occupies two UTF-8 bytes.

    inherited = [entry for entry in entries if entry["name"] == "fixture::inherited"]
    assert len(inherited) == 2
    written_default = inherited[0]["parameters"][0]["default_argument"]
    inherited_default = inherited[1]["parameters"][0]["default_argument"]
    assert written_default["expression"] == inherited_default["expression"] == "11"
    assert written_default["argument_origin"] == "written"
    assert inherited_default["argument_origin"] == "inherited"
    assert inherited_default["expression_source"] == written_default["expression_source"]

    macro_inherited = [entry for entry in entries if entry["name"] == "fixture::macro_inherited"]
    assert len(macro_inherited) == 2
    inherited_macro = macro_inherited[1]["parameters"][0]["default_argument"]
    assert inherited_macro["expression"] == "FUNCTION_DEFAULT(3)"
    assert inherited_macro["argument_origin"] == "inherited"
    generated = next(entry for entry in entries if entry["name"] == "fixture::macro_generated")
    generated_default = generated["parameters"][0]["default_argument"]
    assert generated_default["expression"] == "7"
    assert generated_default["argument_origin"] == "macro_generated"
    assert generated_default["expression_source"]["location_kind"] == "macro_spelling_source"
    owner_methods = [entry for entry in entries if entry["name"] == "fixture::Owner::run"]
    assert len(owner_methods) == 2
    generated_definition = max(owner_methods, key=lambda entry: entry["line"])
    assert generated_definition["parameters"][0]["default_argument"]["expression"] == "9"
    assert generated_definition["parameters"][0]["default_argument"]["argument_origin"] == "inherited"

    holder = next(
        entry for entry in entries if entry["name"] == "fixture::Holder" and entry["kind"] == "ClassTemplateDecl"
    )
    template_parameters = {parameter["name"]: parameter for parameter in holder["template_parameters"]}
    assert template_parameters["U"]["has_default"] is False
    assert template_parameters["U"]["default_argument"] is None
    assert template_parameters["T"]["default_argument"]["kind"] == "type"
    assert template_parameters["T"]["default_argument"]["expression"] == "int"
    assert template_parameters["T"]["default_argument"]["argument_origin"] == "written"
    assert template_parameters["N"]["default_argument"]["kind"] == "expression"
    assert template_parameters["N"]["default_argument"]["expression"] == "sizeof(T)"
    assert template_parameters["N"]["default_argument"]["argument_origin"] == "written"
    assert template_parameters["C"]["default_argument"]["kind"] == "template"
    assert template_parameters["C"]["default_argument"]["expression"] == "Box"
    assert template_parameters["C"]["default_argument"]["argument_origin"] == "written"
    assert all(
        not Path(parameter["default_argument"]["expression_source"]["source"]).is_absolute()
        for parameter in template_parameters.values()
        if parameter["default_argument"]
    )

    source_file = "submodules/systemc/src/defaults.hpp"
    empty, empty_reason = _INVENTORY["_source_slice"](source_file, 0, 0)
    assert empty is None and empty_reason == "source_expression_empty"
    bad_location = {"file": source_file, "offset": len(source.encode()) + 100, "tokLen": 1}
    bad_expression = {
        "range": {
            "begin": {"spellingLoc": bad_location, "expansionLoc": bad_location},
            "end": {"spellingLoc": bad_location, "expansionLoc": bad_location},
        }
    }
    macro_provenance = _INVENTORY["_macro_provenance"](bad_expression, source_file)
    assert macro_provenance[0]["expansion_token"] == {"unavailable_reason": "source_location_out_of_bounds"}
    assert macro_provenance[0]["spelling_tokens"] == [{"unavailable_reason": "source_location_out_of_bounds"}]


def test_redeclared_template_defaults_keep_the_written_declaration_source(tmp_path, monkeypatch):
    """Follow Clang template-parameter inheritance to the authored default span."""
    source = """namespace fixture {
template<class> struct Box {};
template<class T = long, int Width = sizeof(T)> struct Sequence;
template<class T, int Width> struct Sequence { T values[Width]; };
template<template<class> class Container = Box> struct Wrapped;
template<template<class> class Container> struct Wrapped {};
}
"""
    entries = _ast_entries(source, tmp_path, monkeypatch)
    sequences = [
        entry
        for entry in entries
        if entry["name"] == "fixture::Sequence" and entry["kind"] == "ClassTemplateDecl"
    ]
    wrapped = [
        entry
        for entry in entries
        if entry["name"] == "fixture::Wrapped" and entry["kind"] == "ClassTemplateDecl"
    ]
    assert len(sequences) == len(wrapped) == 2

    for declarations, expected in (
        (sequences, {"T": "long", "Width": "sizeof(T)"}),
        (wrapped, {"Container": "Box"}),
    ):
        written, redeclared = sorted(declarations, key=lambda entry: entry["line"])
        written_parameters = {item["name"]: item for item in written["template_parameters"]}
        redeclared_parameters = {item["name"]: item for item in redeclared["template_parameters"]}
        for name, expression in expected.items():
            written_default = written_parameters[name]["default_argument"]
            inherited_default = redeclared_parameters[name]["default_argument"]
            assert written_default["expression"] == inherited_default["expression"] == expression
            assert written_default["argument_origin"] == "written"
            assert inherited_default["argument_origin"] == "inherited"
            assert inherited_default["expression_source"] == written_default["expression_source"]
            assert inherited_default["expression_source"]["line_range"][0] < redeclared["line"]

    assert all("_template_parameter_nodes" not in entry for entry in entries)
    assert all(
        "_inherited_template_parameter_ast_id" not in (parameter.get("default_argument") or {})
        for entry in entries
        for parameter in entry["template_parameters"]
    )


def test_anonymous_sfinae_pointer_defaults_recover_macro_spelling_source(tmp_path, monkeypatch):
    """Recover unnamed dependent pointer defaults from their source macro body."""
    source = """#include <cstddef>
namespace fixture {
template<bool Condition> struct enable_if {};
template<> struct enable_if<true> { using type = void; };
#define SFINAE_POINTER_(Condition) typename enable_if<(Condition)>::type * = NULL
template<class Owner> struct direct_access {
    template<class U> direct_access(const U&, SFINAE_POINTER_(true)) {}
};
template<class Owner> struct vector_iter {
    template<class U> vector_iter(const U&, SFINAE_POINTER_(true)) {}
};
}
"""
    entries = _ast_entries(source, tmp_path, monkeypatch)
    constructors = [entry for entry in entries if entry["kind"] == "CXXConstructorDecl"]
    candidates = [
        entry
        for entry in constructors
        if entry["name"].startswith(("fixture::direct_access::direct_access", "fixture::vector_iter::vector_iter"))
        and len(entry["parameters"]) == 2
    ]
    assert len(candidates) == 2
    for entry in candidates:
        parameter = entry["parameters"][1]
        default = parameter["default_argument"]
        assert parameter["name"] is None
        assert default["expression"] == "NULL"
        assert default["argument_origin"] == "macro_generated"
        span = default["expression_source"]
        assert span["location_kind"] == "macro_spelling_source"
        assert span["source"] == "submodules/systemc/src/defaults.hpp"
        start, stop = span["byte_range"]
        source_bytes = source.encode("utf-8")
        assert source_bytes[start:stop].decode("utf-8") == "NULL"
        assert source.rfind("#define SFINAE_POINTER_") < start < source.index("template<class Owner>")
        assert span["line_range"][0] == span["line_range"][1]


def test_default_metadata_does_not_change_legacy_identity():
    """Hash template defaults outside the legacy identity field whitelist."""
    legacy = {
        "name": "fixture::Holder",
        "kind": "ClassTemplateDecl",
        "signature": None,
        "template_parameters": [{"name": "T", "kind": "TemplateTypeParmDecl", "type": None}],
        "template_arguments": [],
        "template_context": [],
        "source": "submodules/systemc/src/defaults.hpp",
    }
    enriched = deepcopy(legacy)
    enriched["template_parameters"][0].update(
        {
            "has_default": True,
            "default_argument": {"expression": "int", "kind": "type"},
        }
    )
    enriched["parameters"] = [{"has_default": True, "default_argument": {"expression": "7"}}]

    assert _INVENTORY["declaration_id"](enriched) == _INVENTORY["declaration_id"](legacy)

    missing_fields = deepcopy(legacy)
    missing_fields["template_parameters"] = [{"kind": "TemplateTypeParmDecl"}]
    explicit_nulls = deepcopy(legacy)
    explicit_nulls["template_parameters"] = [{"name": None, "kind": "TemplateTypeParmDecl", "type": None}]
    assert _INVENTORY["declaration_id"](missing_fields) != _INVENTORY["declaration_id"](explicit_nulls)

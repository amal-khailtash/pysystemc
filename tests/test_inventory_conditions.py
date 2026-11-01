"""Associate exact declaration source occurrences with lexical branch histories."""

from copy import deepcopy
import hashlib
import json
from pathlib import Path
import runpy
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]
_INVENTORY = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))


def _source(contents, tmp_path, monkeypatch, name="conditions.hpp"):
    """Write source bytes in an isolated portable depot and reset scanner caches."""
    path = tmp_path / "submodules/systemc/src" / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(contents.encode("utf-8") if isinstance(contents, str) else contents)
    monkeypatch.setitem(_INVENTORY["declarations"].__globals__, "ROOT", tmp_path)
    _INVENTORY["source_text"].cache_clear()
    _INVENTORY["_condition_source_index"].cache_clear()
    return path, path.relative_to(tmp_path).as_posix()


def _point(source, contents, token):
    """Locate a token using UTF-8 bytes, independently of the scanner."""
    data = contents.encode("utf-8") if isinstance(contents, str) else contents
    offset = data.index(token.encode("utf-8"))
    return _INVENTORY["_declaration_condition_point"]({"source": source, "offset": offset, "line": 999})


def test_nested_histories_continuations_and_utf8_byte_offsets(tmp_path, monkeypatch):
    """Retain guards, alternatives and outer frames with original physical lines."""
    contents = """#ifndef GUARD
#define GUARD
// éπ before byte-addressed tokens
int outside;
#if FIRST \\
    && SECOND
int first;
#if INNER
int inner;
#elif ALTERNATE
int alternate;
#else
int fallback;
#endif
#elif NEXT
int next;
#else
int last;
#endif
int after;
#endif
"""
    _, source = _source(contents, tmp_path, monkeypatch)
    expected = {
        "outside": [["#ifndef GUARD"]],
        "first": [["#ifndef GUARD"], ["#if FIRST && SECOND"]],
        "inner": [["#ifndef GUARD"], ["#if FIRST && SECOND"], ["#if INNER"]],
        "alternate": [["#ifndef GUARD"], ["#if FIRST && SECOND"], ["#if INNER", "#elif ALTERNATE"]],
        "fallback": [["#ifndef GUARD"], ["#if FIRST && SECOND"], ["#if INNER", "#elif ALTERNATE", "#else"]],
        "next": [["#ifndef GUARD"], ["#if FIRST && SECOND", "#elif NEXT"]],
        "last": [["#ifndef GUARD"], ["#if FIRST && SECOND", "#elif NEXT", "#else"]],
        "after": [["#ifndef GUARD"]],
    }
    for token, context in expected.items():
        record = _point(source, contents, token)
        assert record["conditional_context"] == context
        assert "unavailable_reason" not in record
        assert record["source_occurrence"]["line"] == contents[: contents.index(token)].count("\n") + 1
    directive = _point(source, contents, "SECOND")
    assert directive["conditional_context"] is None
    assert directive["unavailable_reason"] == "source_point_inside_directive"


def test_comments_literals_raw_strings_and_spliced_nondirectives(tmp_path, monkeypatch):
    """Ignore directive-shaped literal text after translation-phase line splicing."""
    contents = """/*
#if FAKE
*/
const char* raw = u8R"tag(
#if RAW_FAKE
#else
)tag";
const char* text = "#if STRING_FAKE";
char quote = '\\'';
auto wide = L'a';
auto utf16 = u'b';
auto utf32 = U'c';
auto utf8 = u8'd';
int count = 1'000;
// continued comment \\
#if COMMENT_FAKE
int line = 1 + \\
#if NOT_AT_LINE_START
/* prefix */ #if REAL
int yes;
#endif
int plain;
"""
    _, source = _source(contents, tmp_path, monkeypatch)
    assert _point(source, contents, "yes")["conditional_context"] == [["#if REAL"]]
    assert _point(source, contents, "plain")["conditional_context"] == []
    assert _point(source, contents, "RAW_FAKE")["conditional_context"] == []
    # A literal before # cannot become whitespace and manufacture a directive.
    contents = '"literal" #if FAKE\nint value;\n'
    _, source = _source(contents, tmp_path, monkeypatch, "literal.hpp")
    assert _point(source, contents, "value")["conditional_context"] == []


@pytest.mark.parametrize(
    ("contents", "reason"),
    [
        ("#if A\n#else\n#else\n#endif\nint value;", "duplicate_source_else"),
        ("#if A\n#else\n#elif B\n#endif\nint value;", "source_elif_after_else"),
        ("#endif\nint value;", "unmatched_source_endif"),
        ("#else\nint value;", "unmatched_source_else"),
        ("#elif B\nint value;", "unmatched_source_elif"),
        ("#if A\nint value;", "unclosed_source_conditional"),
        ("/*\nint value;", "unterminated_source_comment"),
        ('R"tag(\nint value;', "unterminated_source_raw_string"),
        ('"unterminated\nint value;', "unterminated_source_literal"),
        ("%:if A\nint value;", "unsupported_directive_digraph"),
        ("??=if A\nint value;", "unsupported_trigraph_source"),
        ("#if A\n#elifdef B\n#endif\nint value;", "unsupported_conditional_directive"),
        ("#if A /*\ncomment */\n#endif\nint value;", "unsupported_multiline_directive_comment"),
        ("#ébad\nint value;", "unsupported_directive_identifier"),
        ("#if A\n# /* comment */ else\n#else\n#endif\nint value;", "duplicate_source_else"),
    ],
)
def test_malformed_and_unsupported_files_have_explicit_unavailable_context(contents, reason, tmp_path, monkeypatch):
    """Never return an apparently valid empty or partial stack for invalid files."""
    _, source = _source(contents, tmp_path, monkeypatch)
    record = _point(source, contents, "value")
    assert record["conditional_context"] is None
    assert record["unavailable_reason"] == reason


def _ast_declaration(path, name, offset, **location):
    """Build a minimal sourced declaration with a stable portable canonical ID."""
    return {
        "kind": "VarDecl",
        "id": "ast:" + str(offset),
        "name": name,
        "type": {"qualType": "int"},
        "loc": {"file": str(path), "offset": offset, "tokLen": len(name), **location},
    }


def _extract(ast):
    """Assert opt-in source metadata leaves AST and final declaration rows unchanged."""
    before = deepcopy(ast)
    baseline = _INVENTORY["reconcile_declarations"](list(_INVENTORY["declarations"](ast)))
    conditions = []
    entries = list(_INVENTORY["declarations"](ast, collect_conditions=True))
    rows = _INVENTORY["reconcile_declarations"](entries, declaration_conditions=conditions)
    assert rows == baseline
    assert ast == before
    assert all(not key.startswith("_") for row in rows for key in row)
    without_sink = _INVENTORY["reconcile_declarations"](list(_INVENTORY["declarations"](ast, collect_conditions=True)))
    assert without_sink == baseline
    return rows, conditions


def test_same_id_distinct_offsets_and_exact_portable_identity(tmp_path, monkeypatch):
    """Keep every same-ID source site and calculate IDs without condition fields."""
    contents = "#if A\nextern int value;\n#else\nextern int value;\n#endif\n"
    path, source = _source(contents, tmp_path, monkeypatch)
    first = contents.index("value")
    second = contents.index("value", first + 1)
    ast = {"kind": "TranslationUnitDecl", "inner": [_ast_declaration(path, "value", o) for o in (first, second)]}
    rows, conditions = _extract(ast)
    assert len(conditions) == 2
    assert len({_INVENTORY["declaration_id"](r) for r in rows}) == 1
    assert len({c["occurrence_id"] for c in conditions}) == 2
    assert [c["conditional_context"] for c in conditions] == [[["#if A"]], [["#if A", "#else"]]]
    for record in conditions:
        identity = {
            "declaration_id": record["declaration_id"],
            "source": source,
            "offset": record["source_occurrence"]["offset"],
        }
        assert record["occurrence_id"] == hashlib.sha256(json.dumps(identity, sort_keys=True).encode()).hexdigest()
        assert record["location_kind"] == "written_source"


def test_compiled_macro_spelling_and_expansion_are_separate(tmp_path, monkeypatch):
    """Use real Clang source evidence for guarded definitions and invocation sites."""
    definition = "#ifndef DEFINITION\n#define DEFINITION\n#if 0\n#define EMIT int ignored;\n#else\n#define EMIT int emitted;\n#endif\n#endif\n"
    _, definition_source = _source(definition, tmp_path, monkeypatch, "definition.hpp")
    invocation = '#include "definition.hpp"\n#ifndef INVOCATION\n#define INVOCATION\n#if 1\nEMIT\n#endif\n#endif\n'
    path, invocation_source = _source(invocation, tmp_path, monkeypatch, "invocation.hpp")
    result = subprocess.run(
        ["clang++", "-std=c++17", "-Xclang", "-ast-dump=json", "-fsyntax-only", str(path)],
        capture_output=True,
        text=True,
        check=True,
        timeout=30,
    )
    ast = json.loads(result.stdout)
    _INVENTORY["restore_source_files"]([ast])
    rows, conditions = _extract(ast)
    row = next(row for row in rows if row["name"] == "emitted")
    record = next(c for c in conditions if c["declaration_id"] == _INVENTORY["declaration_id"](row))
    assert record["location_kind"] == "expansion_source"
    assert record["source_occurrence"]["source"] == invocation_source
    assert record["conditional_context"] == [["#ifndef INVOCATION"], ["#if 1"]]
    spelling = record["spelling_source"]
    assert spelling["source_occurrence"]["source"] == definition_source
    assert spelling["conditional_context"] is None
    assert spelling["unavailable_reason"] == "source_point_inside_directive"


def test_macro_spelling_outside_directive_keeps_its_own_history(tmp_path, monkeypatch):
    """Keep distinct AST points without importing definition conditions to invocation."""
    contents = "#if DEFINITION\nint spelled;\n#else\nint invocation;\n#endif\n"
    path, source = _source(contents, tmp_path, monkeypatch)
    loc = {
        "spellingLoc": {"file": str(path), "offset": contents.index("spelled"), "tokLen": 7},
        "expansionLoc": {"file": str(path), "offset": contents.index("invocation"), "tokLen": 10},
    }
    ast = _ast_declaration(path, "generated", 0)
    ast["loc"] = loc
    _, conditions = _extract(ast)
    record = conditions[0]
    assert record["source_occurrence"]["source"] == source
    assert record["conditional_context"] == [["#if DEFINITION", "#else"]]
    assert record["spelling_source"]["conditional_context"] == [["#if DEFINITION"]]
    ast["loc"]["spellingLoc"] = {"file": "/outside-depot/scratch", "offset": 0, "tokLen": 1}
    _, conditions = _extract(ast)
    assert conditions[0]["spelling_source"]["unavailable_reason"] == "source_location_unavailable"


def test_unavailable_sources_missing_offsets_and_invalid_byte_bounds(tmp_path, monkeypatch):
    """Retain unresolved records with null identities only when offset is missing."""
    path, source = _source("int value;\n", tmp_path, monkeypatch)
    ast = _ast_declaration(path, "value", 4, line=1)
    ast["loc"].pop("offset")
    _, conditions = _extract(ast)
    assert conditions[0]["occurrence_id"] is None
    assert conditions[0]["unavailable_reason"] == "source_offset_unavailable"
    for point, reason in [
        ({"source": source, "offset": -1}, "source_location_out_of_bounds"),
        ({"source": source, "offset": 999}, "source_location_out_of_bounds"),
        ({"source": "absent.hpp", "offset": 4}, "source_file_unavailable"),
        ({"source": "../../outside.hpp", "offset": 4}, "source_outside_depot"),
        ({"offset": 4}, "source_location_unavailable"),
    ]:
        record = _INVENTORY["_declaration_condition_point"](point)
        assert record["conditional_context"] is None
        assert record["unavailable_reason"] == reason
    _source(b"\xff int value;", tmp_path, monkeypatch, "invalid.hpp")
    record = _INVENTORY["_declaration_condition_point"]({"source": "submodules/systemc/src/invalid.hpp", "offset": 6})
    assert record["unavailable_reason"] == "source_file_not_utf8"

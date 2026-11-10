"""Extract source declarations for the M6A audit without claiming binding support.

Run with uv run python tools/inventory_public_api.py. Clang reads the checked-out
headers; the ignored JSON output preserves overload signatures and access levels.
Semantic evidence and internal/public classification require a separate review.
"""

import argparse
from bisect import bisect_right
from copy import deepcopy
from functools import cache
import hashlib
import json
import re
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADERS = tuple(sorted(path.name for path in (ROOT / "submodules/systemc/src/tlm_utils").glob("*.h")))

DECLARATIONS = {
    "CXXRecordDecl",
    "ClassTemplateDecl",
    "ClassTemplatePartialSpecializationDecl",
    "ClassTemplateSpecializationDecl",
    "FunctionDecl",
    "FunctionTemplateDecl",
    "CXXMethodDecl",
    "CXXConstructorDecl",
    "CXXDestructorDecl",
    "CXXConversionDecl",
    "EnumDecl",
    "EnumConstantDecl",
    "TypedefDecl",
    "TypeAliasDecl",
    "TypeAliasTemplateDecl",
    "VarDecl",
    "FieldDecl",
    "UsingDecl",
    "UsingDirectiveDecl",
    "NamespaceAliasDecl",
}


def json_roots(source):
    """Decode Clang's concatenated JSON roots produced by AST name filtering."""
    decoder = json.JSONDecoder()
    position = 0
    while position < len(source):
        while position < len(source) and source[position].isspace():
            position += 1
        if position == len(source):
            return
        node, position = decoder.raw_decode(source, position)
        yield node


@cache
def source_text(filename):
    """Cache header contents for line recovery from Clang byte offsets."""
    return Path(filename).read_bytes()


_LEGACY_TEMPLATE_PARAMETER_ID_FIELDS = ("name", "kind", "type")


def _portable_base_metadata(bases):
    """Copy Clang base metadata while dropping only compiler-local alias IDs.

    Clang embeds declaration pointer IDs for aliases used as base types. Those
    values vary between compiler invocations, while the surrounding type
    spellings and inheritance properties are useful source metadata.
    """
    portable = deepcopy(bases)

    def remove_alias_ids(value):
        if isinstance(value, dict):
            value.pop("typeAliasDeclId", None)
            for child in value.values():
                remove_alias_ids(child)
        elif isinstance(value, list):
            for child in value:
                remove_alias_ids(child)

    remove_alias_ids(portable)
    return portable


def _resolved_location(location, fallback_filename=None, *, expansion=False):
    """Resolve one Clang source point without serializing compiler-local state.

    Clang offsets address bytes, not Unicode code points. For a macro point,
    callers choose either the source spelling or the invocation expansion.
    """
    if not isinstance(location, dict):
        return None
    key = "expansionLoc" if expansion else "spellingLoc"
    point = location.get(key, location) if ("spellingLoc" in location or "expansionLoc" in location) else location
    if not isinstance(point, dict) or not isinstance(point.get("offset"), int):
        return None
    filename = point.get("file", fallback_filename)
    if not filename:
        return None
    path = Path(filename)
    if not path.is_absolute():
        path = ROOT / path
    path = path.resolve()
    if not path.is_relative_to(ROOT):
        return None
    token_length = point.get("tokLen")
    if not isinstance(token_length, int) or token_length < 0:
        return None
    return {
        "source": path.relative_to(ROOT).as_posix(),
        "offset": point["offset"],
        "token_length": token_length,
    }


def _using_target_reference(target, fallback_filename=None):
    """Copy the portable portion of one Clang UsingShadowDecl target reference.

    Keep its AST pointer only in a private field for exact reconciliation. The
    pointer is removed before inventory serialization and is never interpreted
    as a declaration identity.
    """
    if not isinstance(target, dict):
        return {
            "_target_ast_id": None,
            "kind": None,
            "name": None,
            "signature": None,
            "source": None,
        }
    location = _resolved_location(target.get("loc"), fallback_filename)
    target_type = target.get("type")
    return {
        "_target_ast_id": target.get("id"),
        "kind": target.get("kind"),
        "name": target.get("name"),
        "signature": target_type.get("qualType") if isinstance(target_type, dict) else None,
        "source": location["source"] if location else None,
    }


def _source_span(source_range, fallback_filename=None, *, expansion=False):
    """Return a checked, portable byte span for one Clang source range."""
    if not isinstance(source_range, dict):
        return None, "source_range_missing"
    begin = _resolved_location(source_range.get("begin"), fallback_filename, expansion=expansion)
    end = _resolved_location(source_range.get("end"), fallback_filename, expansion=expansion)
    if begin is None or end is None:
        return None, "source_location_unavailable"
    if begin["source"] != end["source"]:
        return None, "source_range_crosses_files"
    start = begin["offset"]
    stop = end["offset"] + end["token_length"]
    try:
        contents = source_text(ROOT / begin["source"])
    except OSError:
        return None, "source_file_unavailable"
    if start < 0 or stop < start or stop > len(contents):
        return None, "source_range_out_of_bounds"
    while start < stop and contents[start : start + 1].isspace():
        start += 1
    while stop > start and contents[stop - 1 : stop].isspace():
        stop -= 1
    if start == stop:
        return None, "source_expression_empty"
    try:
        expression = contents[start:stop].decode("utf-8")
    except UnicodeDecodeError:
        return None, "source_expression_not_utf8"
    start_line = contents.count(b"\n", 0, start) + 1
    end_line = contents.count(b"\n", 0, stop) + 1
    return {
        "expression": expression,
        "source": begin["source"],
        "byte_range": [start, stop],
        "line_range": [start_line, end_line],
        "location_kind": "written_source",
    }, None


def _range_has_macro(source_range):
    """Return whether either edge of a source range has macro provenance."""
    return isinstance(source_range, dict) and any(
        isinstance(source_range.get(edge), dict) and {"spellingLoc", "expansionLoc"} <= source_range[edge].keys()
        for edge in ("begin", "end")
    )


def _range_within(inner_range, outer_range, filename):
    """Check whether a macro expansion range belongs to this declaration site."""
    inner_begin = _resolved_location(inner_range.get("begin"), filename, expansion=True)
    inner_end = _resolved_location(inner_range.get("end"), filename, expansion=True)
    outer_begin = _resolved_location(outer_range.get("begin"), filename, expansion=True)
    outer_end = _resolved_location(outer_range.get("end"), filename, expansion=True)
    if not all((inner_begin, inner_end, outer_begin, outer_end)):
        return False
    if len({item["source"] for item in (inner_begin, inner_end, outer_begin, outer_end)}) != 1:
        return False
    return outer_begin["offset"] <= inner_begin["offset"] and inner_end["offset"] <= (
        outer_end["offset"] + outer_end["token_length"]
    )


def _default_separator(expression):
    """Find a top-level template-default `=` while ignoring nested syntax."""
    data = expression.encode("utf-8")
    round_depth = square_depth = brace_depth = angle_depth = 0
    quote = None
    escaped = False
    line_comment = block_comment = False
    index = 0
    while index < len(data):
        char = data[index]
        next_char = data[index + 1] if index + 1 < len(data) else None
        if line_comment:
            if char == 10:
                line_comment = False
        elif block_comment:
            if char == 42 and next_char == 47:
                block_comment = False
                index += 1
        elif quote is not None:
            if escaped:
                escaped = False
            elif char == 92:
                escaped = True
            elif char == quote:
                quote = None
        elif char == 47 and next_char == 47:
            line_comment = True
            index += 1
        elif char == 47 and next_char == 42:
            block_comment = True
            index += 1
        elif char in (34, 39):
            quote = char
        elif char == 40:
            round_depth += 1
        elif char == 41:
            round_depth = max(0, round_depth - 1)
        elif char == 91:
            square_depth += 1
        elif char == 93:
            square_depth = max(0, square_depth - 1)
        elif char == 123:
            brace_depth += 1
        elif char == 125:
            brace_depth = max(0, brace_depth - 1)
        elif char == 60:
            angle_depth += 1
        elif char == 62:
            angle_depth = max(0, angle_depth - 1)
        elif char == 61 and not any((round_depth, square_depth, brace_depth, angle_depth)):
            previous = data[index - 1] if index else None
            if previous not in (33, 60, 62, 61) and next_char != 61:
                return index
        index += 1
    return None


def _macro_provenance(expression_node, fallback_filename):
    """Collect compact spelling/expansion spans from a default expression AST."""
    records = {}

    def add_point(point):
        if not isinstance(point, dict) or not {"spellingLoc", "expansionLoc"} <= point.keys():
            return
        spelling = _resolved_location(point["spellingLoc"], fallback_filename)
        expansion = _resolved_location(point["expansionLoc"], fallback_filename, expansion=True)

        def record(location):
            if location is None:
                return {"unavailable_reason": "source_location_unavailable"}
            stop = location["offset"] + location["token_length"]
            try:
                contents = source_text(ROOT / location["source"])
            except OSError:
                return {"unavailable_reason": "source_file_unavailable"}
            if location["offset"] < 0 or location["token_length"] <= 0 or stop > len(contents):
                return {"unavailable_reason": "source_location_out_of_bounds"}
            return {"source": location["source"], "byte_range": [location["offset"], stop]}

        expansion_record = record(expansion)
        key = json.dumps(expansion_record, sort_keys=True, separators=(",", ":"))
        provenance = records.setdefault(key, {"expansion_token": expansion_record, "spelling_tokens": []})
        spelling_record = record(spelling)
        if spelling_record not in provenance["spelling_tokens"]:
            provenance["spelling_tokens"].append(spelling_record)

    def visit(node):
        if isinstance(node, dict):
            source_range = node.get("range")
            if isinstance(source_range, dict):
                for edge in ("begin", "end"):
                    add_point(source_range.get(edge))
            for child in node.get("inner", []):
                visit(child)
        elif isinstance(node, list):
            for child in node:
                visit(child)

    visit(expression_node)
    return [records[key] for key in sorted(records)]


def _extended_macro_stop(source_range, filename, stop):
    """Include call parentheses omitted from Clang's function-like macro range."""
    end = source_range.get("end", {}) if isinstance(source_range, dict) else {}
    if not isinstance(end, dict) or "expansionLoc" not in end:
        return stop
    location = _resolved_location(end, filename, expansion=True)
    if location is None:
        return stop
    token_stop = location["offset"] + location["token_length"]
    if token_stop != stop:
        return stop
    contents = source_text(ROOT / location["source"])
    cursor = token_stop
    while cursor < len(contents):
        if contents[cursor] in b" \t\r\n":
            cursor += 1
        elif contents[cursor : cursor + 2] == b"//":
            newline = contents.find(b"\n", cursor + 2)
            cursor = len(contents) if newline < 0 else newline + 1
        elif contents[cursor : cursor + 2] == b"/*":
            close = contents.find(b"*/", cursor + 2)
            if close < 0:
                return None
            cursor = close + 2
        else:
            break
    if cursor >= len(contents) or contents[cursor] != 40:
        return stop
    depth = 0
    quote = None
    escaped = False
    line_comment = block_comment = False
    while cursor < len(contents):
        char = contents[cursor]
        next_char = contents[cursor + 1] if cursor + 1 < len(contents) else None
        if line_comment:
            if char == 10:
                line_comment = False
        elif block_comment:
            if char == 42 and next_char == 47:
                block_comment = False
                cursor += 1
        elif quote is not None:
            if escaped:
                escaped = False
            elif char == 92:
                escaped = True
            elif char == quote:
                quote = None
        elif char == 47 and next_char == 47:
            line_comment = True
            cursor += 1
        elif char == 47 and next_char == 42:
            block_comment = True
            cursor += 1
        elif char in (34, 39):
            if char == 34 and any(
                contents[max(0, cursor - len(prefix)) : cursor] == prefix
                for prefix in (b"R", b"uR", b"UR", b"LR", b"u8R")
            ):
                return None
            quote = char
        elif char == 40:
            depth += 1
        elif char == 41:
            depth -= 1
            if depth == 0:
                return cursor + 1
        cursor += 1
    return None


def _default_argument(node, filename, *, template_kind=None):
    """Preserve written default source and compact macro provenance safely."""
    expression_node = None
    if template_kind is None:
        expression_node = next(
            (child for child in node.get("inner", []) if isinstance(child, dict) and "range" in child),
            None,
        )
        declaration_span, declaration_reason = _source_span(node.get("range"), filename, expansion=True)
        separator = _default_separator(declaration_span["expression"]) if declaration_span else None
        if declaration_span and separator is not None:
            left = declaration_span["byte_range"][0] + separator + 1
            right = _extended_macro_stop(node.get("range"), filename, declaration_span["byte_range"][1])
            if right is None:
                return {"expression": None, "unavailable_reason": "macro_invocation_boundary_unavailable"}
            span, reason = _source_slice(declaration_span["source"], left, right)
            argument_origin = "written"
        else:
            source_range = expression_node.get("range") if expression_node else None
            if _range_has_macro(source_range) and _range_within(source_range, node.get("range", {}), filename):
                span, reason = _source_span(source_range, filename, expansion=False)
                if span is None:
                    span = _macro_replacement_default(node, filename)
                    reason = None if span is not None else reason
                if span is not None:
                    span["location_kind"] = "macro_spelling_source"
                argument_origin = "macro_generated"
            else:
                span, reason = _source_span(source_range, filename, expansion=True)
                if span is not None:
                    stop = _extended_macro_stop(source_range, filename, span["byte_range"][1])
                    if stop is None:
                        return {"expression": None, "unavailable_reason": "macro_invocation_boundary_unavailable"}
                    if stop != span["byte_range"][1]:
                        span, reason = _source_slice(span["source"], span["byte_range"][0], stop)
                argument_origin = "inherited"
        if span is None:
            return {"expression": None, "unavailable_reason": reason or declaration_reason}
    else:
        inherited_from = node.get("defaultArg", {}).get("inherited from", {})
        inherited_id = inherited_from.get("id") if isinstance(inherited_from, dict) else None
        if inherited_id:
            return {
                "expression": None,
                "kind": template_kind,
                "unavailable_reason": "inherited_template_default_source_pending",
                "argument_origin": "inherited",
                "_inherited_template_parameter_ast_id": inherited_id,
            }
        declaration_span, reason = _source_span(node.get("range"), filename, expansion=True)
        if declaration_span is None:
            return {"expression": None, "kind": template_kind, "unavailable_reason": reason}
        declaration_stop = _extended_macro_stop(node.get("range"), filename, declaration_span["byte_range"][1])
        if declaration_stop is None:
            return {
                "expression": None,
                "kind": template_kind,
                "unavailable_reason": "macro_invocation_boundary_unavailable",
            }
        if declaration_stop != declaration_span["byte_range"][1]:
            declaration_span, reason = _source_slice(
                declaration_span["source"], declaration_span["byte_range"][0], declaration_stop
            )
            if declaration_span is None:
                return {"expression": None, "kind": template_kind, "unavailable_reason": reason}
        separator = _default_separator(declaration_span["expression"])
        if separator is None:
            return {
                "expression": None,
                "kind": template_kind,
                "unavailable_reason": "template_default_separator_unavailable",
            }
        start = declaration_span["byte_range"][0] + separator + 1
        stop = declaration_span["byte_range"][1]
        span, reason = _source_slice(declaration_span["source"], start, stop)
        if span is None:
            return {"expression": None, "kind": template_kind, "unavailable_reason": reason}
        expression_node = next(
            (
                child
                for child in reversed(node.get("inner", []))
                if isinstance(child, dict) and child.get("kind") == "TemplateArgument"
            ),
            None,
        )
        argument_origin = "written"
    result = {
        "expression": span["expression"],
        "expression_source": {key: span[key] for key in ("source", "byte_range", "line_range", "location_kind")},
    }
    if template_kind is not None:
        result["kind"] = template_kind
    result["argument_origin"] = argument_origin if template_kind is None else "written"
    if expression_node is not None:
        result["macro_provenance"] = _macro_provenance(expression_node, filename)
    return result


def _source_slice(source, start, stop):
    """Extract and validate a trimmed UTF-8 slice using absolute byte offsets."""
    try:
        contents = source_text(ROOT / source)
    except OSError:
        return None, "source_file_unavailable"
    if start < 0 or stop < start or stop > len(contents):
        return None, "source_range_out_of_bounds"
    while start < stop and contents[start : start + 1].isspace():
        start += 1
    while stop > start and contents[stop - 1 : stop].isspace():
        stop -= 1
    if start == stop:
        return None, "source_expression_empty"
    try:
        expression = contents[start:stop].decode("utf-8")
    except UnicodeDecodeError:
        return None, "source_expression_not_utf8"
    return {
        "expression": expression,
        "source": source,
        "byte_range": [start, stop],
        "line_range": [contents.count(b"\n", 0, start) + 1, contents.count(b"\n", 0, stop) + 1],
        "location_kind": "written_source",
    }, None


def _macro_replacement_default(node, fallback_filename):
    """Recover an authored macro-default token when nested macro locations cross files.

    Clang can report a default argument such as ``= NULL`` at the final system
    header expansion of ``NULL`` while the parameter itself came from a
    first-party macro. In that case, use the AST spelling point to find the
    containing source macro definition and the expansion point to confirm which
    macro was invoked. The expression text and source span are read from that
    definition; no replacement text is synthesized.
    """
    source_range = node.get("range", {})
    begin = source_range.get("begin", {})
    spelling = _resolved_location(begin, fallback_filename)
    expansion = _resolved_location(begin, fallback_filename, expansion=True)
    if spelling is None or expansion is None:
        return None
    try:
        spelling_bytes = source_text(ROOT / spelling["source"])
        expansion_bytes = source_text(ROOT / expansion["source"])
    except OSError:
        return None

    invocation = re.match(rb"[A-Za-z_][A-Za-z_0-9]*", expansion_bytes[expansion["offset"] :])
    if invocation is None:
        return None
    invoked_name = invocation.group().decode("ascii")

    directive_start = 0
    definition = None
    while directive_start < len(spelling_bytes):
        newline = spelling_bytes.find(b"\n", directive_start)
        line_stop = len(spelling_bytes) if newline < 0 else newline + 1
        first_line = spelling_bytes[directive_start:line_stop].rstrip(b"\r\n")
        directive_stop = line_stop
        continued = first_line.endswith(b"\\")
        while continued and directive_stop < len(spelling_bytes):
            newline = spelling_bytes.find(b"\n", directive_stop)
            next_stop = len(spelling_bytes) if newline < 0 else newline + 1
            line = spelling_bytes[directive_stop:next_stop].rstrip(b"\r\n")
            continued = line.endswith(b"\\")
            directive_stop = next_stop
        if directive_start <= spelling["offset"] < directive_stop:
            if first_line.lstrip().startswith(b"#define"):
                definition = (directive_start, directive_stop)
            break
        directive_start = directive_stop
    if definition is None:
        return None

    start, stop = definition
    logical = []
    byte_offsets = []
    cursor = start
    while cursor < stop:
        if spelling_bytes[cursor : cursor + 2] == b"\\\n":
            cursor += 2
            continue
        if spelling_bytes[cursor : cursor + 3] == b"\\\r\n":
            cursor += 3
            continue
        logical.append(chr(spelling_bytes[cursor]))
        byte_offsets.append(cursor)
        cursor += 1
    text = "".join(logical)
    macro = re.match(r"\s*#\s*define\s+([A-Za-z_][A-Za-z_0-9]*)", text)
    if macro is None or macro.group(1) != invoked_name:
        return None
    body_start = macro.end()
    if body_start < len(text) and text[body_start] == "(":
        depth = 1
        body_start += 1
        while body_start < len(text) and depth:
            if text[body_start] == "(":
                depth += 1
            elif text[body_start] == ")":
                depth -= 1
            body_start += 1
        if depth:
            return None
    while body_start < len(text) and text[body_start].isspace():
        body_start += 1
    body = text[body_start:]
    separator = _default_separator(body)
    if separator is None:
        return None
    expression_start = body_start + separator + 1
    expression_stop = len(text)
    while expression_start < expression_stop and text[expression_start].isspace():
        expression_start += 1
    while expression_stop > expression_start and text[expression_stop - 1].isspace():
        expression_stop -= 1
    if expression_start == expression_stop:
        return None
    span, _ = _source_slice(
        spelling["source"],
        byte_offsets[expression_start],
        byte_offsets[expression_stop - 1] + 1,
    )
    if span is not None:
        span["location_kind"] = "macro_spelling_source"
    return span


def restore_source_files(roots):
    """Restore omitted Clang filenames in serialized source-location order.

    Clang elides repeated filenames across the JSON stream, including within a
    macro location: ``spellingLoc`` is serialized before ``expansionLoc``. Walk
    each node's fields in their JSON order so an explicit spelling filename can
    restore an elided expansion filename. The cursor spans source locations in
    ``loc`` and ``range`` fields and sibling AST nodes.
    """
    current = None

    def restore_location(location):
        """Fill one ordinary or macro source location in serialization order."""
        nonlocal current

        if not isinstance(location, dict) or not location:
            return
        if "spellingLoc" in location or "expansionLoc" in location:
            # Preserve Clang's emitted order (normally spelling, then expansion).
            # `includedFrom.file` is include metadata, not this location's file.
            for key, child in location.items():
                if key in {"spellingLoc", "expansionLoc"}:
                    restore_location(child)
            return
        if "file" in location:
            current = location["file"]
        elif current:
            location["file"] = current

    def restore_range(source_range):
        """Restore begin/end source points in the order Clang serialized them."""
        if not isinstance(source_range, dict):
            return
        for key, location in source_range.items():
            if key in {"begin", "end"}:
                restore_location(location)

    def visit(value):
        """Walk AST JSON fields and child nodes without changing field order."""
        if isinstance(value, list):
            for child in value:
                visit(child)
            return
        if not isinstance(value, dict):
            return
        for key, child in value.items():
            if key == "loc":
                restore_location(child)
            elif key == "range":
                restore_range(child)
            else:
                visit(child)

    for root in roots:
        visit(root)
    return roots


def _friend_location(location, filename):
    """Copy source points, retaining both macro spelling and expansion provenance."""
    if not isinstance(location, dict):
        return None
    if "spellingLoc" in location or "expansionLoc" in location:
        points = {
            key: point
            for key in ("spellingLoc", "expansionLoc")
            if (point := _friend_location(location.get(key), filename)) is not None
        }
        return points or None
    source = location.get("file", filename)
    if not source:
        return None
    path = (ROOT / source).resolve()
    if not path.is_relative_to(ROOT):
        return None
    point = {"source": path.relative_to(ROOT).as_posix()}
    for key in ("line", "col", "offset", "tokLen"):
        if key in location:
            point[key] = location[key]
    if "line" not in point and "offset" in point:
        try:
            point["line"] = source_text(path).count(b"\n", 0, point["offset"]) + 1
        except OSError:
            pass
    return point if "offset" in point or "line" in point else None


@cache
def _condition_source_index(filename):
    """Index lexical conditional histories by original byte offset.

    Translation-phase backslash/newline splicing precedes comment and literal
    recognition. The original header ledger scanner is deliberately independent.
    Invalid or unsupported lexical input makes the entire file unavailable.
    """
    try:
        original = source_text(filename)
    except OSError:
        return None, "source_file_unavailable"
    try:
        original.decode("utf-8")
    except UnicodeDecodeError:
        return None, "source_file_not_utf8"
    if b"??" in original and re.search(rb"\?\?[=/\'()!<>-]", original):
        return None, "unsupported_trigraph_source"
    data = bytearray()
    offsets = []
    index = 0
    while index < len(original):
        splice = re.match(rb"\\\r?\n", original[index : index + 3])
        if splice:
            index += len(splice[0])
        else:
            data.append(original[index])
            offsets.append(index)
            index += 1
    data = bytes(data)
    mask = bytearray(data)

    def hide(start, end, replacement=32):
        """Replace literal/comment bytes while preserving logical line boundaries."""
        for position in range(start, end):
            if data[position] not in (10, 13):
                mask[position] = replacement

    index = 0
    while index < len(data):
        if data.startswith(b"//", index):
            stop = data.find(b"\n", index)
            stop = len(data) if stop < 0 else stop
            hide(index, stop)
            index = stop
        elif data.startswith(b"/*", index):
            stop = data.find(b"*/", index + 2)
            if stop < 0:
                return None, "unterminated_source_comment"
            line_start = data.rfind(b"\n", 0, index) + 1
            if b"\n" in data[index : stop + 2] and re.match(rb"\s*(?:#|%:)", mask[line_start:index]):
                return None, "unsupported_multiline_directive_comment"
            hide(index, stop + 2)
            index = stop + 2
        else:
            raw = re.match(rb'(?:u8|u|U|L)?R"([^\s()\\]{0,16})\(', data[index : index + 24])
            if raw and (index == 0 or not (chr(data[index - 1]).isalnum() or data[index - 1] == 95)):
                stop = data.find(b")" + raw[1] + b'"', index + len(raw[0]))
                if stop < 0:
                    return None, "unterminated_source_raw_string"
                stop += len(raw[1]) + 2
                hide(index, stop, 120)
                index = stop
            elif data[index] in (34, 39):
                number_start = index
                if data[index] == 39:
                    while number_start > 0 and (
                        chr(data[number_start - 1]).isalnum() or data[number_start - 1] in b"_.'"
                    ):
                        number_start -= 1
                number_prefix = data[number_start:index]
                if (
                    data[index] == 39
                    and index + 1 < len(data)
                    and re.match(rb"(?:[0-9]|\.[0-9])", number_prefix)
                    and chr(data[index + 1]).isalnum()
                ):
                    index += 1
                    continue
                quote = data[index]
                stop = index + 1
                while stop < len(data) and data[stop] != quote:
                    if data[stop] in (10, 13):
                        return None, "unterminated_source_literal"
                    stop += 2 if data[stop] == 92 else 1
                if stop >= len(data):
                    return None, "unterminated_source_literal"
                hide(index, stop + 1, 120)
                index = stop + 1
            else:
                index += 1
    stack = []
    else_seen = []
    events = [0]
    contexts = [[]]
    spans = []
    position = 0
    for logical_line in bytes(mask).splitlines(keepends=True):
        end = position + len(logical_line)
        if re.match(rb"\s*%:", logical_line):
            return None, "unsupported_directive_digraph"
        directive_start = re.match(rb"[ \t\v\f\r]*#", logical_line)
        if directive_start:
            match = re.match(rb"[ \t\v\f\r]*#\s*([A-Za-z_][A-Za-z_0-9]*)\b", logical_line)
            if match is None and logical_line[directive_start.end() :].strip():
                return None, "unsupported_directive_identifier"
            first = offsets[position]
            stop = offsets[end - 1] + 1
            # Match the existing textual continuation presentation, retaining
            # comments and literal spelling in directive operands.
            hash_offset = offsets[position + logical_line.index(b"#")]
            physical = original[hash_offset:stop].decode("utf-8").splitlines()
            directive = physical[0].strip()
            for continuation in physical[1:]:
                directive = directive[:-1].rstrip() + " " + continuation.strip()
            operation = match[1].decode("ascii") if match else ""
            spans.append((first, stop))
            if operation in {"if", "ifdef", "ifndef"}:
                stack.append([directive])
                else_seen.append(False)
            elif operation in {"elif", "else"}:
                if not stack:
                    return None, "unmatched_source_" + operation
                if else_seen[-1]:
                    return None, "duplicate_source_else" if operation == "else" else "source_elif_after_else"
                stack[-1].append(directive)
                else_seen[-1] = operation == "else"
            elif operation == "endif":
                if not stack:
                    return None, "unmatched_source_endif"
                stack.pop()
                else_seen.pop()
            elif operation in {"elifdef", "elifndef"}:
                return None, "unsupported_conditional_directive"
            events.append(stop)
            contexts.append(deepcopy(stack))
        position = end
    if stack:
        return None, "unclosed_source_conditional"
    return {"contents": original, "events": events, "contexts": contexts, "spans": spans}, None


def _declaration_condition_point(point):
    """Associate one portable source point without inferring missing byte offsets."""
    occurrence = {key: point.get(key) for key in ("source", "line", "offset")}
    result = {"source_occurrence": occurrence, "conditional_context": None}
    source = occurrence["source"]
    offset = occurrence["offset"]
    if not source:
        result["unavailable_reason"] = "source_location_unavailable"
        return result
    path = (ROOT / source).resolve()
    if not path.is_relative_to(ROOT):
        result["unavailable_reason"] = "source_outside_depot"
        return result
    if not isinstance(offset, int) or isinstance(offset, bool):
        result["unavailable_reason"] = "source_offset_unavailable"
        return result
    indexed, reason = _condition_source_index(path)
    if indexed is None:
        result["unavailable_reason"] = reason
        return result
    if offset < 0 or offset >= len(indexed["contents"]):
        result["unavailable_reason"] = "source_location_out_of_bounds"
        return result
    occurrence["line"] = indexed["contents"].count(b"\n", 0, offset) + 1
    if any(start <= offset < stop for start, stop in indexed["spans"]):
        result["unavailable_reason"] = "source_point_inside_directive"
        return result
    result["conditional_context"] = deepcopy(indexed["contexts"][bisect_right(indexed["events"], offset) - 1])
    return result


def collect_declaration_conditions(entries, sink):
    """Append exact same-row condition records and remove private location metadata.

    Call only after semantic declaration reconciliation in one AST invocation.
    Macro spelling context remains separate from invocation context. No source
    location, name or compiler pointer is matched against a different row.
    """
    for entry in entries:
        location = entry.pop("_condition_location", None)
        if sink is None:
            continue
        # Existing emitted source occurrence is always the primary association.
        point = {key: entry.get(key) for key in ("source", "line", "offset")}
        record = _declaration_condition_point(point)
        record["declaration_id"] = declaration_id(entry)
        record["location_kind"] = "expansion_source" if location and "expansionLoc" in location else "written_source"
        occurrence = record["source_occurrence"]
        if (
            isinstance(occurrence["offset"], int)
            and not isinstance(occurrence["offset"], bool)
            and occurrence["source"]
        ):
            identity = {
                "declaration_id": record["declaration_id"],
                "source": occurrence["source"],
                "offset": occurrence["offset"],
            }
            record["occurrence_id"] = hashlib.sha256(json.dumps(identity, sort_keys=True).encode()).hexdigest()
        else:
            record["occurrence_id"] = None
        if location and "spellingLoc" in location:
            record["spelling_source"] = _declaration_condition_point(location["spellingLoc"] or {})
        sink.append(record)


def _friend_template_metadata(node):
    """Copy target or owner template spelling without importing AST pointer IDs."""
    metadata = {}
    parameters = []
    arguments = []
    for child in node.get("inner", []):
        if child.get("kind") in {"TemplateTypeParmDecl", "NonTypeTemplateParmDecl", "TemplateTemplateParmDecl"}:
            parameter = {
                "name": child.get("name"),
                "kind": child["kind"],
                "type": child.get("type", {}).get("qualType"),
            }
            for key in ("isParameterPack", "depth", "index"):
                if key in child:
                    parameter[key] = child[key]
            parameters.append(parameter)
        elif child.get("kind") == "TemplateArgument":
            arguments.append(
                _portable_base_metadata({key: child[key] for key in ("type", "value", "isPack") if key in child})
            )
    if parameters:
        metadata["template_parameters"] = parameters
    if arguments:
        metadata["template_arguments"] = arguments
    return metadata


def _friend_relationship(node, filename, owner):
    """Collect one lexical friend occurrence with invocation-local endpoint references.

    Source identity excludes the owner/target payload so extraction configurations
    can retain several interpretations of the same occurrence. Only direct child
    declarations and their template pattern children establish target references.
    """
    location = _friend_location(node.get("loc"), filename)
    source_range = {
        edge: point
        for edge in ("begin", "end")
        if (point := _friend_location(node.get("range", {}).get(edge), filename)) is not None
    }
    preferred = location
    if location and ("spellingLoc" in location or "expansionLoc" in location):
        preferred = location.get("expansionLoc", location.get("spellingLoc"))
    if preferred is None:
        preferred = source_range.get("begin")
        if preferred and ("spellingLoc" in preferred or "expansionLoc" in preferred):
            preferred = preferred.get("expansionLoc", preferred.get("spellingLoc"))
    if preferred is None:
        return None
    occurrence = {key: preferred[key] for key in ("source", "line", "offset") if key in preferred}
    occurrence["location_basis"] = "expansion" if location and "expansionLoc" in location else "spelling"
    if location:
        occurrence["location"] = location
    if source_range:
        occurrence["source_range"] = source_range
    direct = [child for child in node.get("inner", []) if child.get("kind") in DECLARATIONS]
    references = []
    for child in direct:
        references.append(child.get("id"))
        if child["kind"] in {"ClassTemplateDecl", "FunctionTemplateDecl"}:
            references.extend(
                pattern.get("id")
                for pattern in child.get("inner", [])
                if pattern.get("kind") in {"CXXRecordDecl", "FunctionDecl"} and not pattern.get("isImplicit")
            )
    if direct:
        child = direct[0]
        target = {
            "kind": child["kind"],
            "name": child.get("name"),
            "signature": child.get("type", {}).get("qualType"),
            **_friend_template_metadata(child),
        }
    else:
        target = {"kind": "type", "written_type": node.get("type", {}).get("qualType")}
        if "desugaredQualType" in node.get("type", {}):
            target["desugared_type"] = node["type"]["desugaredQualType"]
    return {
        "occurrence_id": hashlib.sha256(json.dumps(occurrence, sort_keys=True).encode()).hexdigest(),
        "source_occurrence": occurrence,
        "owner": deepcopy(owner) if owner else {"name": None, "kind": None, "template_context": [], "_ast_ids": []},
        "target": target,
        "_target_ast_ids": references,
        "target_declaration_ids": [],
        "resolution": "child_declaration" if direct else "type_only_unresolved",
    }


def declarations(
    node,
    scope=(),
    filename=None,
    access="public",
    namespace=(),
    template_context=(),
    using_target_refs=(),
    friend_relationships=None,
    _friend_owner=None,
    _friend_template_context=(),
    collect_conditions=False,
    _parent_record_id=None,
):
    """Walk declaration contexts, retaining protected/private candidates for review.

    Function bodies, implicit declarations and locationless AST cross-references
    are excluded. Template patterns
    retain their original dependent signatures rather than a fixed type matrix.
    An optional ``friend_relationships`` list receives lexical source edges;
    reconcile it with these entries within the same compiler invocation.
    Friends with no portable source point or range are excluded from that sink.
    """
    location = node.get("loc", {})
    location = location.get("expansionLoc", location.get("spellingLoc", location))
    filename = location.get("file", filename)
    kind = node["kind"]
    name = node.get("name", "")
    if node.get("isImplicit"):
        return
    # Locationless records can own sourced template members. Traverse their
    # context even though the record itself is not a source declaration row.
    if (
        kind in DECLARATIONS
        and "loc" not in node
        and kind not in {"CXXRecordDecl", "ClassTemplateSpecializationDecl", "ClassTemplatePartialSpecializationDecl"}
    ):
        return
    if kind == "UsingDirectiveDecl":
        name = "using namespace " + node.get("nominatedNamespace", {}).get("name", "<anonymous>")
    if kind in DECLARATIONS and name and filename and location:
        path = (ROOT / filename).resolve()
        if path.is_relative_to(ROOT / "submodules/systemc/src"):
            entry = {
                "ast_id": node.get("id"),
                "previous_decl": node.get("previousDecl"),
                "parent_context": node.get("parentDeclContextId"),
                "_parent_record_id": _parent_record_id,
                "name": "::".join((*scope, name)),
                "kind": kind,
                "signature": node.get("type", {}).get("qualType"),
                "written_static": node.get("storageClass") == "static",
                "static": node.get("storageClass") == "static",
                "template_context": list(template_context),
                "template_arguments": [
                    {key: deepcopy(value) for key, value in child.items() if key in {"type", "value", "isPack"}}
                    for child in node.get("inner", [])
                    if child["kind"] == "TemplateArgument"
                ],
                "template_parameters": [
                    {
                        "name": child.get("name"),
                        "kind": child["kind"],
                        "type": child.get("type", {}).get("qualType"),
                        "has_default": "defaultArg" in child,
                        "default_argument": _default_argument(
                            child,
                            filename,
                            template_kind={
                                "TemplateTypeParmDecl": "type",
                                "NonTypeTemplateParmDecl": "expression",
                                "TemplateTemplateParmDecl": "template",
                            }[child["kind"]],
                        )
                        if "defaultArg" in child
                        else None,
                    }
                    for child in node.get("inner", [])
                    if child["kind"] in {"TemplateTypeParmDecl", "NonTypeTemplateParmDecl", "TemplateTemplateParmDecl"}
                ],
                # Kept only until `reconcile_declarations` resolves Clang's
                # `defaultArg.inherited from` edges for template parameters.
                "_template_parameter_nodes": [
                    child
                    for child in node.get("inner", [])
                    if child["kind"] in {"TemplateTypeParmDecl", "NonTypeTemplateParmDecl", "TemplateTemplateParmDecl"}
                ],
                "source": path.relative_to(ROOT).as_posix(),
                "line": location.get("line")
                or (source_text(path).count(b"\n", 0, location["offset"]) + 1 if "offset" in location else None),
                "offset": location.get("offset"),
                "access": access,
                "written_virtual": node.get("virtual", False),
                "virtual": node.get("virtual", False),
                "_has_override_attribute": any(
                    child.get("kind") == "OverrideAttr" for child in node.get("inner", [])
                ),
                "pure": node.get("pure", False),
                "bases": _portable_base_metadata(node.get("bases", [])),
                "parameters": [
                    {
                        "name": child.get("name"),
                        "type": child.get("type", {}).get("qualType"),
                        "has_default": "init" in child,
                        "default_argument": _default_argument(child, filename) if "init" in child else None,
                    }
                    for child in node.get("inner", [])
                    if child["kind"] == "ParmVarDecl"
                ],
                "python_candidate": "amal.eda.systemc." + ".".join((*scope, name)),
                "classification": "unreviewed",
                "status": "untested",
                "evidence": [],
            }
            if collect_conditions:
                raw_location = node.get("loc", {})
                entry["_condition_location"] = (
                    {
                        key: _friend_location(raw_location[key], filename)
                        for key in ("spellingLoc", "expansionLoc")
                        if key in raw_location
                    }
                    if "spellingLoc" in raw_location or "expansionLoc" in raw_location
                    else _friend_location(raw_location, filename)
                )
            if kind == "UsingDecl":
                entry["_using_target_refs"] = deepcopy(using_target_refs)
            yield entry
    if kind not in {
        "NamespaceDecl",
        "CXXRecordDecl",
        "ClassTemplateDecl",
        "EnumDecl",
        "ClassTemplatePartialSpecializationDecl",
        "ClassTemplateSpecializationDecl",
        "FriendDecl",
        "FunctionTemplateDecl",
        "TypeAliasTemplateDecl",
        "LinkageSpecDecl",
        "TranslationUnitDecl",
    }:
        return
    nested_scope = scope
    if name and (
        kind
        in {
            "NamespaceDecl",
            "CXXRecordDecl",
            "ClassTemplatePartialSpecializationDecl",
            "ClassTemplateSpecializationDecl",
        }
        or (kind == "EnumDecl" and node.get("scopedEnumTag"))
    ):
        nested_scope = (*scope, name)
    if kind in {"ClassTemplateSpecializationDecl", "ClassTemplatePartialSpecializationDecl"}:
        arguments = [
            {key: deepcopy(value) for key, value in child.items() if key in {"type", "value", "isPack"}}
            for child in node.get("inner", [])
            if child["kind"] == "TemplateArgument"
        ]
        template_context = (*template_context, {"name": "::".join(nested_scope), "arguments": arguments})
    if kind == "NamespaceDecl" and name:
        namespace = (*namespace, name)
    if friend_relationships is not None:
        if kind == "ClassTemplateDecl" and name:
            _friend_template_context = (
                *_friend_template_context,
                {"name": "::".join((*scope, name)), **_friend_template_metadata(node)},
            )
        elif kind in {"ClassTemplateSpecializationDecl", "ClassTemplatePartialSpecializationDecl"} and name:
            _friend_template_context = (
                *_friend_template_context,
                {"name": "::".join(nested_scope), **_friend_template_metadata(node)},
            )
        if (
            kind
            in {
                "ClassTemplateDecl",
                "CXXRecordDecl",
                "ClassTemplateSpecializationDecl",
                "ClassTemplatePartialSpecializationDecl",
            }
            and name
        ):
            owner_name = "::".join((*scope, name))
            owner_ids = [node.get("id")]
            if (
                kind == "CXXRecordDecl"
                and _friend_owner
                and _friend_owner["kind"] == "ClassTemplateDecl"
                and _friend_owner["name"] == owner_name
            ):
                owner_ids = [*_friend_owner["_ast_ids"], *owner_ids]
            _friend_owner = {
                "name": owner_name,
                "kind": kind,
                "template_context": deepcopy(list(_friend_template_context)),
                "_ast_ids": owner_ids,
            }
        if kind == "FriendDecl" and filename:
            path = (ROOT / filename).resolve()
            if path.is_relative_to(ROOT / "submodules/systemc/src"):
                relationship = _friend_relationship(node, filename, _friend_owner)
                if relationship is not None:
                    friend_relationships.append(relationship)
    child_access = (
        access
        if kind == "EnumDecl"
        else "private"
        if kind in {"CXXRecordDecl", "ClassTemplateSpecializationDecl", "ClassTemplatePartialSpecializationDecl"}
        and node.get("tagUsed") == "class"
        else "public"
    )
    children = node.get("inner", [])
    nested_parent_record_id = (
        node.get("id")
        if kind in {"CXXRecordDecl", "ClassTemplateSpecializationDecl", "ClassTemplatePartialSpecializationDecl"}
        else _parent_record_id
    )
    for index, child in enumerate(children):
        if child["kind"] == "AccessSpecDecl":
            child_access = child["access"]
        else:
            using_target_refs = []
            if child["kind"] == "UsingDecl":
                for shadow in children[index + 1 :]:
                    if not isinstance(shadow, dict) or shadow.get("kind") != "UsingShadowDecl":
                        break
                    using_target_refs.append(_using_target_reference(shadow.get("target"), filename))
            yield from declarations(
                child,
                namespace if kind == "FriendDecl" else nested_scope,
                filename,
                "public" if kind == "FriendDecl" else child_access,
                namespace,
                template_context,
                using_target_refs,
                friend_relationships,
                _friend_owner,
                _friend_template_context,
                collect_conditions,
                nested_parent_record_id,
            )


def reconcile_declarations(entries, friend_relationships=None, declaration_conditions=None):
    """Resolve semantic owners using Clang redeclaration/context links.

    Out-of-class definitions inherit access and template context from their
    declaration. AST pointer IDs are used only within one compiler invocation
    and removed before serialization. Distinct source occurrences are retained.
    Resolve an optional friend sidecar here before combining compiler invocations;
    endpoint IDs never use spelling guesses or synthetic declaration identities.
    """
    indexed = {entry["ast_id"]: entry for entry in entries if entry.get("ast_id")}
    template_parameter_nodes = {
        parameter_node["id"]: (parameter_node, entry["source"])
        for entry in entries
        for parameter_node in entry.get("_template_parameter_nodes", [])
        if parameter_node.get("id")
    }

    def resolve_template_default(node, filename, seen=()):
        """Follow Clang default-argument links to the declaration that wrote it."""
        inherited_from = node.get("defaultArg", {}).get("inherited from", {})
        inherited_id = inherited_from.get("id") if isinstance(inherited_from, dict) else None
        if not inherited_id:
            kind = {
                "TemplateTypeParmDecl": "type",
                "NonTypeTemplateParmDecl": "expression",
                "TemplateTemplateParmDecl": "template",
            }[node["kind"]]
            return _default_argument(node, filename, template_kind=kind)
        if inherited_id in seen:
            return {
                "expression": None,
                "kind": {
                    "TemplateTypeParmDecl": "type",
                    "NonTypeTemplateParmDecl": "expression",
                    "TemplateTemplateParmDecl": "template",
                }[node["kind"]],
                "unavailable_reason": "inherited_template_default_cycle",
                "argument_origin": "inherited",
            }
        prior = template_parameter_nodes.get(inherited_id)
        if prior is None:
            return {
                "expression": None,
                "kind": {
                    "TemplateTypeParmDecl": "type",
                    "NonTypeTemplateParmDecl": "expression",
                    "TemplateTemplateParmDecl": "template",
                }[node["kind"]],
                "unavailable_reason": "inherited_template_default_source_unavailable",
                "argument_origin": "inherited",
            }
        prior_node, prior_filename = prior
        recovered = resolve_template_default(prior_node, prior_filename, (*seen, inherited_id))
        recovered.pop("_inherited_template_parameter_ast_id", None)
        if recovered.get("expression") is not None:
            recovered.pop("unavailable_reason", None)
        recovered["argument_origin"] = "inherited"
        return recovered

    for entry in entries:
        for index, parameter in enumerate(entry.get("template_parameters", [])):
            if not parameter.get("default_argument"):
                continue
            parameter_nodes = entry.get("_template_parameter_nodes", [])
            if index >= len(parameter_nodes):
                continue
            parameter["default_argument"] = resolve_template_default(parameter_nodes[index], entry["source"])

    for entry in entries:
        previous = indexed.get(entry.get("previous_decl"))
        parent = indexed.get(entry.get("parent_context"))
        if previous:
            for field in ("name", "access", "template_context"):
                entry[field] = deepcopy(previous[field])
            for field in ("parameters", "template_parameters"):
                previous_parameters = previous.get(field, [])
                for index, parameter in enumerate(entry.get(field, [])):
                    default = parameter.get("default_argument")
                    if not default or default.get("argument_origin") != "inherited":
                        continue
                    prior = (
                        previous_parameters[index].get("default_argument") if index < len(previous_parameters) else None
                    )
                    if prior and prior.get("expression") is not None:
                        parameter["default_argument"] = deepcopy(prior)
                        parameter["default_argument"]["argument_origin"] = "inherited"
                    else:
                        parameter["default_argument"] = {
                            "expression": None,
                            "unavailable_reason": "inherited_default_source_unavailable",
                            "argument_origin": "inherited",
                        }
        elif parent:
            entry["name"] = parent["name"] + "::" + entry["name"].rsplit("::", 1)[-1]
            entry["template_context"] = deepcopy(parent["template_context"])
    for entry in entries:
        if not entry.get("previous_decl"):
            continue
        if indexed.get(entry["previous_decl"]) is not None:
            continue
        for field in ("parameters", "template_parameters"):
            for parameter in entry.get(field, []):
                default = parameter.get("default_argument")
                if default and default.get("argument_origin") == "inherited" and default.get("macro_provenance"):
                    parameter["default_argument"] = {
                        "expression": None,
                        "unavailable_reason": "inherited_macro_default_source_unavailable",
                        "argument_origin": "inherited",
                    }
    def previous_declaration_chain(entry):
        """Follow only explicit Clang `previousDecl` IDs in this invocation."""
        chain = [entry]
        seen = {entry.get("ast_id")}
        current = entry
        while current.get("previous_decl"):
            previous_id = current["previous_decl"]
            if previous_id in seen:
                return chain, "previous_declaration_cycle"
            previous = indexed.get(previous_id)
            if previous is None:
                return chain, "previous_declaration_not_in_snapshot"
            chain.append(previous)
            seen.add(previous_id)
            current = previous
        return chain, None

    def resolve_effective_flags(entry):
        """Resolve static/virtual properties from linked declarations only.

        Written AST facts remain occurrence-local. Effective static follows the
        exact redeclaration chain. Effective virtual also uses Clang's explicit
        OverrideAttr; when the AST omits override relationships for a derived
        member, the result stays unknown rather than matching by spelling.
        """
        kind = entry.get("kind")
        nonvirtual_status = (
            "resolved_nonvirtual_constructor" if kind == "CXXConstructorDecl"
            else "resolved_nonvirtual_nonmethod"
            if kind not in {"CXXMethodDecl", "CXXDestructorDecl", "CXXConversionDecl"}
            else None
        )
        chain, chain_error = previous_declaration_chain(entry)
        if chain_error:
            entry["static"] = None
            entry["effective_static_status"] = f"unresolved_{chain_error}"
            entry["virtual"] = False if nonvirtual_status else None
            entry["effective_virtual_status"] = nonvirtual_status or f"unresolved_{chain_error}"
            return

        root = chain[-1]
        via_previous = len(chain) > 1
        if "written_static" in root:
            entry["static"] = root["written_static"]
            entry["effective_static_status"] = (
                "resolved_from_previous_decl" if via_previous else "resolved_from_self"
            )
        else:
            entry["static"] = entry.get("static")
            entry["effective_static_status"] = "unresolved_written_static_unavailable"

        if nonvirtual_status:
            entry["virtual"] = False
            entry["effective_virtual_status"] = nonvirtual_status
            return
        if not any("written_virtual" in candidate for candidate in chain):
            entry["effective_virtual_status"] = "unresolved_written_virtual_unavailable"
            return
        if any(candidate.get("written_virtual") is True for candidate in chain):
            entry["virtual"] = True
            entry["effective_virtual_status"] = (
                "resolved_from_previous_decl_virtual_specifier" if via_previous
                else "resolved_from_virtual_specifier"
            )
            return
        if any(candidate.get("_has_override_attribute") for candidate in chain):
            entry["virtual"] = True
            entry["effective_virtual_status"] = "resolved_from_override_attribute"
            return
        if root.get("written_static") is True:
            entry["virtual"] = False
            entry["effective_virtual_status"] = "resolved_nonvirtual_static_member"
            return

        parent = indexed.get(root.get("_parent_record_id") or root.get("parent_context"))
        if parent is None:
            entry["virtual"] = None
            entry["effective_virtual_status"] = "unresolved_parent_record_not_in_snapshot"
        elif not parent.get("bases"):
            entry["virtual"] = False
            entry["effective_virtual_status"] = "resolved_nonvirtual_no_base_record"
        else:
            entry["virtual"] = None
            entry["effective_virtual_status"] = "unresolved_inherited_override_not_exposed"

    for entry in entries:
        resolve_effective_flags(entry)
    if friend_relationships is not None:
        resolve_friend_relationships(entries, friend_relationships)
    for entry in entries:
        for field in ("ast_id", "previous_decl", "parent_context"):
            value = entry.pop(field, None)
            if field == "ast_id":
                entry["_inventory_ast_id"] = value
        entry.pop("_template_parameter_nodes", None)
        entry.pop("_has_override_attribute", None)
        entry.pop("_parent_record_id", None)
    resolved = resolve_using_targets(entries)
    collect_declaration_conditions(resolved, declaration_conditions)
    return resolved


def resolve_friend_relationships(entries, friend_relationships):
    """Replace exact friend endpoint pointers with canonical IDs in one AST scope.

    Inputs must have completed semantic declaration reconciliation, while their
    AST pointer fields remain available. Type-only friendship stays unresolved;
    direct children filtered out of the inventory never link to same-name rows.
    The sidecar is updated in place and declaration rows are untouched.
    """
    if not friend_relationships:
        return friend_relationships
    requested_ids = {
        reference
        for relationship in friend_relationships
        for reference in [*relationship["owner"].get("_ast_ids", []), *relationship.get("_target_ast_ids", [])]
        if reference
    }
    indexed = {}
    declaration_rows = {}
    for entry in entries:
        if entry.get("ast_id") in requested_ids:
            indexed.setdefault(entry["ast_id"], set()).add(declaration_id(entry))
            declaration_rows[entry["ast_id"]] = entry
    for relationship in friend_relationships:
        owner = relationship["owner"]
        owner_refs = owner.pop("_ast_ids", [])
        owner["declaration_ids"] = sorted(
            {
                identifier
                for reference in owner_refs
                if len(indexed.get(reference, ())) == 1
                for identifier in indexed[reference]
            }
        )
        if not owner["declaration_ids"] or any(len(indexed.get(reference, ())) != 1 for reference in owner_refs):
            owner["unresolved_reason"] = (
                "owner_declaration_mapping_ambiguous"
                if any(len(indexed.get(reference, ())) > 1 for reference in owner_refs)
                else "some_owner_declarations_filtered_or_outside_inventory"
                if owner["declaration_ids"]
                else "owner_filtered_or_outside_inventory"
                if any(owner_refs)
                else "lexical_owner_ast_reference_unavailable"
            )
        references = relationship.pop("_target_ast_ids", [])
        if references and len(indexed.get(references[0], ())) == 1:
            target_row = declaration_rows[references[0]]
            for field in ("kind", "name", "signature"):
                relationship["target"][field] = target_row.get(field)
        target_ids = sorted(
            {
                identifier
                for reference in references
                if len(indexed.get(reference, ())) == 1
                for identifier in indexed[reference]
            }
        )
        relationship["target_declaration_ids"] = target_ids
        if relationship["resolution"] == "type_only_unresolved":
            relationship["unresolved_reason"] = "type_only_friend_has_no_exact_declaration_reference"
        elif any(len(indexed.get(reference, ())) > 1 for reference in references):
            relationship["resolution"] = "target_declaration_mapping_ambiguous"
            relationship["unresolved_reason"] = "target_declaration_mapping_ambiguous"
        elif target_ids and all(reference in indexed for reference in references):
            relationship["resolution"] = "child_declaration"
        elif target_ids:
            relationship["resolution"] = "child_declaration_partial"
            relationship["unresolved_reason"] = "some_target_declarations_filtered_or_outside_inventory"
        else:
            reason = "target_filtered_or_outside_inventory" if any(references) else "target_ast_reference_unavailable"
            relationship["resolution"] = reason
            relationship["unresolved_reason"] = reason
    return friend_relationships


def declaration_id(entry):
    """Return a portable overload identity independent of source line movement.

    Template/specialization kind and source distinguish otherwise identical names.
    The ID is an audit key, not a claim of ABI or cross-release stability.
    """
    identity = {
        key: entry[key] for key in ("name", "kind", "signature", "template_parameters", "template_arguments", "source")
    }
    if "template_parameters" in identity:
        identity["template_parameters"] = [
            {key: parameter[key] for key in _LEGACY_TEMPLATE_PARAMETER_ID_FIELDS if key in parameter}
            for parameter in identity["template_parameters"]
        ]
    identity["template_context"] = entry.get("template_context", [])
    return hashlib.sha256(json.dumps(identity, sort_keys=True).encode()).hexdigest()


def resolve_using_targets(entries):
    """Resolve exact UsingShadowDecl links to portable inventory declarations.

    Inputs must contain one compiler invocation's reconciled declarations.
    Missing targets remain explicit unresolved links; target spelling is never
    used as a lookup key. Callers must not combine separate Clang invocations
    before this resolution because their temporary pointer IDs can collide.
    """
    using_entries = [entry for entry in entries if "_using_target_refs" in entry]
    if not using_entries:
        for entry in entries:
            entry.pop("_inventory_ast_id", None)
        return entries
    requested_ids = {
        reference.get("_target_ast_id")
        for entry in using_entries
        for reference in entry["_using_target_refs"]
        if reference.get("_target_ast_id")
    }
    target_index = {}
    ambiguous_targets = set()
    for entry in entries:
        ast_id = entry.get("_inventory_ast_id")
        if not ast_id or ast_id not in requested_ids:
            continue
        canonical_id = declaration_id(entry)
        candidate = {
            "declaration_id": canonical_id,
            "kind": entry.get("kind"),
            "name": entry.get("name"),
            "signature": entry.get("signature"),
            "source": entry.get("source"),
        }
        previous = target_index.get(ast_id)
        if previous and previous["declaration_id"] != canonical_id:
            ambiguous_targets.add(ast_id)
        else:
            target_index[ast_id] = candidate

    for entry in entries:
        references = entry.pop("_using_target_refs", None)
        if references is not None:
            targets = []
            if not references:
                references = [
                    {
                        "_target_ast_id": None,
                        "kind": None,
                        "name": None,
                        "signature": None,
                        "source": None,
                        "unresolved_reason": "using_shadow_not_emitted",
                    }
                ]
            for reference in references:
                ast_id = reference.pop("_target_ast_id", None)
                target = {
                    "kind": reference.get("kind"),
                    "name": reference.get("name"),
                    "signature": reference.get("signature"),
                    "source": reference.get("source"),
                }
                if ast_id in ambiguous_targets:
                    target["unresolved_reason"] = "target_declaration_mapping_ambiguous"
                elif ast_id in target_index:
                    resolved = target_index[ast_id]
                    for field in ("kind", "name", "signature", "source"):
                        if resolved.get(field) is not None:
                            target[field] = resolved[field]
                    target["declaration_id"] = resolved["declaration_id"]
                elif ast_id:
                    target["unresolved_reason"] = "target_filtered_or_outside_inventory"
                else:
                    target["unresolved_reason"] = reference.get("unresolved_reason", "target_ast_reference_unavailable")
                targets.append(target)
            entry["using_targets"] = targets
        entry.pop("_inventory_ast_id", None)
    return entries


def apply_reviews(entries, reviews):
    """Apply explicit audit records, rejecting stale IDs and incomplete claims.

    Public and internal classifications need source-based rationale. Supported
    records also need Python, ownership, GIL, error and executable evidence fields.
    Unreviewed declarations retain their default status; no reflection is support.
    Raises ValueError for malformed, stale or insufficient review records.
    """
    if not isinstance(reviews, dict):
        raise ValueError("Reviews must be a JSON object keyed by declaration ID")
    indexed = {entry["id"]: entry for entry in entries}
    updates = []
    for identifier, review in reviews.items():
        if identifier not in indexed:
            raise ValueError(f"Unknown declaration ID: {identifier}")
        if not isinstance(review, dict):
            raise ValueError(f"Review must be an object: {identifier}")
        classification = review.get("classification", "unreviewed")
        status = review.get("status", "untested")
        if classification not in {"unreviewed", "public", "internal"}:
            raise ValueError(f"Invalid classification: {identifier}")
        if status not in {"untested", "supported", "failing", "unsupported", "deferred"}:
            raise ValueError(f"Invalid status: {identifier}")
        if review.get("mapping_status", "unreviewed") not in {"unreviewed", "partial", "reviewed", "not_applicable"}:
            raise ValueError(f"Invalid mapping status: {identifier}")
        allowed = {
            "classification",
            "status",
            "rationale",
            "python",
            "ownership",
            "gil",
            "errors",
            "evidence",
            "mapping_status",
        }
        if set(review) - allowed:
            raise ValueError(f"Unknown review fields: {identifier}")
        mapping_status = review.get("mapping_status", "unreviewed")
        if mapping_status == "not_applicable" and classification != "internal":
            raise ValueError(f"Only internal mappings are not applicable: {identifier}")
        if mapping_status == "reviewed" and classification != "public":
            raise ValueError(f"Reviewed mapping must be public: {identifier}")
        if status == "supported" and mapping_status == "partial":
            raise ValueError(f"Supported declaration cannot have partial mapping: {identifier}")
        required = set()
        if mapping_status == "reviewed":
            required.update({"python", "ownership", "gil", "errors"})
        if classification != "unreviewed":
            required.add("rationale")
        if status != "untested":
            required.add("rationale")
        if status == "supported":
            if classification != "public":
                raise ValueError(f"Supported declaration must be public: {identifier}")
            required.update({"python", "ownership", "gil", "errors", "evidence"})
        for field, value in review.items():
            if field == "evidence":
                if not isinstance(value, list) or not all(isinstance(v, str) and v.strip() for v in value):
                    raise ValueError(f"Invalid evidence: {identifier}")
            elif not isinstance(value, str) or not value.strip():
                raise ValueError(f"Invalid {field}: {identifier}")
        for field in required:
            value = review.get(field)
            if field == "evidence":
                valid = isinstance(value, list) and bool(value) and all(isinstance(v, str) and v.strip() for v in value)
            else:
                valid = isinstance(value, str) and bool(value.strip())
            if not valid:
                raise ValueError(f"Missing or invalid {field}: {identifier}")
        updates.append((indexed[identifier], review))
    for entry, review in updates:
        entry.update(review)


def macro_declarations(preprocessed):
    """Extract active macro definitions with portable preprocessor source locations.

    Compiler builtins and translation-unit feature switches have no depot source
    and are excluded. Redefinitions retain separate locations for manual review.
    """
    filename = None
    line_number = 0
    for line in preprocessed.splitlines():
        marker = re.match(r'^# (\d+) "([^"]+)"', line)
        if marker:
            line_number, filename = int(marker[1]), marker[2]
            continue
        if filename and line.startswith("#define "):
            path = (ROOT / filename).resolve()
            if path.is_relative_to(ROOT / "submodules/systemc/src"):
                yield {"definition": line, "source": path.relative_to(ROOT).as_posix(), "line": line_number}
        line_number += 1


def source_directives(source):
    """Yield complete preprocessor directives with their first physical line.

    Backslash continuations are joined so configuration review sees every
    operand. Locations remain one-based physical source lines.
    """
    lines = source.splitlines()
    index = 0
    while index < len(lines):
        first = index + 1
        directive = lines[index].strip()
        index += 1
        if not directive.startswith("#"):
            continue
        while directive.endswith("\\") and index < len(lines):
            directive = directive[:-1].rstrip() + " " + lines[index].strip()
            index += 1
        yield first, directive


def conditional_macro_sites(source):
    """Associate source macro definitions with their enclosing branch history.

    Conditions are recorded as source text, not evaluated. An else/elif branch
    retains earlier alternatives so it cannot be mistaken for an independent
    condition. Raises ValueError for an unbalanced conditional stack.
    """
    stack = []
    for line, directive in source_directives(source):
        match = re.match(r"#\s*(\w+)\b", directive)
        if not match:
            continue
        operation = match[1]
        if operation in {"if", "ifdef", "ifndef"}:
            stack.append([directive])
        elif operation in {"elif", "else"}:
            if not stack:
                raise ValueError(f"Unmatched {operation} at line {line}")
            stack[-1].append(directive)
        elif operation == "endif":
            if not stack:
                raise ValueError(f"Unmatched endif at line {line}")
            stack.pop()
        elif operation == "define":
            yield {
                "line": line,
                "definition_start": directive,
                "conditional_context": deepcopy(stack),
            }
    if stack:
        raise ValueError("Unclosed source conditional")


def header_ledger(preprocessed, entries):
    """Account for every source header using upstream installation manifests.

    Installation is source evidence for distribution, not public modeling status.
    Unreached installed headers and headers without declarations remain explicit
    gaps. Private implementation headers are retained rather than disappearing.
    """
    root = ROOT / "submodules/systemc/src"
    installed = set()
    private = set()
    manifests = [
        root / "Makefile.am",
        *root.rglob("files.am"),
        root / "tlm_core/Makefile.am",
        root / "tlm_utils/Makefile.am",
        root / "sysc/packages/qt/Makefile.am",
    ]
    for manifest in manifests:
        section = None
        for line in manifest.read_text().splitlines():
            match = re.match(r"^(H_FILES|NO_H_FILES|[A-Z_]+)\s*[+:]?=", line)
            if match:
                section = match[1]
            if section not in {"H_FILES", "NO_H_FILES"}:
                continue
            for name in re.findall(
                r"[A-Za-z_0-9/.-]+\.(?:h|inc)|(?<![A-Za-z_])systemc(?![A-Za-z_.])|(?<![A-Za-z_])tlm(?![A-Za-z_.])", line
            ):
                base = root / "sysc" if manifest.name == "files.am" else manifest.parent
                path = base / name
                if path.is_file():
                    (installed if section == "H_FILES" else private).add(path.resolve())
    # These headers belong to QuickThreads demonstration/benchmark programs,
    # not the installed SystemC modeling contract (see their b_/stp_ declarations).
    private.update({root / "sysc/packages/qt/b.h", root / "sysc/packages/qt/stp.h"})
    reached = set()
    for line in preprocessed.splitlines():
        marker = re.match(r'^# \d+ "([^"]+)"', line)
        if marker:
            path = (ROOT / marker[1]).resolve()
            if path.is_relative_to(root):
                reached.add(path)
    counts = {}
    for entry in entries:
        counts[entry["source"]] = counts.get(entry["source"], 0) + 1
    paths = set(root.rglob("*.h")) | set(root.rglob("*.inc")) | {root / "systemc", root / "tlm"}
    for path in sorted(paths):
        source = path.relative_to(ROOT).as_posix()
        directives = list(source_directives(path.read_text(errors="replace")))
        yield {
            "source": source,
            "distribution": "installed"
            if path.resolve() in installed
            else "implementation"
            if path.resolve() in private
            else "unclassified",
            "manifest_rationale": "Upstream H_FILES"
            if path.resolve() in installed
            else "Upstream NO_H_FILES"
            if path.resolve() in private
            else "No matched installation entry",
            "reached": path.resolve() in reached,
            "declaration_candidates": counts.get(source, 0),
            "classification": "unreviewed",
            "conditional_directives": [
                {"line": index, "directive": line.strip()}
                for index, line in directives
                if re.match(r"\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b", line)
            ],
            "source_macros": list(conditional_macro_sites(path.read_text(errors="replace"))),
        }


def classify_source_entries(inventory):
    """Apply conservative source-boundary classification without support claims.

    Private members and implementation headers are internal by accessibility and
    build-manifest evidence. Installed public/protected entries remain candidates,
    because access alone cannot establish a public modeling contract. Each row
    includes the reason and an explicit unresolved semantic mapping.
    """
    headers = {row["source"]: row for row in inventory["headers"]}
    for entry in inventory["declarations"]:
        header = headers[entry["source"]]
        if entry["access"] == "private":
            entry["classification"] = "internal"
            entry["rationale"] = "Private C++ declaration; unavailable to public modeling callers."
        elif header["distribution"] == "implementation":
            entry["classification"] = "internal"
            entry["rationale"] = "Noninstalled implementation header: " + header["manifest_rationale"]
        else:
            entry["classification"] = "public_candidate"
            entry["rationale"] = "Installed header; public or protected access requires modeling-role review."
        entry["mapping_status"] = "unreviewed"
        entry["ownership"] = "unreviewed"
        entry["gil"] = "unreviewed"
        entry["errors"] = "unreviewed"


def write_audit_report(inventory, output):
    """Write a complete header accounting and declaration-family review ledger.

    Explicitly distinguish extraction coverage from public classification and
    semantic support. Every family keeps source links and unresolved review state.
    """
    headers = inventory["headers"]
    lines = [
        "# SystemC source API inventory",
        "",
        f"Pinned revision: `{inventory['systemc_revision']}`.",
        "",
        "This is source accounting, not a completed public API classification or support claim.",
        "Every header is listed below. Declaration candidates remain unreviewed until source and semantic review.",
        "",
        "## Header coverage",
        "",
        "| Header | Distribution | Reached | Candidates | Classification |",
        "| --- | --- | --- | ---: | --- |",
    ]
    for header in headers:
        lines.append(
            f"| {header['source']} | {header['distribution']} | {header['reached']} | {header['declaration_candidates']} | unreviewed |"
        )
    lines += [
        "",
        "## Declaration families",
        "",
        "| Family | Overload/declaration candidates | Sources | Classification | Support |",
        "| --- | ---: | --- | --- | --- |",
    ]
    families = {}
    for entry in inventory["declarations"]:
        family = entry["name"].split("::")[:2]
        key = "::".join(family)
        families.setdefault(key, []).append(entry)
    for family, entries in sorted(families.items()):
        sources = ", ".join(sorted({entry["source"] for entry in entries}))
        classifications = ", ".join(sorted({entry["classification"] for entry in entries}))
        lines.append(f"| `{family}` | {len(entries)} | {sources} | {classifications} | untested |")
    lines += [
        "",
        "## Declaration and overload ledger",
        "",
        "| ID | Declaration | Signature | Source | Access | Classification | Mapping |",
        "| --- | --- | --- | --- | --- | --- | --- |",
    ]
    for entry in inventory["declarations"]:
        signature = (entry["signature"] or entry["kind"]).replace("|", "&#124;")
        name = entry["name"].replace("|", "&#124;")
        lines.append(
            f"| `{entry['id']}` | `{name}` | `{signature}` | {entry['source']}:{entry['line']} | {entry['access']} | {entry['classification']} | {entry['mapping_status']} |"
        )
    if "friend_relationships" in inventory:
        friends = inventory["friend_relationships"]
        sites = {friend["occurrence_id"] for friend in friends}
        exact = sum(friend["resolution"] == "child_declaration" for friend in friends)
        lines += [
            "",
            "## Friend source relationships",
            "",
            f"Friend source sites: {len(sites)}; relationship records: {len(friends)}.",
            f"Exact target records: {exact}; unresolved or partial target records: {len(friends) - exact}.",
            "These records preserve lexical ownership and source access grants. They do not establish Python binding or ADL support.",
            "Type-only targets remain unresolved without an exact AST reference. Friends without a portable source location are excluded.",
        ]
    lines += [
        "",
        "## Remaining inventory gates",
        "",
        "- Resolve installed headers not reached by this configuration and all unclassified headers.",
        "- Review public modeling versus internal declarations, including protected extension hooks.",
        "- Reconcile namespace/global declarations, macro expansions, inherited members and specializations.",
        "- Audit alternate conditional configurations and declarations absent from umbrella headers.",
        "- Map reviewed public overloads to Python spelling, ownership, GIL, errors and executable evidence.",
        "",
    ]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines))


def main():
    """Compile the public umbrella headers and write a portable audit scaffold."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="clang++")
    parser.add_argument("--output", type=Path, default=ROOT / ".ai/PUBLIC_API_INVENTORY.json")
    parser.add_argument("--configuration", choices=("modeling", "baseline", "compatibility"), default="modeling")
    parser.add_argument("--reviews", type=Path, help="JSON object keyed by declaration ID")
    parser.add_argument("--report", type=Path, help="Write header/family accounting as Markdown")
    args = parser.parse_args()
    defines = ["SC_INCLUDE_DYNAMIC_PROCESSES", "SC_INCLUDE_FX"] if args.configuration != "baseline" else []
    if args.configuration == "compatibility":
        defines += [
            "SC_DT_DEPRECATED",
            "SC_DT_MIXED_COMMA_OPERATORS",
            "SC_USE_SC_STRING_OLD",
            "SC_INCLUDE_EXTRA_STD_HEADERS",
            "SC_ENABLE_COVARIANT_VIRTUAL_BASE",
        ]
    source = "".join(f"#define {define}\n" for define in defines) + "#include <tlm>\n"
    source += "".join(f"#include <tlm_utils/{header}>\n" for header in HEADERS)
    # Installed headers absent from the umbrella are still part of the source audit.
    for header in (
        "sysc/communication/sc_host_mutex.h",
        "sysc/communication/sc_host_semaphore.h",
        "sysc/kernel/sc_cor.h",
        "sysc/kernel/sc_name_gen.h",
        "sysc/kernel/sc_runnable.h",
        "sysc/tracing/sc_tracing_ids.h",
        "sysc/utils/sc_string.h",
        "systemc.h",
        "tlm.h",
    ):
        source += f"#include <{header}>\n"
    entries = []
    friend_relationships = []
    declaration_conditions = []
    with tempfile.TemporaryDirectory(prefix="pysystemc-inventory-") as temporary:
        translation = Path(temporary) / "inventory.cpp"
        translation.write_text(source)
        command = [args.compiler, "-std=c++17", "-I", "submodules/systemc/src"]
        for namespace in ("tlm", "sc_core", "sc_dt", "sc_main", "sc_elab_and_sim"):
            result = subprocess.run(
                [
                    *command,
                    "-fsyntax-only",
                    "-Xclang",
                    "-ast-dump=json",
                    "-Xclang",
                    f"-ast-dump-filter={namespace}",
                    str(translation),
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
                check=True,
                timeout=60,
            )
            namespace_entries = []
            namespace_friends = []
            for root in restore_source_files(list(json_roots(result.stdout))):
                if root["kind"] == "NamespaceDecl" and root.get("name") in {namespace, "tlm_utils"}:
                    namespace_entries.extend(
                        declarations(root, friend_relationships=namespace_friends, collect_conditions=True)
                    )
                elif namespace in {"sc_main", "sc_elab_and_sim"} and root.get("name") == namespace:
                    namespace_entries.extend(
                        declarations(root, friend_relationships=namespace_friends, collect_conditions=True)
                    )
            entries.extend(
                reconcile_declarations(
                    namespace_entries,
                    friend_relationships=namespace_friends,
                    declaration_conditions=declaration_conditions,
                )
            )
            friend_relationships.extend(namespace_friends)
        result = subprocess.run(
            [*command, "-dM", "-E", str(translation)],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=True,
            timeout=60,
        )
        attributed = subprocess.run(
            [*command, "-dD", "-E", str(translation)],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=True,
            timeout=60,
        )
    macros = [line for line in result.stdout.splitlines() if line.startswith(("#define SC_", "#define TLM_"))]
    unique = {json.dumps(entry, sort_keys=True): entry for entry in entries}
    for entry in unique.values():
        entry["id"] = declaration_id(entry)
    if args.reviews:
        reviews = json.loads(args.reviews.read_text())
    else:
        reviews = {}
    revision = subprocess.run(
        ["git", "-C", "submodules/systemc", "rev-parse", "HEAD"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=True,
        timeout=10,
    ).stdout.strip()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    inventory = {
        "systemc_revision": revision,
        "schema_version": 4,
        "friend_relationships": list(
            {json.dumps(edge, sort_keys=True): edge for edge in friend_relationships}.values()
        ),
        "declaration_conditions": list(
            {json.dumps(record, sort_keys=True): record for record in declaration_conditions}.values()
        ),
        "utility_headers": list(HEADERS),
        "macro_declarations": list(macro_declarations(attributed.stdout)),
        "configuration": ["C++17", *defines],
        "effective_defines": [
            name
            for name in (
                "SC_INCLUDE_DYNAMIC_PROCESSES",
                "SC_INCLUDE_FX",
                "SC_DT_DEPRECATED",
                "SC_DT_MIXED_COMMA_OPERATORS",
                "SC_FX_EXCLUDE_OTHER",
                "SC_MAX_NBITS",
                "SC_USE_SC_STRING_OLD",
                "SC_ENABLE_COVARIANT_VIRTUAL_BASE",
            )
            if any(line.split()[1] == name for line in macros)
        ],
        "limits": "Declaration scaffold only. Review mappings, access, internals, macros and semantic evidence.",
        "declarations": sorted(
            unique.values(), key=lambda entry: (entry["name"], entry["source"], entry["offset"] or 0)
        ),
        "macros": sorted(macros),
        "headers": list(header_ledger(attributed.stdout, list(unique.values()))),
    }
    inventory["identity_occurrences"] = {}
    for entry in inventory["declarations"]:
        inventory["identity_occurrences"].setdefault(entry["id"], []).append(
            {"source": entry["source"], "line": entry["line"]}
        )
    classify_source_entries(inventory)
    apply_reviews(inventory["declarations"], reviews)
    args.output.write_text(json.dumps(inventory, indent=2) + "\n")
    if args.report:
        write_audit_report(inventory, args.report)
    print(f"Wrote {len(unique)} declaration candidates and {len(macros)} macro candidates to {args.output}")


if __name__ == "__main__":
    main()

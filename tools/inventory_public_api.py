"""Extract source declarations for the M6A audit without claiming binding support.

Run with uv run python tools/inventory_public_api.py. Clang reads the checked-out
headers; the ignored JSON output preserves overload signatures and access levels.
Semantic evidence and internal/public classification require a separate review.
"""

import argparse
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
    else:
        result["argument_origin"] = argument_origin
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


def declarations(node, scope=(), filename=None, access="public", namespace=(), template_context=()):
    """Walk declaration contexts, retaining protected/private candidates for review.

    Function bodies, implicit declarations and locationless AST cross-references
    are excluded. Template patterns
    retain their original dependent signatures rather than a fixed type matrix.
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
            yield {
                "ast_id": node.get("id"),
                "previous_decl": node.get("previousDecl"),
                "parent_context": node.get("parentDeclContextId"),
                "name": "::".join((*scope, name)),
                "kind": kind,
                "signature": node.get("type", {}).get("qualType"),
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
                "source": path.relative_to(ROOT).as_posix(),
                "line": location.get("line")
                or (source_text(path).count(b"\n", 0, location["offset"]) + 1 if "offset" in location else None),
                "offset": location.get("offset"),
                "access": access,
                "virtual": node.get("virtual", False),
                "pure": node.get("pure", False),
                "bases": node.get("bases", []),
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
    child_access = (
        access
        if kind == "EnumDecl"
        else "private"
        if kind in {"CXXRecordDecl", "ClassTemplateSpecializationDecl", "ClassTemplatePartialSpecializationDecl"}
        and node.get("tagUsed") == "class"
        else "public"
    )
    for child in node.get("inner", []):
        if child["kind"] == "AccessSpecDecl":
            child_access = child["access"]
        else:
            yield from declarations(
                child,
                namespace if kind == "FriendDecl" else nested_scope,
                filename,
                "public" if kind == "FriendDecl" else child_access,
                namespace,
                template_context,
            )


def reconcile_declarations(entries):
    """Resolve semantic owners using Clang redeclaration/context links.

    Out-of-class definitions inherit access and template context from their
    declaration. AST pointer IDs are used only within one compiler invocation
    and removed before serialization. Distinct source occurrences are retained.
    """
    indexed = {entry["ast_id"]: entry for entry in entries if entry.get("ast_id")}
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
    for entry in entries:
        for field in ("ast_id", "previous_decl", "parent_context"):
            entry.pop(field, None)
    return entries


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
            for root in restore_source_files(list(json_roots(result.stdout))):
                if root["kind"] == "NamespaceDecl" and root.get("name") in {namespace, "tlm_utils"}:
                    namespace_entries.extend(declarations(root))
                elif namespace in {"sc_main", "sc_elab_and_sim"} and root.get("name") == namespace:
                    namespace_entries.extend(declarations(root))
            entries.extend(reconcile_declarations(namespace_entries))
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

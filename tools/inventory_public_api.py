"""Extract source declarations for the M6A audit without claiming binding support.

Run with uv run python tools/inventory_public_api.py. Clang reads the checked-out
headers; the ignored JSON output preserves overload signatures and access levels.
Semantic evidence and internal/public classification require a separate review.
"""

import argparse
from functools import cache
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADERS = (
    "tlm_quantumkeeper.h",
    "peq_with_get.h",
    "peq_with_cb_and_phase.h",
    "simple_initiator_socket.h",
    "simple_target_socket.h",
    "passthrough_target_socket.h",
    "multi_passthrough_initiator_socket.h",
    "multi_passthrough_target_socket.h",
    "instance_specific_extensions.h",
)
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


def declarations(node, scope=(), filename=None, access="public"):
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
    if node.get("isImplicit") or (kind in DECLARATIONS and "loc" not in node):
        return
    if kind in DECLARATIONS and name and filename:
        path = (ROOT / filename).resolve()
        if path.is_relative_to(ROOT / "submodules/systemc/src"):
            yield {
                "name": "::".join((*scope, name)),
                "kind": kind,
                "signature": node.get("type", {}).get("qualType"),
                "template_parameters": [
                    {"name": child.get("name"), "kind": child["kind"], "type": child.get("type", {}).get("qualType")}
                    for child in node.get("inner", [])
                    if child["kind"] in {"TemplateTypeParmDecl", "NonTypeTemplateParmDecl", "TemplateTemplateParmDecl"}
                ],
                "source": path.relative_to(ROOT).as_posix(),
                "line": location.get("line")
                or (source_text(path).count(b"\n", 0, location["offset"]) + 1 if "offset" in location else None),
                "offset": location.get("offset"),
                "access": access,
                "bases": node.get("bases", []),
                "parameters": [
                    {
                        "name": child.get("name"),
                        "type": child.get("type", {}).get("qualType"),
                        "has_default": "init" in child,
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
    child_access = "private" if kind == "CXXRecordDecl" and node.get("tagUsed") == "class" else "public"
    for child in node.get("inner", []):
        if child["kind"] == "AccessSpecDecl":
            child_access = child["access"]
        else:
            yield from declarations(
                child,
                nested_scope[:-1] if kind == "FriendDecl" else nested_scope,
                filename,
                "public" if kind == "FriendDecl" else child_access,
            )


def declaration_id(entry):
    """Return a portable overload identity independent of source line movement.

    Template/specialization kind and source distinguish otherwise identical names.
    The ID is an audit key, not a claim of ABI or cross-release stability.
    """
    identity = {key: entry[key] for key in ("name", "kind", "signature", "template_parameters", "source")}
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
        allowed = {"classification", "status", "rationale", "python", "ownership", "gil", "errors", "evidence"}
        if set(review) - allowed:
            raise ValueError(f"Unknown review fields: {identifier}")
        required = set()
        if classification != "unreviewed":
            required.add("rationale")
        if status != "untested":
            required.add("rationale")
        if status == "supported":
            if classification != "public":
                raise ValueError(f"Supported declaration must be public: {identifier}")
            required.update({"python", "ownership", "gil", "errors", "evidence"})
        for field in required:
            value = review.get(field)
            if field == "evidence":
                valid = isinstance(value, list) and bool(value) and all(isinstance(v, str) and v.strip() for v in value)
            else:
                valid = isinstance(value, str) and bool(value.strip())
            if not valid:
                raise ValueError(f"Missing or invalid {field}: {identifier}")
        indexed[identifier].update(review)


def main():
    """Compile the public umbrella headers and write a portable audit scaffold."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="clang++")
    parser.add_argument("--output", type=Path, default=ROOT / ".ai/PUBLIC_API_INVENTORY.json")
    parser.add_argument("--configuration", choices=("modeling", "baseline"), default="modeling")
    parser.add_argument("--reviews", type=Path, help="JSON object keyed by declaration ID")
    args = parser.parse_args()
    defines = ["SC_INCLUDE_DYNAMIC_PROCESSES", "SC_INCLUDE_FX"] if args.configuration == "modeling" else []
    source = "".join(f"#define {define}\n" for define in defines) + "#include <tlm>\n"
    source += "".join(f"#include <tlm_utils/{header}>\n" for header in HEADERS)
    entries = []
    with tempfile.TemporaryDirectory(prefix="pysystemc-inventory-") as temporary:
        translation = Path(temporary) / "inventory.cpp"
        translation.write_text(source)
        command = [args.compiler, "-std=c++17", "-I", "submodules/systemc/src"]
        for namespace in ("tlm", "sc_core", "sc_dt"):
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
            for root in json_roots(result.stdout):
                if root["kind"] == "NamespaceDecl" and root.get("name") in {namespace, "tlm_utils"}:
                    entries.extend(declarations(root))
        result = subprocess.run(
            [*command, "-dM", "-E", str(translation)],
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
        apply_reviews(list(unique.values()), json.loads(args.reviews.read_text()))
    revision = subprocess.run(
        ["git", "-C", "submodules/systemc", "rev-parse", "HEAD"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=True,
        timeout=10,
    ).stdout.strip()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(
            {
                "systemc_revision": revision,
                "schema_version": 2,
                "configuration": ["C++17", *defines],
                "effective_defines": [
                    name
                    for name in ("SC_INCLUDE_DYNAMIC_PROCESSES", "SC_INCLUDE_FX")
                    if any(line.split()[1] == name for line in macros)
                ],
                "limits": "Declaration scaffold only. Review mappings, access, internals, macros and semantic evidence.",
                "declarations": sorted(
                    unique.values(), key=lambda entry: (entry["name"], entry["source"], entry["offset"] or 0)
                ),
                "macros": sorted(macros),
            },
            indent=2,
        )
        + "\n"
    )
    print(f"Wrote {len(unique)} declaration candidates and {len(macros)} macro candidates to {args.output}")


if __name__ == "__main__":
    main()

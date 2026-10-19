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


def restore_source_files(roots):
    """Expand Clang's stream-wide omitted file locations before tree traversal.

    Clang compresses consecutive locations across siblings, not just descendants.
    Keeping a stream cursor prevents attributing inline definitions to an earlier
    namespace header. Expansion locations take precedence for macro declarations.
    """
    current = None

    def visit(node):
        """Restore declaration locations in AST serialization order."""
        nonlocal current
        location = node.get("loc", {})
        location = location.get("expansionLoc", location.get("spellingLoc", location))
        if "file" in location:
            current = location["file"]
        elif location and current:
            location["file"] = current
        for child in node.get("inner", []):
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
                    {"name": child.get("name"), "kind": child["kind"], "type": child.get("type", {}).get("qualType")}
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
        if kind == "CXXRecordDecl" and node.get("tagUsed") == "class"
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
        elif parent:
            entry["name"] = parent["name"] + "::" + entry["name"].rsplit("::", 1)[-1]
            entry["template_context"] = deepcopy(parent["template_context"])
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

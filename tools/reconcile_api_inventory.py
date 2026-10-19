"""Freeze source snapshots into a reviewable remaining-work inventory.

The ledger includes every extracted family, even when its modeling role is
unresolved. Candidate test matches are navigation aids, never support evidence.
Run after generating modeling and compatibility snapshots with inventory_public_api.
"""

import argparse
import hashlib
import json
import runpy
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def reconcile(snapshots):
    """Union declaration identities, preserving occurrences and configuration scope.

    Parameters
    ----------
    snapshots : list[dict]
        Source inventories generated at the same pinned SystemC revision.

    Returns
    -------
    dict
        Portable union with one review row per identity and explicit occurrences.

    Raises
    ------
    ValueError
        If source revisions or schema versions differ.
    """
    if len({item["systemc_revision"] for item in snapshots}) != 1:
        raise ValueError("Cannot reconcile different SystemC revisions")
    if {item["schema_version"] for item in snapshots} != {4}:
        raise ValueError("Regenerate all snapshots with schema version 4")
    rows = {}
    for snapshot in snapshots:
        configuration = snapshot["configuration"]
        for entry in snapshot["declarations"]:
            identifier = entry["id"]
            if identifier not in rows:
                rows[identifier] = {**entry, "occurrences": [], "configurations": []}
            row = rows[identifier]
            occurrence = {"source": entry["source"], "line": entry["line"], "access": entry["access"]}
            if occurrence not in row["occurrences"]:
                row["occurrences"].append(occurrence)
            if configuration not in row["configurations"]:
                row["configurations"].append(configuration)
    families = {}
    for row in rows.values():
        family = "::".join(row["name"].split("::")[:2])
        families.setdefault(family, []).append(row["id"])
    fingerprint = hashlib.sha256("\n".join(sorted(rows)).encode()).hexdigest()
    return {
        "systemc_revision": snapshots[0]["systemc_revision"],
        "schema_version": 4,
        "fingerprint": fingerprint,
        "scope": "C++17 Linux modeling and compatibility source configurations; public-role review remains explicit.",
        "declarations": sorted(rows.values(), key=lambda row: (row["name"], row["id"])),
        "families": dict(sorted(families.items())),
        "headers": snapshots[0]["headers"],
        "macro_declarations": [
            json.loads(value)
            for value in sorted(
                {json.dumps(row, sort_keys=True) for item in snapshots for row in item["macro_declarations"]}
            )
        ],
    }


def write_remaining(inventory, output):
    """Write every family with source references and explicit unfinished work.

    This deliberately keeps unclassified candidates in the denominator of review
    work, rather than hiding them or labeling them supported from reflection.
    """
    indexed = {row["id"]: row for row in inventory["declarations"]}
    lines = [
        "# Fixed source-derived API review inventory",
        "",
        f"SystemC revision: `{inventory['systemc_revision']}`.",
        f"Inventory fingerprint: `{inventory['fingerprint']}`.",
        "",
        inventory["scope"],
        "",
        "This ledger is the complete union of the two recorded extraction snapshots.",
        "It is not a claim that extraction or public-contract classification is complete.",
        "Every source family is present, including internal and unresolved candidates.",
        "A row closes only after public/internal review and applicable semantic validation.",
        "No test-file name or native reflection result establishes overload support.",
        "",
        "## Remaining family review",
        "",
        "| Family | Declaration identities | Classification | Source headers | Remaining work |",
        "| --- | ---: | --- | --- | --- |",
    ]
    for family, identifiers in inventory["families"].items():
        entries = [indexed[identifier] for identifier in identifiers]
        classifications = ", ".join(sorted({entry["classification"] for entry in entries}))
        sources = ", ".join(sorted({entry["source"] for entry in entries}))
        public = [entry for entry in entries if entry["classification"] == "public"]
        unresolved = [entry for entry in entries if entry["classification"] not in {"public", "internal"}]
        missing_mapping = [
            entry
            for entry in public
            if any(
                not entry.get(field) or entry.get(field) == "unreviewed"
                for field in ("python", "ownership", "gil", "errors")
            )
        ]
        work = []
        if unresolved:
            work.append("Review modeling role")
        if missing_mapping:
            work.append("Fill missing Python/ownership/GIL/error contracts")
        if any(entry.get("mapping_status") == "partial" for entry in public):
            work.append("Resolve explicitly partial binding mappings")
        if any(entry.get("status") != "supported" for entry in public):
            work.append("Complete semantic evidence for public overload scope")
        remaining = "; ".join(work) or "Internal source review recorded; confirm any access-only exclusions"
        lines.append(
            f"| `{family.replace('|', '&#124;')}` | {len(entries)} | {classifications} | {sources} | {remaining} |"
        )
    lines += [
        "",
        "## Recorded public contracts",
        "",
        "Each row below is a reviewed source contract. Untested status remains open even when selected executable evidence is linked.",
        "",
        "| Identity | Python | Ownership | GIL | Errors | Status | Evidence |",
        "| --- | --- | --- | --- | --- | --- | --- |",
    ]
    for entry in inventory["declarations"]:
        if entry["classification"] != "public":
            continue
        cells = [
            entry["id"],
            entry.get("python", "unreviewed"),
            entry.get("ownership", "unreviewed"),
            entry.get("gil", "unreviewed"),
            entry.get("errors", "unreviewed"),
            entry.get("status", "untested"),
            ", ".join(entry.get("evidence", [])),
        ]
        lines.append("| " + " | ".join(str(cell).replace("|", "&#124;").replace("\n", " ") for cell in cells) + " |")
    lines += [
        "",
        "## Configuration and extraction review",
        "",
        "- Reconcile inactive platform/compiler/ABI branches against the Linux/C++17 baseline; broader platforms are deferred.",
        "- Review all recorded conditional directives and macro definition sites; include guards do not create APIs.",
        "- Review inherited access and using declarations, qualified friends, template instantiations and aliases.",
        "- Review noninstalled implementation headers and installed internal kernel/datatype facilities.",
        "- Map native operators to Python protocols and distinguish overload resolution from native declaration availability.",
        "- Reconcile upstream example variants and APIs without example coverage.",
        "",
        "## Per-declaration review",
        "",
        "PUBLIC_API_UNION.json contains each identity, signature, template context, access,",
        "source occurrence, configurations, classification, mapping and semantic review state.",
        "PUBLIC_API_SOURCE_INVENTORY.md and PUBLIC_API_COMPATIBILITY.md retain source order occurrences.",
        "",
    ]
    output.write_text("\n".join(lines))


def validate_evidence_locations(reviews, root=ROOT):
    """Reject missing, nonportable or out-of-range local review evidence.

    This checks source location integrity, not whether evidence proves semantics.
    Raises ValueError for an invalid path or one-based line number.
    """
    for identifier, review in reviews.items():
        for evidence in review.get("evidence", []):
            filename, separator, line = evidence.rpartition(":")
            if not (separator and line.isdigit()):
                filename, line = evidence, None
            path = Path(filename)
            if path.is_absolute() or ".." in path.parts:
                raise ValueError(f"Nonportable evidence: {identifier}: {evidence}")
            path = root / path
            if not path.is_file():
                raise ValueError(f"Missing evidence: {identifier}: {evidence}")
            if line and not 1 <= int(line) <= len(path.read_text(errors="replace").splitlines()):
                raise ValueError(f"Invalid evidence line: {identifier}: {evidence}")


def main():
    """Reconcile snapshots and publish the local remaining-work ledger."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshots", nargs="+", type=Path)
    parser.add_argument("--reviews", type=Path, help="Apply source-reviewed declaration records to the union")
    parser.add_argument("--output", type=Path, default=ROOT / ".ai/PUBLIC_API_UNION.json")
    parser.add_argument("--report", type=Path, default=ROOT / ".ai/PUBLIC_API_REMAINING.md")
    args = parser.parse_args()
    inventory = reconcile([json.loads(path.read_text()) for path in args.snapshots])
    if args.reviews:
        reviewer = runpy.run_path(str(ROOT / "tools/inventory_public_api.py"))["apply_reviews"]
        reviews = json.loads(args.reviews.read_text())
        validate_evidence_locations(reviews)
        reviewer(inventory["declarations"], reviews)
    args.output.write_text(json.dumps(inventory, indent=2) + "\n")
    write_remaining(inventory, args.report)
    print(f"Wrote union and remaining review ledger: {args.output}, {args.report}")


if __name__ == "__main__":
    main()

"""Freeze source snapshots into a reviewable remaining-work inventory.

The ledger includes every extracted family, even when its modeling role is
unresolved. Candidate test matches are navigation aids, never support evidence.
Run after generating modeling and compatibility snapshots with inventory_public_api.
"""

import argparse
import hashlib
import json
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
        remaining = (
            "Confirm exclusion rationale"
            if all(entry["classification"] == "internal" for entry in entries)
            else "Review modeling role; map every overload; ownership/GIL/errors; executable evidence"
        )
        lines.append(
            f"| `{family.replace('|', '&#124;')}` | {len(entries)} | {classifications} | {sources} | {remaining} |"
        )
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


def main():
    """Reconcile snapshots and publish the local remaining-work ledger."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshots", nargs="+", type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / ".ai/PUBLIC_API_UNION.json")
    parser.add_argument("--report", type=Path, default=ROOT / ".ai/PUBLIC_API_REMAINING.md")
    args = parser.parse_args()
    inventory = reconcile([json.loads(path.read_text()) for path in args.snapshots])
    args.output.write_text(json.dumps(inventory, indent=2) + "\n")
    write_remaining(inventory, args.report)
    print(f"Wrote union and remaining review ledger: {args.output}, {args.report}")


if __name__ == "__main__":
    main()

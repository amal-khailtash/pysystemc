"""Record upstream example build definitions and lexical API references.

Source matches are navigation evidence only. They do not establish an executable
variant, overload resolution, Python support or native equivalence by themselves.
"""

import argparse
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def inventory_examples(api_inventory):
    """Account for CMake executables, source files and candidate API references.

    Parameters
    ----------
    api_inventory : dict
        Reconciled source API snapshot, with declaration families.

    Returns
    -------
    dict
        Source/build records and lexical family references requiring semantic review.
    """
    upstream = ROOT / "submodules/systemc/examples"
    sources = sorted(path for path in upstream.rglob("*") if path.suffix in {".cpp", ".h", ".cc", ".hpp"})
    tokens = {}
    for source in sources:
        # Strip comments before matching names; strings remain lexical candidates.
        content = re.sub(r"/\*.*?\*/|//[^\n]*", "", source.read_text(errors="replace"), flags=re.S)
        for token in set(re.findall(r"\b[A-Za-z_]\w*\b", content)):
            tokens.setdefault(token, []).append(source.relative_to(ROOT).as_posix())
    builds = []
    for path in sorted(upstream.rglob("CMakeLists.txt")):
        content = re.sub(r"#[^\n]*", "", path.read_text())
        for match in re.finditer(r"\badd_executable\s*\(([^)]*)\)", content, re.I | re.S):
            arguments = match.group(1).split()
            builds.append(
                {
                    "target": arguments[0],
                    "group": path.parent.relative_to(upstream).as_posix(),
                    "definition": path.relative_to(ROOT).as_posix(),
                    "arguments": arguments[1:],
                    "line": content.count("\n", 0, match.start()) + 1,
                    "variant_status": "requires review of variables, conditions, shared sources and runtime inputs",
                    "native_status": "untested",
                    "python_ports": [
                        port.relative_to(ROOT).as_posix()
                        for port in sorted((ROOT / "examples" / path.parent.relative_to(upstream)).glob("*.py"))
                    ],
                    "equivalence_status": "not inferred from port presence",
                }
            )
    return {
        "systemc_revision": api_inventory["systemc_revision"],
        "build_targets": builds,
        "source_files": [path.relative_to(ROOT).as_posix() for path in sources],
        "family_references": {
            family: {
                "candidate_sources": tokens.get(family.rsplit("::", 1)[-1], []),
                "status": "lexical candidates only; overload and semantic coverage unreviewed",
            }
            for family in api_inventory["families"]
        },
        "limits": "Literal CMake target declarations and lexical references; variant expansion and equivalence need review.",
    }


def main():
    """Write the local example/declaration reconciliation records."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("inventory", type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / ".ai/PUBLIC_API_EXAMPLES.json")
    args = parser.parse_args()
    result = inventory_examples(json.loads(args.inventory.read_text()))
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(
        f"Recorded {len(result['build_targets'])} executable definitions and {len(result['source_files'])} source files"
    )


if __name__ == "__main__":
    main()

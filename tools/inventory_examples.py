"""Record upstream example build definitions and lexical API references.

Source matches are navigation evidence only. They do not establish an executable
variant, overload resolution, Python support or native equivalence by themselves.
"""

import argparse
import json
from pathlib import Path
import re
import shlex

ROOT = Path(__file__).resolve().parents[1]


def source_dependencies(directory, arguments):
    """Resolve literal target sources and recursive quoted includes.

    Returns portable paths and unresolved arguments/includes. Native headers
    terminate traversal; this does not expand macros or prove buildability.
    """
    upstream = ROOT / "submodules/systemc/examples"
    native = ROOT / "submodules/systemc/src"
    resolved = [(directory / argument).resolve() for argument in arguments if (directory / argument).is_file()]
    pending = list(resolved)
    seen = set()
    headers = set()
    missing = set()
    search = [directory, directory / "include", upstream / "tlm/common/include", upstream / "sysc/2.3/include", native]
    while pending:
        source = pending.pop()
        if source in seen:
            continue
        seen.add(source)
        content = re.sub(r"/\*.*?\*/|//[^\n]*", "", source.read_text(errors="replace"), flags=re.S)
        for name in re.findall(r'#\s*include\s*"([^"]+)"', content):
            dependency = next((folder / name for folder in [source.parent, *search] if (folder / name).is_file()), None)
            if dependency is None:
                missing.add(name)
            elif dependency.resolve().is_relative_to(upstream):
                pending.append(dependency.resolve())
            else:
                headers.add(dependency.resolve())
    return {
        "resolved_source_files": sorted(path.relative_to(ROOT).as_posix() for path in resolved),
        "missing_source_arguments": [argument for argument in arguments if not (directory / argument).is_file()],
        "local_dependency_files": sorted(path.relative_to(ROOT).as_posix() for path in seen),
        "native_header_dependencies": sorted(path.relative_to(ROOT).as_posix() for path in headers),
        "unresolved_quoted_includes": sorted(missing),
        "dependency_scope": "Literal quoted includes; native headers terminate traversal; macro includes unexpanded.",
    }


def runtime_inputs(directory, target, cmake_content, dependencies):
    """Record default CMake stdin/reference and literal dependency file access.

    Parameters are the target directory/name, comment-stripped CMake source,
    and resolved source dependency record. Returns portable input/reference
    paths and optional argument source locations. Macro filenames and optional
    CLI behavior require manual review; no runtime support is inferred.
    """
    calls = [
        shlex.split(match.group(1))
        for match in re.finditer(r"\bconfigure_and_add_test\s*\(([^)]*)\)", cmake_content, re.I | re.S)
    ]
    call = next((call for call in calls if call and call[0] == target), [])
    options = dict(zip(call[1::2], call[2::2]))
    golden = options.get("GOLDEN") or next(
        (name for name in ("golden.log", "results/expected.log") if (directory / name).is_file()), None
    )
    inputs = {options["INPUT"]} if "INPUT" in options else set()
    outputs = set()
    argument_sources = []
    for name in dependencies["local_dependency_files"]:
        source = ROOT / name
        content = re.sub(r"/\*.*?\*/|//[^\n]*", "", source.read_text(errors="replace"), flags=re.S)
        for filename, mode in re.findall(r'fopen\s*\(\s*"([^"]+)"\s*,\s*"([^"]+)"', content):
            (inputs if mode.startswith("r") else outputs).add(filename)
        if re.search(r"argv\s*\[\s*1\s*\]", content):
            argument_sources.append(name)
    return {
        "registered_test": bool(call),
        "configured_stdin": options.get("INPUT"),
        "golden_reference": (directory / golden).relative_to(ROOT).as_posix() if golden else None,
        "golden_reference_exists": (directory / golden).is_file() if golden else None,
        "output_filter": options.get("FILTER"),
        "required_input_files": sorted(inputs),
        "generated_output_files": sorted(outputs),
        "missing_input_files": sorted(name for name in inputs if not (directory / name).is_file()),
        "optional_argument_sources": sorted(argument_sources),
        "scope": "Default CMake test and literal fopen accesses; macros and optional CLI semantics unreviewed.",
    }


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
            dependencies = source_dependencies(path.parent, arguments[1:])
            builds.append(
                {
                    "target": arguments[0],
                    "group": path.parent.relative_to(upstream).as_posix(),
                    "definition": path.relative_to(ROOT).as_posix(),
                    "arguments": arguments[1:],
                    **dependencies,
                    "runtime_review": runtime_inputs(path.parent, arguments[0], content, dependencies),
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

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

# Upstream executable targets and Python ports do not always share a filename.
# Keep this mapping explicit so a directory scan cannot mistake support modules
# for runnable ports or silently omit alternate entry points.
PYTHON_ENTRYPOINTS = {
    ("sysc/2.1/dpipe", "dpipe"): ["examples/sysc/2.1/dpipe/main.py"],
    ("sysc/2.1/forkjoin", "forkjoin"): ["examples/sysc/2.1/forkjoin/forkjoin.py"],
    ("sysc/2.1/reset_signal_is", "reset_signal_is"): ["examples/sysc/2.1/reset_signal_is/reset_signal_is.py"],
    ("sysc/2.1/sc_export", "sc_export"): [
        "examples/sysc/2.1/sc_export/main.py",
        "examples/sysc/2.1/sc_export/main_protocol.py",
    ],
    ("sysc/2.1/sc_report", "sc_report"): ["examples/sysc/2.1/sc_report/main.py"],
    ("sysc/2.1/scx_barrier", "scx_barrier"): ["examples/sysc/2.1/scx_barrier/main.py"],
    ("sysc/2.1/scx_mutex_w_policy", "scx_mutex_w_policy"): [
        "examples/sysc/2.1/scx_mutex_w_policy/scx_mutex_w_policy.py"
    ],
    ("sysc/2.1/specialized_signals", "specialized_signals"): ["examples/sysc/2.1/specialized_signals/main.py"],
    ("sysc/2.3/sc_rvd", "sc_rvd"): ["examples/sysc/2.3/sc_rvd/main.py"],
    ("sysc/2.3/sc_ttd", "sc_ttd"): ["examples/sysc/2.3/sc_ttd/main.py"],
    ("sysc/2.3/simple_async", "simple_async"): ["examples/sysc/2.3/simple_async/main.py"],
    ("sysc/fft/fft_flpt", "fft_flpt"): ["examples/sysc/fft/fft_flpt/main.py"],
    ("sysc/fft/fft_fxpt", "fft_fxpt"): ["examples/sysc/fft/fft_fxpt/main.py"],
    ("sysc/fir", "fir"): ["examples/sysc/fir/main.py"],
    ("sysc/fir", "fir_rtl"): ["examples/sysc/fir/main_rtl.py"],
    ("sysc/pipe", "pipe"): ["examples/sysc/pipe/main.py"],
    ("sysc/pkt_switch", "pkt_switch"): ["examples/sysc/pkt_switch/main.py"],
    ("sysc/risc_cpu", "risc_cpu"): ["examples/sysc/risc_cpu/main.py"],
    ("sysc/rsa", "rsa"): ["examples/sysc/rsa/rsa.py"],
    ("sysc/simple_bus", "simple_bus"): ["examples/sysc/simple_bus/simple_bus_main.py"],
    ("sysc/simple_fifo", "simple_fifo"): ["examples/sysc/simple_fifo/simple_fifo.py"],
    ("sysc/simple_perf", "simple_perf"): ["examples/sysc/simple_perf/simple_perf.py"],
    ("tlm/at_1_phase", "at_1_phase"): ["examples/tlm/at_1_phase/src/at_1_phase.py"],
    ("tlm/at_2_phase", "at_2_phase"): ["examples/tlm/at_2_phase/src/at_2_phase.py"],
    ("tlm/at_4_phase", "at_4_phase"): ["examples/tlm/at_4_phase/src/at_4_phase.py"],
    ("tlm/at_extension_optional", "at_extension_optional"): [
        "examples/tlm/at_extension_optional/src/at_extension_optional.py"
    ],
    ("tlm/at_mixed_targets", "at_mixed_targets"): ["examples/tlm/at_mixed_targets/src/at_mixed_targets.py"],
    ("tlm/at_ooo", "at_ooo"): ["examples/tlm/at_ooo/src/at_ooo.py"],
    ("tlm/lt", "lt"): ["examples/tlm/lt/src/lt.py"],
    ("tlm/lt_dmi", "lt_dmi"): ["examples/tlm/lt_dmi/src/lt_dmi.py"],
    ("tlm/lt_extension_mandatory", "lt_extension_mandatory"): [
        "examples/tlm/lt_extension_mandatory/src/lt_extension_mandatory.py"
    ],
    ("tlm/lt_mixed_endian", "lt_mixed_endian"): ["examples/tlm/lt_mixed_endian/src/lt.py"],
    ("tlm/lt_temporal_decouple", "lt_temporal_decouple"): [
        "examples/tlm/lt_temporal_decouple/src/lt_temporal_decouple.py"
    ],
}

# These files are imported by their mapped entry point, directly or through a
# local helper. CMake runtime input detection remains in runtime_review below.
PYTHON_SUPPORT_FILES = {
    ("sysc/risc_cpu", "risc_cpu"): [
        f"examples/sysc/risc_cpu/{name}.py"
        for name in ("bios", "dcache", "decode", "exec", "fetch", "floating", "icache", "mmxu", "paging", "pic")
    ],
    ("tlm/at_1_phase", "at_1_phase"): ["examples/tlm/common/models.py", "examples/tlm/common/runtime.py"],
    ("tlm/at_2_phase", "at_2_phase"): ["examples/tlm/common/models.py", "examples/tlm/common/runtime.py"],
    ("tlm/at_4_phase", "at_4_phase"): ["examples/tlm/common/models.py", "examples/tlm/common/runtime.py"],
    ("tlm/at_extension_optional", "at_extension_optional"): [
        "examples/tlm/common/models.py",
        "examples/tlm/common/runtime.py",
    ],
    ("tlm/at_mixed_targets", "at_mixed_targets"): [
        "examples/tlm/common/models.py",
        "examples/tlm/common/runtime.py",
    ],
    ("tlm/at_ooo", "at_ooo"): ["examples/tlm/common/models.py", "examples/tlm/common/runtime.py"],
    ("tlm/lt", "lt"): [
        "examples/tlm/common/lt_models.py",
        "examples/tlm/common/models.py",
        "examples/tlm/common/runtime.py",
    ],
    ("tlm/lt_dmi", "lt_dmi"): [
        "examples/tlm/common/dmi_models.py",
        "examples/tlm/common/lt_models.py",
        "examples/tlm/common/models.py",
        "examples/tlm/common/runtime.py",
    ],
    ("tlm/lt_extension_mandatory", "lt_extension_mandatory"): [
        "examples/tlm/common/models.py",
        "examples/tlm/common/runtime.py",
    ],
    ("tlm/lt_mixed_endian", "lt_mixed_endian"): [
        "examples/tlm/common/lt_models.py",
        "examples/tlm/common/models.py",
        "examples/tlm/common/runtime.py",
    ],
    ("tlm/lt_temporal_decouple", "lt_temporal_decouple"): [
        "examples/tlm/common/lt_models.py",
        "examples/tlm/common/models.py",
        "examples/tlm/common/runtime.py",
    ],
}


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
                    "python_entrypoints": [],
                    "python_support_files": [],
                    "python_mapping_status": "unmapped",
                    "python_target_relationship": "unmapped",
                    "python_variant_group": None,
                    "equivalence_status": "not inferred from port presence",
                }
            )
    target_keys = {(row["group"], row["target"]) for row in builds}
    expected_keys = set(PYTHON_ENTRYPOINTS)
    unknown_mapping_keys = sorted(expected_keys - target_keys)
    for row in builds:
        key = (row["group"], row["target"])
        if key not in PYTHON_ENTRYPOINTS:
            continue
        entrypoints = PYTHON_ENTRYPOINTS[key]
        row["python_entrypoints"] = entrypoints
        row["python_support_files"] = PYTHON_SUPPORT_FILES.get(key, [])
        row["python_mapping_status"] = "mapped" if entrypoints else "no_entrypoint"
        if row["group"] == "sysc/fir":
            row["python_target_relationship"] = "native_variant"
            row["python_variant_group"] = "sysc/fir"
        elif row["group"] == "sysc/2.1/sc_export":
            row["python_target_relationship"] = "shared_native_target"
            row["python_variant_group"] = "sysc/2.1/sc_export"
        else:
            row["python_target_relationship"] = "one_to_one"
            row["python_variant_group"] = None

    mapped_entrypoints = sorted(path for paths in PYTHON_ENTRYPOINTS.values() for path in paths)
    mapped_support_files = sorted({path for paths in PYTHON_SUPPORT_FILES.values() for path in paths})
    python_scope = {
        path.relative_to(ROOT).as_posix()
        for parent in (ROOT / "examples/sysc", ROOT / "examples/tlm")
        for path in parent.rglob("*.py")
    }
    unmapped_python_files = sorted(python_scope - set(mapped_entrypoints) - set(mapped_support_files))
    unmatched_native_targets = sorted(target_keys - expected_keys)
    missing_python_files = sorted(
        path for path in [*mapped_entrypoints, *mapped_support_files] if not (ROOT / path).is_file()
    )
    return {
        "systemc_revision": api_inventory["systemc_revision"],
        "build_targets": builds,
        "python_mapping": {
            "schema_version": 1,
            "entrypoint_count": len(mapped_entrypoints),
            "support_file_count": len(mapped_support_files),
            "entrypoints": mapped_entrypoints,
            "support_files": mapped_support_files,
            "unmapped_python_files": unmapped_python_files,
            "missing_python_files": missing_python_files,
            "unmatched_native_targets": unmatched_native_targets,
            "missing_native_targets": unknown_mapping_keys,
            "completeness": (
                "complete"
                if not missing_python_files
                and not unknown_mapping_keys
                and not unmatched_native_targets
                and not unmapped_python_files
                else "requires_review"
            ),
            "scope": "Explicit upstream executable mapping; support modules listed separately. Presence does not imply execution or equivalence.",
        },
        "source_files": [path.relative_to(ROOT).as_posix() for path in sources],
        "family_references": {
            family: {
                "candidate_sources": tokens.get(family.rsplit("::", 1)[-1], []),
                "status": "lexical candidates only; overload and semantic coverage unreviewed",
            }
            for family in api_inventory["families"]
        },
        "limits": "Literal CMake target declarations and lexical references; CMake conditional expansion and equivalence need review.",
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

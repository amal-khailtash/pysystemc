"""Validate tutorials and upstream examples against an installed SystemC wheel.

The default command preserves the original console and three-tutorial smoke
check. Pass ``--all`` to verify installed runtime source bytes against checkout,
then stage the Python/native examples and run the existing pytest comparators
from the installed interpreter without adding the checkout's ``src`` directory
to any Python import path.
"""

import argparse
import ast
from collections import Counter
import hashlib
import importlib.machinery
import importlib.metadata
import importlib.util
import json
import os
from pathlib import Path
from pathlib import PurePosixPath
import shutil
import subprocess
import sys
import tempfile
import types
import xml.etree.ElementTree as ElementTree


ISOLATED_OVERRIDES = (
    "FMT_HOME",
    "SPDLOG_HOME",
    "SYSTEMC_HOME",
    "SYSTEMC_SOURCE_DIR",
    "SYSTEMC_INSTALL_DIR",
    "SYSTEMC_LIBRARY_DIR",
    "PYTHONPATH",
    "PYTHONHOME",
)
"""Environment variables cleared or replaced during isolated wheel validation."""

STAGED_TEST_FILES = (
    "__init__.py",
    "support.py",
    "test_upstream.py",
    "test_tutorials.py",
)
"""Only existing regression-test files copied into the temporary test root."""

REGRESSION_TEST_FILES = ("test_upstream.py", "test_tutorials.py")
"""Explicit pytest modules whose source functions define the expected node set."""

TUTORIAL_ENTRYPOINTS = (
    "examples/helloworld/py/helloworld.py",
    "examples/counter/py/counter_tb.py",
    "examples/simple_fifo/py/simple_fifo.py",
    "examples/toy/event_cancel.py",
    "examples/toy/mixed_gate.py",
    "examples/toy/trace_cleanup.py",
    "examples/toy/adder.py",
    "examples/toy/up_counter.py",
)
"""Runnable tutorial/toy entries not included in the upstream target map."""

OTHER_PYTHON_FILES = (
    {"path": "examples/counter/py/counter.py", "role": "tutorial support module"},
    {"path": "examples/run_sysc_cpp.py", "role": "native runner wrapper"},
    {"path": "examples/counter/py/system.py", "role": "empty stub"},
    {"path": "examples/counter/py/tb.py", "role": "empty stub"},
)
"""Four remaining Python files, counted outside the runnable-entrypoint set."""

DEFAULT_TUTORIALS = (
    ("helloworld/py/helloworld.py", "Simulation finished"),
    ("counter/py/counter_tb.py", "@74 ns :: Incremented Counter 0"),
    ("simple_fifo/py/simple_fifo.py", "Visit www.accellera.org"),
)
"""The three legacy installed tutorial smoke cases and their output markers."""

RUNTIME_SOURCE_ROOTS = (PurePosixPath("amal/eda/systemc"), PurePosixPath("amal/eda/cli"))
"""First-party runtime package roots included in the installed source parity gate."""

RUNTIME_SOURCE_SUFFIXES = frozenset({".py", ".cpp", ".h", ".hpp"})
"""Runtime source file types shipped with the Python packages."""


def repository_root() -> Path:
    """Return the checkout root containing this checker.

    Returns:
        Path: Absolute project root.
    """
    return Path(__file__).resolve().parents[1]


def package_details() -> tuple[Path, Path]:
    """Locate the installed binding without importing or initializing the kernel.

    Returns:
        tuple[Path, Path]: The resolved Python package directory and native prefix.

    Raises:
        AssertionError: If the installed SystemC shared library or umbrella header is absent.
    """
    temporary_parents = []
    search = list(sys.path)
    try:
        for name in ("amal", "amal.eda"):
            module = sys.modules.get(name)
            if module is not None:
                search = list(getattr(module, "__path__", ()))
                if not search:
                    raise AssertionError(f"installed {name} package has no search path")
                continue
            spec = importlib.machinery.PathFinder.find_spec(name, search)
            if spec is None or spec.submodule_search_locations is None:
                raise AssertionError(f"installed {name} package is missing from the active interpreter")
            module = types.ModuleType(name)
            module.__path__ = list(spec.submodule_search_locations)
            module.__package__ = name
            module.__spec__ = spec
            sys.modules[name] = module
            temporary_parents.append((name, module))
            search = module.__path__
        systemc_spec = importlib.machinery.PathFinder.find_spec("amal.eda.systemc", search)
        if systemc_spec is None or systemc_spec.origin is None:
            raise AssertionError("installed amal.eda.systemc package is missing from the active interpreter")
        package_dir = Path(systemc_spec.origin).resolve().parent
    finally:
        for name, module in reversed(temporary_parents):
            if sys.modules.get(name) is module:
                del sys.modules[name]
    native = package_dir / "_native"
    library = native / "lib" / "libsystemc.so"
    header = native / "include" / "systemc"
    assert library.is_file(), f"installed SystemC library is missing: {library}"
    assert header.is_file(), f"installed SystemC header is missing: {header}"
    return package_dir, native


def _append_process_log(log_path: Path | None, label: str, result: subprocess.CompletedProcess[str]) -> None:
    """Append a labeled subprocess result when an installed-check log is requested.

    Args:
        log_path: Optional persistent log destination; ``None`` leaves legacy output unchanged.
        label: Short case identifier written before process output.
        result: Completed child result containing captured streams and status.

    Returns:
        None: Writes stdout and stderr when a destination is provided.
    """
    if log_path is None:
        return
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("a") as stream:
        stream.write(f"=== {label} (returncode {result.returncode}) ===\n")
        stream.write("--- stdout ---\n")
        stream.write(result.stdout)
        stream.write("\n--- stderr ---\n")
        stream.write(result.stderr)
        stream.write("\n")


def run_console_and_tutorial_smoke(
    example_root: Path,
    env: dict[str, str] | None = None,
    log_path: Path | None = None,
) -> None:
    """Run the compatibility console check and legacy tutorial cases.

    Args:
        example_root: Root containing the ``examples`` directory to execute.
        env: Optional isolated child environment; omitted for legacy default behavior.
        log_path: Optional path retaining all subprocess output for the comprehensive run.

    Raises:
        AssertionError: If installed native resources, console behavior, or a tutorial assertion fails.
        subprocess.TimeoutExpired: If a bounded example exceeds its timeout.
    """
    package_dir, _ = package_details()
    libraries = list((package_dir / "_native/lib").glob("libsystemc.so*"))
    assert [path.name for path in libraries] == ["libsystemc.so"], libraries
    executable = shutil.which("systemc-cpp")
    assert executable, "installed console entry is missing"
    with tempfile.TemporaryDirectory(prefix="pysystemc-installed-smoke-") as directory:
        work = Path(directory)
        source = work / "entry.cpp"
        source.write_text(
            "#include <systemc>\n#include <string>\n"
            'int sc_main(int n, char** v) { return n == 3 && std::string(v[1]) == "café" '
            '&& std::string(v[2]) == "--flag" ? 7 : 9; }\n'
        )
        result = subprocess.run(
            [executable, "run", str(source), "--", "café", "--flag"],
            cwd=work,
            env=env,
            capture_output=True,
            text=True,
            timeout=60,
        )
        _append_process_log(log_path, "systemc-cpp Unicode arguments and status", result)
        assert result.returncode == 7, result.stdout + result.stderr
        for relative, marker in DEFAULT_TUTORIALS:
            result = subprocess.run(
                [sys.executable, str(example_root / relative)],
                cwd=work,
                env=env,
                capture_output=True,
                text=True,
                timeout=60,
            )
            _append_process_log(log_path, relative, result)
            assert result.returncode == 0, result.stdout + result.stderr
            assert marker in result.stdout, result.stdout


def stage_validation_tree(checkout_root: Path, stage_root: Path) -> None:
    """Copy the complete example/reference trees and selected tests for isolation.

    Args:
        checkout_root: Project checkout holding examples, upstream sources and tests.
        stage_root: Empty or nonexistent destination preserving repository-relative paths.

    Raises:
        FileNotFoundError: If a required source tree or selected test file is absent.
        FileExistsError: If the destination already contains staged content.
    """
    checkout_root = checkout_root.resolve()
    stage_root = stage_root.resolve()
    if stage_root.exists() and any(stage_root.iterdir()):
        raise FileExistsError(f"staging destination is not empty: {stage_root}")
    stage_root.mkdir(parents=True, exist_ok=True)
    shutil.copytree(checkout_root / "examples", stage_root / "examples", dirs_exist_ok=True)
    upstream_examples = checkout_root / "submodules/systemc/examples"
    shutil.copytree(upstream_examples, stage_root / "submodules/systemc/examples", dirs_exist_ok=True)
    tests_source = checkout_root / "tests"
    tests_destination = stage_root / "tests"
    tests_destination.mkdir(parents=True, exist_ok=True)
    for relative in STAGED_TEST_FILES:
        shutil.copy2(tests_source / relative, tests_destination / relative)


def _is_runtime_source_path(relative: PurePosixPath) -> bool:
    """Return whether an installed manifest path is a first-party runtime source.

    Args:
        relative: Distribution-relative path, normally below ``amal/eda``.

    Returns:
        bool: True for eligible Python/C++ runtime files, excluding caches and native data.
    """
    if relative.is_absolute() or ".." in relative.parts or {"_native", "__pycache__"} & set(relative.parts):
        return False
    if relative.suffix not in RUNTIME_SOURCE_SUFFIXES:
        return False
    return any(relative.parts[: len(root.parts)] == root.parts for root in RUNTIME_SOURCE_ROOTS)


def _checkout_runtime_sources(checkout_root: Path) -> dict[str, Path]:
    """Build the portable first-party runtime file roster from checkout source.

    Args:
        checkout_root: Repository root containing ``src/amal/eda``.

    Returns:
        dict[str, Path]: Repository-relative package paths mapped to source files.
    """
    source_root = checkout_root / "src"
    sources: dict[str, Path] = {}
    for root in RUNTIME_SOURCE_ROOTS:
        package_root = source_root.joinpath(*root.parts)
        for path in package_root.rglob("*"):
            if not path.is_file():
                continue
            relative = PurePosixPath(path.relative_to(source_root).as_posix())
            if _is_runtime_source_path(relative):
                sources[relative.as_posix()] = path
    return sources


def runtime_source_parity(
    checkout_root: Path,
    installed_distribution=None,
    installed_package_dir: Path | None = None,
) -> dict[str, object]:
    """Compare checkout runtime bytes with the installed distribution's source manifest.

    The installed file roster comes from distribution ``.files`` metadata; this helper
    does not import ``amal`` or initialize the SystemC kernel.

    Args:
        checkout_root: Repository root containing first-party runtime sources.
        installed_distribution: Optional metadata Distribution, primarily for native-free tests.
        installed_package_dir: Optional active SystemC package directory returned by package discovery.

    Returns:
        dict[str, object]: Portable counts and exact missing, extra, duplicate and hash mismatch details.
    """
    checkout_root = checkout_root.resolve()
    checkout_sources = _checkout_runtime_sources(checkout_root)
    checkout_package_counts = {
        root.as_posix(): sum(relative.startswith(f"{root.as_posix()}/") for relative in checkout_sources)
        for root in RUNTIME_SOURCE_ROOTS
    }
    manifest_error = None
    if installed_distribution is None:
        try:
            installed_distribution = importlib.metadata.distribution("pysystemc")
        except importlib.metadata.PackageNotFoundError as error:
            installed_distribution = None
            manifest_error = str(error)

    installed_sources: dict[str, Path] = {}
    ignored_manifest_count = 0
    duplicate_manifest_paths: list[str] = []
    systemc_package_origin_matches = True
    if installed_distribution is not None:
        files = installed_distribution.files
        if files is None:
            manifest_error = "installed distribution does not provide a .files manifest"
        else:
            manifest_entries = list(files)
            runtime_entries = []
            for entry in manifest_entries:
                relative = PurePosixPath(str(entry).replace("\\", "/"))
                if not _is_runtime_source_path(relative):
                    continue
                runtime_entries.append((relative.as_posix(), entry))
            ignored_manifest_count = len(manifest_entries) - len(runtime_entries)
            for relative, entry in runtime_entries:
                if relative in installed_sources:
                    duplicate_manifest_paths.append(relative)
                    continue
                installed_sources[relative] = Path(installed_distribution.locate_file(entry))
    if installed_package_dir is not None:
        manifest_package = installed_sources.get("amal/eda/systemc/__init__.py")
        systemc_package_origin_matches = (
            manifest_package is not None
            and manifest_package.resolve() == (installed_package_dir / "__init__.py").resolve()
        )

    checkout_paths = set(checkout_sources)
    installed_paths = set(installed_sources)
    installed_package_counts = {
        root.as_posix(): sum(relative.startswith(f"{root.as_posix()}/") for relative in installed_sources)
        for root in RUNTIME_SOURCE_ROOTS
    }
    missing_paths = checkout_paths - installed_paths
    extra_paths = installed_paths - checkout_paths
    mismatches = []
    for relative in sorted(checkout_paths & installed_paths):
        installed_path = installed_sources[relative]
        if not installed_path.is_file():
            missing_paths.add(relative)
            continue
        checkout_digest = hashlib.sha256(checkout_sources[relative].read_bytes()).hexdigest()
        installed_digest = hashlib.sha256(installed_path.read_bytes()).hexdigest()
        if checkout_digest != installed_digest:
            mismatches.append(
                {
                    "path": relative,
                    "checkout_sha256": checkout_digest,
                    "installed_sha256": installed_digest,
                }
            )
    duplicate_manifest_paths = sorted(set(duplicate_manifest_paths))
    missing = sorted(missing_paths)
    extra = sorted(extra_paths)
    checkout_packages_nonempty = all(checkout_package_counts.values())
    installed_count_matches_checkout = len(installed_sources) == len(checkout_sources)
    passed = (
        manifest_error is None
        and checkout_packages_nonempty
        and installed_count_matches_checkout
        and systemc_package_origin_matches
        and not missing
        and not extra
        and not duplicate_manifest_paths
        and not mismatches
    )
    return {
        "status": "passed" if passed else "failed",
        "expected_runtime_source_count": len(checkout_sources),
        "checkout_runtime_source_count": len(checkout_sources),
        "installed_runtime_source_count": len(installed_sources),
        "checkout_package_source_counts": checkout_package_counts,
        "installed_package_source_counts": installed_package_counts,
        "checkout_packages_nonempty": checkout_packages_nonempty,
        "installed_count_matches_checkout": installed_count_matches_checkout,
        "systemc_package_origin_matches_manifest": systemc_package_origin_matches,
        "ignored_installed_manifest_entry_count": ignored_manifest_count,
        "missing_count": len(missing),
        "missing_paths": missing,
        "extra_count": len(extra),
        "extra_paths": extra,
        "duplicate_manifest_paths": duplicate_manifest_paths,
        "mismatch_count": len(mismatches),
        "mismatches": mismatches,
        "manifest_error": manifest_error,
    }


def build_isolated_validation_env(base_env: dict[str, str], stage_root: Path, bundled_prefix: Path) -> dict[str, str]:
    """Return a child environment that selects only staged Python tests and wheel resources.

    Args:
        base_env: Environment inherited by the installed checker process.
        stage_root: Temporary staged tree used for test and sitecustomize imports.
        bundled_prefix: Installed wheel's bundled ``_native`` resource prefix.

    Returns:
        dict[str, str]: A copied environment with source discovery removed and wheel resources selected.
    """
    environment = dict(base_env)
    for name in ISOLATED_OVERRIDES:
        environment.pop(name, None)
    environment["SYSTEMC_HOME"] = str(bundled_prefix.resolve())
    environment["PYTHONPATH"] = str(stage_root.resolve())
    return environment


def _sitecustomize_source(expected_origin: Path) -> str:
    """Build a startup guard that checks installed package origin without importing it.

    Args:
        expected_origin: Exact installed ``amal.eda.systemc`` module file expected in children.

    Returns:
        str: Python source for ``sitecustomize.py`` used by pytest and xdist children.
    """
    expected_literal = repr(str(expected_origin.resolve()))
    return f'''"""Reject checkout or staging imports before any test process loads SystemC."""
import importlib.machinery
import importlib.util
from pathlib import Path
import sys
import types

expected = Path({expected_literal}).resolve()
search = sys.path
temporary_packages = []
for name in ("amal", "amal.eda"):
    if name in sys.modules:
        module = sys.modules[name]
        search = list(getattr(module, "__path__", ()))
        continue
    spec = importlib.machinery.PathFinder.find_spec(name, search)
    if spec is None or spec.submodule_search_locations is None:
        raise SystemExit("cannot locate installed package " + name)
    module = types.ModuleType(name)
    module.__path__ = list(spec.submodule_search_locations)
    module.__package__ = name
    module.__spec__ = spec
    sys.modules[name] = module
    temporary_packages.append((name, module))
    search = list(spec.submodule_search_locations)

spec = importlib.util.find_spec("amal.eda.systemc")
if spec is None or spec.origin is None:
    raise SystemExit("cannot resolve installed amal.eda.systemc")
actual = Path(spec.origin).resolve()
if actual != expected:
    raise SystemExit("wrong SystemC package origin: " + str(actual) + " != " + str(expected))
for loaded_name, loaded in tuple(sys.modules.items()):
    if loaded_name == "amal.eda.systemc" and getattr(loaded, "__file__", None):
        if Path(loaded.__file__).resolve() != expected:
            raise SystemExit("loaded SystemC module has wrong origin")
for name, module in reversed(temporary_packages):
    if sys.modules.get(name) is module:
        del sys.modules[name]
'''


def write_sitecustomize(stage_root: Path, expected_origin: Path) -> Path:
    """Write the installed-origin guard into the staged root.

    Args:
        stage_root: Staged tests/examples root placed on child ``PYTHONPATH``.
        expected_origin: Exact imported package file from the installed wheel.

    Returns:
        Path: The newly written sitecustomize module.
    """
    stage_root.mkdir(parents=True, exist_ok=True)
    guard = stage_root / "sitecustomize.py"
    guard.write_text(_sitecustomize_source(expected_origin))
    return guard


def inventory_mapping(checkout_root: Path) -> dict[str, object]:
    """Load explicit upstream entrypoint-to-target mapping for a portable report.

    Args:
        checkout_root: Checkout containing ``tools/inventory_examples.py``.

    Returns:
        dict[str, object]: Entrypoint counts, native-target count, and portable mappings.

    Raises:
        RuntimeError: If the expected inventory module cannot be loaded.
    """
    module_path = checkout_root / "tools/inventory_examples.py"
    spec = importlib.util.spec_from_file_location("_installed_example_inventory", module_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load example inventory module: {module_path}")
    inventory = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(inventory)
    upstream = [
        {"group": group, "target": target, "entrypoints": list(paths)}
        for (group, target), paths in inventory.PYTHON_ENTRYPOINTS.items()
    ]
    mapped = [path for row in upstream for path in row["entrypoints"]]
    entrypoints = sorted([*mapped, *TUTORIAL_ENTRYPOINTS])
    support_path_list = [path for paths in inventory.PYTHON_SUPPORT_FILES.values() for path in paths]
    support_files = sorted(set(support_path_list))
    other_paths = [row["path"] for row in OTHER_PYTHON_FILES]
    runnable_paths = set(entrypoints)
    support_paths = set(support_files)
    other_path_set = set(other_paths)
    role_overlaps = sorted(
        (runnable_paths & support_paths) | (runnable_paths & other_path_set) | (support_paths & other_path_set)
    )
    duplicate_entrypoints = sorted(path for path, count in Counter(entrypoints).items() if count > 1)
    duplicate_other_paths = sorted(path for path, count in Counter(other_paths).items() if count > 1)
    duplicate_classifications = sorted(set(duplicate_entrypoints) | set(duplicate_other_paths) | set(role_overlaps))
    classified_paths = runnable_paths | support_paths | other_path_set
    actual_python_files = sorted(
        path.relative_to(checkout_root).as_posix() for path in (checkout_root / "examples").rglob("*.py")
    )
    return {
        "python_file_count": len(actual_python_files),
        "runnable_entrypoint_count": len(entrypoints),
        "runnable_entrypoints": entrypoints,
        "upstream_entrypoint_count": len(mapped),
        "native_target_count": len(upstream),
        "upstream_targets": upstream,
        "upstream_support_file_count": len(support_files),
        "upstream_support_files": support_files,
        "other_python_file_count": len(OTHER_PYTHON_FILES),
        "other_python_files": list(OTHER_PYTHON_FILES),
        "unclassified_python_files": sorted(set(actual_python_files) - classified_paths),
        "stale_classified_python_files": sorted(classified_paths - set(actual_python_files)),
        "duplicate_classified_python_files": duplicate_classifications,
        "classification_reconciles": not duplicate_classifications and set(actual_python_files) == classified_paths,
        "support_execution_evidence": "Support modules are not counted as runnable passes; their execution is not asserted individually.",
    }


def _dotted_name(node: ast.AST) -> str:
    """Return a dotted name for an AST name/attribute expression, or empty text.

    Args:
        node: Decorator expression or call target from a parsed test module.

    Returns:
        str: Dotted identifier spelling, or an empty string for other expressions.
    """
    if isinstance(node, ast.Name):
        return node.id
    if isinstance(node, ast.Attribute):
        parent = _dotted_name(node.value)
        return f"{parent}.{node.attr}" if parent else ""
    return ""


def expected_test_nodes(test_root: Path) -> set[tuple[str, str]]:
    """Derive the non-parametrized selected pytest nodes from staged source ASTs.

    Args:
        test_root: Staged directory containing the explicitly selected test modules.

    Returns:
        set[tuple[str, str]]: Exact ``(module stem, top-level test function)`` pairs.

    Raises:
        AssertionError: If selection uses generated, parametrized, class-based, or fixture-parametrized nodes.
        FileNotFoundError: If a selected module or staged fixture support module is absent.
        SyntaxError: If selected Python source cannot be parsed.
    """
    expected: set[tuple[str, str]] = set()
    for relative in ("support.py", *REGRESSION_TEST_FILES):
        path = test_root / relative
        tree = ast.parse(path.read_text(), filename=str(path))
        if any(
            isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)) and node.name == "pytest_generate_tests"
            for node in tree.body
        ):
            raise AssertionError(f"generated pytest node selection is unsupported: {path}")
        if relative in REGRESSION_TEST_FILES and any(
            isinstance(node, ast.Call) and _dotted_name(node.func).split(".")[-1] == "parametrize"
            for node in ast.walk(tree)
        ):
            raise AssertionError(f"parametrized test selection is unsupported: {path}")
        if any(
            isinstance(node, ast.ClassDef)
            and any(
                isinstance(member, (ast.FunctionDef, ast.AsyncFunctionDef)) and member.name.startswith("test_")
                for member in node.body
            )
            for node in tree.body
        ):
            raise AssertionError(f"class-based test selection is unsupported: {path}")
        for node in tree.body:
            if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                continue
            if node.name.startswith("test_"):
                for decorator in node.decorator_list:
                    target = decorator.func if isinstance(decorator, ast.Call) else decorator
                    if _dotted_name(target).split(".")[-1] == "parametrize":
                        raise AssertionError(f"parametrized test selection is unsupported: {path}:{node.name}")
                if relative in REGRESSION_TEST_FILES:
                    expected.add((Path(relative).stem, node.name))
            for decorator in node.decorator_list:
                if not isinstance(decorator, ast.Call):
                    continue
                if _dotted_name(decorator.func).split(".")[-1] == "fixture" and any(
                    keyword.arg == "params" for keyword in decorator.keywords
                ):
                    raise AssertionError(f"parametrized fixture selection is unsupported: {path}:{node.name}")
    modules = {module for module, _ in expected}
    required_modules = {Path(relative).stem for relative in REGRESSION_TEST_FILES}
    if modules != required_modules:
        raise AssertionError(f"expected a nonempty top-level test function set from each selected module: {modules}")
    return expected


def _check_pytest_dependencies() -> None:
    """Fail with an actionable message unless pytest and pytest-xdist are installed.

    Raises:
        SystemExit: If either test dependency is unavailable in the wheel interpreter.
    """
    missing = [name for name in ("pytest", "xdist") if importlib.util.find_spec(name) is None]
    if missing:
        missing_text = ", ".join(missing)
        raise SystemExit(
            f"--all requires {missing_text} in the installed-wheel interpreter; "
            "run it with the wheel and test tools available, for example: "
            "uv run --no-project --with <wheel> --with pytest --with pytest-xdist "
            "python -I tools/check_installed_examples.py --all"
        )


def _junit_module_name(classname: str) -> str:
    """Resolve a JUnit classname to its selected source module stem.

    Args:
        classname: JUnit testcase classname, usually a dotted Python module path.

    Returns:
        str: Selected module stem when present, otherwise the normalized final component.
    """
    components = classname.replace("\\", "/").replace("/", ".").split(".")
    selected_modules = {Path(relative).stem for relative in REGRESSION_TEST_FILES}
    matches = [component for component in components if component in selected_modules]
    if len(matches) == 1:
        return matches[0]
    tail = components[-1] if components else ""
    return tail.removesuffix(".py")


def _parse_junit(junit_path: Path) -> tuple[list[dict[str, str]], list[dict[str, str]], dict[str, int]]:
    """Read JUnit failures, individual outcomes, and aggregate error/skip counts.

    Args:
        junit_path: pytest JUnit XML file written by the copied regression tests.

    Returns:
        tuple: Failure records, normalized case records, and aggregate XML counters.
    """
    root = ElementTree.parse(junit_path).getroot()
    failures: list[dict[str, str]] = []
    cases: list[dict[str, str]] = []
    for case in root.iter("testcase"):
        suite_name = case.attrib.get("classname", "")
        result = "passed"
        if case.find("failure") is not None or case.find("error") is not None:
            result = "failed"
        elif case.find("skipped") is not None:
            result = "skipped"
        cases.append(
            {
                "module": _junit_module_name(suite_name),
                "test": case.attrib.get("name", ""),
                "result": result,
                "suite": suite_name,
            }
        )
        for element_name in ("failure", "error"):
            element = case.find(element_name)
            if element is not None:
                failures.append(
                    {
                        "test": case.attrib.get("name", ""),
                        "suite": suite_name,
                        "type": element.attrib.get("type", element_name),
                        "message": element.attrib.get("message", ""),
                        "details": (element.text or "").strip(),
                    }
                )
    suites = [root] if root.tag == "testsuite" else list(root.iter("testsuite"))
    counts = {
        key: sum(int(suite.attrib.get(key, "0")) for suite in suites) for key in ("failures", "errors", "skipped")
    }
    return failures, cases, counts


def reconcile_regression_results(
    expected_nodes: set[tuple[str, str]],
    cases: list[dict[str, str]],
    xml_counts: dict[str, int],
    pytest_returncode: int,
    classification_reconciles: bool,
) -> dict[str, object]:
    """Require exact source-node coverage, passing outcomes, and full file classification.

    Args:
        expected_nodes: Top-level test function identities derived from staged source ASTs.
        cases: Normalized JUnit testcase records, including the result state.
        xml_counts: Aggregate JUnit failure, error and skip counters.
        pytest_returncode: Exit status from the explicitly selected pytest command.
        classification_reconciles: Whether every example Python file has one valid inventory role.

    Returns:
        dict[str, object]: Regression decision plus missing, duplicate, unexpected and wrong-suite node details.
    """
    actual_nodes = [(case["module"], case["test"]) for case in cases]
    actual_counts = Counter(actual_nodes)
    expected_node_ids = {f"{module}::{test}" for module, test in expected_nodes}
    observed_node_ids = [f"{module}::{test}" for module, test in actual_nodes]
    missing = sorted(expected_nodes - set(actual_nodes))
    unexpected = sorted(set(actual_nodes) - expected_nodes)
    duplicates = sorted(node for node, count in actual_counts.items() if count > 1)
    expected_by_name: dict[str, set[str]] = {}
    for module, test in expected_nodes:
        expected_by_name.setdefault(test, set()).add(module)
    wrong_suite = [
        {
            "observed": f"{module}::{test}",
            "expected_modules": sorted(expected_by_name[test]),
        }
        for module, test in actual_nodes
        if test in expected_by_name and module not in expected_by_name[test]
    ]
    exact_nodes = not missing and not unexpected and not duplicates and len(actual_nodes) == len(expected_nodes)
    has_test_failure = any(case["result"] == "failed" for case in cases) or pytest_returncode != 0
    no_skips_or_errors = (
        not any(case["result"] == "skipped" for case in cases)
        and xml_counts.get("errors", 0) == 0
        and xml_counts.get("skipped", 0) == 0
        and xml_counts.get("failures", 0) == 0
    )
    all_cases_passed = len(cases) == len(expected_nodes) and all(case["result"] == "passed" for case in cases)
    if has_test_failure:
        status = "failed"
    elif exact_nodes and no_skips_or_errors and all_cases_passed and classification_reconciles:
        status = "passed"
    else:
        status = "incomplete"
    return {
        "status": status,
        "exact_selected_node_set": exact_nodes,
        "expected_node_ids": sorted(expected_node_ids),
        "observed_node_ids": sorted(observed_node_ids),
        "missing_node_ids": [f"{module}::{test}" for module, test in missing],
        "unexpected_node_ids": [f"{module}::{test}" for module, test in unexpected],
        "duplicate_node_ids": [f"{module}::{test}" for module, test in duplicates],
        "wrong_suite_nodes": wrong_suite,
        "no_skips_or_errors": no_skips_or_errors,
        "all_cases_passed": all_cases_passed,
        "example_classification_reconciles": classification_reconciles,
        "junit_counts": {key: xml_counts.get(key, 0) for key in ("failures", "errors", "skipped")},
        "pytest_returncode": pytest_returncode,
    }


def _suite_statuses(expected_nodes: set[tuple[str, str]], cases: list[dict[str, str]]) -> dict[str, str]:
    """Summarize expected tutorial/upstream nodes without guessing from substrings.

    Args:
        expected_nodes: Exact expected source identities.
        cases: Normalized JUnit testcase records.

    Returns:
        dict[str, str]: ``tutorials`` and ``upstream`` status values.
    """
    groups = {"test_tutorials": "tutorials", "test_upstream": "upstream"}
    result: dict[str, str] = {}
    for module, group in groups.items():
        expected = {test for selected_module, test in expected_nodes if selected_module == module}
        observed = [case for case in cases if case["module"] == module]
        states = [case["result"] for case in observed]
        if any(state == "failed" for state in states):
            status = "failed"
        elif any(state == "skipped" for state in states):
            status = "skipped"
        elif len(observed) == len(expected) and {case["test"] for case in observed} == expected:
            status = "passed"
        else:
            status = "incomplete"
        result[group] = status
    return result


def run_all_validation(checkout_root: Path, output_dir: Path, package_dir: Path, bundled_prefix: Path) -> int:
    """Run installed-wheel smoke checks and all copied behavior/native comparators.

    Args:
        checkout_root: Project root used only to stage example and native reference sources.
        output_dir: Persistent directory receiving JUnit XML, logs, and JSON summary.
        package_dir: Resolved installed wheel package directory.
        bundled_prefix: Wheel's bundled native resource prefix.

    Returns:
        int: Zero when all checks pass, otherwise one.

    Raises:
        SystemExit: If pytest dependencies are missing.
        AssertionError: If the imported package or native resources resolve outside the wheel.
    """
    checkout_root = checkout_root.resolve()
    output_dir = output_dir.resolve()
    package_dir = package_dir.resolve()
    stage_source = checkout_root / "src"
    if package_dir.is_relative_to(stage_source):
        raise AssertionError(f"installed SystemC package resolves inside checkout src: {package_dir}")
    output_dir.mkdir(parents=True, exist_ok=True)
    report_path = output_dir / "installed-example-report.json"
    junit_path = output_dir / "installed-example-results.xml"
    pytest_log_path = output_dir / "installed-example-pytest.log"
    console_log_path = output_dir / "installed-example-console.log"
    for path in (report_path, junit_path, pytest_log_path, console_log_path):
        path.unlink(missing_ok=True)
    metadata = inventory_mapping(checkout_root)
    source_parity = runtime_source_parity(checkout_root, installed_package_dir=package_dir)
    if source_parity["status"] != "passed":
        report = {
            "schema_version": 1,
            "mapping": metadata,
            "installed_runtime_source_parity": source_parity,
            "installed_console_smoke_result": "untested",
            "regression_result": "failed",
            "failure": {
                "type": "AssertionError",
                "message": "installed runtime source manifest or bytes do not match checkout",
            },
            "reports": {
                "junit_xml": junit_path.name,
                "pytest_log": pytest_log_path.name,
                "console_log": console_log_path.name,
            },
        }
        report_path.write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps(report, indent=2))
        return 1

    native_library = bundled_prefix / "lib" / "libsystemc.so"
    native_header = bundled_prefix / "include" / "systemc"
    if not native_library.is_file() or not native_header.is_file():
        raise AssertionError(f"bundled SystemC resources are incomplete under {bundled_prefix}")
    _check_pytest_dependencies()
    smoke_result = "untested"
    try:
        with tempfile.TemporaryDirectory(prefix="pysystemc-example-stage-") as staging_directory:
            stage_root = Path(staging_directory)
            stage_validation_tree(checkout_root, stage_root)
            expected_nodes = expected_test_nodes(stage_root / "tests")
            write_sitecustomize(stage_root, package_dir / "__init__.py")
            environment = build_isolated_validation_env(os.environ, stage_root, bundled_prefix)
            run_console_and_tutorial_smoke(stage_root / "examples", env=environment, log_path=console_log_path)
            smoke_result = "passed"
            command = [
                sys.executable,
                "-m",
                "pytest",
                "-n",
                "4",
                "-q",
                "--junitxml",
                str(junit_path),
                "--log-file",
                str(pytest_log_path),
                "--log-level",
                "INFO",
                *(str(stage_root / "tests" / relative) for relative in REGRESSION_TEST_FILES),
            ]
            with console_log_path.open("a") as log:
                log.write("=== copied pytest comparator suite ===\n")
                pytest_result = subprocess.run(
                    command,
                    cwd=stage_root,
                    env=environment,
                    stdout=log,
                    stderr=subprocess.STDOUT,
                    text=True,
                    timeout=60 * 60,
                )
            failures, cases, junit_counts = _parse_junit(junit_path)
            suite_statuses = _suite_statuses(expected_nodes, cases)
            regression_audit = reconcile_regression_results(
                expected_nodes,
                cases,
                junit_counts,
                pytest_result.returncode,
                bool(metadata["classification_reconciles"]),
            )
            regression_status = str(regression_audit["status"])
            collected_count = len(cases)
            report = {
                "schema_version": 1,
                "artifact_validation": {
                    "distribution": "pysystemc",
                    "version": importlib.metadata.version("pysystemc"),
                    "python": sys.version.split()[0],
                    "installed_package_location": "active interpreter site-packages/amal/eda/systemc",
                    "bundled_native_prefix": "amal/eda/systemc/_native",
                    "native_resources": {
                        "systemc_library": "present",
                        "systemc_header": "present",
                    },
                    "source_import_guard": "passed",
                },
                "mapping": metadata,
                "installed_runtime_source_parity": source_parity,
                "example_classification_result": "passed" if metadata["classification_reconciles"] else "incomplete",
                "installed_console_smoke_result": smoke_result,
                "installed_behavior_result": suite_statuses.get("tutorials", "untested"),
                "native_equivalence_result": suite_statuses.get("upstream", "untested"),
                "regression_result": regression_status,
                "test_function_count_expected": len(expected_nodes),
                "test_function_count_collected": collected_count,
                "test_node_audit": regression_audit,
                "staged_support_files_are_not_passes": True,
                "staging_manifest": {
                    "example_tree": "examples/",
                    "native_reference_tree": "submodules/systemc/examples/",
                    "test_files": [f"tests/{relative}" for relative in STAGED_TEST_FILES],
                    "project_src_staged": False,
                    "pythonpath": "staged root only",
                },
                "child_environment": {
                    "source_discovery_overrides": "unset",
                    "PYTHONPATH": "temporary staged root",
                    "SYSTEMC_HOME": "amal/eda/systemc/_native",
                    "inherited_UV_settings_preserved": True,
                },
                "reports": {
                    "junit_xml": junit_path.name,
                    "pytest_log": pytest_log_path.name,
                    "console_log": console_log_path.name,
                },
                "failures": failures,
            }
            report_path.write_text(json.dumps(report, indent=2) + "\n")
            print(json.dumps(report, indent=2))
            return 0 if regression_status == "passed" else 1
    except Exception as error:
        # Preserve a machine-readable failure even if staging, smoke checks or
        # a bounded pytest run fails before producing JUnit XML.
        if smoke_result == "untested":
            smoke_result = "failed"
        failure_report = {
            "schema_version": 1,
            "mapping": metadata,
            "installed_runtime_source_parity": source_parity,
            "installed_console_smoke_result": smoke_result,
            "regression_result": "failed",
            "failure": {"type": type(error).__name__, "message": str(error)},
            "reports": {
                "junit_xml": junit_path.name,
                "pytest_log": pytest_log_path.name,
                "console_log": console_log_path.name,
            },
        }
        report_path.write_text(json.dumps(failure_report, indent=2) + "\n")
        raise


def main() -> int:
    """Parse checker arguments and run default or comprehensive installed validation.

    Returns:
        int: Process status suitable for ``sys.exit``.
    """
    root = repository_root()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--all",
        action="store_true",
        help="run the staged 41-test installed behavior/native-equivalence regression suite",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="retain machine-readable and pytest reports here (default: build/verification/installed)",
    )
    arguments = parser.parse_args()
    package_dir, bundled_prefix = package_details()
    if not arguments.all:
        run_console_and_tutorial_smoke(root / "examples")
        print("Installed console arguments/status and tutorials passed")
        return 0
    output_dir = arguments.output_dir or root / "build/verification/installed"
    if not output_dir.is_absolute():
        output_dir = Path.cwd() / output_dir
    return run_all_validation(root, output_dir.resolve(), package_dir, bundled_prefix)


if __name__ == "__main__":
    sys.exit(main())

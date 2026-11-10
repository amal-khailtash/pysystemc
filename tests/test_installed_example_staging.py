"""Native-free checks for installed-example staging and import isolation helpers."""

import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import types

import pytest


ROOT = Path(__file__).resolve().parents[1]
FROZEN_CHECKOUT_RUNTIME_SOURCE_COUNT = 111
"""Current checkout roster size, retained as a test-only review guard."""


def _load_checker():
    """Load the checker module without importing the native SystemC package.

    Returns:
        module: The loaded ``tools/check_installed_examples.py`` module.

    Raises:
        AssertionError: If the checker source cannot be loaded.
    """
    checker_path = ROOT / "tools/check_installed_examples.py"
    spec = importlib.util.spec_from_file_location("installed_example_checker", checker_path)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_stage_copies_only_requested_test_files_and_complete_source_trees(tmp_path):
    """Preserve source-relative layout while excluding project code and extra tests.

    Args:
        tmp_path: pytest-provided isolated directory for the fake checkout and stage.

    Returns:
        None: Assertions establish which trees and files are copied.
    """
    checker = _load_checker()
    checkout = tmp_path / "checkout"
    stage = tmp_path / "stage"
    for relative, content in (
        ("examples/sysc/port/main.py", "print('port')\n"),
        ("examples/run_sysc_cpp.py", "print('runner')\n"),
        ("submodules/systemc/examples/sysc/port/main.cpp", "int sc_main() { return 0; }\n"),
        ("tests/__init__.py", '"""test package"""\n'),
        ("tests/support.py", "ROOT = None\n"),
        ("tests/test_upstream.py", "def test_upstream(): pass\n"),
        ("tests/test_tutorials.py", "def test_tutorial(): pass\n"),
        ("tests/test_unselected.py", "def test_extra(): pass\n"),
        ("src/amal/eda/systemc/__init__.py", "raise RuntimeError('source import')\n"),
    ):
        path = checkout / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)

    checker.stage_validation_tree(checkout, stage)

    assert (stage / "examples/sysc/port/main.py").read_text() == "print('port')\n"
    assert (stage / "examples/run_sysc_cpp.py").is_file()
    assert (stage / "submodules/systemc/examples/sysc/port/main.cpp").is_file()
    assert sorted(path.name for path in (stage / "tests").iterdir()) == sorted(checker.STAGED_TEST_FILES)
    assert not (stage / "src").exists()


def test_isolated_validation_environment_clears_overrides_and_keeps_uv(tmp_path):
    """Clear source selectors while preserving UV settings and unrelated variables.

    Args:
        tmp_path: pytest-provided isolated directory used for temporary path values.

    Returns:
        None: Assertions verify the copied child environment.
    """
    checker = _load_checker()
    inherited = {
        "PATH": "/usr/bin",
        "PYTHONPATH": "/checkout/src",
        "PYTHONHOME": "/python-home",
        "FMT_HOME": "/fmt",
        "SPDLOG_HOME": "/spdlog",
        "SYSTEMC_HOME": "/checkout/submodules/systemc",
        "SYSTEMC_SOURCE_DIR": "/source",
        "SYSTEMC_INSTALL_DIR": "/install",
        "SYSTEMC_LIBRARY_DIR": "/lib",
        "UV_CACHE_DIR": "/uv-cache",
    }

    environment = checker.build_isolated_validation_env(inherited, tmp_path / "stage", tmp_path / "wheel/_native")

    assert inherited["SYSTEMC_HOME"] == "/checkout/submodules/systemc"
    assert environment["SYSTEMC_HOME"] == str((tmp_path / "wheel/_native").resolve())
    assert environment["PYTHONPATH"] == str((tmp_path / "stage").resolve())
    assert environment["UV_CACHE_DIR"] == "/uv-cache"
    assert environment["PATH"] == "/usr/bin"
    assert not (set(environment) & (set(checker.ISOLATED_OVERRIDES) - {"SYSTEMC_HOME", "PYTHONPATH"}))


def test_package_details_resolves_namespace_parents_without_importing_leaf(tmp_path, monkeypatch):
    """Resolve namespace parents temporarily and restore them on success and failure.

    Args:
        tmp_path: pytest-provided fake site-packages trees with namespace parents.
        monkeypatch: pytest fixture isolating sys.path and any preloaded package objects.

    Returns:
        None: Installed paths, leaf non-import and sys.modules restoration are asserted.
    """
    checker = _load_checker()
    package_names = ("amal", "amal.eda", "amal.eda.systemc")
    for name in package_names:
        monkeypatch.delitem(sys.modules, name, raising=False)

    site_packages = tmp_path / "site-packages"
    package = site_packages / "amal/eda/systemc"
    package.mkdir(parents=True)
    sentinel = tmp_path / "kernel-imported.txt"
    (package / "__init__.py").write_text(f"from pathlib import Path\nPath({str(sentinel)!r}).write_text('executed')\n")
    native = package / "_native"
    (native / "lib").mkdir(parents=True)
    (native / "include").mkdir()
    (native / "lib/libsystemc.so").touch()
    (native / "include/systemc").touch()
    monkeypatch.setattr(sys, "path", [str(site_packages)])

    resolved_package, resolved_native = checker.package_details()

    assert resolved_package == package.resolve()
    assert resolved_native == native.resolve()
    assert not sentinel.exists()
    assert all(name not in sys.modules for name in package_names)

    missing_root = tmp_path / "missing-child-site-packages"
    (missing_root / "amal").mkdir(parents=True)
    monkeypatch.setattr(sys, "path", [str(missing_root)])
    with pytest.raises(AssertionError, match="installed amal.eda package is missing"):
        checker.package_details()
    assert all(name not in sys.modules for name in package_names)


def test_package_details_preserves_existing_namespace_parent_modules(tmp_path, monkeypatch):
    """Keep preexisting namespace parent objects intact during leaf lookup.

    Args:
        tmp_path: pytest-provided fake site-packages namespace tree.
        monkeypatch: pytest fixture restoring any parent objects after the test.

    Returns:
        None: Both existing parent module identities survive native-free lookup.
    """
    checker = _load_checker()
    site_packages = tmp_path / "site-packages"
    package = site_packages / "amal/eda/systemc"
    package.mkdir(parents=True)
    sentinel = tmp_path / "kernel-imported.txt"
    (package / "__init__.py").write_text(f"from pathlib import Path\nPath({str(sentinel)!r}).write_text('executed')\n")
    native = package / "_native"
    (native / "lib").mkdir(parents=True)
    (native / "include").mkdir()
    (native / "lib/libsystemc.so").touch()
    (native / "include/systemc").touch()

    root_module = types.ModuleType("amal")
    root_module.__path__ = [str(site_packages / "amal")]
    eda_module = types.ModuleType("amal.eda")
    eda_module.__path__ = [str(site_packages / "amal/eda")]
    monkeypatch.setitem(sys.modules, "amal", root_module)
    monkeypatch.setitem(sys.modules, "amal.eda", eda_module)
    monkeypatch.delitem(sys.modules, "amal.eda.systemc", raising=False)
    monkeypatch.setattr(sys, "path", [str(site_packages)])

    resolved_package, _resolved_native = checker.package_details()

    assert resolved_package == package.resolve()
    assert sys.modules["amal"] is root_module
    assert sys.modules["amal.eda"] is eda_module
    assert "amal.eda.systemc" not in sys.modules
    assert not sentinel.exists()


def test_sitecustomize_checks_exact_origin_without_hiding_real_package_import(tmp_path):
    """Validate package origin at startup, then allow the real installed package to initialize.

    Args:
        tmp_path: pytest-provided isolated directory for a fake installed package and guard.

    Returns:
        None: Assertions check accepted and rejected origins in native-free child processes.

    Raises:
        subprocess.TimeoutExpired: If either isolated startup-guard process exceeds 15 seconds.
    """
    checker = _load_checker()
    stage = tmp_path / "stage"
    site_packages = tmp_path / "site-packages"
    package = site_packages / "amal/eda/systemc"
    package.mkdir(parents=True)
    (site_packages / "amal/__init__.py").write_text('"""fake installed root"""\n')
    (site_packages / "amal/eda/__init__.py").write_text('"""fake installed namespace"""\n')
    sentinel = tmp_path / "kernel-imported.txt"
    (package / "__init__.py").write_text(f"from pathlib import Path\nPath({str(sentinel)!r}).write_text('executed')\n")
    expected_origin = package / "__init__.py"
    checker.write_sitecustomize(stage, expected_origin)
    environment = dict(os.environ)
    environment["PYTHONPATH"] = os.pathsep.join((str(stage), str(site_packages)))
    command = [
        sys.executable,
        "-c",
        "import sys; print('PREIMPORT=' + str('amal.eda.systemc' in sys.modules)); "
        "import amal.eda.systemc; print(amal.eda.systemc.__file__)",
    ]

    result = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=15)

    assert result.returncode == 0, result.stdout + result.stderr
    assert "PREIMPORT=False" in result.stdout
    assert str(expected_origin.resolve()) in result.stdout
    assert sentinel.read_text() == "executed"

    checker.write_sitecustomize(stage, tmp_path / "wrong-origin.py")
    rejected = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=15)
    assert rejected.returncode != 0
    assert "wrong SystemC package origin" in rejected.stderr


def test_expected_test_nodes_are_top_level_source_functions_and_reject_parametrization(tmp_path):
    """Derive the exact selected test identities and reject generated node expansion.

    Args:
        tmp_path: pytest-provided isolated directory holding synthetic staged modules.

    Returns:
        None: Assertions verify top-level collection and parametrization gating.
    """
    checker = _load_checker()
    tests = tmp_path / "tests"
    tests.mkdir()
    (tests / "support.py").write_text('"""Shared test helpers."""\n')
    (tests / "test_upstream.py").write_text(
        '"""Upstream suite."""\n'
        "def test_native():\n    def test_nested():\n        pass\n"
        "\nclass Helpers:\n    def helper():\n        pass\n"
    )
    (tests / "test_tutorials.py").write_text('"""Tutorial suite."""\ndef test_smoke():\n    pass\n')

    assert checker.expected_test_nodes(tests) == {
        ("test_upstream", "test_native"),
        ("test_tutorials", "test_smoke"),
    }

    (tests / "test_upstream.py").write_text(
        '"""Upstream suite."""\n'
        "import pytest\n"
        "@pytest.mark.parametrize('value', [1, 2])\n"
        "def test_native(value):\n    pass\n"
    )
    try:
        checker.expected_test_nodes(tests)
    except AssertionError as error:
        assert "parametrized test selection is unsupported" in str(error)
    else:
        raise AssertionError("parametrized selected test was accepted as a one-node AST selection")

    (tests / "test_upstream.py").write_text(
        '"""Upstream suite."""\nclass Tests:\n    def test_native(self):\n        pass\n'
    )
    try:
        checker.expected_test_nodes(tests)
    except AssertionError as error:
        assert "class-based test selection is unsupported" in str(error)
    else:
        raise AssertionError("class-based test was accepted as a top-level function selection")


def test_repository_regression_manifest_is_ast_derived_and_nonparametrized():
    """Guard the current selected source modules without importing their runtime dependencies.

    Returns:
        None: Assertions establish the known 41 top-level tests and both suite modules.
    """
    checker = _load_checker()
    expected = checker.expected_test_nodes(ROOT / "tests")

    assert len(expected) == 41
    assert {module for module, _ in expected} == {"test_upstream", "test_tutorials"}


def test_parse_junit_maps_selected_suites_and_skip_counts(tmp_path):
    """Normalize JUnit module names and aggregate skips from the same parsed report.

    Args:
        tmp_path: pytest-provided isolated directory holding synthetic JUnit XML.

    Returns:
        None: Assertions verify module identities, case outcomes and aggregate counters.
    """
    checker = _load_checker()
    report = tmp_path / "results.xml"
    report.write_text(
        '<testsuites tests="2" failures="0" errors="0" skipped="1">'
        '<testsuite name="pytest" tests="2" failures="0" errors="0" skipped="1">'
        '<testcase classname="tests.test_upstream" name="test_native" />'
        '<testcase classname="tests.test_tutorials" name="test_smoke"><skipped /></testcase>'
        "</testsuite></testsuites>"
    )

    failures, cases, counts = checker._parse_junit(report)

    assert failures == []
    assert [(case["module"], case["test"], case["result"]) for case in cases] == [
        ("test_upstream", "test_native", "passed"),
        ("test_tutorials", "test_smoke", "skipped"),
    ]
    assert counts == {"failures": 0, "errors": 0, "skipped": 1}


def test_regression_acceptance_requires_exact_nodes_passes_and_complete_inventory():
    """Reject omitted, duplicated, swapped-suite, skipped and unclassified results.

    Returns:
        None: Each incomplete result is rejected while the exact passing inventory is accepted.
    """
    checker = _load_checker()
    expected = {
        ("test_upstream", "test_native"),
        ("test_tutorials", "test_smoke"),
    }

    def case(module, test, result="passed"):
        """Build one normalized JUnit testcase record for acceptance checks."""
        return {"module": module, "test": test, "result": result, "suite": module}

    valid_cases = [case("test_upstream", "test_native"), case("test_tutorials", "test_smoke")]
    valid = checker.reconcile_regression_results(expected, valid_cases, {}, 0, True)
    assert valid["status"] == "passed"
    assert valid["exact_selected_node_set"] is True

    omitted = checker.reconcile_regression_results(expected, valid_cases[:1], {}, 0, True)
    assert omitted["status"] == "incomplete"
    assert omitted["missing_node_ids"] == ["test_tutorials::test_smoke"]

    duplicated = checker.reconcile_regression_results(expected, [*valid_cases, valid_cases[0]], {}, 0, True)
    assert duplicated["status"] == "incomplete"
    assert duplicated["duplicate_node_ids"] == ["test_upstream::test_native"]

    wrong_suite = checker.reconcile_regression_results(
        expected,
        [case("test_tutorials", "test_native"), valid_cases[1]],
        {},
        0,
        True,
    )
    assert wrong_suite["status"] == "incomplete"
    assert wrong_suite["wrong_suite_nodes"] == [
        {"observed": "test_tutorials::test_native", "expected_modules": ["test_upstream"]}
    ]

    unclassified = checker.reconcile_regression_results(expected, valid_cases, {}, 0, False)
    assert unclassified["status"] == "incomplete"
    assert unclassified["example_classification_reconciles"] is False

    skipped = checker.reconcile_regression_results(
        expected,
        [case("test_upstream", "test_native"), case("test_tutorials", "test_smoke", "skipped")],
        {"skipped": 1},
        0,
        True,
    )
    assert skipped["status"] == "incomplete"
    assert skipped["no_skips_or_errors"] is False


def test_inventory_mapping_rejects_unclassified_and_duplicate_role_files(tmp_path, monkeypatch):
    """Reconcile every example Python path once across entrypoint/support/other roles.

    Args:
        tmp_path: pytest-provided isolated fake checkout.
        monkeypatch: pytest fixture used to select a tiny portable classification.

    Returns:
        None: Assertions verify valid, unclassified and duplicate-role inventories.
    """
    checker = _load_checker()
    checkout = tmp_path / "checkout"
    inventory = checkout / "tools/inventory_examples.py"
    inventory.parent.mkdir(parents=True)
    (checkout / "examples").mkdir()
    (checkout / "examples/runnable.py").write_text('"""Runnable example."""\n')
    inventory.write_text("PYTHON_ENTRYPOINTS = {}\nPYTHON_SUPPORT_FILES = {}\n")
    monkeypatch.setattr(checker, "TUTORIAL_ENTRYPOINTS", ("examples/runnable.py",))
    monkeypatch.setattr(checker, "OTHER_PYTHON_FILES", ())

    valid = checker.inventory_mapping(checkout)
    assert valid["classification_reconciles"] is True
    assert valid["unclassified_python_files"] == []
    assert valid["stale_classified_python_files"] == []

    (checkout / "examples/orphan.py").write_text('"""Not classified."""\n')
    unclassified = checker.inventory_mapping(checkout)
    assert unclassified["classification_reconciles"] is False
    assert unclassified["unclassified_python_files"] == ["examples/orphan.py"]

    (checkout / "examples/orphan.py").unlink()
    inventory.write_text(
        "PYTHON_ENTRYPOINTS = {('sysc/demo', 'demo'): ['examples/runnable.py']}\nPYTHON_SUPPORT_FILES = {}\n"
    )
    duplicate = checker.inventory_mapping(checkout)
    assert duplicate["classification_reconciles"] is False
    assert duplicate["duplicate_classified_python_files"] == ["examples/runnable.py"]


def test_repository_example_inventory_reconciles_all_python_files():
    """Keep the checked-in inventory complete without importing the SystemC runtime.

    Returns:
        None: Assertions report any unclassified, stale or multiply classified paths.
    """
    checker = _load_checker()
    mapping = checker.inventory_mapping(ROOT)

    assert mapping["classification_reconciles"] is True, mapping
    assert "examples/counter/py/tb.py" in mapping["runnable_entrypoints"]
    assert {
        "path": "examples/counter/py/system.py",
        "role": "tutorial support module",
    } in mapping["other_python_files"]


class _FakeDistribution:
    """Expose a portable installed-file manifest and its fake site-packages paths."""

    def __init__(self, site_packages, files):
        """Store a fake installation root and distribution-relative file entries."""
        self.site_packages = Path(site_packages)
        self.files = list(files)

    def locate_file(self, relative):
        """Resolve one fake distribution entry below its site-packages root."""
        return self.site_packages / Path(str(relative))


def _make_fake_runtime_install(tmp_path, checker, source_count=FROZEN_CHECKOUT_RUNTIME_SOURCE_COUNT):
    """Create matching source rosters plus ignored native/cache data.

    Args:
        tmp_path: Pytest temporary root for the fake checkout and installed tree.
        checker: Loaded checker module providing runtime source roots.
        source_count: Number of matching runtime source files in each roster.

    Returns:
        tuple[Path, _FakeDistribution]: Fake checkout and installed distribution metadata.
    """
    checkout = tmp_path / "checkout"
    site_packages = tmp_path / "site-packages"
    paths = [
        "amal/eda/systemc/__init__.py",
        "amal/eda/systemc/_runtime.py",
        "amal/eda/cli/__init__.py",
    ]
    paths.extend(f"amal/eda/systemc/runtime_{index:03}.py" for index in range(source_count - len(paths)))
    manifest = []
    for index, relative in enumerate(paths):
        content = f"runtime source {index}\n".encode()
        for root in (checkout / "src", site_packages):
            target = root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(content)
        manifest.append(Path(relative))

    ignored = (
        "amal/eda/systemc/_native/include/first_party_adapter.hpp",
        "amal/eda/systemc/__pycache__/cached.py",
        "amal/eda/cli/README.md",
        "amal/eda/cli/commands.pyi",
    )
    for relative in ignored:
        target = site_packages / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text("ignored data\n")
        manifest.append(Path(relative))
    return checkout, _FakeDistribution(site_packages, manifest)


def test_runtime_source_parity_checks_exact_bytes_and_ignores_native_data(tmp_path):
    """Accept a byte-identical frozen runtime roster without considering bundled native data.

    Args:
        tmp_path: Pytest-provided fake checkout/install root.

    Returns:
        None: Parity counts, hash result and ignored manifest count are asserted.
    """
    checker = _load_checker()
    checkout, installed = _make_fake_runtime_install(tmp_path, checker)

    parity = checker.runtime_source_parity(
        checkout,
        installed,
        installed.site_packages / "amal/eda/systemc",
    )

    assert parity["status"] == "passed", parity
    assert parity["checkout_runtime_source_count"] == FROZEN_CHECKOUT_RUNTIME_SOURCE_COUNT
    assert parity["installed_runtime_source_count"] == FROZEN_CHECKOUT_RUNTIME_SOURCE_COUNT
    assert parity["systemc_package_origin_matches_manifest"] is True
    assert parity["ignored_installed_manifest_entry_count"] == 4
    assert parity["missing_paths"] == []
    assert parity["extra_paths"] == []
    assert parity["mismatches"] == []


def test_checkout_runtime_source_roster_includes_reviewed_native_adapters():
    """Guard the currently reviewed Python/C++ runtime source roster without imports.

    Returns:
        None: The roster includes the integer and event-registration headers added
        since the historical 111-file fixture, alongside its existing sources.
    """
    checker = _load_checker()

    roster = checker._checkout_runtime_sources(ROOT)
    assert len(roster) == FROZEN_CHECKOUT_RUNTIME_SOURCE_COUNT + 2
    assert "amal/eda/systemc/sc_dt/integers.h" in roster
    assert "amal/eda/systemc/sc_core/event_list_registration.h" in roster


def test_runtime_source_parity_accepts_additional_legitimate_checkout_sources(tmp_path):
    """Accept equal checkout/install rosters after a legitimate source addition.

    Args:
        tmp_path: Pytest-provided fake checkout/install root.

    Returns:
        None: Dynamic counts and parity success are asserted after adding one source.
    """
    checker = _load_checker()
    checkout, installed = _make_fake_runtime_install(
        tmp_path,
        checker,
        source_count=FROZEN_CHECKOUT_RUNTIME_SOURCE_COUNT + 1,
    )

    parity = checker.runtime_source_parity(
        checkout,
        installed,
        installed.site_packages / "amal/eda/systemc",
    )

    assert parity["status"] == "passed", parity
    assert parity["expected_runtime_source_count"] == FROZEN_CHECKOUT_RUNTIME_SOURCE_COUNT + 1
    assert parity["checkout_runtime_source_count"] == FROZEN_CHECKOUT_RUNTIME_SOURCE_COUNT + 1
    assert parity["installed_runtime_source_count"] == FROZEN_CHECKOUT_RUNTIME_SOURCE_COUNT + 1


def test_runtime_source_parity_rejects_stale_missing_and_extra_installed_sources(tmp_path):
    """Reject same-version stale bytes and installed source-set drift with portable paths.

    Args:
        tmp_path: Pytest-provided fake checkout/install root.

    Returns:
        None: Hash mismatch, missing and extra report details are asserted.
    """
    checker = _load_checker()
    checkout, installed = _make_fake_runtime_install(tmp_path, checker)
    first, second = installed.files[:2]
    (installed.locate_file(first)).write_text("stale wheel byte\n")
    installed.files.remove(second)
    extra = Path("amal/eda/cli/stale_module.py")
    (installed.locate_file(extra)).parent.mkdir(parents=True, exist_ok=True)
    installed.locate_file(extra).write_text("stale extra module\n")
    installed.files.append(extra)

    parity = checker.runtime_source_parity(checkout, installed)

    assert parity["status"] == "failed"
    assert parity["checkout_runtime_source_count"] == FROZEN_CHECKOUT_RUNTIME_SOURCE_COUNT
    assert parity["installed_runtime_source_count"] == FROZEN_CHECKOUT_RUNTIME_SOURCE_COUNT
    assert parity["missing_paths"] == [second.as_posix()]
    assert parity["extra_paths"] == [extra.as_posix()]
    assert parity["mismatch_count"] == 1
    assert parity["mismatches"][0]["path"] == first.as_posix()
    assert parity["mismatches"][0]["checkout_sha256"] != parity["mismatches"][0]["installed_sha256"]


def test_all_validation_stops_before_smoke_when_installed_sources_are_stale(tmp_path, monkeypatch):
    """Write a portable parity failure report without entering installed/native smoke.

    Args:
        tmp_path: Pytest-provided fake checkout, distribution and output tree.
        monkeypatch: Pytest fixture replacing metadata and smoke entrypoints.

    Returns:
        None: The checker exits with source parity failure and leaves smoke untested.
    """
    checker = _load_checker()
    checkout = tmp_path / "checkout"
    (checkout / "tools").mkdir(parents=True)
    (checkout / "tools/inventory_examples.py").write_text("PYTHON_ENTRYPOINTS = {}\nPYTHON_SUPPORT_FILES = {}\n")
    (checkout / "examples").mkdir()
    for relative in ("amal/eda/systemc/__init__.py", "amal/eda/cli/__init__.py"):
        source = checkout / "src" / relative
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_text("checkout runtime\n")
    site_packages = tmp_path / "site-packages"
    manifest = []
    for relative in ("amal/eda/systemc/__init__.py", "amal/eda/cli/__init__.py"):
        installed_file = site_packages / relative
        installed_file.parent.mkdir(parents=True, exist_ok=True)
        installed_file.write_text("stale same-version wheel runtime\n")
        manifest.append(Path(relative))
    installed = _FakeDistribution(site_packages, manifest)
    monkeypatch.setattr(checker.importlib.metadata, "distribution", lambda name: installed)
    smoke_calls = []
    monkeypatch.setattr(
        checker,
        "run_console_and_tutorial_smoke",
        lambda *args, **kwargs: smoke_calls.append((args, kwargs)),
    )
    report_dir = tmp_path / "report"

    result = checker.run_all_validation(
        checkout,
        report_dir,
        site_packages / "amal/eda/systemc",
        tmp_path / "wheel/amal/eda/systemc/_native",
    )

    report = json.loads((report_dir / "installed-example-report.json").read_text())
    assert result == 1
    assert smoke_calls == []
    assert report["installed_runtime_source_parity"]["status"] == "failed"
    assert report["installed_runtime_source_parity"]["mismatch_count"] == 2
    assert report["installed_console_smoke_result"] == "untested"
    assert "site-packages" not in (report_dir / "installed-example-report.json").read_text()

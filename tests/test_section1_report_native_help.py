"""Acceptance checks for curated, metadata-only ``sc_report`` help."""

from __future__ import annotations

import importlib.util
from pathlib import Path
import re
import subprocess
import sys
import types


ROOT = Path(__file__).resolve().parents[1]
SYSTEMC_HEADER = ROOT / "submodules/systemc/src/sysc/utils/sc_report.h"
REPORT_MODEL = ROOT / "tests/models/section1_report_native_help.py"


def _load_formatter():
    """Load formatter constants without initializing the native SystemC kernel."""
    package_root = ROOT / "src/amal/eda/systemc"
    package_name = "_section1_report_help_test_package"
    package = types.ModuleType(package_name)
    package.__path__ = [str(package_root)]
    sys.modules[package_name] = package

    for module_name in ("_namespace", "_native_help"):
        qualified_name = f"{package_name}.{module_name}"
        spec = importlib.util.spec_from_file_location(qualified_name, package_root / f"{module_name}.py")
        assert spec is not None and spec.loader is not None
        module = importlib.util.module_from_spec(spec)
        sys.modules[qualified_name] = module
        spec.loader.exec_module(module)
    return sys.modules[f"{package_name}._native_help"]


def _public_sections(header: str) -> list[str]:
    """Return public access blocks from the sc_report class declaration."""
    class_start = header.index("class SC_API sc_report :")
    class_end = header.index("};", class_start)
    class_body = header[class_start:class_end]
    pattern = re.compile(
        r"(?m)^\s*public\s*:[^\n]*\n(?P<body>.*?)(?=^\s*(?:public|protected|private)\s*:|\Z)",
        re.DOTALL,
    )
    return [match.group("body") for match in pattern.finditer(class_body)]


def _normalize_signature(signature: str, method_name: str) -> str:
    """Canonicalize header whitespace and qualify names used by registry text."""
    normalized = " ".join(signature.split())
    normalized = re.sub(r"\s+([*&])", r"\1", normalized)
    normalized = re.sub(r"\(\s+", "(", normalized)
    normalized = re.sub(r"\s+\)", ")", normalized)
    normalized = re.sub(r"\s+\(", "(", normalized)
    normalized = normalized.replace("sc_severity ", "sc_core::sc_severity ")
    normalized = normalized.replace("sc_time&", "sc_core::sc_time&")
    normalized = normalized.replace("sc_report&", "sc_core::sc_report&")
    return re.sub(
        rf"\b{re.escape(method_name)}\s*\(",
        f"sc_core::sc_report::{method_name}(",
        normalized,
        count=1,
    )


def _header_signatures(header: str, method_names: tuple[str, ...]) -> set[str]:
    """Extract the selected declarations, stopping before inline method bodies."""
    declaration_pattern = {
        method_name: re.compile(
            rf"(?m)^\s*(?P<signature>[^;{{}}]*?\b{re.escape(method_name)}\s*\([^;{{}}]*\)"
            r"\s*(?:const\s*)?(?:SC_NOEXCEPT_\s*)?)(?=;|\s*\{)"
        )
        for method_name in method_names
    }
    found: set[str] = set()
    for public_section in _public_sections(header):
        for method_name, pattern in declaration_pattern.items():
            match = pattern.search(public_section)
            if match:
                found.add(_normalize_signature(match.group("signature"), method_name))
    return found


def test_curated_report_signatures_match_the_pinned_public_header():
    """Keep the curated registry in parity with all twelve public declarations."""
    formatter = _load_formatter()
    declarations = formatter._SC_REPORT_DECLARATIONS_BY_CPP_NAME["sc_core::sc_report"]
    method_names = tuple(
        re.search(r"sc_core::sc_report::([A-Za-z_]\w*)\(", signature).group(1)
        for signature in declarations
    )
    actual = _header_signatures(SYSTEMC_HEADER.read_text(), method_names)

    assert len(declarations) == 12
    assert len(set(method_names)) == 12
    assert actual == set(declarations), f"header/registry mismatch: {actual ^ set(declarations)}"


def test_exact_sc_report_help_preserves_identity_and_existing_overload_help(tmp_path: Path):
    """Use a fresh process to check exact-key help, method metadata and proxy identity."""
    result = subprocess.run(
        [sys.executable, str(REPORT_MODEL)],
        cwd=tmp_path,
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    assert "SECTION1_REPORT_NATIVE_HELP_OK" in result.stdout

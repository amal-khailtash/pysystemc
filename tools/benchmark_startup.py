"""Measure Python and SystemC startup in isolated interpreter processes.

The benchmark records one initial process and a configurable number of
subsequent processes for each selected scenario. It preserves inherited
environment settings and operating-system caches; results therefore describe
initial and repeated warm behavior, not filesystem-cold startup.
"""

from __future__ import annotations

import argparse
from datetime import UTC, datetime
import importlib.metadata
import json
import math
import os
from pathlib import Path
import platform
import statistics
import subprocess
import sys
import tempfile
from time import perf_counter
from typing import Any

FORMAT_VERSION = 1
DEFAULT_REPETITIONS = 10
DEFAULT_TIMEOUT_SECONDS = 300.0
MARKER = "__PYSYSTEMC_STARTUP_BENCHMARK__"

SCENARIOS = (
    "systemc-py-help",
    "systemc-py-version",
    "systemc-cpp-help",
    "systemc-cpp-version",
    "interpreter-baseline",
    "cppyy-import",
    "systemc-import",
    "model-construction",
    "sc-dt-template-use",
    "tlm-use",
    "repl-initialization",
)

_WORKLOADS = {
    "systemc-py-help": r"""
import importlib.abc
import runpy
import sys

class _NativeImportGuard(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        del path, target
        if fullname == "cppyy" or fullname.startswith("cppyy.") or fullname == "amal.eda.systemc" or fullname.startswith("amal.eda.systemc."):
            raise AssertionError(f"native import attempted during systemc-py help: {fullname}")
        return None

sys.meta_path.insert(0, _NativeImportGuard())
sys.argv = ["systemc-py", "--help"]
started = perf_counter()
try:
    runpy.run_module("amal.eda.cli.__main__", run_name="__main__")
except SystemExit as exc:
    assert exc.code in (None, 0), f"systemc-py help exited with {exc.code!r}"
assert "cppyy" not in sys.modules and "amal.eda.systemc" not in sys.modules
result = {"check": "systemc-py help dispatch succeeded; native imports absent", "versions": {}, "phases_seconds": {"cli_dispatch_seconds": perf_counter() - started}}
""",
    "systemc-py-version": r"""
import importlib.abc
import runpy
import sys

class _NativeImportGuard(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        del path, target
        if fullname == "cppyy" or fullname.startswith("cppyy.") or fullname == "amal.eda.systemc" or fullname.startswith("amal.eda.systemc."):
            raise AssertionError(f"native import attempted during systemc-py version: {fullname}")
        return None

sys.meta_path.insert(0, _NativeImportGuard())
sys.argv = ["systemc-py", "--version"]
started = perf_counter()
try:
    runpy.run_module("amal.eda.cli.__main__", run_name="__main__")
except SystemExit as exc:
    assert exc.code in (None, 0), f"systemc-py version exited with {exc.code!r}"
assert "cppyy" not in sys.modules and "amal.eda.systemc" not in sys.modules
result = {"check": "systemc-py version dispatch succeeded; native imports absent", "versions": {}, "phases_seconds": {"cli_dispatch_seconds": perf_counter() - started}}
""",
    "systemc-cpp-help": r"""
import importlib.abc
import sys

class _NativeImportGuard(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        del path, target
        if fullname == "cppyy" or fullname.startswith("cppyy.") or fullname == "amal.eda.systemc" or fullname.startswith("amal.eda.systemc."):
            raise AssertionError(f"native import attempted during systemc-cpp help: {fullname}")
        return None

sys.meta_path.insert(0, _NativeImportGuard())
started = perf_counter()
from amal.eda.cli.cpp import main

try:
    main(["--help"])
except SystemExit as exc:
    assert exc.code in (None, 0), f"systemc-cpp help exited with {exc.code!r}"
assert "cppyy" not in sys.modules and "amal.eda.systemc" not in sys.modules
result = {"check": "systemc-cpp help dispatch succeeded; native imports absent", "versions": {}, "phases_seconds": {"cli_dispatch_seconds": perf_counter() - started}}
""",
    "systemc-cpp-version": r"""
import importlib.abc
import sys

class _NativeImportGuard(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        del path, target
        if fullname == "cppyy" or fullname.startswith("cppyy.") or fullname == "amal.eda.systemc" or fullname.startswith("amal.eda.systemc."):
            raise AssertionError(f"native import attempted during systemc-cpp version: {fullname}")
        return None

sys.meta_path.insert(0, _NativeImportGuard())
started = perf_counter()
from amal.eda.cli.cpp import main

try:
    main(["--version"])
except SystemExit as exc:
    assert exc.code in (None, 0), f"systemc-cpp version exited with {exc.code!r}"
assert "cppyy" not in sys.modules and "amal.eda.systemc" not in sys.modules
result = {"check": "systemc-cpp version dispatch succeeded; native imports absent", "versions": {}, "phases_seconds": {"cli_dispatch_seconds": perf_counter() - started}}
""",
    "interpreter-baseline": r"""
assert sys.version_info >= (3, 14), f"Python 3.14+ required, got {sys.version}"
result = {"check": "interpreter initialized", "versions": {}, "phases_seconds": {"benchmark_runner_ready_seconds": perf_counter() - process_ready}}
""",
    "cppyy-import": r"""
import os

inherited_extra_cling_args = os.environ.get("EXTRA_CLING_ARGS")
os.environ.setdefault("EXTRA_CLING_ARGS", "-std=c++17")
started = perf_counter()
import cppyy
cppyy_import_seconds = perf_counter() - started
assert hasattr(cppyy, "gbl"), "cppyy did not expose its global namespace"
result = {
    "check": "cppyy global namespace available",
    "versions": {"cppyy": getattr(cppyy, "__version__", "unknown")},
    "phases_seconds": {"cppyy_import_seconds": cppyy_import_seconds},
    "scenario_configuration": {
        "effective_extra_cling_args": os.environ["EXTRA_CLING_ARGS"],
        "cppyy_default_applied": inherited_extra_cling_args is None,
    },
}
""",
    "systemc-import": r"""
started = perf_counter()
import amal.eda.systemc as systemc
systemc_import_seconds = perf_counter() - started
release = str(systemc.sc_core.sc_release())
assert release, "SystemC release string was empty"
result = {"check": "SystemC imported", "versions": {"systemc": release}, "phases_seconds": {"systemc_import_seconds": systemc_import_seconds}}
""",
    "model-construction": r"""
started = perf_counter()
from amal.eda.systemc import sc_core
systemc_import_seconds = perf_counter() - started

started = perf_counter()
class StartupBenchmarkModel(sc_core.sc_module):
    def __init__(self):
        super().__init__("startup_benchmark_model")
        self.signal = sc_core.sc_signal[int]("signal")

model = StartupBenchmarkModel()
assert str(model.name()) == "startup_benchmark_model", f"unexpected model name: {model.name()}"
assert model.signal.read() == 0, "new signal did not have its default value"
result = {"check": "sc_module and typed signal constructed", "versions": {"systemc": str(sc_core.sc_release())}, "phases_seconds": {"systemc_import_seconds": systemc_import_seconds, "first_model_construction_seconds": perf_counter() - started}}
""",
    "sc-dt-template-use": r"""
started = perf_counter()
from amal.eda.systemc import sc_dt, sc_core
systemc_import_seconds = perf_counter() - started

started = perf_counter()
value = sc_dt.sc_biguint[257](123456789)
first_template_use_seconds = perf_counter() - started
assert value.to_uint64() == 123456789, f"arbitrary-width sc_biguint value mismatch: {value.to_uint64()}"
result = {"check": "sc_biguint[257] instantiated and converted", "versions": {"systemc": str(sc_core.sc_release())}, "phases_seconds": {"systemc_import_seconds": systemc_import_seconds, "first_template_use_seconds": first_template_use_seconds}}
""",
    "tlm-use": r"""
started = perf_counter()
from amal.eda.systemc import tlm, sc_core
systemc_import_seconds = perf_counter() - started

started = perf_counter()
payload = tlm.tlm_generic_payload()
payload.set_address(0x123456789ABC)
first_tlm_use_seconds = perf_counter() - started
assert int(payload.get_address()) == 0x123456789ABC, "TLM address did not round-trip"
result = {"check": "generic payload constructed and address round-tripped", "versions": {"systemc": str(sc_core.sc_release())}, "phases_seconds": {"systemc_import_seconds": systemc_import_seconds, "first_tlm_use_seconds": first_tlm_use_seconds}}
""",
    "repl-initialization": r"""
started = perf_counter()
from amal.eda.systemc.repl import Repl
repl_module_import_seconds = perf_counter() - started

started = perf_counter()
repl = Repl(no_history=True)
repl_initialization_seconds = perf_counter() - started
assert {"sc_core", "sc_dt", "tlm", "cppyy"}.issubset(repl.namespace), "REPL namespace is incomplete"
assert repl.session is not None and repl.console is not None, "REPL did not initialize its console and prompt session"
result = {"check": "REPL initialized without requesting input", "versions": {"systemc": str(repl.namespace["sc_core"].sc_release())}, "phases_seconds": {"repl_module_import_seconds": repl_module_import_seconds, "repl_initialization_seconds": repl_initialization_seconds}}
""",
}


class BenchmarkError(RuntimeError):
    """A scenario failed its output checks or exceeded its timeout."""

    def __init__(
        self,
        message: str,
        *,
        returncode: int | None = None,
        stdout: str = "",
        stderr: str = "",
    ) -> None:
        """Retain complete child diagnostics for failed sample records."""
        super().__init__(message)
        self.returncode = returncode
        self.stdout = stdout
        self.stderr = stderr


def _child_source(scenario: str) -> str:
    """Build a child program that runs one checked workload and reports RSS."""
    workload = _WORKLOADS[scenario]
    return f"""\
import json
import sys
from time import perf_counter

try:
    import resource
except ImportError:
    resource = None

process_ready = perf_counter()
{workload}

if resource is not None:
    peak = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    # Linux and the BSDs report KiB; macOS reports bytes.
    peak_bytes = int(peak if sys.platform == "darwin" else peak * 1024)
    peak_method = "resource.ru_maxrss"
elif sys.platform == "win32":
    import ctypes
    from ctypes import wintypes

    class _ProcessMemoryCounters(ctypes.Structure):
        _fields_ = [
            ("cb", wintypes.DWORD),
            ("PageFaultCount", wintypes.DWORD),
            ("PeakWorkingSetSize", ctypes.c_size_t),
            ("WorkingSetSize", ctypes.c_size_t),
            ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
            ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
            ("PagefileUsage", ctypes.c_size_t),
            ("PeakPagefileUsage", ctypes.c_size_t),
        ]

    counters = _ProcessMemoryCounters()
    counters.cb = ctypes.sizeof(counters)
    process = ctypes.windll.kernel32.GetCurrentProcess()
    ok = ctypes.windll.psapi.GetProcessMemoryInfo(process, ctypes.byref(counters), counters.cb)
    if not ok:
        raise OSError("GetProcessMemoryInfo failed")
    peak_bytes = int(counters.PeakWorkingSetSize)
    peak_method = "GetProcessMemoryInfo.PeakWorkingSetSize"
else:
    peak_bytes = None
    peak_method = "unsupported"

payload = {{"scenario": {scenario!r}, "ok": True, "check": result["check"], "versions": result["versions"], "phases_seconds": result["phases_seconds"], "scenario_configuration": result.get("scenario_configuration", {{}}), "peak_rss_bytes": peak_bytes, "peak_rss_method": peak_method}}
print({MARKER!r} + json.dumps(payload, sort_keys=True))
"""


def _parse_child_output(stdout: str, scenario: str) -> dict[str, Any]:
    """Parse and validate the structured child result from captured output."""
    for line in reversed(stdout.splitlines()):
        if line.startswith(MARKER):
            try:
                payload = json.loads(line[len(MARKER) :])
            except json.JSONDecodeError as exc:
                raise BenchmarkError(f"{scenario}: malformed child result: {exc}") from exc
            if payload.get("scenario") != scenario or payload.get("ok") is not True:
                raise BenchmarkError(f"{scenario}: child result did not identify a successful matching scenario")
            if not isinstance(payload.get("check"), str) or not payload["check"]:
                raise BenchmarkError(f"{scenario}: child result omitted its output check")
            rss = payload.get("peak_rss_bytes")
            if rss is not None and (isinstance(rss, bool) or not isinstance(rss, int) or rss < 0):
                raise BenchmarkError(f"{scenario}: invalid peak RSS value {rss!r}")
            phases = payload.get("phases_seconds")
            if not isinstance(phases, dict) or any(
                not isinstance(value, (int, float)) or isinstance(value, bool) or not math.isfinite(value) or value < 0
                for value in phases.values()
            ):
                raise BenchmarkError(f"{scenario}: invalid phase timing values")
            return payload
    raise BenchmarkError(f"{scenario}: child omitted its structured result marker")


def _run_once(scenario: str, timeout_seconds: float) -> dict[str, Any]:
    """Run one scenario in a fresh process and return checked timing data."""
    command = [os.path.abspath(sys.executable), "-c", _child_source(scenario)]
    started = perf_counter()
    with tempfile.TemporaryDirectory(prefix="pysystemc-startup-") as sample_cwd:
        try:
            completed = subprocess.run(
                command,
                check=False,
                capture_output=True,
                text=True,
                timeout=timeout_seconds,
                cwd=sample_cwd,
                # env=None deliberately preserves the complete inherited environment.
            )
        except subprocess.TimeoutExpired as exc:
            stdout = exc.stdout.decode("utf-8", "replace") if isinstance(exc.stdout, bytes) else (exc.stdout or "")
            stderr = exc.stderr.decode("utf-8", "replace") if isinstance(exc.stderr, bytes) else (exc.stderr or "")
            raise BenchmarkError(
                f"{scenario}: exceeded {timeout_seconds:g}s timeout",
                stdout=stdout,
                stderr=stderr,
            ) from exc
    elapsed = perf_counter() - started
    if completed.returncode != 0:
        detail = (completed.stderr or completed.stdout).strip()[-2000:]
        raise BenchmarkError(
            f"{scenario}: child exited {completed.returncode}: {detail}",
            returncode=completed.returncode,
            stdout=completed.stdout,
            stderr=completed.stderr,
        )
    try:
        payload = _parse_child_output(completed.stdout, scenario)
    except BenchmarkError as exc:
        raise BenchmarkError(
            str(exc),
            returncode=completed.returncode,
            stdout=completed.stdout,
            stderr=completed.stderr,
        ) from exc
    expected_output = {
        "systemc-py-help": "Explore native SystemC",
        "systemc-py-version": "systemc-py v",
        "systemc-cpp-help": "Run native SystemC C++ models.",
        "systemc-cpp-version": "systemc-cpp v",
    }.get(scenario)
    if expected_output is not None and expected_output not in completed.stdout:
        raise BenchmarkError(
            f"{scenario}: expected output {expected_output!r} was absent",
            returncode=completed.returncode,
            stdout=completed.stdout,
            stderr=completed.stderr,
        )
    return {
        "wall_seconds": elapsed,
        "peak_rss_bytes": payload["peak_rss_bytes"],
        "peak_rss_method": payload["peak_rss_method"],
        "phases_seconds": payload["phases_seconds"],
        "output_check": payload["check"],
        "versions": payload["versions"],
        "scenario_configuration": payload["scenario_configuration"],
        "stdout_bytes": len(completed.stdout.encode("utf-8")),
        "stderr_bytes": len(completed.stderr.encode("utf-8")),
    }


def _summary(samples: list[dict[str, Any]]) -> dict[str, Any]:
    """Summarize successful measured runs using median and range spread."""
    if not samples:
        return {
            "successful_sample_count": 0,
            "median_wall_seconds": None,
            "min_wall_seconds": None,
            "max_wall_seconds": None,
            "wall_spread_seconds": None,
            "median_peak_rss_bytes": None,
            "min_peak_rss_bytes": None,
            "max_peak_rss_bytes": None,
            "phase_summary": {},
        }
    times = [float(sample["wall_seconds"]) for sample in samples]
    rss_values = [sample["peak_rss_bytes"] for sample in samples if sample["peak_rss_bytes"] is not None]
    phase_names = sorted({name for sample in samples for name in sample.get("phases_seconds", {})})
    phase_summary: dict[str, dict[str, float]] = {}
    for name in phase_names:
        values = [float(sample["phases_seconds"][name]) for sample in samples if name in sample["phases_seconds"]]
        phase_summary[name] = {
            "median_seconds": statistics.median(values),
            "min_seconds": min(values),
            "max_seconds": max(values),
            "spread_seconds": max(values) - min(values),
        }
    return {
        "successful_sample_count": len(samples),
        "median_wall_seconds": statistics.median(times),
        "min_wall_seconds": min(times),
        "max_wall_seconds": max(times),
        "wall_spread_seconds": max(times) - min(times),
        "median_peak_rss_bytes": int(statistics.median(rss_values)) if rss_values else None,
        "min_peak_rss_bytes": min(rss_values) if rss_values else None,
        "max_peak_rss_bytes": max(rss_values) if rss_values else None,
        "phase_summary": phase_summary,
    }


def _attempt_once(scenario: str, timeout_seconds: float) -> dict[str, Any]:
    """Record one scenario outcome without preventing later scenarios from running."""
    try:
        sample = _run_once(scenario, timeout_seconds)
    except BenchmarkError as exc:
        return {
            "status": "failed",
            "failure": str(exc),
            "returncode": exc.returncode,
            "stdout": exc.stdout,
            "stderr": exc.stderr,
        }
    return {"status": "passed", **sample}


def _distribution_version(name: str) -> str | None:
    """Return an installed distribution version without importing its package."""
    try:
        return importlib.metadata.version(name)
    except importlib.metadata.PackageNotFoundError:
        return None


def _configuration() -> dict[str, Any]:
    """Collect portable runtime facts without recording sensitive values."""
    relevant_environment = (
        "EXTRA_CLING_ARGS",
        "CLING_STANDARD_PCH",
        "CLING_REBUILD_PCH",
        "CPPYY_BACKEND_LIBRARY",
        "SYSTEMC_HOME",
    )
    return {
        "python_version": platform.python_version(),
        "python_implementation": platform.python_implementation(),
        "platform": platform.platform(),
        "machine": platform.machine(),
        "python_executable_name": Path(sys.executable).name,
        "distributions": {
            "pysystemc": _distribution_version("pysystemc"),
            "cppyy": _distribution_version("cppyy"),
        },
        "inherited_environment": {
            "uv_variable_names": sorted(name for name in os.environ if name.startswith("UV_")),
            "relevant_flags_present": {name: name in os.environ for name in relevant_environment},
            "values_recorded": False,
        },
    }


def _build_parser() -> argparse.ArgumentParser:
    """Create the benchmark command-line parser."""
    parser = argparse.ArgumentParser(
        description=(
            "Measure fresh-process startup, imports, first native use, and REPL readiness. "
            "Initial and repeated runs keep inherited settings and OS caches unchanged."
        ),
    )
    parser.add_argument(
        "--scenario",
        action="append",
        choices=SCENARIOS,
        help="scenario to run (repeat to select several; default: all)",
    )
    parser.add_argument(
        "--repetitions",
        type=int,
        default=DEFAULT_REPETITIONS,
        help=f"measured fresh processes after the initial run (default: {DEFAULT_REPETITIONS})",
    )
    parser.add_argument(
        "--timeout-seconds",
        type=float,
        default=DEFAULT_TIMEOUT_SECONDS,
        help=f"per-process timeout (default: {DEFAULT_TIMEOUT_SECONDS:g})",
    )
    parser.add_argument("--output", type=Path, help="write JSON to this path instead of stdout")
    return parser


def run_benchmark(scenarios: list[str], repetitions: int, timeout_seconds: float) -> dict[str, Any]:
    """Run initial and repeated fresh-process samples for selected scenarios.

    Args:
        scenarios: Names from :data:`SCENARIOS` in the requested order.
        repetitions: Number of measured processes after each initial process.
        timeout_seconds: Maximum duration for each child process.

    Returns:
        A JSON-compatible document with host configuration, validation results,
        raw samples, and median/range summaries.

    Raises:
        ValueError: If repetitions or timeout values are invalid.

    Child failures and timeouts are retained as per-run result records. Later
    scenarios continue, and the remaining repetitions for a failed scenario
    are marked skipped.
    """
    if repetitions < 1:
        raise ValueError("repetitions must be at least one")
    if not math.isfinite(timeout_seconds) or timeout_seconds <= 0:
        raise ValueError("timeout-seconds must be a finite positive number")
    unknown = sorted(set(scenarios) - set(SCENARIOS))
    if unknown:
        raise ValueError(f"unknown scenario(s): {', '.join(unknown)}")

    result: dict[str, Any] = {
        "format_version": FORMAT_VERSION,
        "generated_at_utc": datetime.now(UTC).isoformat(),
        "methodology": {
            "process_model": "one fresh subprocess per scenario sample",
            "initial_run": "recorded separately before measured repetitions",
            "cache_policy": "preserve inherited cppyy settings and OS caches; no cache drop",
            "temperature": "initial and repeated warm behavior; not filesystem-cold",
            "timeout_seconds": timeout_seconds,
            "measured_repetitions": repetitions,
            "rss": "per-child peak working set normalized to bytes where supported",
            "wall_time_scope": "includes subprocess launch, interpreter startup, workload, and harness measurement overhead",
            "inner_phase_scope": "phase clocks start after child bootstrap and exclude interpreter startup",
            "failure_policy": "retain complete failed child output; skip remaining repetitions after a failed run",
        },
        "configuration": _configuration(),
        "scenarios": {},
    }

    for scenario in scenarios:
        initial = _attempt_once(scenario, timeout_seconds)
        measured = []
        if initial["status"] == "failed":
            measured.extend(
                {"status": "skipped", "reason": "initial run failed; repeated attempts skipped"}
                for _ in range(repetitions)
            )
        else:
            for repetition in range(repetitions):
                sample = _attempt_once(scenario, timeout_seconds)
                measured.append(sample)
                if sample["status"] == "failed":
                    remaining = repetitions - repetition - 1
                    measured.extend(
                        {"status": "skipped", "reason": "measured run failed; later attempts skipped"}
                        for _ in range(remaining)
                    )
                    break
        successful = [sample for sample in measured if sample["status"] == "passed"]
        failed_count = sum(sample["status"] == "failed" for sample in measured)
        failed_count += initial["status"] == "failed"
        skipped_count = sum(sample["status"] == "skipped" for sample in measured)
        result["scenarios"][scenario] = {
            "initial_run": initial,
            "samples": measured,
            "status": "failed" if failed_count else "passed",
            "failed_run_count": failed_count,
            "skipped_run_count": skipped_count,
            "summary": _summary(successful),
        }
    return result


def main(argv: list[str] | None = None) -> int:
    """Parse arguments, execute scenarios, and write the JSON result."""
    parser = _build_parser()
    args = parser.parse_args(argv)
    if args.repetitions < 1:
        parser.error("--repetitions must be at least one")
    if not math.isfinite(args.timeout_seconds) or args.timeout_seconds <= 0:
        parser.error("--timeout-seconds must be finite and greater than zero")
    try:
        document = run_benchmark(args.scenario or list(SCENARIOS), args.repetitions, args.timeout_seconds)
    except BenchmarkError as exc:
        parser.exit(1, f"benchmark failed: {exc}\n")

    rendered = json.dumps(document, indent=2, sort_keys=True) + "\n"
    if args.output is None:
        sys.stdout.write(rendered)
    else:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    failed = [name for name, data in document["scenarios"].items() if data["status"] == "failed"]
    if failed:
        print(f"benchmark completed with failing scenarios: {', '.join(failed)}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

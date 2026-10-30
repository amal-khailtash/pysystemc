"""Profile Python/SystemC startup and selected lazy native first uses.

Every measured sample runs in a fresh child interpreter. The import hook
preserves the package's real import order, records inclusive loader timings,
and wraps cppyy operations after cppyy itself loads. This tool never edits
runtime modules or changes inherited UV/CPPYY settings.
"""

from __future__ import annotations

import argparse
import functools
import hashlib
import importlib
import importlib.abc
import importlib.machinery
import json
import math
import os
from pathlib import Path
import platform
import statistics
import subprocess
import sys
import tempfile
import time
from types import ModuleType
from typing import Any


_CPPYY_OPERATIONS = ("load_library", "add_library_path", "add_include_path", "include", "cppdef")
_DEFAULT_PROBES = ("sc_biguint_257", "sc_signal_bool", "tlm_socket_73")
_RESULT_ENV = "PYSC_PROFILE_STARTUP_RESULT"


class ProfileRecorder:
    """Collect import, native-operation, and explicit first-use intervals."""

    def __init__(self) -> None:
        self.imports: list[dict[str, Any]] = []
        self.cppyy_calls: list[dict[str, Any]] = []
        self.operations: list[dict[str, Any]] = []
        self.instrumentation_errors: list[str] = []


class TimedLoader(importlib.abc.Loader):
    """Delegate module execution while recording its inclusive duration."""

    def __init__(self, fullname: str, loader: Any, recorder: ProfileRecorder) -> None:
        self.fullname = fullname
        self.loader = loader
        self.recorder = recorder

    def create_module(self, spec):
        """Delegate optional custom module creation to the wrapped loader."""
        create = getattr(self.loader, "create_module", None)
        return create(spec) if create is not None else None

    def exec_module(self, module: ModuleType) -> None:
        """Execute the original loader once and retain success or failure."""
        started = time.perf_counter_ns()
        failure = None
        try:
            self.loader.exec_module(module)
        except BaseException as error:
            failure = {"type": type(error).__name__, "message": str(error)}
            raise
        finally:
            self.recorder.imports.append(
                {
                    "module": self.fullname,
                    "elapsed_ns": time.perf_counter_ns() - started,
                    "inclusive": True,
                    "error": failure,
                }
            )
        if self.fullname == "cppyy":
            _wrap_cppyy_operations(module, self.recorder)

    def __getattr__(self, name: str) -> Any:
        """Keep optional loader protocol attributes available to importlib."""
        return getattr(self.loader, name)


class TimingFinder(importlib.abc.MetaPathFinder):
    """Wrap only cppyy and the configured binding package's Python modules."""

    def __init__(self, recorder: ProfileRecorder, package: str) -> None:
        self.recorder = recorder
        self.package = package

    def _selected(self, fullname: str) -> bool:
        return (
            fullname == "cppyy"
            or fullname.startswith("cppyy.")
            or fullname in (self.package,)
            or fullname.startswith(self.package + ".")
        )

    def find_spec(self, fullname, path=None, target=None):
        """Ask later finders for a spec and wrap its original loader."""
        if not self._selected(fullname):
            return None
        for finder in tuple(sys.meta_path):
            if finder is self:
                continue
            find = getattr(finder, "find_spec", None)
            if find is None:
                continue
            spec = find(fullname, path, target)
            if spec is None:
                continue
            if spec.loader is not None and not isinstance(spec.loader, TimedLoader):
                spec.loader = TimedLoader(fullname, spec.loader, self.recorder)
            return spec
        return None


def install_import_timing(recorder: ProfileRecorder, package: str) -> TimingFinder:
    """Install an in-process import hook without importing runtime packages."""
    finder = TimingFinder(recorder, package)
    sys.meta_path.insert(0, finder)
    return finder


def _safe_resource_label(operation: str, args: tuple[Any, ...], kwargs: dict[str, Any]) -> dict[str, Any]:
    """Summarize native call inputs without emitting full paths or C++ source."""
    value = args[0] if args else kwargs.get("path", kwargs.get("filename", ""))
    if operation == "cppdef":
        source = value if isinstance(value, (str, bytes)) else b""
        encoded = source.encode("utf-8", errors="replace") if isinstance(source, str) else source
        return {"source_bytes": len(encoded), "source_sha256": hashlib.sha256(encoded).hexdigest()}
    if isinstance(value, os.PathLike):
        value = os.fspath(value)
    if isinstance(value, str):
        normalized = value.replace("\\", "/")
        return {"resource": normalized.rsplit("/", 1)[-1]}
    return {"resource_type": type(value).__name__}


def _wrap_cppyy_operations(module: ModuleType, recorder: ProfileRecorder) -> None:
    """Replace available cppyy module call points with timing pass-throughs."""
    for operation in _CPPYY_OPERATIONS:
        original = getattr(module, operation, None)
        if not callable(original):
            continue

        @functools.wraps(original)
        def timed(*args, __operation=operation, __original=original, **kwargs):
            caller = sys._getframe(1).f_globals.get("__name__", "unknown")
            event = {
                "operation": __operation,
                "caller_module": caller,
                **_safe_resource_label(__operation, args, kwargs),
            }
            started = time.perf_counter_ns()
            failure = None
            try:
                return __original(*args, **kwargs)
            except BaseException as error:
                failure = {"type": type(error).__name__, "message": str(error)}
                raise
            finally:
                event["elapsed_ns"] = time.perf_counter_ns() - started
                event["error"] = failure
                recorder.cppyy_calls.append(event)

        try:
            setattr(module, operation, timed)
        except (AttributeError, TypeError) as error:
            recorder.instrumentation_errors.append(f"cppyy.{operation}: {type(error).__name__}")


def _record_operation(recorder: ProfileRecorder, name: str, callback) -> Any:
    """Time one explicit reflection or first-use callback and retain failures."""
    started = time.perf_counter_ns()
    failure = None
    try:
        return callback()
    except BaseException as error:
        failure = {"type": type(error).__name__, "message": str(error)}
        raise
    finally:
        recorder.operations.append(
            {"operation": name, "elapsed_ns": time.perf_counter_ns() - started, "error": failure}
        )


def _check(recorder: ProfileRecorder, checks: list[dict[str, Any]], name: str, callback) -> None:
    """Run a separately timed functional assertion and preserve its result."""
    started = time.perf_counter_ns()
    try:
        detail = callback()
        checks.append({"name": name, "passed": True, "detail": detail})
    except BaseException as error:
        checks.append(
            {
                "name": name,
                "passed": False,
                "error": {"type": type(error).__name__, "message": str(error)},
            }
        )
        raise
    finally:
        recorder.operations.append(
            {"operation": f"functional_check:{name}", "elapsed_ns": time.perf_counter_ns() - started}
        )


def _startup_checks(module: ModuleType, checks: list[dict[str, Any]], recorder: ProfileRecorder) -> None:
    """Verify inexpensive exported API behavior outside startup timing."""
    _check(
        recorder,
        checks,
        "public_namespaces",
        lambda: _assert_public_namespaces(module),
    )
    _check(recorder, checks, "systemc_version", lambda: _check_version(module))
    _check(recorder, checks, "native_time_conversion", lambda: _check_time(module))


def _assert_public_namespaces(module: ModuleType) -> str:
    """Check that the package exposes its primary SystemC namespaces."""
    assert hasattr(module, "sc_core")
    assert hasattr(module, "sc_dt")
    assert hasattr(module, "tlm")
    return "sc_core, sc_dt, tlm imported"


def _check_version(module: ModuleType) -> str:
    """Check native version dispatch using the established API."""
    version = str(module.sc_core.sc_version())
    assert version
    return version


def _check_time(module: ModuleType) -> str:
    """Check a small native time conversion without entering simulation."""
    value = module.sc_core.sc_time(1, module.sc_core.SC_NS).to_seconds()
    assert math.isclose(float(value), 1e-9, rel_tol=1e-12, abs_tol=0.0), value
    return f"1 ns -> {float(value):.12g} seconds"


def _run_reflection(module: ModuleType, recorder: ProfileRecorder, checks: list[dict[str, Any]]) -> None:
    """Time native namespace lookup and Python module directory reflection."""
    import cppyy

    native_namespaces = {
        "sc_core": cppyy.gbl.sc_core,
        "sc_dt": cppyy.gbl.sc_dt,
        "tlm": cppyy.gbl.tlm,
    }
    python_namespaces = {"sc_core": module.sc_core, "sc_dt": module.sc_dt, "tlm": module.tlm}
    for name, native in native_namespaces.items():
        values = _record_operation(recorder, f"native_dir:{name}", lambda native=native: dir(native))
        checks.append({"name": f"native_dir:{name}", "passed": bool(values), "count": len(values)})
        assert values
    for name, native, member in (
        ("sc_core", native_namespaces["sc_core"], "sc_module"),
        ("sc_dt", native_namespaces["sc_dt"], "sc_biguint"),
        ("tlm", native_namespaces["tlm"], "tlm_generic_payload"),
    ):
        value = _record_operation(recorder, f"native_lookup:{name}.{member}", lambda: getattr(native, member))
        checks.append({"name": f"native_lookup:{name}.{member}", "passed": value is not None})
        assert value is not None
    for name, namespace in python_namespaces.items():
        values = _record_operation(recorder, f"python_dir:{name}", namespace.__dir__)
        checks.append({"name": f"python_dir:{name}", "passed": bool(values), "count": len(values)})
        assert values


def _run_first_use(module: ModuleType, probe: str, recorder: ProfileRecorder, checks: list[dict[str, Any]]) -> None:
    """Resolve one native template specialization and check its identity."""
    targets = {
        "sc_biguint_257": (lambda: module.sc_dt.sc_biguint[257], "sc_dt::sc_biguint<257>"),
        "sc_signal_bool": (lambda: module.sc_core.sc_signal[bool], "sc_core::sc_signal<bool"),
        "tlm_socket_73": (lambda: module.tlm.tlm_initiator_socket[73], "tlm::tlm_initiator_socket<73"),
    }
    try:
        getter, expected_prefix = targets[probe]
    except KeyError as error:
        raise ValueError(f"unknown first-use probe: {probe}") from error
    specialization = _record_operation(recorder, f"first_use:{probe}", getter)
    cpp_name = getattr(specialization, "__cpp_name__", "")
    assert cpp_name.startswith(expected_prefix), (cpp_name, expected_prefix)
    same = getter()
    assert same is specialization
    checks.append({"name": f"template_specialization:{probe}", "passed": True, "cpp_name": cpp_name})


def child_main(package: str, case: str, probe: str | None) -> int:
    """Import the requested package under instrumentation and save child JSON."""
    cppyy_preloaded = "cppyy" in sys.modules
    package_preloaded = package in sys.modules
    recorder = ProfileRecorder()
    if cppyy_preloaded:
        recorder.instrumentation_errors.append("cppyy was imported before the timing hook")
    if package_preloaded:
        recorder.instrumentation_errors.append(f"{package} was imported before the timing hook")
    install_import_timing(recorder, package)
    checks: list[dict[str, Any]] = []
    child_error = None
    import_started = time.perf_counter_ns()
    import_elapsed = None
    try:
        module = importlib.import_module(package)
        import_elapsed = time.perf_counter_ns() - import_started
        _startup_checks(module, checks, recorder)
        if case == "reflection":
            _run_reflection(module, recorder, checks)
        elif case == "first-use":
            _run_first_use(module, probe or _DEFAULT_PROBES[0], recorder, checks)
    except BaseException as error:
        child_error = {"type": type(error).__name__, "message": str(error)}
        import_elapsed = import_elapsed or time.perf_counter_ns() - import_started
        import traceback

        traceback.print_exc()
    payload = {
        "case": case,
        "probe": probe,
        "package": package,
        "cppyy_preloaded_before_hook": cppyy_preloaded,
        "package_preloaded_before_hook": package_preloaded,
        "import_wall_ns": import_elapsed,
        "imports": recorder.imports,
        "cppyy_calls": recorder.cppyy_calls,
        "operations": recorder.operations,
        "checks": checks,
        "instrumentation_errors": recorder.instrumentation_errors,
        "error": child_error,
    }
    result_path = os.environ.get(_RESULT_ENV)
    if result_path:
        Path(result_path).write_text(json.dumps(payload, indent=2), encoding="utf-8")
    return 1 if child_error else 0


def _decode_output(value: str | bytes | None) -> str:
    """Normalize subprocess output while preserving captured partial output."""
    if value is None:
        return ""
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return value


def run_child(
    command: list[str],
    env: dict[str, str],
    timeout: float,
    result_path: Path,
    cwd: Path | None = None,
) -> dict[str, Any]:
    """Run one child and retain outputs, exit status, timeout, and profile JSON."""
    started = time.perf_counter_ns()
    timed_out = False
    try:
        completed = subprocess.run(
            command,
            env=env,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=timeout,
            check=False,
        )
        returncode = completed.returncode
        stdout = _decode_output(completed.stdout)
        stderr = _decode_output(completed.stderr)
    except subprocess.TimeoutExpired as error:
        timed_out = True
        returncode = None
        stdout = _decode_output(error.stdout)
        stderr = _decode_output(error.stderr)
    sample: dict[str, Any] = {
        "elapsed_ns": time.perf_counter_ns() - started,
        "returncode": returncode,
        "timed_out": timed_out,
        "stdout": stdout,
        "stderr": stderr,
        "profile": None,
    }
    if result_path.is_file():
        try:
            sample["profile"] = json.loads(result_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            sample["profile_error"] = f"{type(error).__name__}: {error}"
    return sample


def _environment_key_names() -> list[str]:
    """Return relevant variable names only, never values that may be sensitive."""
    return sorted(
        key
        for key in os.environ
        if key.startswith(("UV_", "CPPYY_", "CLING_"))
        or key
        in ("EXTRA_CLING_ARGS", "SYSTEMC_HOME", "SYSTEMC_SOURCE_DIR", "SYSTEMC_INSTALL_DIR", "SYSTEMC_LIBRARY_DIR")
    )


def _child_environment(result_path: Path) -> dict[str, str]:
    """Copy inherited settings unchanged and add only the private result path."""
    env = dict(os.environ)
    env[_RESULT_ENV] = str(result_path)
    return env


def run_profile(args: argparse.Namespace) -> dict[str, Any]:
    """Collect independent fresh-child samples and summarize top-level latency."""
    samples = []
    for index in range(args.repeat):
        with tempfile.TemporaryDirectory(prefix="pysystemc-profile-") as temporary:
            result_path = Path(temporary) / "profile.json"
            env = _child_environment(result_path)
            command = [
                sys.executable,
                str(Path(__file__).resolve()),
                "--_child",
                "--package",
                args.package,
                "--case",
                args.case,
            ]
            if args.probe:
                command.extend(("--probe", args.probe))
            sample = run_child(command, env, args.timeout, result_path, cwd=Path(temporary))
            sample["sample_index"] = index
            samples.append(sample)
    summary, accepted, failed = _summarize_samples(samples, args.case, args.probe, args.package)
    return {
        "schema_version": 1,
        "case": args.case,
        "probe": args.probe,
        "package": args.package,
        "repeat": args.repeat,
        "timeout_seconds": args.timeout,
        "python": {
            "version": sys.version.split()[0],
            "implementation": platform.python_implementation(),
            "executable_basename": Path(sys.executable).name,
        },
        "platform": sys.platform,
        "environment_key_names": _environment_key_names(),
        "timing_semantics": {
            "module_imports": "per-module exec_module durations are inclusive and nested; do not sum them",
            "cppyy_calls": "individual pass-through call durations can be nested in import/module intervals",
            "process_latency": "child elapsed includes process launch and interpreter startup; import_wall_ns does not",
            "environment": "inherited environment is copied unchanged except for a private child result-file variable; only relevant variable names are reported",
        },
        "summary": summary,
        "accepted_sample_indices": [sample["sample_index"] for sample in accepted],
        "failed_samples": failed,
        "samples": samples,
    }


def _sample_acceptance(sample: dict[str, Any], case: str, probe: str | None, package: str) -> tuple[bool, list[str]]:
    """Validate one cold-child sample before including its latency in statistics."""
    reasons = []
    if sample.get("returncode") != 0:
        reasons.append("child did not exit successfully")
    if sample.get("timed_out"):
        reasons.append("child timed out")
    profile = sample.get("profile")
    if not isinstance(profile, dict):
        reasons.append("profile payload missing or malformed")
        return False, reasons
    for key, expected in (("case", case), ("probe", probe), ("package", package)):
        if profile.get(key) != expected:
            reasons.append(f"profile {key} mismatch")
    if profile.get("error") is not None:
        reasons.append("child profile reports an error")
    if profile.get("instrumentation_errors"):
        reasons.append("instrumentation reported an error")
    if (
        profile.get("cppyy_preloaded_before_hook") is not False
        or profile.get("package_preloaded_before_hook") is not False
    ):
        reasons.append("measured module was already loaded before instrumentation")
    imports = profile.get("imports")
    if not isinstance(imports, list) or not imports:
        reasons.append("module import timings missing")
    else:
        imported_names = {event.get("module") for event in imports if isinstance(event, dict)}
        if "cppyy" not in imported_names or package not in imported_names:
            reasons.append("cppyy or package loader timing missing")
        if any(
            not isinstance(event, dict)
            or event.get("inclusive") is not True
            or isinstance(event.get("elapsed_ns"), bool)
            or not isinstance(event.get("elapsed_ns"), (int, float))
            or not math.isfinite(float(event.get("elapsed_ns")))
            or event.get("elapsed_ns") <= 0
            for event in imports
        ):
            reasons.append("one or more module import timings are invalid")
    if not isinstance(profile.get("cppyy_calls"), list) or not profile["cppyy_calls"]:
        reasons.append("cppyy call timings missing")
    checks = profile.get("checks")
    if not isinstance(checks, list) or not checks:
        reasons.append("functional checks missing")
    elif any(not isinstance(check, dict) or check.get("passed") is not True for check in checks):
        reasons.append("one or more functional checks failed")
    elapsed = profile.get("import_wall_ns")
    if (
        isinstance(elapsed, bool)
        or not isinstance(elapsed, (int, float))
        or not math.isfinite(float(elapsed))
        or elapsed <= 0
    ):
        reasons.append("import timing is not finite and positive")
    process_elapsed = sample.get("elapsed_ns")
    if (
        isinstance(process_elapsed, bool)
        or not isinstance(process_elapsed, (int, float))
        or not math.isfinite(float(process_elapsed))
        or process_elapsed <= 0
    ):
        reasons.append("child timing is not finite and positive")
    return not reasons, reasons


def _summarize_samples(
    samples: list[dict[str, Any]], case: str, probe: str | None, package: str
) -> tuple[dict[str, Any], list[dict[str, Any]], list[dict[str, Any]]]:
    """Summarize only accepted samples while retaining every rejected sample."""
    accepted = []
    failed = []
    for sample in samples:
        valid, reasons = _sample_acceptance(sample, case, probe, package)
        if valid:
            accepted.append(sample)
        else:
            failed.append({**sample, "reasons": reasons})
    timings = [sample["profile"]["import_wall_ns"] for sample in accepted]
    summary = {
        "successful_samples": len(accepted),
        "failed_samples": len(failed),
        "timed_out_samples": sum(1 for sample in samples if sample.get("timed_out")),
        "import_wall_median_ns": int(statistics.median(timings)) if timings else None,
        "import_wall_min_ns": min(timings) if timings else None,
        "import_wall_max_ns": max(timings) if timings else None,
    }
    return summary, accepted, failed


def _parser() -> argparse.ArgumentParser:
    """Build the startup-profiler command line."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=("startup", "reflection", "first-use"), default="startup")
    parser.add_argument("--probe", choices=_DEFAULT_PROBES)
    parser.add_argument("--package", default="amal.eda.systemc")
    parser.add_argument("--repeat", type=int, default=3)
    parser.add_argument("--timeout", type=float, default=180.0)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--_child", action="store_true", help=argparse.SUPPRESS)
    return parser


def main(argv: list[str] | None = None) -> int:
    """Run instrumented child samples and write structured JSON output."""
    parser = _parser()
    args = parser.parse_args(argv)
    if args.repeat < 1 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("--repeat and --timeout must be positive")
    if args.probe is not None and args.case != "first-use":
        parser.error("--probe is only valid with --case first-use")
    if args.case == "first-use" and args.probe is None:
        args.probe = _DEFAULT_PROBES[0]
    if args._child:
        return child_main(args.package, args.case, args.probe)
    if args.output is None:
        parser.error("--output is required")
    profile = run_profile(args)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(profile, indent=2), encoding="utf-8")
    failures = profile["summary"]["successful_samples"] != args.repeat
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())

"""Native-free tests for the isolated startup profiler instrumentation."""

from __future__ import annotations

import importlib.util
import json
import os
from pathlib import Path
import sys
from argparse import Namespace
from types import ModuleType


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "profile_startup.py"


def load_profiler() -> ModuleType:
    """Load the profiler module without importing cppyy or SystemC."""
    before = {name: sys.modules.get(name) for name in ("cppyy", "amal.eda.systemc")}
    spec = importlib.util.spec_from_file_location("profile_startup_under_test", TOOL)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    for name, previous in before.items():
        assert sys.modules.get(name) is previous
    return module


class FakeLoader:
    """Provide a fake module that records initialization order."""

    def __init__(self, name: str, events: list[str], *, fail: bool = False) -> None:
        self.name = name
        self.events = events
        self.fail = fail

    def create_module(self, spec):
        """Use importlib's ordinary module allocation."""
        return None

    def exec_module(self, module: ModuleType) -> None:
        """Initialize fake cppyy, package, or package child modules."""
        self.events.append(f"{self.name}:begin")
        if self.fail:
            raise RuntimeError(f"fake import failure: {self.name}")
        if self.name == "cppyy":

            def cppdef(source: str) -> bool:
                self.events.append(f"cppdef:{source}")
                return True

            module.cppdef = cppdef
        elif self.name == "fake_runtime":
            module.__path__ = []
            module.__dict__["_events"] = self.events
            exec(
                "def _initialize():\n"
                "    import importlib\n"
                "    cppyy = importlib.import_module('cppyy')\n"
                "    _events.append('fake_runtime:cppyy-ready')\n"
                "    cppyy.cppdef('setup')\n"
                "    importlib.import_module('fake_runtime.child')\n"
                "    _events.append('fake_runtime:done')\n",
                module.__dict__,
            )
            module._initialize()
        elif self.name == "fake_runtime.child":
            exec(
                "def _initialize():\n"
                "    import importlib\n"
                "    importlib.import_module('cppyy').cppdef('child')\n"
                "    import time\n"
                "    time.sleep(0.003)\n",
                module.__dict__,
            )
            module._initialize()
        self.events.append(f"{self.name}:end")


class FakeFinder:
    """Resolve a fixed fake module tree for loader-wrapper tests."""

    def __init__(self, events: list[str], *, fail_root: bool = False) -> None:
        self.events = events
        self.fail_root = fail_root
        self.find_counts: dict[str, int] = {}

    def find_spec(self, fullname, path=None, target=None):
        """Return fake specs for cppyy and the package plus one child."""
        if fullname not in ("cppyy", "fake_runtime", "fake_runtime.child"):
            return None
        self.find_counts[fullname] = self.find_counts.get(fullname, 0) + 1
        loader = FakeLoader(fullname, self.events, fail=self.fail_root and fullname == "fake_runtime")
        return importlib.util.spec_from_loader(fullname, loader, is_package=fullname == "fake_runtime")


def remove_fake_modules() -> None:
    """Remove synthetic imports so tests cannot affect one another."""
    for name in ("fake_runtime.child", "fake_runtime", "cppyy"):
        sys.modules.pop(name, None)


def test_loader_timing_preserves_order_once_and_nested_call_attribution() -> None:
    """Cppyy wrappers must be active before package setup and child calls."""
    profiler = load_profiler()
    recorder = profiler.ProfileRecorder()
    events: list[str] = []
    fake_finder = FakeFinder(events)
    timing_finder = profiler.install_import_timing(recorder, "fake_runtime")
    sys.meta_path.insert(1, fake_finder)
    try:
        import importlib

        importlib.import_module("fake_runtime")
        importlib.import_module("fake_runtime")
    finally:
        sys.meta_path.remove(timing_finder)
        sys.meta_path.remove(fake_finder)
        remove_fake_modules()

    assert events == [
        "fake_runtime:begin",
        "cppyy:begin",
        "cppyy:end",
        "fake_runtime:cppyy-ready",
        "cppdef:setup",
        "fake_runtime.child:begin",
        "cppdef:child",
        "fake_runtime.child:end",
        "fake_runtime:done",
        "fake_runtime:end",
    ]
    assert fake_finder.find_counts == {"fake_runtime": 1, "cppyy": 1, "fake_runtime.child": 1}
    assert [event["module"] for event in recorder.imports] == [
        "cppyy",
        "fake_runtime.child",
        "fake_runtime",
    ]
    assert all(event["inclusive"] for event in recorder.imports)
    assert recorder.imports[-1]["elapsed_ns"] >= recorder.imports[1]["elapsed_ns"]
    assert [event["source_sha256"] for event in recorder.cppyy_calls]
    assert [event["caller_module"] for event in recorder.cppyy_calls] == [
        "fake_runtime",
        "fake_runtime.child",
    ]


def test_failed_import_is_recorded_and_original_exception_propagates() -> None:
    """Loader errors retain timing evidence without being swallowed."""
    profiler = load_profiler()
    recorder = profiler.ProfileRecorder()
    events: list[str] = []
    fake_finder = FakeFinder(events, fail_root=True)
    timing_finder = profiler.install_import_timing(recorder, "fake_runtime")
    sys.meta_path.insert(1, fake_finder)
    try:
        import importlib

        try:
            importlib.import_module("fake_runtime")
        except RuntimeError as error:
            assert str(error) == "fake import failure: fake_runtime"
        else:
            raise AssertionError("fake import exception did not propagate")
    finally:
        sys.meta_path.remove(timing_finder)
        sys.meta_path.remove(fake_finder)
        remove_fake_modules()

    assert len(recorder.imports) == 1
    assert recorder.imports[0]["module"] == "fake_runtime"
    assert recorder.imports[0]["error"] == {
        "type": "RuntimeError",
        "message": "fake import failure: fake_runtime",
    }


def test_child_runner_retains_nonzero_output_and_timeout_output(tmp_path, monkeypatch) -> None:
    """Failure and timeout results keep all output captured before exit/kill."""
    profiler = load_profiler()
    result_file = tmp_path / "no-result.json"
    failed = profiler.run_child(
        [sys.executable, "-c", "import sys; print('out'); print('err', file=sys.stderr); sys.exit(7)"],
        dict(os.environ),
        2.0,
        result_file,
    )
    assert failed["returncode"] == 7
    assert not failed["timed_out"]
    assert failed["stdout"] == "out\n"
    assert "err" in failed["stderr"]

    def timeout_with_output(command, **kwargs):
        """Supply captured bytes without depending on interpreter startup latency."""
        raise profiler.subprocess.TimeoutExpired(
            command,
            kwargs["timeout"],
            output=b"before-timeout\n",
            stderr=b"stderr-before-timeout\n",
        )

    monkeypatch.setattr(profiler.subprocess, "run", timeout_with_output)
    timed_out = profiler.run_child([sys.executable, "-c", "pass"], dict(os.environ), 0.1, result_file)
    assert timed_out["timed_out"]
    assert timed_out["returncode"] is None
    assert "before-timeout" in timed_out["stdout"]
    assert "stderr-before-timeout" in timed_out["stderr"]


def test_child_runner_times_out_real_subprocess(tmp_path) -> None:
    """A real sleeping child times out even when startup produces no output."""
    profiler = load_profiler()
    timed_out = profiler.run_child(
        [sys.executable, "-c", "import time; time.sleep(30)"],
        dict(os.environ),
        0.1,
        tmp_path / "no-result.json",
    )
    assert timed_out["timed_out"]
    assert timed_out["returncode"] is None
    assert timed_out["stdout"] == ""
    assert timed_out["stderr"] == ""
    assert timed_out["profile"] is None


def _write_fake_package(root: Path) -> None:
    """Create a tiny package that exercises the child harness without native code."""
    package = root / "fake_profile_package"
    package.mkdir()
    (root / "cppyy.py").write_text(
        "def load_library(value): return value\n"
        "def add_library_path(value): return value\n"
        "def add_include_path(value): return value\n"
        "def include(value): return value\n"
        "def cppdef(value): return True\n",
        encoding="utf-8",
    )
    (package / "__init__.py").write_text(
        "import cppyy\n"
        "cppyy.load_library('fake-systemc.so')\n"
        "class _Time:\n"
        "    def to_seconds(self): return 1e-9\n"
        "class _Core:\n"
        "    SC_NS = 1\n"
        "    def sc_version(self): return 'fake 1.0'\n"
        "    def sc_time(self, value, unit): return _Time()\n"
        "sc_core = _Core()\n"
        "sc_dt = object()\n"
        "tlm = object()\n",
        encoding="utf-8",
    )


def test_summary_accepts_only_valid_instrumented_fake_children(tmp_path, monkeypatch) -> None:
    """A real fake child is accepted; bad profiles and failed timings are excluded."""
    profiler = load_profiler()
    fake_root = tmp_path / "fake-site"
    fake_root.mkdir()
    _write_fake_package(fake_root)
    inherited_pythonpath = os.environ.get("PYTHONPATH", "")
    monkeypatch.setenv("PYTHONPATH", os.pathsep.join(filter(None, (str(fake_root), inherited_pythonpath))))
    monkeypatch.setenv("UV_PROJECT_ENVIRONMENT", "uv-sensitive-value")
    monkeypatch.setenv("CPPYY_API_PATH", "cppyy-sensitive-value")
    args = Namespace(
        case="startup",
        probe=None,
        package="fake_profile_package",
        repeat=1,
        timeout=10.0,
        output=tmp_path / "unused.json",
    )
    result = profiler.run_profile(args)
    sample = result["samples"][0]
    assert result["summary"]["successful_samples"] == 1
    assert result["summary"]["failed_samples"] == 0
    assert result["summary"]["import_wall_median_ns"] == sample["profile"]["import_wall_ns"]
    accepted, reasons = profiler._sample_acceptance(sample, "startup", None, "fake_profile_package")
    assert accepted, reasons

    bad_exit = {**sample, "returncode": 9, "sample_index": 1}
    bad_instrumentation = {
        **sample,
        "sample_index": 2,
        "profile": {**sample["profile"], "instrumentation_errors": ["hook failure"]},
    }
    malformed = {**sample, "sample_index": 3, "profile": {"case": "startup"}}
    mismatched = {
        **sample,
        "sample_index": 4,
        "profile": {**sample["profile"], "package": "other_package"},
    }
    missing = {**sample, "sample_index": 5, "profile": None, "stdout": "retained output"}
    summary, accepted_samples, failed_samples = profiler._summarize_samples(
        [bad_exit, bad_instrumentation, malformed, mismatched, missing],
        "startup",
        None,
        "fake_profile_package",
    )
    assert not accepted_samples
    assert summary["successful_samples"] == 0
    assert summary["import_wall_median_ns"] is None
    assert len(failed_samples) == 5
    assert failed_samples[-1]["stdout"] == "retained output"
    assert all("reasons" in item for item in failed_samples)

    mixed_summary, accepted_samples, failed_samples = profiler._summarize_samples(
        [sample, bad_exit], "startup", None, "fake_profile_package"
    )
    assert mixed_summary["successful_samples"] == 1
    assert mixed_summary["import_wall_median_ns"] == sample["profile"]["import_wall_ns"]
    serialized = json.dumps(result)
    assert "uv-sensitive-value" not in serialized
    assert "cppyy-sensitive-value" not in serialized
    assert result["environment_key_names"] and "UV_PROJECT_ENVIRONMENT" in result["environment_key_names"]

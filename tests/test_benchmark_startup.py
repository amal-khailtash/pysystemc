"""Verify startup benchmark result handling without loading native bindings."""

import json
import subprocess

import pytest

from tools import benchmark_startup


def _sample_payload(scenario: str = "interpreter-baseline") -> str:
    """Return one structured child record for subprocess harness tests."""
    payload = {
        "scenario": scenario,
        "ok": True,
        "check": "interpreter initialized",
        "versions": {},
        "phases_seconds": {"interpreter_ready_seconds": 0.001},
        "scenario_configuration": {},
        "peak_rss_bytes": 2048,
        "peak_rss_method": "test",
    }
    return benchmark_startup.MARKER + json.dumps(payload) + "\n"


def test_parse_child_output_requires_matching_checked_scenario() -> None:
    """Reject child output without a valid matching success record."""
    record = benchmark_startup._parse_child_output(_sample_payload(), "interpreter-baseline")
    assert record["peak_rss_bytes"] == 2048

    with pytest.raises(benchmark_startup.BenchmarkError, match="omitted its structured result"):
        benchmark_startup._parse_child_output("workload output only\n", "interpreter-baseline")
    with pytest.raises(benchmark_startup.BenchmarkError, match="matching scenario"):
        benchmark_startup._parse_child_output(_sample_payload("cppyy-import"), "interpreter-baseline")


def test_every_generated_child_program_compiles() -> None:
    """Catch syntax errors in inline workloads without running native imports."""
    for scenario in benchmark_startup.SCENARIOS:
        compile(benchmark_startup._child_source(scenario), f"<{scenario}>", "exec")


def test_run_once_preserves_environment_and_records_child_output(monkeypatch: pytest.MonkeyPatch) -> None:
    """Measure a valid fresh child result without replacing inherited env."""
    observed: dict[str, object] = {}

    def fake_run(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
        observed["command"] = command
        observed.update(kwargs)
        assert benchmark_startup.Path(str(kwargs["cwd"])).is_dir()
        return subprocess.CompletedProcess(command, 0, _sample_payload(), "")

    monkeypatch.setattr(benchmark_startup.subprocess, "run", fake_run)
    sample = benchmark_startup._run_once("interpreter-baseline", 4.0)

    assert "env" not in observed
    assert observed["timeout"] == 4.0
    assert sample["output_check"] == "interpreter initialized"
    assert sample["peak_rss_bytes"] == 2048
    assert sample["wall_seconds"] >= 0


def test_run_once_reports_timeout_and_child_failure(monkeypatch: pytest.MonkeyPatch) -> None:
    """Convert timeout and nonzero child status into actionable failures."""

    def timeout(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
        del kwargs
        raise subprocess.TimeoutExpired(command, 0.01)

    monkeypatch.setattr(benchmark_startup.subprocess, "run", timeout)
    with pytest.raises(benchmark_startup.BenchmarkError, match="exceeded 0.01s timeout"):
        benchmark_startup._run_once("interpreter-baseline", 0.01)

    def failed(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
        del kwargs
        return subprocess.CompletedProcess(command, 2, "", "scenario assertion failed")

    monkeypatch.setattr(benchmark_startup.subprocess, "run", failed)
    with pytest.raises(benchmark_startup.BenchmarkError, match="exited 2: scenario assertion failed"):
        benchmark_startup._run_once("interpreter-baseline", 1.0)


def test_run_benchmark_keeps_failures_and_continues(monkeypatch: pytest.MonkeyPatch) -> None:
    """Retain a scenario failure while still collecting later scenarios."""
    attempted: list[str] = []

    def fake_run_once(scenario: str, timeout_seconds: float) -> dict[str, object]:
        del timeout_seconds
        attempted.append(scenario)
        if scenario == "cppyy-import":
            raise benchmark_startup.BenchmarkError("cppyy PCH initialization failed")
        return {
            "wall_seconds": 0.25,
            "peak_rss_bytes": 1024,
            "peak_rss_method": "test",
            "phases_seconds": {"interpreter_ready_seconds": 0.01},
        }

    monkeypatch.setattr(benchmark_startup, "_run_once", fake_run_once)
    result = benchmark_startup.run_benchmark(["interpreter-baseline", "cppyy-import"], 1, 2.0)

    assert attempted == ["interpreter-baseline", "interpreter-baseline", "cppyy-import"]
    assert result["scenarios"]["interpreter-baseline"]["status"] == "passed"
    cppyy_result = result["scenarios"]["cppyy-import"]
    assert cppyy_result["status"] == "failed"
    assert cppyy_result["failed_run_count"] == 1
    assert cppyy_result["skipped_run_count"] == 1
    assert cppyy_result["initial_run"]["failure"] == "cppyy PCH initialization failed"
    assert cppyy_result["samples"] == [{"status": "skipped", "reason": "initial run failed; repeated attempts skipped"}]
    assert cppyy_result["summary"]["successful_sample_count"] == 0


def test_main_writes_report_and_returns_failure_status(tmp_path, monkeypatch: pytest.MonkeyPatch) -> None:
    """Write complete scenario results before returning nonzero for failures."""
    report_path = tmp_path / "missing" / "startup.json"
    monkeypatch.setattr(
        benchmark_startup,
        "run_benchmark",
        lambda scenarios, repetitions, timeout: {
            "scenarios": {"cppyy-import": {"status": "failed"}},
            "attempt": {"scenarios": scenarios, "repetitions": repetitions, "timeout": timeout},
        },
    )

    status = benchmark_startup.main(["--scenario", "cppyy-import", "--repetitions", "1", "--output", str(report_path)])

    assert status == 1
    assert json.loads(report_path.read_text())["scenarios"]["cppyy-import"]["status"] == "failed"


def test_cli_scenario_requires_expected_text_output(monkeypatch: pytest.MonkeyPatch) -> None:
    """Ensure successful CLI process exit also includes the expected text."""

    def fake_run(command: list[str], **kwargs: object) -> subprocess.CompletedProcess[str]:
        del kwargs
        marker = _sample_payload("systemc-py-help")
        return subprocess.CompletedProcess(command, 0, marker, "")

    monkeypatch.setattr(benchmark_startup.subprocess, "run", fake_run)
    with pytest.raises(benchmark_startup.BenchmarkError, match="expected output .* was absent"):
        benchmark_startup._run_once("systemc-py-help", 1.0)


def test_summary_reports_median_and_spread() -> None:
    """Summarize timing and normalized RSS across measured samples."""
    samples = [
        {"wall_seconds": 1.0, "peak_rss_bytes": 1024, "phases_seconds": {"use": 0.1}},
        {"wall_seconds": 2.0, "peak_rss_bytes": 2048, "phases_seconds": {"use": 0.2}},
        {"wall_seconds": 4.0, "peak_rss_bytes": 4096, "phases_seconds": {"use": 0.6}},
    ]

    summary = benchmark_startup._summary(samples)

    assert summary["median_wall_seconds"] == 2.0
    assert summary["wall_spread_seconds"] == 3.0
    assert summary["median_peak_rss_bytes"] == 2048
    assert summary["min_peak_rss_bytes"] == 1024
    assert summary["max_peak_rss_bytes"] == 4096
    assert summary["phase_summary"]["use"] == {
        "median_seconds": 0.2,
        "min_seconds": 0.1,
        "max_seconds": 0.6,
        "spread_seconds": 0.5,
    }

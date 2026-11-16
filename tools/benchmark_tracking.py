"""Measure actual verification-recorder on/off cost in ten fixed TLM pairs.

This measures whole child invocation cost, including an extra recorder interpreter,
log capture and ignored local ledger/plan synchronization. It does not isolate the
recorder's Python statements or measure profiling, coverage, or VCD overhead.
"""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import sys
from time import perf_counter

ROOT = Path(__file__).resolve().parents[1]
HERE = ROOT / "build/performance/tracking-overhead"
MARKER = "__END_TO_END__"
PAIRS = 10


def source_hashes() -> dict[str, str]:
    """Return all tracked runtime hashes to guard the frozen measurement source."""
    paths = subprocess.check_output(["git", "ls-files", "src"], cwd=ROOT, text=True).splitlines()
    return {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in paths}


def infrastructure_hashes() -> dict[str, str]:
    """Guard workload and recorder code independently of runtime source files."""
    return {
        name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
        for name in ("tools/benchmark_end_to_end.py", ".ai/record_verification_run.py", ".ai/sync_verification_plan.py")
    }


def run_sample(role: str, pair: int, hashes: dict[str, str], infrastructure: dict[str, str]) -> dict:
    """Run one fresh checked simulation, with a 60-second whole-child deadline.

    Args:
        role: ``off`` for direct execution or ``on`` for the actual recorder.
        pair: Fixed alternating pair index, zero through nine.
        hashes: Frozen current runtime source hashes.
        infrastructure: Frozen workload and recorder code hashes.
    Returns:
        Checked simulation result, whole invocation wall time and relative logs.
    Raises:
        AssertionError: Source, native behavior or output markers disagree.
        subprocess.TimeoutExpired: The fresh child exceeds its deadline.
    """
    assert source_hashes() == hashes and infrastructure_hashes() == infrastructure
    log = HERE / f"{pair:02d}-{role}.log"
    command = [
        sys.executable,
        str(ROOT / "tools/benchmark_end_to_end.py"),
        "--child",
        "--source",
        str(ROOT),
        "--workload",
        "tlm_transport",
        "--count",
        "20000",
        "--engine",
        "python",
    ]
    environment = dict(os.environ)
    if role == "on":
        environment["PYSYSTEMC_VERIFICATION_TIMEOUT_SECONDS"] = "60"
        command = [
            sys.executable,
            str(ROOT / ".ai/record_verification_run.py"),
            f"tracking-on-{pair:02d}",
            log.relative_to(ROOT).as_posix(),
            "--",
            *command,
        ]
        started = perf_counter()
        process = subprocess.run(command, cwd=ROOT, env=environment, capture_output=True, text=True, timeout=60)
        wall = perf_counter() - started
        (HERE / f"{pair:02d}-recorder.log").write_text(process.stdout + "\n" + process.stderr)
        output = log.read_text()
    else:
        started = perf_counter()
        process = subprocess.run(command, cwd=ROOT, env=environment, capture_output=True, text=True, timeout=60)
        wall = perf_counter() - started
        output = process.stdout + "\n" + process.stderr
        log.write_text(output)
    assert process.returncode == 0, log
    payload = [json.loads(line[len(MARKER) :]) for line in output.splitlines() if line.startswith(MARKER)]
    assert len(payload) == 1, log
    result = payload[0]
    assert result["python"] == "3.14.0"
    assert result["workload"] == "tlm_transport" and result["engine"] == "python"
    assert result["behavior"] == {
        "callbacks": 20000,
        "responses": 20000,
        "checksum": 600110000,
        "last_address": 60004,
        "annotated_ns": 20000,
        "time_ns": 0,
    }
    assert source_hashes() == hashes and infrastructure_hashes() == infrastructure
    return {"pair": pair, "role": role, "wall_seconds": wall, "log": log.relative_to(ROOT).as_posix(), **result}


def update_progress(pairs: int) -> None:
    """Update the public batch measurement count outside all sample timing."""
    path = ROOT / "docs/_static/todo-batch-progress.json"
    if not path.exists():
        return
    value = json.loads(path.read_text())
    item = value["items"]["tracking"]
    item["status"] = "running"
    item["detail"] = f"Actual recorder on/off: {pairs}/10 alternating pairs retained; no resampling."
    path.write_text(json.dumps(value, indent=2) + "\n")
    page = ROOT / "docs/development/todo-batch-progress.md"
    lines = page.read_text().splitlines()
    lines = [
        f"| tracking | running | {item['detail']} |" if line.startswith("| tracking |") else line for line in lines
    ]
    page.write_text("\n".join(lines) + "\n")


def main() -> None:
    """Run ten pairs once, retaining each sample immediately; never resample."""
    HERE.mkdir(parents=True, exist_ok=True)
    report = HERE / "results.json"
    partial = HERE / "partial.json"
    if report.exists() or partial.exists():
        raise RuntimeError("Fixed tracking round already started; do not resample")
    hashes = source_hashes()
    infrastructure = infrastructure_hashes()
    recorder_source_sha256 = hashlib.sha256((ROOT / ".ai/record_verification_run.py").read_bytes()).hexdigest()
    records_path = ROOT / ".ai/VERIFICATION_RUNS.json"
    initial_records = json.loads(records_path.read_text())
    initial_ledger_bytes = records_path.stat().st_size
    initial_ledger_sha256 = hashlib.sha256(records_path.read_bytes()).hexdigest()
    sync_source_sha256 = hashlib.sha256((ROOT / ".ai/sync_verification_plan.py").read_bytes()).hexdigest()
    samples = []
    for pair in range(PAIRS):
        for role in ("off", "on") if pair % 2 == 0 else ("on", "off"):
            samples.append(run_sample(role, pair, hashes, infrastructure))
            partial.write_text(json.dumps(samples, indent=2) + "\n")
        assert samples[-1]["native_library_sha256"] == samples[-2]["native_library_sha256"]
        update_progress(pair + 1)
        print(f"tracking {pair + 1}/{PAIRS} pairs", flush=True)
    medians = {
        role: statistics.median(s["wall_seconds"] for s in samples if s["role"] == role) for role in ("off", "on")
    }
    loop_medians = {
        role: statistics.median(s["run_seconds"] for s in samples if s["role"] == role) for role in ("off", "on")
    }
    deltas = [
        next(s["wall_seconds"] for s in samples if s["role"] == "on" and s["pair"] == p)
        - next(s["wall_seconds"] for s in samples if s["role"] == "off" and s["pair"] == p)
        for p in range(PAIRS)
    ]
    final_records = json.loads(records_path.read_text())
    added_records = final_records[len(initial_records) :]
    assert [row["label"] for row in added_records] == [f"tracking-on-{p:02d}" for p in range(PAIRS)]
    assert all(row["status"] == "passed" for row in added_records)
    assert len({s["native_library_sha256"] for s in samples}) == 1
    assert hashlib.sha256((ROOT / ".ai/record_verification_run.py").read_bytes()).hexdigest() == recorder_source_sha256
    result = {
        "schema": "pysystemc-tracking-overhead-v1",
        "status": "measured",
        "pairs": PAIRS,
        "source_hashes": hashes,
        "infrastructure_hashes": infrastructure,
        "samples": samples,
        "recorder_source_sha256": recorder_source_sha256,
        "recorder_rows_appended": len(added_records),
        "initial_ledger_records": len(initial_records),
        "initial_ledger_bytes": initial_ledger_bytes,
        "initial_ledger_sha256": initial_ledger_sha256,
        "final_ledger_bytes": records_path.stat().st_size,
        "sync_source_sha256": sync_source_sha256,
        "recorder_source": (ROOT / ".ai/record_verification_run.py").read_text(),
        "wall_medians": medians,
        "loop_medians": loop_medians,
        "paired_wall_deltas_seconds": deltas,
        "wall_overhead_percent": 100 * (medians["on"] / medians["off"] - 1),
        "off_reduction_percent": 100 * (1 - medians["off"] / medians["on"]),
        "on_slower_pairs": sum(d > 0 for d in deltas),
        "tracking": "Actual .ai/record_verification_run.py: extra interpreter, launch/log capture, ledger and plan synchronization.",
        "scope": "Whole fresh Python TLM invocation; original 20000-transfer loop also retained. Profiling, coverage and VCD off.",
        "limits": "Sequential alternating samples; no confidence interval or isolated bookkeeping-statement attribution. W509 retained in both roles. No resampling or optimization.",
        "child_timeout_seconds": 60,
    }
    report.write_text(json.dumps(result, indent=2) + "\n")
    print(
        json.dumps(
            {
                k: result[k]
                for k in ("pairs", "wall_medians", "loop_medians", "wall_overhead_percent", "on_slower_pairs")
            }
        )
    )


if __name__ == "__main__":
    main()

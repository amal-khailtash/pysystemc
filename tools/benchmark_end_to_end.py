"""Measure three fixed workloads across frozen revisions in isolated processes.

The ten alternating pairs run with VCD, coverage and profiling disabled. Native
reference models check selected behavior using the same Python stimulus. This
measures revision effects on these workloads, not a universal package gain.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import statistics
import subprocess
import sys
import tarfile
import tempfile
from time import perf_counter

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "291d569cc6bf396bed2f89c019adb9452d2b9580"
CURRENT = "fc53a971a6d6db0c01db5986affa4ae1d8953212"
PAIRS = 10
CYCLES = 20000
TRANSFERS = 20000
MARKER = "__END_TO_END__"
GATES = ["contract", "equivalence", "import", "event_signal", "tlm_transport", "report", "validation_commit"]
PROGRESS = ROOT / "docs/_static/performance-end-to-end-progress.json"
REPORT = ROOT / "docs/_static/performance-end-to-end.json"


def save(path: Path, value: dict) -> None:
    """Atomically write a portable JSON report, creating its parent directory."""
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n")
    temporary.replace(path)
    if path == PROGRESS:
        render_progress(value)


def render_progress(value: dict) -> None:
    """Render the live seven-gate checklist for direct reading in the repository."""
    lines = [
        "# End-to-end performance progress",
        "",
        f"Status: **{value['status']}**.",
        "",
        f"Measured pairs: **{value['completed_measurement_pairs']}/{value['total_measurement_pairs']}**.",
        "",
        "| Gate | Status | Detail |",
        "|---|---|---|",
    ]
    for name, gate in value["gates"].items():
        detail = gate.get("error", gate.get("checks", gate.get("report", "")))
        if "pairs_completed" in gate:
            detail = f"{gate['pairs_completed']}/{gate['pairs_total']} pairs"
        lines.append(f"| {name} | {gate['status']} | {detail} |")
    lines += [
        "",
        "Baseline: `" + value["baseline_commit"] + "`. Current: `" + value["current_commit"] + "`.",
        "",
        "Fixed scope: import, event/signal model, annotated TLM transport; ten alternating pairs each.",
        "20,000 cycles/transfers. VCD, coverage and profiling off. Same Python/native dependencies.",
        "60 seconds per subprocess; 30-minute round deadline. A failed gate stops the round.",
        "No extra workloads, resampling, optimization search or percentage summation.",
        "",
        "Machine-readable progress: `../_static/performance-end-to-end-progress.json`.",
        "Results: `../_static/performance-end-to-end.json` (published after measurement).",
        "",
    ]
    (ROOT / "docs/development/performance-end-to-end-progress.md").write_text("\n".join(lines))


def contract() -> dict:
    """Return the frozen workload counts and finite gate statuses."""
    return {
        "schema": "pysystemc-end-to-end-progress-v1",
        "status": "running",
        "baseline_commit": BASELINE,
        "current_commit": CURRENT,
        "pairs_per_workload": PAIRS,
        "cycles": CYCLES,
        "transfers": TRANSFERS,
        "subprocess_timeout_seconds": 60,
        "measurement_deadline_seconds": 1800,
        "no_resampling": True,
        "tracking": "VCD off; coverage off; profiling off; recorder and validation outside phase timing",
        "gates": {name: {"status": "pending"} for name in GATES},
        "completed_measurement_pairs": 0,
        "total_measurement_pairs": 30,
        "stop_rule": "One fixed round. Stop on timeout or behavior mismatch; publish failures. No optimization or extra workload.",
    }


def child(source: Path, workload: str, count: int, engine: str) -> dict:
    """Run one fresh kernel and check values, callbacks and native time.

    Args:
        source: Frozen revision root containing ``src`` and shared native resources.
        workload: Import, event/signal or annotated blocking transport workload.
        count: Positive number of cycles or transfers; ignored for import.
        engine: Python model or native C++ behavioral reference.

    Returns:
        Import, elaboration and run times with checked semantic results.

    Raises:
        AssertionError: Import identity, instrumentation or model results disagree.

    Call only once in a subprocess. Modules, sockets, callbacks and payloads stay
    alive through checks and process exit. Native/Python exceptions propagate.
    """
    assert sys.version_info[:2] == (3, 14), sys.version
    assert sys.gettrace() is None and sys.getprofile() is None
    assert not os.environ.get("COVERAGE_PROCESS_START"), "Coverage auto-start is enabled"
    sys.path.insert(0, str(source / "src"))
    started = perf_counter()
    from amal.eda.systemc import sc_core as sc, tlm, tlm_utils
    import cppyy
    import amal.eda.systemc._runtime as runtime

    imported = perf_counter()
    for name, module in tuple(sys.modules.items()):
        if name == "amal" or name.startswith("amal."):
            file = getattr(module, "__file__", None)
            if file:
                assert Path(file).resolve().is_relative_to((source / "src").resolve()), (name, file)
    result = {
        "workload": workload,
        "engine": engine,
        "import_seconds": imported - started,
        "cppyy": cppyy.__version__,
        "python": sys.version.split()[0],
        "native_library_sha256": hashlib.sha256(
            (Path(runtime.SYSTEMC_LIBRARY_DIR) / "libsystemc.so").read_bytes()
        ).hexdigest(),
    }
    elaboration_started = perf_counter()
    if workload == "import":
        result.update(elaboration_seconds=0.0, run_seconds=0.0, behavior={"imported": True})
        return result
    if workload == "event_signal":
        if engine == "native":
            cppyy.cppdef(r"""
namespace end_to_end_reference
{
/** Child-owned counter; callbacks need no Python references or GIL. */
struct Counter : sc_core::sc_module
{
  sc_core::sc_in<bool> clock{"clock"};
  sc_core::sc_out<unsigned int> output{"output"};
  unsigned int calls = 0;
  SC_HAS_PROCESS(Counter);
  /** Bind ports before starting; native report exceptions propagate. */
  explicit Counter(sc_core::sc_module_name name) : sc_core::sc_module(name)
  {
    SC_METHOD(tick);
    sensitive << clock.pos();
    dont_initialize();
  }
  /** Write the next value on a rising edge; port storage is borrowed. */
  void tick()
  {
    output.write(++calls % 65536);
  }
};
}  // namespace end_to_end_reference
""")
            model = cppyy.gbl.end_to_end_reference.Counter("counter")
        else:

            class Counter(sc.sc_module):
                """Own a method callback and borrowed signal ports until exit."""

                def __init__(self):
                    """Register a rising-edge counter without initialization."""
                    name = sc.sc_module_name("counter")
                    super().__init__(name)
                    self.clock = sc.sc_in[bool]("clock")
                    self.output = sc.sc_out["unsigned int"]("output")
                    self.calls = 0
                    self.sc_method("tick", self.tick)
                    self.sensitive(self.clock.pos())
                    self.dont_initialize()

                def tick(self):
                    """Update the count and schedule its signal value."""
                    self.calls += 1
                    self.output.write(self.calls % 65536)

            model = Counter()
        clock = sc.sc_signal[bool]("clock")
        output = sc.sc_signal["unsigned int"]("output")
        model.clock(clock)
        model.output(output)
        sc.sc_start(sc.SC_ZERO_TIME)
        run_started = perf_counter()
        checksum = 0
        for _ in range(count):
            clock.write(True)
            sc.sc_start(1, sc.SC_NS)
            checksum += int(output.read())
            clock.write(False)
            sc.sc_start(1, sc.SC_NS)
        ended = perf_counter()
        expected = sum(i % 65536 for i in range(1, count + 1))
        assert int(model.calls) == count
        assert int(output.read()) == count % 65536
        assert checksum == expected
        assert sc.sc_time_stamp() == sc.sc_time(2 * count, sc.SC_NS)
        behavior = {"callbacks": count, "value": count % 65536, "checksum": checksum, "time_ns": 2 * count}
    else:
        name = sc.sc_module_name("top")
        _top = sc.sc_module(name)
        initiator = tlm_utils.callback_initiator_socket("initiator")
        if engine == "native":
            cppyy.cppdef(r"""
namespace end_to_end_reference
{
/** Child-owned native target; caller owns payload and annotation storage. */
struct Target : sc_core::sc_module
{
  tlm_utils::simple_target_socket<Target> socket{"socket"};
  unsigned int calls = 0;
  /** Register transport before binding; native exceptions propagate. */
  explicit Target(sc_core::sc_module_name name) : sc_core::sc_module(name)
  {
    socket.register_b_transport(this, &Target::transport);
  }
  /** Mutate borrowed state without waiting; callback needs no Python GIL. */
  void transport(tlm::tlm_generic_payload& payload, sc_core::sc_time& delay)
  {
    ++calls;
    payload.set_address((payload.get_address() * 3 + 7) % 65536);
    payload.set_response_status(tlm::TLM_OK_RESPONSE);
    delay += sc_core::sc_time(1, sc_core::SC_NS);
  }
};
}  // namespace end_to_end_reference
""")
            target = cppyy.gbl.end_to_end_reference.Target("target")
            socket = target.socket
        else:

            class Target:
                """Retain a nonwaiting Python callback through socket lifetime."""

                def __init__(self):
                    """Initialize the observed callback count."""
                    self.calls = 0

                def transport(self, payload, delay):
                    """Transform a borrowed address and annotate one nanosecond."""
                    self.calls += 1
                    payload.set_address((payload.get_address() * 3 + 7) % 65536)
                    payload.set_response_status(tlm.TLM_OK_RESPONSE)
                    delay.__assign__(delay + sc.sc_time(1, sc.SC_NS))

            target = Target()
            socket = tlm_utils.callback_target_socket("target")
            socket.register_b_transport(target.transport)
        initiator.bind(socket)
        interface = initiator.get_interface()
        payload = tlm.tlm_generic_payload()
        payload.set_command(tlm.TLM_WRITE_COMMAND)
        payload.set_data_length(0)
        payload.set_streaming_width(0)
        delay = sc.sc_time(0, sc.SC_NS)
        sc.sc_start(sc.SC_ZERO_TIME)
        run_started = perf_counter()
        checksum = 0
        responses = 0
        for index in range(count):
            payload.set_address(index)
            payload.set_response_status(tlm.TLM_INCOMPLETE_RESPONSE)
            interface.b_transport(payload, delay)
            checksum += int(payload.get_address())
            responses += int(payload.is_response_ok())
        ended = perf_counter()
        assert int(target.calls) == count
        assert responses == count
        assert checksum == sum((i * 3 + 7) % 65536 for i in range(count))
        assert int(payload.get_address()) == ((count - 1) * 3 + 7) % 65536
        assert delay == sc.sc_time(count, sc.SC_NS)
        assert sc.sc_time_stamp() == sc.SC_ZERO_TIME
        behavior = {
            "callbacks": count,
            "responses": responses,
            "checksum": checksum,
            "last_address": int(payload.get_address()),
            "annotated_ns": count,
            "time_ns": 0,
        }
    result.update(
        elaboration_seconds=run_started - elaboration_started, run_seconds=ended - run_started, behavior=behavior
    )
    return result


def snapshot(revision: str, destination: Path) -> dict:
    """Extract tracked source without modifying Git or the selected environment."""
    archive = subprocess.check_output(["git", "archive", revision, "src"], cwd=ROOT)
    destination.mkdir(parents=True, exist_ok=True)
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        tar.extractall(destination, filter="data")
    for name in ("build", "submodules"):
        (destination / name).symlink_to(ROOT / name, target_is_directory=True)
    paths = subprocess.check_output(
        ["git", "ls-tree", "-r", "--name-only", revision, "src"], cwd=ROOT, text=True
    ).splitlines()
    hashes = {path: hashlib.sha256((destination / path).read_bytes()).hexdigest() for path in paths}
    return {"revision": revision, "tracked_files": len(hashes), "source_hashes": hashes}


def sample(source: Path, workload: str, count: int, engine: str, log: Path) -> dict:
    """Run an uninstrumented child with a hard 60-second deadline and retain output."""
    start = perf_counter()
    with tempfile.TemporaryDirectory(prefix="pysystemc-end-to-end-") as directory:
        command = [
            sys.executable,
            str(Path(__file__).resolve()),
            "--child",
            "--source",
            str(source),
            "--workload",
            workload,
            "--count",
            str(count),
            "--engine",
            engine,
        ]
        process = subprocess.run(command, cwd=directory, capture_output=True, text=True, timeout=60)
    wall = perf_counter() - start
    log.write_text(process.stdout + "\n" + process.stderr)
    if process.returncode:
        raise RuntimeError(f"{workload}/{engine} failed; see {log.relative_to(ROOT)}")
    lines = [line[len(MARKER) :] for line in process.stdout.splitlines() if line.startswith(MARKER)]
    assert len(lines) == 1, log
    result = json.loads(lines[0])
    assert result["python"] == "3.14.0"
    result["process_wall_seconds"] = wall
    result["log"] = log.relative_to(ROOT).as_posix()
    return result


def run() -> None:
    """Complete one fixed round or publish its failure and stop without retries."""
    progress = contract()
    save(PROGRESS, progress)
    here = ROOT / "build/performance/end-to-end"
    here.mkdir(parents=True, exist_ok=True)
    evidence = {"schema": "pysystemc-end-to-end-v1", "contract": contract(), "preflight": [], "samples": []}
    started = perf_counter()
    active = "contract"
    try:
        sources = {label: here / label for label in ("baseline", "current")}
        evidence["snapshots"] = {
            label: snapshot(revision, sources[label])
            for label, revision in [("baseline", BASELINE), ("current", CURRENT)]
        }
        progress["gates"][active] = {"status": "passed"}
        save(PROGRESS, progress)
        active = "equivalence"
        for workload in ("import", "event_signal", "tlm_transport"):
            checked = []
            for label in ("baseline", "current"):
                item = sample(sources[label], workload, 100, "python", here / f"preflight-{label}-{workload}.log")
                evidence["preflight"].append({"revision": label, **item})
                checked.append(item)
            assert checked[0]["behavior"] == checked[1]["behavior"]
            assert checked[0]["native_library_sha256"] == checked[1]["native_library_sha256"]
            if workload != "import":
                native = sample(sources["current"], workload, 100, "native", here / f"preflight-native-{workload}.log")
                evidence["preflight"].append({"revision": "current", **native})
                assert native["behavior"] == checked[0]["behavior"]
        progress["gates"][active] = {
            "status": "passed",
            "checks": "Baseline/current and native reference behavior match; source imports guarded; native library identical.",
        }
        save(PROGRESS, progress)
        for workload, count in [("import", 1), ("event_signal", CYCLES), ("tlm_transport", TRANSFERS)]:
            active = workload
            for pair in range(PAIRS):
                for label in ("baseline", "current") if pair % 2 == 0 else ("current", "baseline"):
                    assert perf_counter() - started < 1800, "Measurement deadline exceeded"
                    item = sample(
                        sources[label], workload, count, "python", here / f"{workload}-{pair:02d}-{label}.log"
                    )
                    evidence["samples"].append({"revision": label, "pair": pair, **item})
                    save(here / "raw.json", evidence)
                pair_samples = evidence["samples"][-2:]
                assert pair_samples[0]["behavior"] == pair_samples[1]["behavior"]
                assert pair_samples[0]["native_library_sha256"] == pair_samples[1]["native_library_sha256"]
                progress["completed_measurement_pairs"] += 1
                progress["gates"][active] = {"status": "running", "pairs_completed": pair + 1, "pairs_total": PAIRS}
                save(PROGRESS, progress)
                print(
                    f"{workload}: {pair + 1}/{PAIRS} pairs; {progress['completed_measurement_pairs']}/30 overall",
                    flush=True,
                )
            progress["gates"][active]["status"] = "passed"
            save(PROGRESS, progress)
        active = "report"
        summaries = {}
        for workload in ("import", "event_signal", "tlm_transport"):
            rows = [r for r in evidence["samples"] if r["workload"] == workload]
            phases = {}
            for phase in ("import_seconds", "elaboration_seconds", "run_seconds", "process_wall_seconds"):
                medians = {
                    label: statistics.median(r[phase] for r in rows if r["revision"] == label)
                    for label in ("baseline", "current")
                }
                deltas = []
                for pair in range(PAIRS):
                    a = next(r[phase] for r in rows if r["pair"] == pair and r["revision"] == "baseline")
                    b = next(r[phase] for r in rows if r["pair"] == pair and r["revision"] == "current")
                    if a:
                        deltas.append(100 * (1 - b / a))
                phases[phase] = {
                    "baseline_median_seconds": medians["baseline"],
                    "current_median_seconds": medians["current"],
                    "elapsed_reduction_percent": 100 * (1 - medians["current"] / medians["baseline"])
                    if medians["baseline"]
                    else None,
                    "paired_reduction_percent": deltas,
                    "current_wins": sum(x > 0 for x in deltas),
                    "baseline_range_seconds": [
                        min(r[phase] for r in rows if r["revision"] == "baseline"),
                        max(r[phase] for r in rows if r["revision"] == "baseline"),
                    ],
                    "current_range_seconds": [
                        min(r[phase] for r in rows if r["revision"] == "current"),
                        max(r[phase] for r in rows if r["revision"] == "current"),
                    ],
                }
            summaries[workload] = phases
        evidence.update(
            status="completed_measurement",
            summaries=summaries,
            elapsed_seconds=perf_counter() - started,
            limits=[
                "One host, warm filesystem caches; each sample is a fresh interpreter/kernel.",
                "Same installed dependencies and native SystemC; revision source trees are frozen and guarded.",
                "Python driver/checksum work is included in simulation timing; assertions and recorder I/O are outside phase timing.",
                "TLM workload uses a reused payload and nonwaiting address transform with delay annotation, not a memory or thread-throughput model.",
                "Native preflight uses shared Python stimulus; selected behavior equivalence does not certify every API.",
                "No single package-wide percentage; no timing percentages summed; no optimization adopted.",
            ],
        )
        save(here / "raw.json", evidence)
        evidence["evidence_sha256"] = {
            path.relative_to(ROOT).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in here.glob("*.log")
        }
        save(REPORT, evidence)
        progress["gates"][active] = {"status": "passed", "report": REPORT.relative_to(ROOT).as_posix()}
        progress["status"] = "measurements_complete_pending_validation_commit"
        save(PROGRESS, progress)
    except Exception as error:
        progress["status"] = "stopped_failed_gate"
        progress["gates"][active] = {"status": "failed", "error": str(error)}
        evidence.update(status="failed", failed_gate=active, error=str(error))
        save(here / "raw.json", evidence)
        save(REPORT, evidence)
        save(PROGRESS, progress)
        raise


def main() -> None:
    """Parse either the fixed round or one internal isolated-child command."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--child", action="store_true")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--workload", choices=["import", "event_signal", "tlm_transport"])
    parser.add_argument("--count", type=int, default=100)
    parser.add_argument("--engine", choices=["python", "native"], default="python")
    parser.add_argument("--initialize", action="store_true")
    args = parser.parse_args()
    if args.initialize:
        save(PROGRESS, contract())
    elif args.child:
        assert args.source and args.workload and args.count > 0
        print(MARKER + json.dumps(child(args.source, args.workload, args.count, args.engine)))
    else:
        run()


if __name__ == "__main__":
    main()

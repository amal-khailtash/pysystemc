"""Compare a checked counter simulation with VCD tracing disabled and enabled.

Each sample uses a fresh kernel subprocess with a 60-second deadline. Python
and C++ method counters share the same Python stimulus. Timing includes signal
writes and kernel advances, excludes imports/elaboration and trace inspection,
and includes trace creation/closure. This is a workload measurement, not a
package-wide speedup or a comparison with an earlier project revision.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import statistics
import subprocess
import sys
import tempfile
from time import perf_counter

MARKER = "__SIMULATION_BENCHMARK__"


def run_child(engine: str, traced: bool, cycles: int) -> dict:
    """Run one counter and validate callback counts, values, time and VCD data.

    Args:
        engine: ``python`` or ``native`` method implementation.
        traced: Whether to create and close a VCD file in the timed region.
        cycles: Positive number of rising edges driven by Python.

    Returns:
        Timing and validated scalar results for one fresh-kernel sample.

    Raises:
        AssertionError: Callback, signal, time or trace results disagree.

    Native objects and the Python callback stay alive until process exit. Call
    this only once per subprocess; the kernel cannot be reset for another run.
    Native report errors propagate, and the trace closes on simulation errors.
    """
    from amal.eda.systemc import sc_core as sc
    import cppyy

    class Counter(sc.sc_module):
        """Retain a Python method callback and its bound native ports."""

        def __init__(self):
            """Register one rising-edge process with initialization disabled."""
            module_name = sc.sc_module_name("counter")
            super().__init__(module_name)
            self.clock = sc.sc_in[bool]("clock")
            self.output = sc.sc_out["unsigned int"]("output")
            self.calls = 0
            self.sc_method("tick", self.tick)
            self.sensitive(self.clock.pos())
            self.dont_initialize()

        def tick(self):
            """Count each positive edge and schedule the corresponding output."""
            self.calls += 1
            self.output.write(self.calls % 65536)

    if engine == "native":
        cppyy.cppdef(r"""
namespace simulation_benchmark
{
/** Method counter owned by its child process; native callbacks retain no Python references. */
struct Counter : sc_core::sc_module
{
  sc_core::sc_in<bool> clock{"clock"};
  sc_core::sc_out<unsigned int> output{"output"};
  unsigned int calls = 0;
  SC_HAS_PROCESS(Counter);
  /** Register the native method; ports must be bound before starting simulation. */
  explicit Counter(sc_core::sc_module_name name) : sc_core::sc_module(name)
  {
    SC_METHOD(tick);
    sensitive << clock.pos();
    dont_initialize();
  }
  /** Schedule a borrowed port write; native report exceptions propagate to the driver. */
  void tick()
  {
    output.write(++calls % 65536);
  }
};
}  // namespace simulation_benchmark
""")
        model = cppyy.gbl.simulation_benchmark.Counter("counter")
    else:
        model = Counter()
    clock = sc.sc_signal[bool]("clock")
    output = sc.sc_signal["unsigned int"]("output")
    model.clock(clock)
    model.output(output)
    sc.sc_start(sc.SC_ZERO_TIME)
    started = perf_counter()
    trace = sc.sc_create_vcd_trace_file("counter") if traced else None
    if trace is not None:
        sc.sc_trace(trace, clock, "clock")
        sc.sc_trace(trace, output, "count")
    try:
        for _ in range(cycles):
            clock.write(True)
            sc.sc_start(1, sc.SC_NS)
            clock.write(False)
            sc.sc_start(1, sc.SC_NS)
    finally:
        if trace is not None:
            sc.sc_close_vcd_trace_file(trace)
    seconds = perf_counter() - started
    assert int(model.calls) == cycles
    assert int(output.read()) == cycles % 65536
    assert sc.sc_time_stamp() == sc.sc_time(2 * cycles, sc.SC_NS)
    transitions = None
    if traced:
        text = Path("counter.vcd").read_text()
        declarations = [line.split() for line in text.splitlines() if line.startswith("$var")]
        identifier = next(row[3] for row in declarations if row[4] == "count")
        values = [
            int(line.split()[0][1:], 2)
            for line in text.splitlines()
            if line.startswith("b") and line.split()[-1] == identifier
        ]
        assert values == [i % 65536 for i in range(1, cycles + 1)], (len(values), values[:3])
        transitions = len(values)
    return {
        "engine": engine,
        "traced": traced,
        "cycles": cycles,
        "seconds": seconds,
        "callbacks": int(model.calls),
        "value": int(output.read()),
        "time_ns": 2 * cycles,
        "vcd_count_transitions": transitions,
    }


def sample(engine: str, traced: bool, cycles: int) -> dict:
    """Launch one isolated kernel with a 60-second timeout.

    Args:
        engine: Counter implementation passed to :func:`run_child`.
        traced: Enable VCD tracing for this sample.
        cycles: Positive number of counter cycles.

    Returns:
        The child's checked timing and result dictionary.

    Raises:
        RuntimeError: The child failed; its stdout and stderr are included.
        subprocess.TimeoutExpired: The child exceeded its deadline.

    A temporary directory owns the child's waveform and is removed on exit.
    """
    with tempfile.TemporaryDirectory(prefix="pysystemc-timing-") as directory:
        command = [
            sys.executable,
            str(Path(__file__).resolve()),
            "--child",
            "--engine",
            engine,
            "--cycles",
            str(cycles),
        ]
        if traced:
            command.append("--trace")
        result = subprocess.run(command, cwd=directory, capture_output=True, text=True, timeout=60)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        return json.loads(next(line[len(MARKER) :] for line in result.stdout.splitlines() if line.startswith(MARKER)))


def main() -> None:
    """Measure alternating tracing pairs and write checked, scoped JSON results."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--child", action="store_true")
    parser.add_argument("--engine", choices=("python", "native"), default="python")
    parser.add_argument("--trace", action="store_true")
    parser.add_argument("--cycles", type=int, default=20000)
    parser.add_argument("--pairs", type=int, default=3)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.cycles < 1 or args.pairs < 1:
        parser.error("cycles and pairs must be positive")
    if args.child:
        print(MARKER + json.dumps(run_child(args.engine, args.trace, args.cycles)))
        return
    samples = []
    for pair in range(args.pairs):
        for engine in ("python", "native"):
            for traced in (False, True) if pair % 2 == 0 else (True, False):
                samples.append({"pair": pair, **sample(engine, traced, args.cycles)})
    medians = {}
    for engine in ("python", "native"):
        off = statistics.median(s["seconds"] for s in samples if s["engine"] == engine and not s["traced"])
        on = statistics.median(s["seconds"] for s in samples if s["engine"] == engine and s["traced"])
        medians[engine] = {
            "trace_off_seconds": off,
            "trace_on_seconds": on,
            "trace_overhead_percent": 100 * (on / off - 1),
        }
    report = {
        "status": "passed",
        "python": sys.version.split()[0],
        "scope": "Same Python stimulus; method counter; VCD tracing overhead, not optimization gain. Imports/elaboration excluded; warm OS caches retained.",
        "pairs": args.pairs,
        "cycles": args.cycles,
        "samples": samples,
        "medians": medians,
    }
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + "\n")
    else:
        print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()

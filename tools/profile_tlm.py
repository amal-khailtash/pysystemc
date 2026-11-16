"""Profile only the existing TLM transport loop in three fresh subprocesses.

Profile timings diagnose Python/native call boundaries and are never used as
performance measurements. Imports and elaboration are outside the profile.
The model, transaction count and semantic checks match the prior fixed workload.
"""

from __future__ import annotations

import argparse
import cProfile
import importlib.util
import json
from pathlib import Path
import pstats
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
HERE = ROOT / "build/performance/tlm-profile"
PROGRESS = ROOT / "docs/_static/performance-tlm-profile-progress.json"
MARKER = "__TLM_PROFILE__"
GATES = ["contract", "profile", "candidate_choice", "correctness", "measurement", "decision", "docs_commit"]


def save_progress(value: dict) -> None:
    """Write machine and readable progress with the fixed stopping rule."""
    PROGRESS.parent.mkdir(parents=True, exist_ok=True)
    temporary = PROGRESS.with_suffix(".tmp")
    temporary.write_text(json.dumps(value, indent=2) + "\n")
    temporary.replace(PROGRESS)
    lines = [
        "# TLM profiling and candidate progress",
        "",
        f"Status: **{value['status']}**.",
        "",
        "| Gate | Status | Detail |",
        "|---|---|---|",
    ]
    for name, gate in value["gates"].items():
        lines.append(f"| {name} | {gate['status']} | {gate.get('detail', '')} |")
    lines += [
        "",
        "Fixed scope: three diagnostic profiles of the existing 20,000-transfer workload.",
        "At most one candidate; three alternating feasibility pairs. Confirmation runs only",
        "if reduction is at least 5% with 3/3 wins, then ten pairs require at least 5%",
        "and 10/10 wins. No resampling or second candidate. Failed gates stop measurement.",
        "Every child has a 60-second deadline. Each execution gate has a fixed outer timeout.",
        "Profile timings are separate from uninstrumented candidate measurements.",
        "Python 3.14; VCD and coverage off. No broader audit or new workload.",
        "",
        "Machine tracker: `../_static/performance-tlm-profile-progress.json`.",
        "",
    ]
    (ROOT / "docs/development/performance-tlm-profile-progress.md").write_text("\n".join(lines))


def initialize() -> None:
    """Freeze the finite profile, screen and conditional confirmation contract."""
    save_progress(
        {
            "schema": "pysystemc-tlm-profile-progress-v1",
            "status": "running",
            "baseline_commit": "29251f038a0b095082bb07499892459d9a576aab",
            "profiles": 3,
            "transfers": 20000,
            "candidate_limit": 1,
            "screen_pairs": 3,
            "confirmation_pairs": 10,
            "minimum_reduction_percent": 5,
            "resampling": False,
            "child_timeout_seconds": 60,
            "gates": {name: {"status": "passed" if name == "contract" else "pending"} for name in GATES},
        }
    )


def load_benchmark():
    """Import the shared frozen model without importing SystemC yet."""
    spec = importlib.util.spec_from_file_location("end_to_end", ROOT / "tools/benchmark_end_to_end.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def profile_child(index: int) -> dict:
    """Capture the checked transport loop only, in one fresh kernel.

    Args:
        index: Profile artifact suffix, 0 through 2.
    Returns:
        Checked model results and relative native/Python diagnostic statistics.
    Raises:
        AssertionError: Timing boundaries, instrumentation or model checks fail.

    The shared model retains callback, module, socket and transaction ownership.
    Kernel/GIL exceptions propagate. Call only once per subprocess.
    """
    benchmark = load_benchmark()
    original_clock = benchmark.perf_counter
    profiler = cProfile.Profile()
    calls = 0

    def boundary():
        """Enable at loop entry and disable before final assertions."""
        nonlocal calls
        calls += 1
        if calls == 4:
            profiler.enable()
        elif calls == 5:
            profiler.disable()
        return original_clock()

    benchmark.perf_counter = boundary
    result = benchmark.child(ROOT, "tlm_transport", 20000, "python")
    assert calls == 5 and sys.getprofile() is None
    HERE.mkdir(parents=True, exist_ok=True)
    profiler.dump_stats(str(HERE / f"profile-{index}.pstats"))
    stats = pstats.Stats(profiler)
    functions = []
    for (file, line, name), (primitive, total, self_seconds, cumulative, callers) in stats.stats.items():
        path = Path(file)
        if path.is_absolute():
            file = path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else "<dependency>/" + path.name
        functions.append(
            {
                "file": file,
                "line": line,
                "function": name,
                "primitive_calls": primitive,
                "total_calls": total,
                "self_seconds": self_seconds,
                "cumulative_seconds": cumulative,
            }
        )
    return {
        "index": index,
        "scope": "cProfile diagnostic of transport loop only; not comparable to unprofiled latency",
        "behavior": result["behavior"],
        "python": result["python"],
        "cppyy": result["cppyy"],
        "native_library_sha256": result["native_library_sha256"],
        "profile_total_seconds": stats.total_tt,
        "functions": sorted(functions, key=lambda row: row["self_seconds"], reverse=True),
    }


def run_profiles() -> None:
    """Run exactly three diagnostic children and retain failure output."""
    HERE.mkdir(parents=True, exist_ok=True)
    progress = json.loads(PROGRESS.read_text())
    progress["gates"]["profile"] = {"status": "running", "detail": "0/3 diagnostic profiles"}
    save_progress(progress)
    profiles = []
    try:
        for index in range(3):
            result = subprocess.run(
                [sys.executable, str(Path(__file__).resolve()), "--child", "--index", str(index)],
                capture_output=True,
                text=True,
                timeout=60,
                cwd=HERE,
            )
            (HERE / f"profile-{index}.log").write_text(result.stdout + "\n" + result.stderr)
            if result.returncode:
                raise RuntimeError(f"Profile {index} failed; see build/performance/tlm-profile/profile-{index}.log")
            lines = [line[len(MARKER) :] for line in result.stdout.splitlines() if line.startswith(MARKER)]
            assert len(lines) == 1
            profiles.append(json.loads(lines[0]))
            (HERE / "profiles.json").write_text(json.dumps(profiles, indent=2) + "\n")
            progress["gates"]["profile"]["detail"] = f"{index + 1}/3 diagnostic profiles"
            save_progress(progress)
        assert len({p["native_library_sha256"] for p in profiles}) == 1
        assert all(p["behavior"] == profiles[0]["behavior"] for p in profiles)
        progress["gates"]["profile"]["status"] = "passed"
        save_progress(progress)
        print("3/3 diagnostic profiles; same checked 20,000-transfer behavior; no timing-gain claim")
    except Exception as error:
        progress["status"] = "stopped_failed_gate"
        progress["gates"]["profile"] = {"status": "failed", "detail": str(error)}
        save_progress(progress)
        raise


def main() -> None:
    """Run the fixed diagnostic gate or initialize its visible contract."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--initialize", action="store_true")
    parser.add_argument("--child", action="store_true")
    parser.add_argument("--index", type=int, choices=range(3), default=0)
    args = parser.parse_args()
    if args.initialize:
        initialize()
    elif args.child:
        print(MARKER + json.dumps(profile_child(args.index)))
    else:
        run_profiles()


if __name__ == "__main__":
    main()

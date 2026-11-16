# Three-priority batch results

The requested tracking, correctness and example jobs are complete. Portability
was dropped. No optimization was adopted and no additional job is queued.

## Tracking on/off

Tracking means the actual `.ai/record_verification_run.py` invocation: an extra
Python interpreter, simulation launch/log capture, run-ledger append and plan
synchronization. Ten fixed pairs alternated off/on and on/off. All 20 fresh kernels
ran the unchanged 20,000-transfer TLM loop; profiling, coverage and VCD were off.
Each child had 60 seconds; the outer round had 1,200 seconds. No resampling.

| Median metric | Tracking off | Tracking on |
|---|---:|---:|
| Whole invocation | 6.430736 s | 6.775435 s |
| Original transport loop | 134.362 ms | 131.982 ms |

Tracking-on whole invocation overhead was **5.36%**
relative to off; disabling it reduced median invocation time by
**5.09%** relative to on. On was slower in
**8/10 pairs**. The loop code is identical; its timing
variation is not a runtime optimization. Recorder accounting happens outside
that loop. Off captures output in memory and persists it after timing; on includes
the recorder's child-log write within timing. The delta includes this output-sink
and launch difference; it is not an isolated bookkeeping-statement measurement.

All 113 runtime hashes, workload/recorder/synchronizer hashes, child marker
payloads, native library identity, callbacks, responses, checksum, addresses,
annotation and kernel time were checked. Exactly 10 successful actual recorder
rows were appended. Ledger size and code hashes are retained to scope the result.
No other validation/simulation jobs ran during measurement. These sequential
samples have no confidence interval; W509 remains in both roles. Results describe
this checkout's recorder state and workload, not universal tracking overhead.
[Raw published evidence](../_static/performance-tracking-overhead.json) retains
all samples and the recorder source; local logs are in
`build/performance/tracking-overhead/`. Historical optimization accounting stays 25/20.

## Correctness

Commit `648cabe` adds three native/Python signal contract cases at 65, 137 and 257
bits. Each checks full high/low values, deferred reads, unchanged-write event
suppression, three callbacks at 0, 1 and 2 ns, exact native delta ordering and retained
Python callback ownership after external references are deleted and GC runs.
An initial fixture assumed zero-time restarts dispatch every pending update and
that the first delta is positive; its failure logs are preserved. Mirrored bounded
1 ns stimulus and native comparisons corrected the fixture without runtime edits.
This is a finite signal/method contract family; other templates and thread/GIL
lifecycles remain outside these new cases.

## Examples and acceptance

Commit `04c5d55` expands the existing `examples/sysc/pkt_switch` port's native
comparisons to seeds 1, 7 and 23. Every packet/detail/drop record must agree, traffic
must be nonempty, and final drop arithmetic must be consistent. Only the staged
native copy's wall-time seed is replaced. All mapped upstream targets already had
ports, so this job adds input coverage to the selected directory.

Root acceptance passed all **6 comparison cases** and **16 installed-harness
checks**. Three plain seed tests preserve the installed checker's selection
contract; it now selects 43 cases. This batch did not run all 43 in an installed
wheel. Current collection is **1,002 tests**; the accepted full 997-test regression
is historical evidence for unchanged runtime sources. No fresh full 1,002-test
or coverage claim is made. [Validation evidence](../_static/todo-batch-validation.json)
records cases, source hashes and preserved failures.

Python 3.14 remains the baseline; 3.15 is deferred. Broader inputs, topologies,
configuration and lifecycle/GIL contracts remain outside this selected batch.
See the [closed progress tracker](todo-batch-progress.md).

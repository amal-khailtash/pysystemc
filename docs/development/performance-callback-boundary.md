# Bounded callback boundary result

The sole void-validator specialization was **rejected** at the fixed screen:
1.59% median reduction with 2/3 wins. Acceptance
required at least 5% and all three wins. No confirmation, resampling or second
candidate followed. Production runtime code is unchanged; historical accounting
remains 25 accepted workload slices /20 optimization commits. This round is closed.

The cost review reused the three diagnostic profiles from the completed
[TLM round](performance-tlm-profile.md); no new profiles were collected.
Target.transport self time was 78.7–85.5 ms per 20,000 calls, but this Python
frame also includes cppyy native calls: address transformation, response mutation,
and delay addition/assignment. That self time is not a measurement of removable
GIL or callback bridge overhead. Child Python frames separately report sc_time
construction at 27.1–29.5 ms and module lookup at 24.0–25.8 ms. Those previously
evaluated candidates were not retried. The native blocking callback bridge is
already one std::function forward; ownership, GIL entry, exception propagation
and guarded waits remain necessary parts of its contract.

The result-validation wrapper reported 7.9–8.8 ms self time. The sole prototype
selected a void validator at registration for blocking transport and DMI
invalidation, avoiding the method-name branch in each invocation. It retained
variadic argument forwarding, callable ownership, exact None-result enforcement,
error text and the existing native bridge. Other return families were unchanged.
Independent Luna review considered the benefit unsubstantiated; root chose this
small, distinct specialization for the fixed empirical screen, with no assumed gain.

Correctness passed retained/released callable checks, argument identity,
non-None result rejection, original exception identity and recovery, and native
result-contract tests on both snapshots. Four 100-transfer Python/native
reference models agreed. Six additional fresh baseline/candidate models passed
blocking wait/reset, duplicate-registration retention and callback exceptions.
An initial baseline weakref assertion failed because the harness retained its own
ValueError traceback. Clearing that test-owned traceback fixed the harness;
initial failure logs were preserved, and no timing had run before the correction.
The runtime candidate was unchanged.

| Uninstrumented 20,000-transfer screen | Result |
|---|---:|
| Baseline median | 135.097 ms |
| Candidate median | 132.953 ms |
| Median reduction | 1.5873% |
| Paired reductions | -1.0736%, 0.7928%, 3.0530% |
| Candidate wins | 2/3 |

Order was baseline/candidate, candidate/baseline, baseline/candidate. Every sample
had a fresh kernel and 60-second deadline; correctness/screen outer deadlines
were 900 seconds, lifecycle 600 seconds. The original loop, model and 20,000
transfers were unchanged. Profiling, coverage and VCD were off; import, setup,
elaboration and final assertions were outside timing. All six samples agreed on
20,000 callbacks/responses, checksum 600110000, last address 60004, 20,000 ns
annotated delay, and zero kernel time, using the same native library.

Root reconciliation checked snapshot/Git hashes, source diff, marked child log
payloads, ordering, behavior, native identity, medians and paired reductions.
Lifecycle source guards and completion markers were checked separately.
The [JSON evidence](../_static/performance-callback-boundary.json) preserves
source manifests, prototype diff, diagnostic attribution, correctness/lifecycle
results, every timing sample and the evaluation harnesses. Raw logs remain in
`build/performance/callback-boundary/`. The
[seven-gate tracker](performance-callback-progress.md) records closure.

This is one synchronous annotated TLM workload; it establishes no broad callback,
waiting-throughput or package gain. Native W509 remains in both roles. Profiling
self times do not isolate native/GIL costs. Tracking/bookkeeping overhead was not
measured. Python 3.14 is the baseline; 3.15 stays deferred. GLM/Qwen review was
unavailable due to the previously recorded AMD HTTP 401; credentials were not
retried. No subsequent candidate is queued.

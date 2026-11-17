# Bounded TLM profiling result

The single candidate was **rejected**: its median transport-loop reduction was
1.37%, below the fixed 5% screen threshold. Production runtime code is unchanged.
This round is closed; no confirmation, resampling, second candidate or new workload
is queued. Historical accounting remains 25 accepted slices /20 optimization commits.

Three diagnostic cProfile runs used the existing 20,000-transfer TLM workload on
Python 3.14.0, cppyy 3.5.0 and the shared native SystemC library. Only the transport
loop was profiled. Target callback self time was approximately 79–86 ms,
`_time.construct` 27–29 ms, `_KernelModule.__getattribute__` 24–26 ms over 40,000
calls, and callback-result wrapping about 8 ms. These instrumented times identify
costs; they do not predict savings or compare with uninstrumented latency.

The frozen candidate added a direct `ModuleType.__getattribute__` lookup guarded
by exact module type and unchanged captured MRO. Native flag reads and assignments
retained their existing path; subclasses and changed MROs retained `super()`.
The previously rejected exact native time-constructor experiment was not retried.

Two fresh module checks passed ordinary identity, dynamic/missing attributes,
descriptors, subclass overrides, intervening mixins, actual `__bases__` mutation
and restoration, and live native flag edges. Four fresh Python/native reference
models at 100 transfers agreed on callbacks, responses, checksum, address,
annotated delay and zero kernel time.

| Uninstrumented screen metric | Result |
|---|---:|
| Baseline median transport loop | 134.304 ms |
| Candidate median transport loop | 132.467 ms |
| Median reduction | 1.3684% |
| Paired reductions, in execution order | 7.7122%, 3.0929%, 0.4545% |
| Candidate wins | 3/3 |
| Required screen acceptance | at least 5% and 3/3 wins |

Three pairs alternated baseline/candidate, candidate/baseline, baseline/candidate.
Each measured child executed 20,000 transfers with profiling, coverage and VCD
off; imports, setup, elaboration and final assertions were outside timing.
All six children agreed on 20,000 callbacks/responses, checksum 600110000,
last address 60004, 20,000 ns annotated delay and zero kernel time.
Each child had a 60-second deadline; correctness and screen gates each had a
900-second outer limit. Both execution gates completed normally. The performance
threshold failed, which ends measurement under the original contract.

Root reconciliation checked all 226 snapshot hashes, all 113 baseline Git source
hashes, marked child log payloads, execution order, behavior, shared native identity,
medians and paired reductions. An independent Luna review agreed. The published
[JSON evidence](../_static/performance-tlm-profile.json) includes all profiles,
correctness and screen samples, source manifests, candidate diff and evaluation
harness. Raw logs remain under `build/performance/tlm-profile/`.
The [seven-gate tracker](performance-tlm-profile-progress.md) records closure.

The retained top-module construction name emits native W509 in both roles.
This is one synchronous annotated-transport workload; it establishes no waiting,
memory-throughput, broader API or universal package gain. Bookkeeping/tracking
overhead was not measured. Python 3.15 remains deferred. Requested GLM/Qwen review
was unavailable because the AMD endpoint returned HTTP 401 after trusted TLS
verification; no on-prem review was obtained or accepted.

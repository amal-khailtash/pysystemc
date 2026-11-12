Performance evidence
====================

The measured changes through commit ``21497a9`` cover 23 workload slices in
18 performance-related commits. There is no measured package-wide cumulative
improvement. Percentages for different operations must not be added together.
These are historical measurements from Python 3.14.0, cppyy 3.5.0 and SystemC
2.3.4 on the recorded Linux checkout; subsequent correctness changes do not
constitute new performance measurements. The port-value dispatch change in
commit ``f0daf48`` adds one accepted workload slice, described below, bringing
the total to 24 accepted workload slices across 19 performance commits.
The member ``next_trigger`` cache in commit ``854707d`` brings the total to
25 accepted workload slices across 20 performance commits; its confirmation
and validation are described below.

The timing loops measure the named operations. Recorder output and native
correctness checks run outside those loops. These results therefore do not
measure the whole-process cost of tracking, tracing, or verification.

.. list-table:: Recorded workload results
   :header-rows: 1
   :widths: 38 35 14 13

   * - Workload
     - Before → after
     - Less elapsed
     - Pairs / wins
   * - sc_core help; exact documentation compared
     - 49.849 → 31.905 ms per namespace_help call
     - 36.0%
     - 10 / not recorded
   * - sc_dt help; exact documentation compared
     - 56.428 → 51.254 ms per namespace_help call
     - 9.2%
     - 10 / not recorded
   * - tlm help; exact documentation compared
     - 201.575 → 197.282 ms per namespace_help call
     - 2.1%
     - 10 / not recorded
   * - tlm_utils help; exact documentation compared
     - 50.300 → 50.889 ms per namespace_help call
     - 1.2% slower
     - 10 / not recorded
   * - next trigger type classification
     - 0.174 → 0.138 seconds per 20,000 callbacks
     - 20.4%
     - 10 / 10
   * - wait type classification
     - 0.421 → 0.354 seconds per 20,000 resumptions
     - 16.0%
     - 10 / 10
   * - native first loader startup
     - 23.490 → 7.355 seconds per warm package import
     - 68.7%
     - 3 / not recorded
   * - native logging startup
     - 7.222 → 4.677 seconds per warm package import
     - 35.2%
     - not recorded
   * - tlm endian converter
     - 31.056 → 22.768 ms per 20,000 request/response conversion pairs
     - 26.7%
     - 10 / 10
   * - signal value conversion
     - 87.675 → 38.458 ms per 20,000 int/257-bit reads
     - 56.1%
     - 10 / 10
   * - datatype enum conversion
     - 43.379 → 35.879 ms per 20,000 typed enum conversions
     - 17.3%
     - 10 / 9
   * - reference wrapper construction
     - 32.950 → 29.427 ms per 20,000 custom/257-bit wrapper constructions
     - 10.7%
     - 10 / 10
   * - named arithmetic dispatch
     - 35.174 → 22.977 ms per 20,000 add calls
     - 34.7%
     - 10 / 10
   * - binary arithmetic dispatch
     - 70.312 → 62.019 ms per 20,000 additions
     - 11.8%
     - 10 / 10
   * - compound arithmetic dispatch
     - 48.487 → 40.795 ms per 20,000 compound additions
     - 15.9%
     - 10 / 10
   * - numeric shift dispatch
     - 37.641 → 30.469 ms per 20,000 left/right shifts
     - 19.1%
     - 10 / 10
   * - scalar bit dispatch
     - 74.576 → 66.999 ms per 20,000 conjunctions
     - 10.2%
     - 10 / 10
   * - datatype concat dispatch
     - 47.427 → 32.981 ms per 20,000 text concatenations
     - 30.5%
     - 10 / 10
   * - nonblocking socket enum metadata
     - 93.416 → 86.092 ms per 20,000 public forward/backward calls
     - 7.8%
     - 10 / 10
   * - vector assignment dispatch
     - 58.907 → 48.541 ms per 20,000 arbitrary-width assignments
     - 17.6%
     - 10 / 10
   * - vector constructor dispatch
     - 109.217 → 99.770 ms per 20,000 arbitrary-width constructors
     - 8.6%
     - 10 / 10
   * - integer assignment dispatch
     - 25.822 → 17.230 ms per 20,000 integer-template assignments
     - 33.3%
     - 10 / 10
   * - integer constructor dispatch
     - 63.713 → 54.409 ms per 20,000 integer-template constructors
     - 14.6%
     - 10 / 10

The two startup results are separate matched checkpoints: loader ordering
measured 23.490 → 7.355 seconds, and lean native logging later measured
7.222 → 4.677 seconds. No single run compared the original startup path with
the final combined implementation. The small TLM help change was noisy, and
``tlm_utils`` help was slightly slower; neither should be described as a
reliable general speedup.

The versioned :download:`machine-readable checkpoint
<../_static/performance-results.json>` preserves workload sizes, values,
commit identifiers, limits, and hashes of the retained local evidence.
Raw matrices and validation logs remain local verification artifacts. Their
absence from a source checkout is recorded rather than treated as reproduced
evidence. Rejected candidates and unmeasured correctness repairs are not
counted as adopted performance gains.

Native-type endian lookup screen
--------------------------------

After the correctness closeout, an isolated cache for native C++ type keys was
screened using 20,000 complete generic endian request/response operations with
a user-defined sixteen-byte word, a 32-byte bus, high addresses, reads, writes
and byte enables. Three alternating fresh-process pairs passed native parity
and state checks. Median elapsed time was 1.567815 seconds before and 1.536346
seconds with the candidate (2.01% lower, three paired wins). The workload includes
owned-buffer allocation, byte preparation and assertions.

This candidate was rejected because it did not meet the 5% feasibility threshold.
Production remains unchanged; the screen adds no adopted gain. The result is
recorded separately from the historical accepted workloads in the JSON report.

Vector operator dispatch screen
-------------------------------

An isolated cache of successful native operator callables was screened with
20,000 Python AND/OR/XOR calls over 137-bit two-state and 257-bit four-state
vectors, including forward, reflected and compound routes. Three alternating
fresh-process pairs passed native smoke and final-state replay checks. Median
elapsed time was 128.204 ms before and 122.139 ms with the candidate (4.73%
lower, three paired wins). Timing includes dispatch-loop bookkeeping. Native
smoke checks every operator route; replay checks the last result and all final
compound targets rather than every intermediate result.

The candidate was rejected below the 5% feasibility threshold. Production
remains unchanged, and no adopted gain or package-wide speedup is claimed.
Fixture failures in four earlier smoke attempts are retained: numeric formatting
of X/Z values, native recognition of a leading ``0X`` as a numeric prefix, and
an incorrect expected record count. Warmup now uses separate mutable targets
so measured public operations and native replay start from identical state.

Port value dispatch
-------------------

Commit ``f0daf48`` caches successful native ``sc_port_value`` conversion callables
by the full matched input or inout base name. Output ports retain their native
inout conversion. Every call still scans the native bases and invokes the
current bound interface; the cache retains neither ports nor returned values.
Selection and native invocation failures leave a cold cache empty.

Ten alternating fresh-process pairs over 20,000 conversions measured
56.411 → 41.356 ms (26.69% less elapsed, ten paired wins). The workload rotates
through input, inout and output ports for native integers and unrestricted
257-bit integers. Timing includes a checksum of returned values and excludes
import, elaboration, writes, warmup and native validation. It establishes a gain
for warmed port conversion, with no package-wide or general simulation estimate.

Six affected native regression tests passed, including port events and VCD
declarations and transitions. Both uv archives match all 113 current runtime
files. An isolated installed Python 3.14 probe verifies native casts, borrowed
storage and updates, one virtual read per cold/warm call, native read-error
recovery and PATH restoration. The broader completion suite and coverage remain
evidence for the preceding source snapshot. The change was committed locally;
no release was published.

Cached finder specialization screen
-----------------------------------

After the native numeric string crash was repaired, an isolated cached-finder
creator retained its native specialization lazily instead of rebuilding the
wrapper on every call. Three serialized fresh-process pairs passed native
identity, first-method, validation, ownership and signal-update checks. Median
time for 20,000 calls fell from 160.964 to 130.353 ms (19.02% lower), but the
candidate won only two of the three pairs.

The screen requires at least 5% reduction and three paired wins. This candidate
was rejected because one pair regressed; finder production code remains
unchanged. Timing covers warmed calls through already-selected creators, with
no elaboration, startup or simulation-throughput estimate. No adopted gain is
added. The numeric string correction is a separate untimed correctness repair.

Native blocking dispatch screen
-------------------------------

A cache of successfully used native callable metadata was tested with complete
integer-return functor calls, each suspending a SystemC thread for 1 ns. Three
serialized pairs of 20,000 calls produced medians of 446.32 ms before and
422.32 ms for the candidate, a 5.38% reduction, with two paired wins. The
candidate was rejected because acceptance requires all three pairs to improve.
Production dispatch remains unchanged; no adopted workload was added.

Both variants checked native counter and simulation-time parity, cold and warm
exceptions, invalid return contracts, null objects, and transient object release.
The timing includes checksum bookkeeping. No ten-pair confirmation or broader
reset/kill, package, startup, or end-to-end performance claim follows from this
screen. Evidence is retained under
``build/performance/native-blocking-cache/``.

Member scheduling timed-wait screen
-----------------------------------

A cache of successfully used member scheduling callables, retained within each
owner and method closure by argument type spellings, was screened with complete
module member ``wait(sc_time)`` calls. Three serialized fresh-process pairs of
20,000 waits measured 367.504 ms before and 354.009 ms for the candidate (3.67%
lower, three paired wins). The candidate was rejected below the 5% feasibility
threshold; production remains unchanged and no adopted workload was added.

Native reference member waits matched the resume count and simulation time.
Two duration values confirmed current arguments after warming the signature;
invalid arguments were rejected without advancing simulation. A separate
controlled factory fixture checked cold and warm errors, scheduler unwind,
closure and method isolation, current values and borrowed-object release.
Those controlled checks do not establish native reset/kill or allocator-failure
behavior. Timing includes checksum bookkeeping. Native method-process
``next_trigger`` throughput and expression lifetimes were not measured in this
screen. Evidence is retained under
``build/performance/member-scheduler-cache/``.

Core stream insertion dispatch screen
-------------------------------------

A cache of successful native stream insertion callables was screened with
20,000 insertions of ``sc_time(12, SC_NS)`` into a fresh ``std::ostringstream``.
Three serialized fresh-process pairs measured 69.056 ms before and 54.286 ms
for the candidate (21.39% less elapsed, two paired wins). The candidate was
rejected because acceptance requires all three pairs to improve. Production
remains unchanged and no accepted workload or performance commit was added.

Both variants matched six standalone native reference rows covering status,
time, channels, ports, wide and custom payloads, derived streams and errors.
The entire timed output matched an independent C++ insertion loop; a separate
13 ns value checked current spelling after warming the same specialization.
A controlled wrapper fixture checked cold and warm failures, recovery, exact
specializations, fallback and borrowed-object release. It does not establish
native allocator-failure reachability. Timing excludes final text retrieval,
reference rendering and smoke. Prepared installed-package checks were not
executed; no confirmation or adoption gate is pending. Evidence is retained
under ``build/performance/core-stream-dispatch/``.

Member next_trigger dispatch cache
----------------------------------

Successful native member ``next_trigger`` callables are reused by exact argument
type spelling in a separate closure for each owner class. Arguments are
classified on every call, native registration still runs with the GIL held,
and cache entries retain callable metadata without retaining owners or event
expressions. Failed first calls do not populate the cache. The member ``wait``
factory retains its previous dispatch and unwind behavior.

Three serialized fresh-process pairs of 200,000 timed registrations passed
the screen with a 12.26% reduction and three wins. Ten confirmation pairs
measured 591.639 ms before and 513.113 ms with the cache (13.27% less elapsed,
ten wins). Each Python method ran alongside an independent native C++ method
with matching registration counts, checksums and timestamps. Timing includes
Python bookkeeping and the same native reference work in both variants;
it excludes imports, 100 warm callbacks and final validation. This is one
module ``next_trigger(sc_time)`` workload, not a package-wide gain or a
measurement of tracking overhead.

Cold and warm thread-context failures matched native report 520 and recovery
times. Public module/channel models exercised all time, event, OR/AND list and
numeric overloads, with native-equivalence tests covering reports, member
wait cleanup and scheduler queries. A controlled regression checks error
recovery, current values, exact signatures, closure isolation and borrowed
object release. These checks do not characterize concurrent kernels, mutable
cppyy reflection or native allocator failure. Evidence is retained under
``build/performance/member-next-trigger-cache/``. Python 3.15 remains deferred.

All 113 runtime files matched the wheel, sdist and isolated installed package;
112 runtime files were unchanged from the preceding package. Installed public
trigger callbacks and both native and Python 3.14 thread waits passed, including
Python finally cleanup and process-handle lifecycle. Eight targeted tests
passed without skips. Integration collection at that checkpoint was 993.
The fresh Python 3.14 regression run passed all 995 tests with zero failures,
errors or skips. Independent XML/log reconciliation and all 113 runtime source
hashes passed. The prior global coverage result remains historical.

Follow-up lifetime regressions compare two owner instances sharing the installed
member closure against independent native methods with private event sets.
They cover interleaved cold and warm time, event, OR/AND expression and timed
event signatures, with all four expression proxies collected. A separate
native comparison resets a method with registered sensitivity at 2 ns, kills
it at 3 ns and confirms that a 4 ns event does not resume it. Three targeted
regressions passed; current collection is 995. All 113 accepted runtime hashes
remain unchanged, so the preceding package gate is reused. This follow-up adds
correctness evidence without adding a performance workload. Destruction while
registered, concurrent kernels and mutable reflection remain uncharacterized.

Sampled member edge dispatch screen
-----------------------------------

A cache of successful native sampled-edge callables was screened with
100,000 alternating ``at_posedge``/``at_negedge`` waits in a Python CTHREAD.
The candidate retained per-call signal MRO classification, GIL release and
reset/kill unwind. Three serialized fresh-process pairs measured 6.134 s
before and 5.995 s for the candidate (2.28% less elapsed, two wins). The
candidate was rejected: acceptance requires at least 5% less elapsed and
all three pairs to improve. Production remains unchanged.

A native CTHREAD performed the same sampled bool schedule alongside Python,
with a 1 ns sampling clock and a signal toggled every 2 ns. Counts, checksums,
edge values and timestamps matched. Timing includes Python bookkeeping and
the native reference/stimulus work equally in both variants; it excludes
imports, 100 warm calls and final checks. A separate public model matched
36 native rows, including positive and negative bool/logic edges. Reset/kill
cleanup matched an independent native reference after warming both operations.
Controlled factory checks covered cold/warm failures, GIL behavior, current
values, datatype/edge isolation and borrowed release. All 113 runtime hashes
retained the preceding accepted package bytes.

Two initial smoke failures were benchmark fixture errors (a port passed in
place of a signal interface and a stale timing key); corrected smoke passed.
Failed runs were excluded from timing acceptance. No confirmation, integration
or package gate is pending for this rejected candidate. Evidence is retained
under ``build/performance/member-edge-cache/``.

Event expression append guard screen
------------------------------------

Hoisting the canonical ``(sc_event, list_type)`` guard tuple into each append
closure was screened with 200,000 timed three-event OR/AND registrations in a
Python method process. Each registration constructs a fresh expression and
executes one chained append guard. Three serialized fresh-process pairs
measured 1.371 s before and 1.320 s for the candidate (3.71% less elapsed,
three wins). The candidate was rejected because the reduction falls below
the 5% feasibility gate. Production remains unchanged.

A native C++ method performed the same expression and timeout schedule in
each kernel. Callback counts, checksums and simulation times matched; no
events were notified, so every callback resumed after 1 ns. Timing includes
expression construction, registration, Python bookkeeping and the identical
native reference work. It excludes imports, 100 warm callbacks and final
checks. This result does not establish a package-wide gain or tracking cost.

The public event-list model matched C++, including event and matching-list
appends, mixed-list rejection without consuming the left expression, and
automatic-list transfer, copy and discard. A controlled actual-factory fixture
checked canonical and derived event types, current operands, native errors
and borrowed-object release. It does not establish native allocator-failure
reachability or mutable reflection behavior. All 113 runtime hashes remain
unchanged. Installed package checks were prepared but not executed; no
confirmation or adoption gate is pending. Evidence is retained under
``build/performance/event-append-guard/``.

Fresh regression closeout
-------------------------

The full four-worker suite passed 995 tests in 1,964.80 seconds on Python
3.14.0. Independent acceptance reconciled the JUnit testcase outcomes, suite
counters and pytest log, and verified all 113 runtime files against the accepted
package manifest. The first attempt stopped at the recorder's 300-second
whole-command limit; the completed retry allowed 3,600 seconds for the suite
while preserving the existing 60-second simulation subprocess deadlines.

This is regression evidence for the current source, not a new coverage or
performance measurement. The historical 4,539/4,548 statement coverage snapshot
and its nine classified residuals remain separate. Evidence:
``build/verification/full-suite-995-root-retry.xml`` and
``build/verification/full-suite-995-root-accept.json``.

Exact numeric-unit constructor screen rejected
----------------------------------------------

Selecting the exact native ``sc_time(double, sc_time_unit)`` initializer did
not improve construction throughput. Three serialized alternating pairs of
20,000 constructions measured 23.912 ms before and 26.866 ms for the candidate,
12.36% more elapsed time, with zero paired wins. Both roles restored the raw
cppyy initializer before installing exactly one frozen Python wrapper.
Independent acceptance checked all 113 unchanged runtime files, native tick
checksums, analytical tick arithmetic and independently owned result storage.

An initial candidate changed very large integer conversion diagnostics from
``TypeError`` to ``OverflowError``. A narrowed candidate kept integers above
1,023 bits on the original dispatcher and passed the boundary comparison before
timing. Both candidates remain experiments; production is unchanged. The
narrowed candidate also matched all 31 native public constructor rows and
passed the controlled route/lifetime contract. Boundary checks include recovery
after errors; nonfinite and out-of-range observations do not define portable
native behavior. Evidence: ``build/performance/time-unit-overload/``.

The performance gate failed, so no ten-pair confirmation, integration or package
validation is pending for this candidate. Accepted accounting remains 25
workload slices across 20 performance commits.

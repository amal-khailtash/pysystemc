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

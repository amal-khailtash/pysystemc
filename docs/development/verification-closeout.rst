Python 3.14 verification closeout
=================================

The five remaining baseline obligations have concrete dispositions: three
speculative compatibility branches are retired, and both runtime native
member-pointer assembly routes have a supported typed adapter. Final integrated
regression passed all 996 tests with zero failures, errors or skips in
1966.56 seconds. All 113 runtime files match the source, wheel,
source archive and isolated installed package. The earlier clean 995-test result
remains historical evidence.

Baseline obligation dispositions
--------------------------------

.. list-table:: Disposition of the five former remaining obligations
   :header-rows: 1
   :widths: 30 20 50

   * - Item
     - Status
     - Evidence and public behavior
   * - Resolved-read fallback helper
     - Retired
     - Native ``sc_signal_resolved`` inherits public virtual ``read()``.
       Native and bridge values match before and after a channel update.
       No supported missing-read configuration was established.
   * - Resolved-read fallback installation
     - Retired
     - Native reflection exposes inherited read, including private-hiding
       probes. An assignment to an incomplete reflected class is replaced
       when its native definition arrives; that is not a supported fallback.
   * - Missing socket accessor fallback
     - Retired
     - All six simple, tagged and multi callback socket factory products expose
       both inherited port accessors. Missing reflection now raises an API
       error. Indexed and direct interface access configure blocking transport
       for Python thread waits.
   * - Runtime member pointer through ``sc_assemble_vector``
     - Supported through symbol capture
     - ``member_type.from_pointer("namespace::selection")`` copies the current
       native variable into a typed token. Independent C++ equivalence checks
       member identity, arbitrary width, null, const and inherited conversion.
   * - Runtime member pointer through ``vector.assemble``
     - Supported through symbol capture
     - The same token supports the member API. Later variable changes affect
       new tokens; copies preserve their original pointer. Error recovery and
       retained vector ownership have regression checks.

Retirement does not mean an unavailable compatibility branch was exercised.
Raw cppyy 3.5 member-pointer views still cannot be safely marshalled. Passing
such a view raises an actionable ``TypeError``; callers capture the native
variable by qualified identifier instead. Arbitrary C++ expressions are rejected.

The baseline ledger now has 147/147 obligations addressed: 132 exercised and
15 retired, including these two adapted routes and three retired branches.
All final gates for these finite obligations passed. Exhaustive verification of every public API contract remains incomplete.
The 12,681 declaration reviews and 292 accepted finite categories describe
source review and selected behavior; they do not establish every template,
configuration, error, GIL/thread or lifetime contract.

Validation scope and finish line
--------------------------------

Fresh Python statement coverage is **4,587/4,594 (99.85%)**,
with **7 missed statements, all classified**, and zero excluded lines.
The coverage run passed all 996 tests with zero failures, errors or skips in
2102.14 seconds. Independent acceptance reconciled the XML and log,
coverage totals, current statement locations and all 113 runtime source hashes.
Python statement coverage does not instrument the native C++ bridge or certify
every public API contract.

The earlier 4,539/4,548 snapshot had nine missed statements. Four of those
statements were removed by the committed fallback and raw-view fixes; five still
exist. Real native metadata probes confirm that the current member policy has
no reflected ``member_type`` alias and the callback setter wrappers have no
``__overload__`` selector. Their metadata-forwarding branches remain conditional
and unexercised. Defensive error guards are retained and recorded separately.
No synthetic private failure is reported as supported native behavior.

Every current miss, its exact source statement and its classification is in
``docs/_static/verification-current-coverage.json``. This closes the current
coverage measurement and classification gate; the remaining unexercised guards
and metadata conditions remain visible.

The 60 accepted example audits are reused. Broader example inputs, topologies,
process lifecycles and trace variants remain outside the selected scopes.
Python 3.14 is the baseline; Python 3.15 is deferred.

Independent acceptance reconciled all 996 JUnit testcase outcomes and counters
with the pytest log and current runtime hashes. Four fresh installed-package
models passed native pointer capture, resolved reads, socket accessors and
thread/reset/kill behavior. No validation or implementation item remains pending
for this finite closeout.
Evidence and exact disposition counts are recorded in
``docs/_static/verification-closeout.json``.

Final installed-example acceptance
----------------------------------

The final Python 3.14 wheel passes all 41 selected installed comparator tests,
with zero failures, errors or skips. All 113 installed runtime source files
match both the checkout and the accepted wheel archive. Console smoke,
behavior and native equivalence checks pass with checkout imports excluded.

The current inventory has 43 runnable entrypoints. The earlier count of 42
omitted ``examples/counter/py/tb.py``, the conventional counter testbench alias;
the accepted sensitivity comparator runs both that alias and
``counter_tb.py``. This inventory correction adds no new model behavior claim.
Broader inputs, topologies, lifecycle and trace variants remain outside this
finite acceptance. Counts, artifact identity and evidence digests are recorded
in ``docs/_static/verification-final-installed-examples.json``.

Source-review ledger completion
-------------------------------

The runtime review ledger previously listed 111 files and omitted
``_bootstrap.py`` and ``sc_core/event_list_registration.h``. Both files have
now been read completely and matched to current package hashes and existing
assertions. All 113 packaged runtime paths have source-review entries. Existing
review statuses are preserved; adding an entry does not certify every runtime
contract.

The bootstrap audit checks private loader preparation, exact environment
restoration, fallback and cleanup. Its 32 focused cases pass in the accepted
regression. Concurrent native or newly starting threads can observe the temporary
process-wide ``PATH``; arbitrary import and filesystem races remain outside the
selected tests. The event-list audit checks borrowed process identity,
unpublished registration cleanup and list ownership. All 14 rollback cases pass,
including two partial-registration shapes. These tests inject ``bad_alloc`` after
real registration; they do not establish recovery from genuine allocator
exhaustion or priority-queue corruption. No concrete defect was found in either
source review.

The added entries, source hashes, selected testcase names and limits are recorded
in ``docs/_static/runtime-source-review-completion.json``. Broader API contract
verification remains incomplete.

Performance exploration remains paused. The 25 accepted workload slices across
20 performance commits retain their individual scopes. There is no measured
package-wide cumulative gain or tracking-overhead comparison. No new benchmark
or optimization screen is part of this finite closeout.

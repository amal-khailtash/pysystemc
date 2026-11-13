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

Historical coverage remains 4,539/4,548 with nine classified residual statements.
Removing compatibility branches changes the current source, so those counts
are retained as a historical snapshot rather than relabeled as current coverage.
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

Performance exploration remains paused. The 25 accepted workload slices across
20 performance commits retain their individual scopes. There is no measured
package-wide cumulative gain or tracking-overhead comparison. No new benchmark
or optimization screen is part of this finite closeout.

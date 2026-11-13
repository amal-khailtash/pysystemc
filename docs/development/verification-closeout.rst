Python 3.14 verification closeout
=================================

The tested Python 3.14 baseline has a clean regression result: 995 tests passed,
with zero failures, errors or skips. Independent acceptance reconciled the
JUnit testcase outcomes and counters with the pytest log. All 113 runtime
source files match the accepted package manifest, and all 60 example source
files match their accepted source-audit hashes.

This closes the regression and source-audit gates for the tested baseline
scopes. Exhaustive verification of every public API contract remains incomplete.
The 12,681 declaration reviews and 292 accepted finite categories describe
source review and selected behavior; they do not establish every template,
configuration, error, GIL/thread or lifetime contract.

Five remaining baseline items
-----------------------------

The original baseline ledger has 142 of 147 obligations addressed. The five
remaining items are explicit limitations, with no failing regression test in
the selected public alternatives:

.. list-table:: Remaining baseline obligations
   :header-rows: 1
   :widths: 35 20 45

   * - Item
     - Status
     - Tested public path or condition for further work
   * - Resolved-read fallback helper
     - Configuration-dependent; untested branch
     - Native resolved reads have parity tests. The helper requires a real
       supported configuration without the reflected native read method.
   * - Resolved-read fallback installation
     - Configuration-dependent; untested branch
     - Native read is available on the tested baseline. A missing-method
       configuration is needed to validate installation without replacing
       native reflection artificially.
   * - Missing socket accessor fallback
     - Configuration-dependent; untested branch
     - Concrete available socket accessors have regression evidence. This
       compatibility branch depends on the socket family and reflected API.
   * - Raw pointer-to-member through ``sc_assemble_vector``
     - Unsupported by cppyy 3.5
     - Typed member tokens and named member access have native parity tests.
       Retesting the raw route requires a supported member-pointer representation
       or a safe adapter.
   * - Raw pointer-to-member through ``vector.assemble``
     - Unsupported by cppyy 3.5
     - Typed tokens and named members remain the supported routes. A raw pointer
       conversion failure is not a failure of those alternatives.

These five capability obligations are distinct from the nine residual statements
in the historical 4,539/4,548 coverage snapshot. Coverage has not been remeasured
by this closeout. Broader example inputs, topologies, process lifecycles and trace
variants also remain outside the accepted selected scopes. Python 3.15 is deferred.

Finish line and further work
----------------------------

No test, build or installation gate remains pending for the accepted baseline
artifacts within their recorded scopes. Further correctness work requires a
specific failing public case, a supported configuration that reaches a conditional
branch, or a defined expansion of the tested contract set. Those items remain
visible; this closeout does not relabel them as passing or exhaustively verified.

Performance exploration is paused at this closeout. The 25 accepted workload
slices across 20 performance commits retain their individual scopes. Rejected
experiments do not close correctness obligations and do not count as gains.
There is no measured package-wide cumulative gain or tracking-overhead comparison.

Evidence is summarized in ``docs/_static/verification-closeout.json``. The fresh
regression XML and independent reconciliation are retained under
``build/verification/full-suite-995-root-*`` and
``build/verification/baseline-closeout/root.json``.

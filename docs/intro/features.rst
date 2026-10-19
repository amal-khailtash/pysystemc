Features
========

Features marked ☑ have been implemented and tested for the scope described.
Items marked ☐ remain incomplete or untested.

See :doc:`../api/coverage` for tested combinations,
:doc:`../api/bindings` for usage and lifetime rules, and
:doc:`development` for future development.

Environment and runtime
-----------------------

* ☑ Linux, Python 3.14, C++17 and SystemC 2.3.4 baseline with native pthread processes.
* ☑ Runtime template instantiation with arbitrary native-valid widths and types;
  no fixed precompiled specialization registry.
* ☑ uv dependency locking, synchronization and native distribution builds through uv_build.
* ☑ CMake and Autotools native preparation with a shared discovered installation prefix.
* ☑ Header/library discovery, external installation overrides and dependency diagnostics.
* ☑ Cling configuration, native ABI configuration and CPyCppyy header compatibility repair.
* ☑ Installed-header operation without reading kernel implementation source at import.
* ☑ One-time native library/bridge loading and deliberate Python namespace exports/help.
* ☐ Broader operating-system, compiler, Python and SystemC version matrices.

Modules, processes and simulation
---------------------------------

* ☑ Python module hosts, unique process registration and independent callback dispatch.
* ☑ Native module hierarchy and lifecycle hooks in elaboration/simulation order.
* ☑ Static sensitivity to interfaces, ports, events and clock edges; dont_initialize.
* ☑ Method, thread, clocked-thread and dynamically spawned processes.
* ☑ Guarded timed, event, OR/AND event-list and cycle waits; dynamic next_trigger.
* ☑ Dynamic joins and process termination events.
* ☑ Selected synchronous/asynchronous reset and kill cleanup through guarded Python waits.
* ☑ Bounded simulation, pause/resume, stop, simulation status and terminal lifecycle hooks.
* ☑ Entry callable/argv handling, exception propagation and idempotent shutdown.
* ☑ Python process callback retention and selected native shutdown release checks.
* ☐ Arbitrary throw_it, direct native waits and channel-internal reset/unwind completeness.
* ☐ Full process-control, lifecycle-error and live host/kernel deletion matrices.

Simulation contexts follow native singleton semantics. Independent simulations
run in fresh subprocesses. Reset/kill requires a running SystemC process;
guarded Python waits preserve scheduler unwind. Python cleanup must propagate
unwind and must not wait again. Restarting a stopped kernel is not supported.

Channels, ports and modeling utilities
--------------------------------------

* ☑ Native clocks, signals, buffers and input/output/inout ports with update scheduling.
* ☑ Binding diagnostics, hierarchy and explicit export get_interface forwarding.
* ☑ Immediate, delta and timed events, cancellation and event queues.
* ☑ Blocking FIFO exchange and bounded FIFO port traffic on pthreads.
* ☑ Native mutex/semaphore wakeup and scoped locking.
* ☑ Resolved signals/ports and selected writer-contention policies.
* ☑ One-writer, many-writer and unchecked writer template policies in tested cases.
* ☑ Native vectors, member assemblies and attribute ownership operations.
* ☑ Report handlers, action masks, severity/id precedence, suppression and forcing.
* ☑ Hierarchy traversal, status/version/time introspection and namespace help.
* ☐ Complete channel, port-policy, vector, report and event overload/type/error audits.

Datatypes and tracing
---------------------

* ☑ Native signed/unsigned fixed-width and arbitrary-width large integer templates.
* ☑ Selected overflow, assignment, comparison, conversion and bit/range mutation checks.
* ☑ Bit vectors, four-state logic vectors and typed signal/port propagation.
* ☑ Dynamic signed/unsigned/vector storage and writable concatenations.
* ☑ Fixed-point and fast fixed-point construction, arithmetic and selected policies.
* ☑ Named VCD tracing of stable native storage with declarations and transitions checked.
* ☑ Selected scalar, vector, resolved, fixed-point, time and event tracing.
* ☑ Selected WIF tracing and explicit timescale configuration.
* ☑ Deterministic trace closure, including Python exception paths.
* ☐ Complete datatype/proxy/concat/implicit-conversion and overload matrices.
* ☐ Complete fixed-point quantization/overflow and tracing type/format matrices.

Native datatype validity rules apply: sc_int/sc_uint are limited to 64 bits;
sc_bigint/sc_biguint widths are parameterized. Tested widths are evidence rather
than a supported-width registry. Trace storage must survive native use.

Public binding implementation and audit
---------------------------------------

M6A remains incomplete. The fixed requirements below show the remaining work;
delivered behavior is summarized separately under each group. Partial evidence
keeps the corresponding requirement open. The IDs match the development plan.

A. Public declaration inventory and Python mapping — incomplete
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☐ **A1** — Inventory every public declaration/overload in systemc/sysc.
* ☐ **A2** — Inventory every public declaration/overload in tlm/tlm_core.
* ☐ **A3** — Inventory every public declaration/overload in tlm_utils.
* ☐ **A4** — Map each inventory entry to Python spelling, ownership, GIL, errors and evidence.
* ☐ **A5** — Classify internal facilities with source-based rationale.
* ☐ **A6** — Reconcile inventory with examples and identify APIs absent from examples.

Implemented and tested: The Clang extractor retains overloads, defaults, bases, specialization
arguments, namespace friends, active macro locations and configuration metadata. Review input
validates records atomically.

Still required: Complete source classification and semantic mappings remain unreviewed. Inactive
branches, qualified friends and inherited/specialized members need reconciliation; emitted rows do
not prove public API completeness.

B. Custom interfaces and channels — partially implemented
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☐ **B1** — Complete native signal/FIFO scalar/type/partial-interface/reset/error/reference matrices.
* ☐ **B2** — Shared interface/event identity across different adapter specializations.
* ☐ **B3** — Adapter teardown and release of retained callback owners before shutdown.
* ☐ **B4** — Complete custom-channel error/reset/GIL/access-path matrix.

Implemented and tested: Structural ports/exports, explicit reference trampolines, signal/FIFO
adapters, mutex/semaphore/event queues and callback result/copy contracts are implemented for tested
cases. Wide references, scalar storage and timed native FIFO/synchronization comparisons pass.

Still required: Complete type/conversion/tracing/reset/error coverage remains open, including plain
char/floating policies. Shared owner events work, but cross-specialization interface identity and
pre-shutdown owner/result release are incomplete.

C. TLM interface families — partial
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☐ **C1** — Full public overload/type/event/lifetime and independent native reference matrices.

Implemented and tested: TLM1 put/get/peek/transport/analysis, combined/master/slave/FIFO families
and TLM2 transport/debug/DMI adapters have selected semantic tests. Callback types/bounds, copied
write values and failed get/peek output preservation are checked.

Still required: Reconcile every public overload, type, readiness event, lifetime and
native-reference case. Adapter-owned readiness events and complete owner/type/reset matrices still
require review.

D. Callable sockets — partial
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☐ **D1** — Independent native multi socket reference matrices.
* ☐ **D2** — Complete multi/passthrough hierarchy, conversion and all native access paths.
* ☐ **D3** — Complete tagged custom protocol, callback error, reset and lifetime matrices.
* ☐ **D4** — Complete socket public overload/template inventory and owner teardown contract.

Implemented and tested: Simple/tagged/multi/passthrough registrations, selected
hierarchy/access/policy/custom payload cases and callback contracts are tested. An independent
width-73 native multi reference now matches 12 records across two connections.

Still required: The native multi reference is representative: custom protocols, topology/access
variants and lifetime breadth remain open. Complete multi/passthrough hierarchy, tagged
custom/reset/error matrices, overload inventory and teardown remain required.

E. TLM timing, payloads, extensions and endian utilities — partial
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☐ **E1** — Complete generic/aligned native type and byte-enable reference matrices.
* ☐ **E2** — Complete extension clone/copy/free/mm and payload memory-manager ownership matrices.
* ☐ **E3** — Complete timing/extension/endian public declarations and context/lifetime audit.

Implemented and tested: Guarded and custom quantumkeeper hooks, queues, selected extension lifecycle
checks and endian conversion families have executable evidence. Word and selected generic/single
cases have independent native references.

Still required: Generic/aligned native type/byte-enable matrices, extension clone/copy/free/mm
ownership and full utility declaration/context/lifetime review remain incomplete.

F. Core semantics and final gates — incomplete
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☐ **F1** — Full datatype/proxy/concat/implicit-conversion/overload audit.
* ☐ **F2** — Full writer/port/fixed-point policy audit.
* ☐ **F3** — Full tracing type/format and vector assembly audit.
* ☐ **F4** — Full report/process-control/lifecycle error audit.
* ☐ **F5** — Implement applicable arbitrary throw_it, channel-internal reset and direct-wait behavior.
* ☐ **F6** — Verify host/kernel lifetime and adapter ownership release.
* ☐ **F7** — Reconciled full regression and coverage for the complete public API.
* ☐ **F8** — Review first-party adapters and verify latest installed wheel and sdist.
* ☐ **F9** — Verify every selected public-contract entry; obtain explicit scope decisions for unrepresentable APIs.

Implemented and tested: Core modeling, selected guarded process control and API guides are
implemented. Release 0.10.0 passed 156 tests, wheel/sdist build and installed
runtime/pthread/console checks; later multi groups passed targeted checks.

Still required: Release validation does not close the final M6A gate. Full source-derived
datatype/policy/tracing/vector/report/process/lifetime audits and final reconciled coverage/artifact
validation remain required.

Examples and C++ interoperability
---------------------------------

* ☑ Finite hello/counter/FIFO tutorials with semantic assertions and waveform checks.
* ☑ Selected upstream ports with independent native value/time/order comparisons,
  including fork/join, barrier, report, reset and hierarchical exports.
* ☑ Mixed Python/C++ models with native ports crossing the language boundary.
* ☑ Installed C++ runner and console entry point with source/argument/error handling.
* ☐ Complete inventory of every upstream executable variant and its dependencies.
* ☐ Complete Python ports of all upstream sysc and tlm groups with native equivalence.
* ☐ Per-variant documentation, licensing, regression and installed-artifact checks.

The following upstream example groups still need a complete variant audit and
Python/native comparison:

* ☐ ``sysc/2.1/dpipe``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/2.1/forkjoin``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/2.1/reset_signal_is``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/2.1/sc_export``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/2.1/sc_report``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/2.1/scx_barrier``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/2.1/scx_mutex_w_policy``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/2.1/specialized_signals``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/2.3/sc_rvd``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/2.3/sc_ttd``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/2.3/simple_async``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/fft/fft_flpt``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/fft/fft_fxpt``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/fir``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/pipe``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/pkt_switch``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/risc_cpu``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/rsa``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/simple_bus``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/simple_fifo``: complete executable-variant audit and Python/native equivalence.
* ☐ ``sysc/simple_perf``: complete executable-variant audit and Python/native equivalence.
* ☐ ``tlm/at_1_phase``: complete executable-variant audit and Python/native equivalence.
* ☐ ``tlm/at_2_phase``: complete executable-variant audit and Python/native equivalence.
* ☐ ``tlm/at_4_phase``: complete executable-variant audit and Python/native equivalence.
* ☐ ``tlm/at_extension_optional``: complete executable-variant audit and Python/native equivalence.
* ☐ ``tlm/at_mixed_targets``: complete executable-variant audit and Python/native equivalence.
* ☐ ``tlm/at_ooo``: complete executable-variant audit and Python/native equivalence.
* ☐ ``tlm/lt``: complete executable-variant audit and Python/native equivalence.
* ☐ ``tlm/lt_dmi``: complete executable-variant audit and Python/native equivalence.
* ☐ ``tlm/lt_extension_mandatory``: complete executable-variant audit and Python/native equivalence.
* ☐ ``tlm/lt_mixed_endian``: complete executable-variant audit and Python/native equivalence.
* ☐ ``tlm/lt_temporal_decouple``: complete executable-variant audit and Python/native equivalence.

Packaging, documentation and quality
------------------------------------

* ☑ Native wheel/sdist packaging with dependency resources and license notices.
* ☑ manylinux distribution build and repaired-wheel consumer verification workflow.
* ☑ Tag-driven trusted PyPI publication and documentation build triggers.
* ☑ Semantic regression CI, lint/format checks and strict Sphinx documentation builds.
* ☑ Quickstart, architecture/lifetime, API guides, examples and troubleshooting.
* ☑ Optional native sanitizer/leak validation tooling.
* ☐ Final full regression, measured coverage and installed wheel/sdist gates for the complete public API.
* ☐ Complete public declaration-to-Python semantic mapping and upstream reconciliation.

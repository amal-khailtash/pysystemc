Features
========

Features marked ☑ are complete for the scope described: runtime items are
implemented and tested; inventory items record source contracts and explicit gaps.
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

A. Public declaration inventory and Python mapping — inventoried
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☑ **A1** — Inventory pinned public declarations/overloads in systemc/sysc.
* ☑ **A2** — Inventory pinned public declarations/overloads in tlm/tlm_core.
* ☑ **A3** — Inventory pinned public declarations/overloads in tlm_utils.
* ☑ **A4** — Record Python call candidates or explicit unverified mappings, ownership, GIL, errors and evidence.
* ☑ **A5** — Classify internal facilities with source-based rationale.
* ☑ **A6** — Reconcile recorded example targets and identify families absent from examples.

The source inventory is closed for pinned SystemC revision
``5fc1469339775b54b1fcc2020ac744a58be5b50e``: 15,778 declaration identities,
including 12,701 public and 3,077 internal entries. Both modeling and compatibility
configurations, recorded active/inactive macros, friends, aliases and inheritance
links are accounted for. These checkmarks describe inventory completion.

Python contracts preserve 11,642 partial mappings and 1,059 reviewed mappings;
partial means exact binding behavior remains unverified. Source evidence and
candidate Python syntax do not establish runtime support. Template instantiations,
ADL, inherited access/ambiguity, callback retention, GIL and error behavior still
require validation where recorded as untested. Known failures remain failing.

All 33 upstream executable targets have source/input/configuration records:
All 33 native/Python baseline comparisons pass (performance FIFO uses a fixed seed). Alternate inputs, platforms and build configurations remain untested.
The remaining implementation work stays in the existing groups below; the local
per-declaration inventory and example ledgers retain the detailed evidence.

B. Custom interfaces and channels — partially implemented
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☐ **B1** — Complete native signal/FIFO scalar/type/partial-interface/reset/error/reference matrices.
* ☐ **B2** — Shared interface/event identity across different adapter specializations.
* ☐ **B3** — Adapter teardown and release of retained callback owners before shutdown.
* ☐ **B4** — Complete custom-channel error/reset/GIL/access-path matrix.

Implemented and tested: Structural ports/exports, explicit reference trampolines, signal/FIFO
adapters, mutex/semaphore/event queues and callback result/copy contracts are implemented for tested
cases. Wide references, scalar storage and timed native FIFO/synchronization comparisons pass.
Floating signal read/get_data_ref callbacks reject nonnumeric values, preserve
sampled storage on rejection and recover for float/double/long double.
``release_interface_callbacks()`` releases structural owners and retained results
after native stop; adapters remain alive and later callbacks reject access.
Guarded structural FIFO read/write and TLM get/put/peek callbacks preserve
reset/kill cleanup ordering in timed native-process tests.
Each path covers timed, event, OR/AND, timed-event/list, cycle and counted-cycle
guarded waits with explicit static clock sensitivity for int and 137-bit
unsigned values; wide write/put callback arguments retain their high bits.
Scalar/wide and TLM analysis owner collection, plus replaced-reference
collection, are checked after final
end-of-simulation reads; cleanup during a running process is rejected.
Existing combined adapters are reused for compatible inherited signal/FIFO/TLM
views. Scalar FIFO reads use native pair callbacks, including signed/unsigned
64-bit results; integral range/type failures preserve native outputs and recover.

Still required: Complete type/conversion/tracing/reset/error coverage remains open, including plain
char/floating policies. Shared owner events work, but cross-specialization interface identity and
pre-shutdown owner/result release are incomplete.
Complete signal/FIFO owners select combined adapters even when first bound
through a partial interface. Complete TLM get/peek owners likewise select a
combined adapter through partial-first bindings. Transport protocol mapping and
complete cross-specialization/lifetime matrices remain open.

C. TLM interface families — partial
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☐ **C1** — Full public overload/type/event/lifetime and independent native reference matrices.

Implemented and tested: TLM1 put/get/peek/transport/analysis, combined/master/slave/FIFO families
and TLM2 transport/debug/DMI adapters have selected semantic tests. Callback types/bounds, copied
write values and failed get/peek output preservation are checked.
Inherited get/peek views share their corresponding combined readiness events;
delayed notification count/time matches an independent native reference.

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
* ☑ Inventory all 33 recorded upstream executable targets and source dependencies; alternate runtime equivalence remains untested.
* ☐ Complete Python ports of all upstream sysc and tlm groups with native equivalence.
* ☐ Per-variant documentation, licensing, regression and installed-artifact checks.

The following list records completion of the executable-variant audit and
Python/native comparison for the selected Linux/Python 3.14/pthread baseline.
Common compiler/platform build alternatives remain outside this baseline; see
:doc:`examples` for audited inputs and semantic checks.

* ☑ ``sysc/2.1/dpipe``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/2.1/forkjoin``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/2.1/reset_signal_is``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/2.1/sc_export``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/2.1/sc_report``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/2.1/scx_barrier``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/2.1/scx_mutex_w_policy``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/2.1/specialized_signals``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/2.3/sc_rvd``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/2.3/sc_ttd``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/2.3/simple_async``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/fft/fft_flpt``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/fft/fft_fxpt``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/fir``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/pipe``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/pkt_switch``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/risc_cpu``: All ten Python CPU components match native default instruction/register records and shutdown timing; alternate images remain untested.
* ☑ ``sysc/rsa``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/simple_bus``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/simple_fifo``: executable-variant audit and Python/native equivalence on the selected baseline.
* ☑ ``sysc/simple_perf``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.
* ☑ ``tlm/at_1_phase``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.
* ☑ ``tlm/at_2_phase``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.
* ☑ ``tlm/at_4_phase``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.
* ☑ ``tlm/at_extension_optional``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.
* ☑ ``tlm/at_mixed_targets``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.
* ☑ ``tlm/at_ooo``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.
* ☑ ``tlm/lt``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.
* ☑ ``tlm/lt_dmi``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.
* ☑ ``tlm/lt_extension_mandatory``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.
* ☑ ``tlm/lt_mixed_endian``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.
* ☑ ``tlm/lt_temporal_decouple``: supplied stimulus native/Python value/time comparison passed; alternate configurations remain untested.

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

Fixed public API feature inventory
----------------------------------

The following list contains every public family in the pinned source inventory,
including constants, aliases, functions, classes and their overloads. Families
are grouped by native namespace and enclosing public name. Each count refers to
unique declaration identities, including compatibility declarations; it is not a
count of working Python features. These are details of the existing A–F scope,
not additional requirements discovered during implementation.

Download :download:`the complete per-declaration inventory <../_static/public-api-inventory.csv>`
for all 12,701 public entries, exact native signatures and Python candidate
spellings, ownership, GIL, errors, source evidence and individual statuses.
The downloadable file is committed documentation, independent of local AI records.

**Status rules:** ``untested`` means runtime semantics have not been verified for
that declaration's complete scope. ``failing`` records a known binding failure.
``partial`` describes unresolved Python mapping evidence; ``reviewed`` describes
a source/call-form review and does not establish runtime support. Representative
supported combinations appear earlier on this page and in :doc:`../api/coverage`.
The conservative declaration statuses deliberately avoid promoting representative
tests to support for every overload. Internal entries are excluded from this
public feature list; all 3,077 retain source classification in the local audit.

sc_core public families
~~~~~~~~~~~~~~~~~~~~~~~

* ☐ ``sc_core::CFT`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::PHASH_DEFAULT_GROW_FACTOR`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::PHASH_DEFAULT_INIT_TABLE_SIZE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::PHASH_DEFAULT_MAX_DENSITY`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::PHASH_DEFAULT_REORDER_FLAG`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_ABORT`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_ALL_BOUND`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_BEFORE_END_OF_ELABORATION`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_BEFORE_TIMESTEP`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_BIND_PROXY_NIL`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_CACHE_REPORT`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_CTHREAD_PROC_`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_DEBUG`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_DEFAULT_CATCH_ACTIONS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_DEFAULT_ERROR_ACTIONS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_DEFAULT_FATAL_ACTIONS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_DEFAULT_INFO_ACTIONS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_DEFAULT_STACK_SIZE`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_DEFAULT_WARNING_ACTIONS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_DISPLAY`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_DO_NOTHING`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_ELABORATION`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_END_OF_ELABORATION`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_END_OF_INITIALIZATION`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_END_OF_SIMULATION`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_END_OF_UPDATE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_ERROR`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_EXIT_ON_STARVATION`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_FATAL`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_FS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_FULL`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_HIERARCHY_CHAR`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_HIGH`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_ID_ABORT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_ASSERTION_FAILED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_ASSIGNMENT_FAILED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_ATTEMPT_TO_BIND_CLOCK_TO_OUTPUT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_ATTEMPT_TO_WRITE_TO_CLOCK_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_BACK_ON_EMPTY_LIST_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_BAD_SC_MODULE_CONSTRUCTOR_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_BIND_IF_TO_PORT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_BIND_PORT_TO_PORT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_CANNOT_CONVERT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_CLOCK_HIGH_TIME_ZERO_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_CLOCK_LOW_TIME_ZERO_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_CLOCK_PERIOD_ZERO_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_COMPLETE_BINDING_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_CONTEXT_BEGIN_FAILED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_CONTEXT_END_FAILED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_CONVERSION_FAILED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_CYCLE_MISSES_EVENTS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_DEFAULT_TIME_UNIT_CHANGED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_DISABLE_WILL_ORPHAN_PROCESS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_DONT_INITIALIZE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_EMPTY_PROCESS_HANDLE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_END_MODULE_NOT_CALLED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_EVENT_LIST_FAILED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_EVENT_ON_NULL_PROCESS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_EXPORT_OUTSIDE_MODULE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_FIND_EVENT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_FRONT_ON_EMPTY_LIST_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_GEN_UNIQUE_NAME_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_GET_IF_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_HALT_NOT_ALLOWED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_HIER_NAME_INCORRECT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_IEEE_1666_DEPRECATION_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_ILLEGAL_CHARACTERS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_IMMEDIATE_NOTIFICATION_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_IMMEDIATE_SELF_NOTIFICATION_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INCOMPATIBLE_TYPES_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INCOMPATIBLE_VECTORS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INCONSISTENT_API_CONFIG_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INIT_FAILED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INSERT_EXPORT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INSERT_MODULE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INSERT_PORT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INSERT_PRIM_CHANNEL_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INSTANCE_EXISTS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INTERNAL_ERROR_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INVALID_CTE_WL_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INVALID_DIV_WL_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INVALID_FIFO_SIZE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INVALID_FX_VALUE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INVALID_MAX_WL_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INVALID_N_BITS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INVALID_O_MODE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INVALID_SEMAPHORE_VALUE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_INVALID_WL_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_JOIN_ON_METHOD_HANDLE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_KILL_PROCESS_WHILE_UNITIALIZED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_LENGTH_MISMATCH_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_LOGIC_X_TO_BOOL_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_LOGIC_Z_TO_BOOL_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_MAKE_SENSITIVE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_MAKE_SENSITIVE_NEG_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_MAKE_SENSITIVE_POS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_METHOD_TERMINATION_EVENT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_MODULE_CTHREAD_AFTER_START_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_MODULE_METHOD_AFTER_START_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_MODULE_NAME_STACK_EMPTY_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_MODULE_THREAD_AFTER_START_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_MORE_THAN_ONE_FIFO_READER_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_MORE_THAN_ONE_FIFO_WRITER_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_MORE_THAN_ONE_SIGNAL_DRIVER_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NAME_EXISTS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NEXT_TRIGGER_NOT_ALLOWED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NOTIFY_DELAYED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NOT_EXPECTING_DYNAMIC_EVENT_NOTIFY_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NOT_IMPLEMENTED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NO_ASYNC_UPDATE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NO_BOOL_RETURNED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NO_DEFAULT_EVENT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NO_INT_RETURNED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NO_PROCESS_SEMANTICS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NO_SC_LOGIC_RETURNED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_NO_SC_START_ACTIVITY_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_OPERAND_NOT_BOOL_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_OPERAND_NOT_SC_LOGIC_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_OPERATION_FAILED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_OPERATION_ON_NON_SPECIALIZED_SIGNAL_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_OUT_OF_BOUNDS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_OUT_OF_RANGE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_PHASE_CALLBACKS_UNSUPPORTED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_PHASE_CALLBACK_FORBIDDEN_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_PHASE_CALLBACK_NOT_IMPLEMENTED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_PHASE_CALLBACK_REGISTER_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_PORT_OUTSIDE_MODULE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_PROCESS_ALREADY_UNWINDING_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_PROCESS_CONTROL_CORNER_CASE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_REGISTER_ID_FAILED_`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::SC_ID_REMOVE_MODULE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_REMOVE_PORT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_REMOVE_PRIM_CHANNEL_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_RESET_PROCESS_WHILE_NOT_RUNNING_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_RESOLVED_PORT_NOT_BOUND_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_RETHROW_UNWINDING_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SC_BV_CANNOT_CONTAIN_X_AND_Z_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SC_EXPORT_ALREADY_BOUND_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SC_EXPORT_HAS_NO_INTERFACE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SC_EXPORT_NOT_BOUND_AFTER_CONSTRUCTION_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SC_EXPORT_NOT_REGISTERED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SC_MODULE_NAME_REQUIRED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SC_MODULE_NAME_USE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SET_DEFAULT_TIME_UNIT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SET_STACK_SIZE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SET_TIME_RESOLUTION_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SIMULATION_START_AFTER_ERROR_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SIMULATION_START_AFTER_STOP_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SIMULATION_START_UNEXPECTED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SIMULATION_STOP_CALLED_TWICE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SIMULATION_TIME_OVERFLOW_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_SIMULATION_UNCAUGHT_EXCEPTION_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_STACK_SETUP_FAILED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_STOP_MODE_AFTER_START_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_STRING_TOO_LONG_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_THROW_IT_IGNORED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_THROW_IT_WHILE_NOT_RUNNING_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TIME_CONVERSION_FAILED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_ALREADY_INITIALIZED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_CLOSE_EMPTY_FILE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_FOPEN_FAILED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_INVALID_ENUM_VALUE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_INVALID_TIMESCALE_UNIT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_OBJECT_IGNORED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_OBJECT_NAME_FILTERED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_REVERSED_TIME_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_TIMESCALE_DEFAULT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_TIMESCALE_UNIT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_VCD_DELTA_CYCLE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_TRACING_VCD_TIME_RESOLUTION_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_UNKNOWN_ERROR_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_UNKNOWN_PROCESS_TYPE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_VALUE_NOT_VALID_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_VC6_MAX_PROCESSES_EXCEEDED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_VC6_PROCESS_HELPER_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_VECTOR_BIND_EMPTY_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_VECTOR_CONTAINS_LOGIC_VALUE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_VECTOR_INIT_CALLED_TWICE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_VECTOR_NONOBJECT_ELEMENTS_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_VECTOR_TOO_LONG_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_VECTOR_TOO_SHORT_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_WAIT_DURING_UNWINDING_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_WAIT_NEGATIVE_CYCLES_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_WAIT_NOT_ALLOWED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_WAIT_N_INVALID_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_WATCHING_NOT_ALLOWED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_WITHOUT_MESSAGE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_WRAP_SM_NOT_DEFINED_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_WRONG_VALUE_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_ID_ZERO_LENGTH_`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::SC_INCLUDE_DESCENDANTS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_INFO`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_INTERRUPT`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_INVALID_DESCENDANTS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_LOG`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_LOW`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_MANY_WRITERS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_MAX_SEVERITY`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_MEDIUM`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_METHOD_PROC_`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_MS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_NONE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_NO_DESCENDANTS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_NO_PROC_`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_NS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_ONE_OR_MORE_BOUND`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_ONE_WRITER`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_PAUSED`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_PS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_RUNNING`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_RUN_TO_TIME`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_SEC`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_SIM_ERROR`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_SIM_OK`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_SIM_USER_STOP`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_START_OF_SIMULATION`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_STATUS_ANY`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_STATUS_LAST`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_STOP`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_STOPPED`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_STOP_FINISH_DELTA`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_STOP_IMMEDIATE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_THREAD_PROC_`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_THROW`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_UNCHECKED_WRITERS`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_UNITIALIZED`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_UNSPECIFIED`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_US`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_WARNING`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_ZERO_OR_MORE_BOUND`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::SC_ZERO_TIME`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::at_negedge`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::at_posedge`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::default_int_hash_fn`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::default_ptr_hash_fn`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::default_str_hash_fn`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::halt`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::next_trigger`` — 12 declarations; runtime: 12 untested; mapping: 12 partial.
* ☐ ``sc_core::notify`` — 3 declarations; runtime: 3 untested; mapping: 3 partial.
* ☐ ``sc_core::operator!=`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::operator%`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::operator&`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::operator*`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::operator+`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::operator-`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::operator/`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::operator<`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::operator<<`` — 8 declarations; runtime: 8 untested; mapping: 8 partial.
* ☐ ``sc_core::operator==`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::operator|`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_abort`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_actions`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_allow_process_control_corners`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_argc`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_argv`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_assemble_vector`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``sc_core::sc_assertion_failed`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_attr_base`` — 5 declarations; runtime: 5 untested; mapping: 3 partial, 2 reviewed.
* ☐ ``sc_core::sc_attr_cltn`` — 17 declarations; runtime: 17 untested; mapping: 3 partial, 14 reviewed.
* ☐ ``sc_core::sc_attribute`` — 7 declarations; runtime: 7 untested; mapping: 4 partial, 3 reviewed.
* ☐ ``sc_core::sc_behavior`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_bind`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_bind_proxy`` — 6 declarations; runtime: 6 untested; mapping: 6 partial.
* ☐ ``sc_core::sc_buffer`` — 14 declarations; runtime: 14 untested; mapping: 6 partial, 8 reviewed.
* ☐ ``sc_core::sc_byte_heap`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``sc_core::sc_channel`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_clock`` — 16 declarations; runtime: 16 untested; mapping: 16 partial.
* ☐ ``sc_core::sc_close_vcd_trace_file`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_close_wif_trace_file`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_copyright`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_copyright_string`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_create_vcd_trace_file`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_create_wif_trace_file`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_cref`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_curr_proc_kind`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_delta_count`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_delta_count_at_current_time`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_descendant_inclusion_info`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_direct_access`` — 13 declarations; runtime: 13 untested; mapping: 13 partial.
* ☐ ``sc_core::sc_elab_and_sim`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_enable_name_checking`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_end_of_simulation_invoked`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_event`` — 30 declarations; runtime: 30 untested; mapping: 9 partial, 21 reviewed.
* ☐ ``sc_core::sc_event_and_expr`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_event_and_list`` — 11 declarations; runtime: 11 untested; mapping: 8 partial, 3 reviewed.
* ☐ ``sc_core::sc_event_expr`` — 26 declarations; runtime: 26 untested; mapping: 26 partial.
* ☐ ``sc_core::sc_event_finder`` — 13 declarations; runtime: 13 untested; mapping: 13 partial.
* ☐ ``sc_core::sc_event_finder_t`` — 13 declarations; runtime: 13 untested; mapping: 13 partial.
* ☐ ``sc_core::sc_event_list`` — 5 declarations; runtime: 5 untested; mapping: 5 partial.
* ☐ ``sc_core::sc_event_or_expr`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_event_or_list`` — 11 declarations; runtime: 11 untested; mapping: 8 partial, 3 reviewed.
* ☐ ``sc_core::sc_event_queue`` — 9 declarations; runtime: 9 untested; mapping: 2 partial, 7 reviewed.
* ☐ ``sc_core::sc_event_queue_if`` — 4 declarations; runtime: 4 untested; mapping: 4 reviewed.
* ☐ ``sc_core::sc_event_queue_port`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_exception`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_export`` — 15 declarations; runtime: 15 untested; mapping: 7 partial, 8 reviewed.
* ☐ ``sc_core::sc_export_base`` — 13 declarations; runtime: 13 untested; mapping: 3 partial, 10 reviewed.
* ☐ ``sc_core::sc_fifo`` — 23 declarations; runtime: 23 untested; mapping: 5 partial, 18 reviewed.
* ☐ ``sc_core::sc_fifo_blocking_in_if`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``sc_core::sc_fifo_blocking_out_if`` — 3 declarations; runtime: 3 untested; mapping: 3 partial.
* ☐ ``sc_core::sc_fifo_in`` — 24 declarations; runtime: 24 untested; mapping: 24 partial.
* ☐ ``sc_core::sc_fifo_in_if`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``sc_core::sc_fifo_nonblocking_in_if`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``sc_core::sc_fifo_nonblocking_out_if`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``sc_core::sc_fifo_out`` — 23 declarations; runtime: 23 untested; mapping: 23 partial.
* ☐ ``sc_core::sc_fifo_out_if`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``sc_core::sc_find_event`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_find_object`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_gen_unique_name`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_get_curr_process_handle`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_get_curr_process_kind`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_get_curr_simcontext`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_get_current_object`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_get_current_process_handle`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_get_default_time_unit`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_get_hierarchical_name`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_get_last_created_process_handle`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_get_parent`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_get_simulator_status`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_get_status`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_get_stop_mode`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_get_time_resolution`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_get_top_level_events`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_get_top_level_objects`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_handle_exception`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_hierarchical_name_exists`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_in`` — 130 declarations; runtime: 130 untested; mapping: 130 partial.
* ☐ ``sc_core::sc_in_clk`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_in_resolved`` — 20 declarations; runtime: 20 untested; mapping: 20 partial.
* ☐ ``sc_core::sc_in_rv`` — 21 declarations; runtime: 21 untested; mapping: 21 partial.
* ☐ ``sc_core::sc_initialize`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_inout`` — 124 declarations; runtime: 124 untested; mapping: 124 partial.
* ☐ ``sc_core::sc_inout_clk`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_inout_resolved`` — 24 declarations; runtime: 24 untested; mapping: 24 partial.
* ☐ ``sc_core::sc_inout_rv`` — 25 declarations; runtime: 25 untested; mapping: 25 partial.
* ☐ ``sc_core::sc_interface`` — 8 declarations; runtime: 8 untested; mapping: 2 partial, 6 reviewed.
* ☐ ``sc_core::sc_is_prerelease`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_is_running`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_is_unwinding`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_join`` — 6 declarations; runtime: 6 untested; mapping: 1 partial, 5 reviewed.
* ☐ ``sc_core::sc_logic_resolution_tbl`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_lv_resolve`` — 3 declarations; runtime: 3 untested; mapping: 3 partial.
* ☐ ``sc_core::sc_max_time`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_member_access`` — 15 declarations; runtime: 15 untested; mapping: 15 partial.
* ☐ ``sc_core::sc_module`` — 62 declarations; runtime: 62 untested; mapping: 62 partial.
* ☐ ``sc_core::sc_module_name`` — 5 declarations; runtime: 5 untested; mapping: 5 partial.
* ☐ ``sc_core::sc_mutex`` — 9 declarations; runtime: 9 untested; mapping: 4 partial, 5 reviewed.
* ☐ ``sc_core::sc_mutex_if`` — 5 declarations; runtime: 5 untested; mapping: 1 partial, 4 reviewed.
* ☐ ``sc_core::sc_name_gen`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_object`` — 32 declarations; runtime: 32 untested; mapping: 7 partial, 25 reviewed.
* ☐ ``sc_core::sc_out`` — 27 declarations; runtime: 27 untested; mapping: 27 partial.
* ☐ ``sc_core::sc_out_clk`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_out_resolved`` — 23 declarations; runtime: 23 untested; mapping: 23 partial.
* ☐ ``sc_core::sc_out_rv`` — 24 declarations; runtime: 24 untested; mapping: 24 partial.
* ☐ ``sc_core::sc_pause`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_pdhash`` — 18 declarations; runtime: 18 untested; mapping: 18 partial.
* ☐ ``sc_core::sc_pdhash_iter`` — 11 declarations; runtime: 11 untested; mapping: 11 partial.
* ☐ ``sc_core::sc_pending_activity`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_pending_activity_at_current_time`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_pending_activity_at_future_time`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_phash`` — 91 declarations; runtime: 91 untested; mapping: 91 partial.
* ☐ ``sc_core::sc_phash_base`` — 30 declarations; runtime: 30 untested; mapping: 30 partial.
* ☐ ``sc_core::sc_phash_base_iter`` — 14 declarations; runtime: 14 untested; mapping: 14 partial.
* ☐ ``sc_core::sc_phash_elem`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_phash_iter`` — 13 declarations; runtime: 13 untested; mapping: 13 partial.
* ☐ ``sc_core::sc_plist`` — 16 declarations; runtime: 16 untested; mapping: 16 partial.
* ☐ ``sc_core::sc_plist_base`` — 19 declarations; runtime: 19 untested; mapping: 19 partial.
* ☐ ``sc_core::sc_plist_base_iter`` — 14 declarations; runtime: 14 untested; mapping: 14 partial.
* ☐ ``sc_core::sc_plist_elem`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_plist_iter`` — 10 declarations; runtime: 10 untested; mapping: 10 partial.
* ☐ ``sc_core::sc_plist_map_fn`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_port`` — 77 declarations; runtime: 77 untested; mapping: 54 partial, 23 reviewed.
* ☐ ``sc_core::sc_port_b`` — 127 declarations; runtime: 127 untested; mapping: 30 partial, 97 reviewed.
* ☐ ``sc_core::sc_port_base`` — 21 declarations; runtime: 21 untested; mapping: 4 partial, 17 reviewed.
* ☐ ``sc_core::sc_port_policy`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_ppq`` — 19 declarations; runtime: 19 untested; mapping: 19 partial.
* ☐ ``sc_core::sc_ppq_base`` — 9 declarations; runtime: 9 untested; mapping: 9 partial.
* ☐ ``sc_core::sc_prim_channel`` — 41 declarations; runtime: 41 untested; mapping: 28 partial, 13 reviewed.
* ☐ ``sc_core::sc_process_b`` — 3 declarations; runtime: 3 untested; mapping: 3 partial.
* ☐ ``sc_core::sc_process_handle`` — 38 declarations; runtime: 38 untested; mapping: 12 partial, 26 reviewed.
* ☐ ``sc_core::sc_process_monitor`` — 4 declarations; runtime: 4 untested; mapping: 3 partial, 1 reviewed.
* ☐ ``sc_core::sc_pvector`` — 26 declarations; runtime: 26 untested; mapping: 26 partial.
* ☐ ``sc_core::sc_ref`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_register_hierarchical_name`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_release`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_report`` — 25 declarations; runtime: 25 untested; mapping: 2 partial, 23 reviewed.
* ☐ ``sc_core::sc_report_close_default_log`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_report_compose_message`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_report_handler`` — 32 declarations; runtime: 32 untested; mapping: 32 reviewed.
* ☐ ``sc_core::sc_report_handler_proc`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_reset`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_scoped_lock`` — 5 declarations; runtime: 5 untested; mapping: 2 partial, 3 reviewed.
* ☐ ``sc_core::sc_semaphore`` — 10 declarations; runtime: 10 untested; mapping: 4 partial, 6 reviewed.
* ☐ ``sc_core::sc_semaphore_if`` — 6 declarations; runtime: 6 untested; mapping: 1 partial, 5 reviewed.
* ☐ ``sc_core::sc_sensitive`` — 28 declarations; runtime: 28 untested; mapping: 28 partial.
* ☐ ``sc_core::sc_sensitive_neg`` — 23 declarations; runtime: 23 untested; mapping: 23 partial.
* ☐ ``sc_core::sc_sensitive_pos`` — 23 declarations; runtime: 23 untested; mapping: 23 partial.
* ☐ ``sc_core::sc_set_default_time_unit`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_set_random_seed`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_set_stack_size`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_set_stop_mode`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_set_time_resolution`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_severity`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_signal`` — 64 declarations; runtime: 64 untested; mapping: 64 partial.
* ☐ ``sc_core::sc_signal_channel`` — 6 declarations; runtime: 6 untested; mapping: 6 partial.
* ☐ ``sc_core::sc_signal_in_if`` — 37 declarations; runtime: 37 untested; mapping: 37 partial.
* ☐ ``sc_core::sc_signal_inout_if`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``sc_core::sc_signal_resolved`` — 14 declarations; runtime: 14 untested; mapping: 14 partial.
* ☐ ``sc_core::sc_signal_rv`` — 16 declarations; runtime: 16 untested; mapping: 16 partial.
* ☐ ``sc_core::sc_signal_t`` — 119 declarations; runtime: 119 untested; mapping: 119 partial.
* ☐ ``sc_core::sc_signal_write_if`` — 13 declarations; runtime: 13 untested; mapping: 13 partial.
* ☐ ``sc_core::sc_simcontext`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_simulation_time`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_spawn`` — 3 declarations; runtime: 3 untested; mapping: 3 reviewed.
* ☐ ``sc_core::sc_spawn_options`` — 21 declarations; runtime: 21 untested; mapping: 2 partial, 19 reviewed.
* ☐ ``sc_core::sc_start`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``sc_core::sc_start_of_simulation_invoked`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_starvation_policy`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_status`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_stop`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_stop_mode`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_strhash`` — 19 declarations; runtime: 19 untested; mapping: 19 partial.
* ☐ ``sc_core::sc_strhash_cmp`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_strhash_iter`` — 11 declarations; runtime: 11 untested; mapping: 11 partial.
* ☐ ``sc_core::sc_strhash_kdup`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_strhash_kfree`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_time`` — 34 declarations; runtime: 34 untested; mapping: 9 partial, 25 reviewed.
* ☐ ``sc_core::sc_time_params`` — 3 declarations; runtime: 3 untested; mapping: 3 partial.
* ☐ ``sc_core::sc_time_stamp`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_time_to_pending_activity`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_time_tuple`` — 10 declarations; runtime: 10 untested; mapping: 3 partial, 7 reviewed.
* ☐ ``sc_core::sc_time_unit`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_trace`` — 66 declarations; runtime: 66 untested; mapping: 66 partial.
* ☐ ``sc_core::sc_trace_delta_cycles`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_trace_file`` — 36 declarations; runtime: 36 untested; mapping: 36 partial.
* ☐ ``sc_core::sc_unregister_hierarchical_name`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::sc_unwind_exception`` — 5 declarations; runtime: 5 untested; mapping: 5 partial.
* ☐ ``sc_core::sc_vector`` — 43 declarations; runtime: 43 untested; mapping: 5 partial, 38 reviewed.
* ☐ ``sc_core::sc_vector_assembly`` — 50 declarations; runtime: 50 untested; mapping: 3 partial, 47 reviewed.
* ☐ ``sc_core::sc_vector_base`` — 11 declarations; runtime: 11 untested; mapping: 11 partial.
* ☐ ``sc_core::sc_vector_iter`` — 33 declarations; runtime: 33 untested; mapping: 19 partial, 14 reviewed.
* ☐ ``sc_core::sc_verbosity`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_version`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_version_major`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_version_minor`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_version_originator`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_version_patch`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_version_prerelease`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_version_release_date`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_version_string`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_core::sc_vpool`` — 62 declarations; runtime: 62 untested; mapping: 62 partial.
* ☐ ``sc_core::sc_write_comment`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::sc_writer_policy`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::timed_out`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_core::tprintf`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_core::wait`` — 13 declarations; runtime: 13 untested; mapping: 13 partial.

sc_dt public families
~~~~~~~~~~~~~~~~~~~~~

* ☐ ``sc_dt::Log_0`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::Log_1`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::Log_X`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::Log_Z`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_BIN`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_BIN_SM`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_BIN_US`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_CSD`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_DEC`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_E`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_F`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_HEX`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_HEX_SM`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_HEX_US`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_LATER`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_LOGIC_0`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::SC_LOGIC_1`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::SC_LOGIC_X`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::SC_LOGIC_Z`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::SC_NOBASE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_NOW`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_OCT`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_OCT_SM`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_OCT_US`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_OFF`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_ON`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_RND`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_RND_CONV`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_RND_INF`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_RND_MIN_INF`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_RND_ZERO`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_SAT`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_SAT_SYM`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_SAT_ZERO`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_TC_`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_TRN`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_TRN_ZERO`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_US_`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_WRAP`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::SC_WRAP_SM`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::add`` — 200 declarations; runtime: 200 untested; mapping: 200 partial.
* ☐ ``sc_dt::and_reduce`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``sc_dt::b_and`` — 22 declarations; runtime: 22 untested; mapping: 22 partial.
* ☐ ``sc_dt::b_not`` — 6 declarations; runtime: 6 untested; mapping: 6 partial.
* ☐ ``sc_dt::b_or`` — 22 declarations; runtime: 22 untested; mapping: 22 partial.
* ☐ ``sc_dt::b_xor`` — 22 declarations; runtime: 22 untested; mapping: 22 partial.
* ☐ ``sc_dt::concat`` — 170 declarations; runtime: 170 untested; mapping: 170 partial.
* ☐ ``sc_dt::div`` — 200 declarations; runtime: 200 untested; mapping: 200 partial.
* ☐ ``sc_dt::equal`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``sc_dt::int64`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::lrotate`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``sc_dt::lshift`` — 6 declarations; runtime: 6 untested; mapping: 6 partial.
* ☐ ``sc_dt::mult`` — 200 declarations; runtime: 200 untested; mapping: 200 partial.
* ☐ ``sc_dt::nand_reduce`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``sc_dt::neg`` — 6 declarations; runtime: 6 untested; mapping: 6 partial.
* ☐ ``sc_dt::nor_reduce`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``sc_dt::not_equal`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``sc_dt::operator!=`` — 291 declarations; runtime: 291 untested; mapping: 291 partial.
* ☐ ``sc_dt::operator%`` — 69 declarations; runtime: 1 failing, 68 untested; mapping: 69 partial.
* ☐ ``sc_dt::operator&`` — 115 declarations; runtime: 115 untested; mapping: 115 partial.
* ☐ ``sc_dt::operator&=`` — 6 declarations; runtime: 6 untested; mapping: 6 partial.
* ☐ ``sc_dt::operator*`` — 191 declarations; runtime: 191 untested; mapping: 191 partial.
* ☐ ``sc_dt::operator+`` — 194 declarations; runtime: 194 untested; mapping: 194 partial.
* ☐ ``sc_dt::operator,`` — 170 declarations; runtime: 170 untested; mapping: 170 partial.
* ☐ ``sc_dt::operator-`` — 203 declarations; runtime: 203 untested; mapping: 203 partial.
* ☐ ``sc_dt::operator/`` — 191 declarations; runtime: 191 untested; mapping: 191 partial.
* ☐ ``sc_dt::operator<`` — 185 declarations; runtime: 185 untested; mapping: 185 partial.
* ☐ ``sc_dt::operator<<`` — 67 declarations; runtime: 1 failing, 66 untested; mapping: 67 partial.
* ☐ ``sc_dt::operator<=`` — 185 declarations; runtime: 185 untested; mapping: 185 partial.
* ☐ ``sc_dt::operator==`` — 268 declarations; runtime: 268 untested; mapping: 268 partial.
* ☐ ``sc_dt::operator>`` — 185 declarations; runtime: 185 untested; mapping: 185 partial.
* ☐ ``sc_dt::operator>=`` — 185 declarations; runtime: 185 untested; mapping: 185 partial.
* ☐ ``sc_dt::operator>>`` — 68 declarations; runtime: 1 failing, 67 untested; mapping: 68 partial.
* ☐ ``sc_dt::operator^`` — 115 declarations; runtime: 115 untested; mapping: 115 partial.
* ☐ ``sc_dt::operator^=`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``sc_dt::operator|`` — 115 declarations; runtime: 115 untested; mapping: 115 partial.
* ☐ ``sc_dt::operator|=`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``sc_dt::operator~`` — 3 declarations; runtime: 3 untested; mapping: 3 partial.
* ☐ ``sc_dt::or_reduce`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``sc_dt::reverse`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``sc_dt::rrotate`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``sc_dt::rshift`` — 6 declarations; runtime: 6 untested; mapping: 6 partial.
* ☐ ``sc_dt::sc_abs`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_dt::sc_bigint`` — 48 declarations; runtime: 48 untested; mapping: 48 partial.
* ☐ ``sc_dt::sc_biguint`` — 48 declarations; runtime: 48 untested; mapping: 48 partial.
* ☐ ``sc_dt::sc_bit`` — 42 declarations; runtime: 42 untested; mapping: 42 partial.
* ☐ ``sc_dt::sc_bitref`` — 78 declarations; runtime: 78 untested; mapping: 78 partial.
* ☐ ``sc_dt::sc_bitref_conv_r`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``sc_dt::sc_bitref_r`` — 44 declarations; runtime: 44 untested; mapping: 44 partial.
* ☐ ``sc_dt::sc_bv`` — 37 declarations; runtime: 37 untested; mapping: 37 partial.
* ☐ ``sc_dt::sc_bv_base`` — 55 declarations; runtime: 55 untested; mapping: 55 partial.
* ☐ ``sc_dt::sc_concatref`` — 59 declarations; runtime: 59 untested; mapping: 57 partial, 2 reviewed.
* ☐ ``sc_dt::sc_concref`` — 25 declarations; runtime: 25 untested; mapping: 25 partial.
* ☐ ``sc_dt::sc_concref_r`` — 20 declarations; runtime: 20 untested; mapping: 20 partial.
* ☐ ``sc_dt::sc_context`` — 34 declarations; runtime: 2 failing, 32 untested; mapping: 34 partial.
* ☐ ``sc_dt::sc_context_begin`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_digit`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_enc`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_fix`` — 353 declarations; runtime: 353 untested; mapping: 353 partial.
* ☐ ``sc_dt::sc_fix_fast`` — 353 declarations; runtime: 353 untested; mapping: 353 partial.
* ☐ ``sc_dt::sc_fixed`` — 137 declarations; runtime: 137 untested; mapping: 137 partial.
* ☐ ``sc_dt::sc_fixed_fast`` — 137 declarations; runtime: 137 untested; mapping: 137 partial.
* ☐ ``sc_dt::sc_fmt`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_fxcast_context`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_fxcast_switch`` — 9 declarations; runtime: 9 untested; mapping: 9 partial.
* ☐ ``sc_dt::sc_fxnum`` — 173 declarations; runtime: 173 untested; mapping: 173 partial.
* ☐ ``sc_dt::sc_fxnum_bitref`` — 19 declarations; runtime: 19 untested; mapping: 19 partial.
* ☐ ``sc_dt::sc_fxnum_bitref_r`` — 5 declarations; runtime: 5 untested; mapping: 5 partial.
* ☐ ``sc_dt::sc_fxnum_fast`` — 173 declarations; runtime: 173 untested; mapping: 173 partial.
* ☐ ``sc_dt::sc_fxnum_fast_bitref`` — 19 declarations; runtime: 19 untested; mapping: 19 partial.
* ☐ ``sc_dt::sc_fxnum_fast_bitref_r`` — 5 declarations; runtime: 5 untested; mapping: 5 partial.
* ☐ ``sc_dt::sc_fxnum_fast_observer`` — 8 declarations; runtime: 8 untested; mapping: 8 partial.
* ☐ ``sc_dt::sc_fxnum_fast_subref`` — 32 declarations; runtime: 32 untested; mapping: 32 partial.
* ☐ ``sc_dt::sc_fxnum_fast_subref_r`` — 24 declarations; runtime: 24 untested; mapping: 24 partial.
* ☐ ``sc_dt::sc_fxnum_observer`` — 8 declarations; runtime: 8 untested; mapping: 8 partial.
* ☐ ``sc_dt::sc_fxnum_subref`` — 32 declarations; runtime: 32 untested; mapping: 32 partial.
* ☐ ``sc_dt::sc_fxnum_subref_r`` — 24 declarations; runtime: 24 untested; mapping: 24 partial.
* ☐ ``sc_dt::sc_fxtype_context`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_fxtype_params`` — 23 declarations; runtime: 23 untested; mapping: 23 partial.
* ☐ ``sc_dt::sc_fxval`` — 164 declarations; runtime: 164 untested; mapping: 164 partial.
* ☐ ``sc_dt::sc_fxval_fast`` — 164 declarations; runtime: 164 untested; mapping: 164 partial.
* ☐ ``sc_dt::sc_fxval_fast_observer`` — 8 declarations; runtime: 8 untested; mapping: 8 partial.
* ☐ ``sc_dt::sc_fxval_observer`` — 8 declarations; runtime: 8 untested; mapping: 8 partial.
* ☐ ``sc_dt::sc_generic_base`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``sc_dt::sc_int`` — 59 declarations; runtime: 59 untested; mapping: 59 partial.
* ☐ ``sc_dt::sc_int_base`` — 98 declarations; runtime: 98 untested; mapping: 98 partial.
* ☐ ``sc_dt::sc_int_bitref`` — 13 declarations; runtime: 13 untested; mapping: 13 partial.
* ☐ ``sc_dt::sc_int_bitref_r`` — 15 declarations; runtime: 15 untested; mapping: 15 partial.
* ☐ ``sc_dt::sc_int_subref`` — 24 declarations; runtime: 24 untested; mapping: 24 partial.
* ☐ ``sc_dt::sc_int_subref_r`` — 30 declarations; runtime: 30 untested; mapping: 30 partial.
* ☐ ``sc_dt::sc_io_base`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_io_show_base`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_length_context`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_length_param`` — 11 declarations; runtime: 11 untested; mapping: 11 partial.
* ☐ ``sc_dt::sc_logic`` — 51 declarations; runtime: 51 untested; mapping: 36 partial, 15 reviewed.
* ☐ ``sc_dt::sc_logic_0`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_logic_1`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_logic_X`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_logic_Z`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_logic_value_t`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``sc_dt::sc_lv`` — 38 declarations; runtime: 38 untested; mapping: 38 partial.
* ☐ ``sc_dt::sc_lv_base`` — 54 declarations; runtime: 54 untested; mapping: 54 partial.
* ☐ ``sc_dt::sc_max`` — 3 declarations; runtime: 3 untested; mapping: 3 partial.
* ☐ ``sc_dt::sc_min`` — 3 declarations; runtime: 3 untested; mapping: 3 partial.
* ☐ ``sc_dt::sc_numrep`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_o_mode`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_proxy`` — 484 declarations; runtime: 484 untested; mapping: 484 partial.
* ☐ ``sc_dt::sc_q_mode`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_signed`` — 200 declarations; runtime: 200 untested; mapping: 200 partial.
* ☐ ``sc_dt::sc_signed_bitref`` — 14 declarations; runtime: 14 untested; mapping: 14 partial.
* ☐ ``sc_dt::sc_signed_bitref_r`` — 16 declarations; runtime: 16 untested; mapping: 16 partial.
* ☐ ``sc_dt::sc_signed_subref`` — 24 declarations; runtime: 24 untested; mapping: 24 partial.
* ☐ ``sc_dt::sc_signed_subref_r`` — 30 declarations; runtime: 30 untested; mapping: 30 partial.
* ☐ ``sc_dt::sc_string_old`` — 48 declarations; runtime: 48 untested; mapping: 48 partial.
* ☐ ``sc_dt::sc_subref`` — 30 declarations; runtime: 30 untested; mapping: 30 partial.
* ☐ ``sc_dt::sc_subref_r`` — 22 declarations; runtime: 22 untested; mapping: 22 partial.
* ☐ ``sc_dt::sc_switch`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sc_ufix`` — 353 declarations; runtime: 353 untested; mapping: 353 partial.
* ☐ ``sc_dt::sc_ufix_fast`` — 353 declarations; runtime: 353 untested; mapping: 353 partial.
* ☐ ``sc_dt::sc_ufixed`` — 137 declarations; runtime: 137 untested; mapping: 137 partial.
* ☐ ``sc_dt::sc_ufixed_fast`` — 137 declarations; runtime: 137 untested; mapping: 137 partial.
* ☐ ``sc_dt::sc_uint`` — 59 declarations; runtime: 59 untested; mapping: 59 partial.
* ☐ ``sc_dt::sc_uint_base`` — 98 declarations; runtime: 98 untested; mapping: 98 partial.
* ☐ ``sc_dt::sc_uint_bitref`` — 13 declarations; runtime: 13 untested; mapping: 13 partial.
* ☐ ``sc_dt::sc_uint_bitref_r`` — 15 declarations; runtime: 15 untested; mapping: 15 partial.
* ☐ ``sc_dt::sc_uint_subref`` — 24 declarations; runtime: 24 untested; mapping: 24 partial.
* ☐ ``sc_dt::sc_uint_subref_r`` — 29 declarations; runtime: 29 untested; mapping: 29 partial.
* ☐ ``sc_dt::sc_unsigned`` — 200 declarations; runtime: 200 untested; mapping: 200 partial.
* ☐ ``sc_dt::sc_unsigned_bitref`` — 14 declarations; runtime: 14 untested; mapping: 14 partial.
* ☐ ``sc_dt::sc_unsigned_bitref_r`` — 16 declarations; runtime: 16 untested; mapping: 16 partial.
* ☐ ``sc_dt::sc_unsigned_subref`` — 25 declarations; runtime: 25 untested; mapping: 25 partial.
* ☐ ``sc_dt::sc_unsigned_subref_r`` — 30 declarations; runtime: 30 untested; mapping: 30 partial.
* ☐ ``sc_dt::sc_value_base`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``sc_dt::sc_without_context`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::sub`` — 200 declarations; runtime: 200 untested; mapping: 200 partial.
* ☐ ``sc_dt::to_string`` — 6 declarations; runtime: 6 untested; mapping: 6 partial.
* ☐ ``sc_dt::uint64`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``sc_dt::xnor_reduce`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``sc_dt::xor_reduce`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.

sc_elab_and_sim public families
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☐ ``sc_elab_and_sim`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.

sc_main public families
~~~~~~~~~~~~~~~~~~~~~~~

* ☐ ``sc_main`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.

tlm public families
~~~~~~~~~~~~~~~~~~~

* ☐ ``tlm::BEGIN_REQ`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::BEGIN_RESP`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::END_REQ`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::END_RESP`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_ACCEPTED`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_ADDRESS_ERROR_RESPONSE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_BIG_ENDIAN`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_BURST_ERROR_RESPONSE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_COMMAND_ERROR_RESPONSE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_COMPLETED`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_FULL_PAYLOAD`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_FULL_PAYLOAD_ACCEPTED`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_GENERIC_ERROR_RESPONSE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_IGNORE_COMMAND`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_INCOMPLETE_RESPONSE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_INITIATOR_SOCKET`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_LITTLE_ENDIAN`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_MIN_PAYLOAD`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_MULTI_INITIATOR_SOCKET`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_MULTI_SOCKET`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_MULTI_TARGET_SOCKET`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_OK_RESPONSE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_READ_COMMAND`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_TARGET_SOCKET`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_UNKNOWN_ENDIAN`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_UNKNOWN_SOCKET`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_UPDATED`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::TLM_WRITE_COMMAND`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::UNINITIALIZED_PHASE`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::get_host_endianness`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::has_host_endianness`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::host_has_little_endianness`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::max_num_extensions`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::operator<<`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_analysis_fifo`` — 6 declarations; runtime: 6 untested; mapping: 6 partial.
* ☐ ``tlm::tlm_analysis_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_analysis_port`` — 8 declarations; runtime: 8 untested; mapping: 2 partial, 6 reviewed.
* ☐ ``tlm::tlm_analysis_triple`` — 10 declarations; runtime: 10 untested; mapping: 5 partial, 5 reviewed.
* ☐ ``tlm::tlm_array`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_base_initiator_socket`` — 29 declarations; runtime: 29 untested; mapping: 9 partial, 20 reviewed.
* ☐ ``tlm::tlm_base_initiator_socket_b`` — 11 declarations; runtime: 11 untested; mapping: 1 partial, 10 reviewed.
* ☐ ``tlm::tlm_base_protocol_types`` — 3 declarations; runtime: 3 untested; mapping: 3 reviewed.
* ☐ ``tlm::tlm_base_socket_if`` — 11 declarations; runtime: 11 untested; mapping: 1 partial, 10 reviewed.
* ☐ ``tlm::tlm_base_target_socket`` — 32 declarations; runtime: 32 untested; mapping: 11 partial, 21 reviewed.
* ☐ ``tlm::tlm_base_target_socket_b`` — 11 declarations; runtime: 11 untested; mapping: 1 partial, 10 reviewed.
* ☐ ``tlm::tlm_blocking_get_if`` — 4 declarations; runtime: 4 untested; mapping: 4 reviewed.
* ☐ ``tlm::tlm_blocking_get_peek_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_blocking_master_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_blocking_peek_if`` — 4 declarations; runtime: 4 untested; mapping: 4 reviewed.
* ☐ ``tlm::tlm_blocking_put_if`` — 3 declarations; runtime: 3 untested; mapping: 3 reviewed.
* ☐ ``tlm::tlm_blocking_slave_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_blocking_transport_if`` — 3 declarations; runtime: 3 untested; mapping: 3 reviewed.
* ☐ ``tlm::tlm_bw_direct_mem_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_bw_nonblocking_transport_if`` — 3 declarations; runtime: 3 untested; mapping: 3 reviewed.
* ☐ ``tlm::tlm_bw_transport_if`` — 4 declarations; runtime: 4 untested; mapping: 4 reviewed.
* ☐ ``tlm::tlm_command`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::tlm_copyright`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_copyright_string`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_delayed_analysis_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_delayed_write_if`` — 3 declarations; runtime: 3 untested; mapping: 3 reviewed.
* ☐ ``tlm::tlm_dmi`` — 28 declarations; runtime: 28 untested; mapping: 28 reviewed.
* ☐ ``tlm::tlm_endianness`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_event_finder_t`` — 5 declarations; runtime: 5 untested; mapping: 5 partial.
* ☐ ``tlm::tlm_extension`` — 16 declarations; runtime: 16 untested; mapping: 3 partial, 13 reviewed.
* ☐ ``tlm::tlm_extension_base`` — 6 declarations; runtime: 6 untested; mapping: 1 partial, 5 reviewed.
* ☐ ``tlm::tlm_fifo`` — 45 declarations; runtime: 45 untested; mapping: 45 partial.
* ☐ ``tlm::tlm_fifo_config_size_if`` — 5 declarations; runtime: 5 untested; mapping: 5 reviewed.
* ☐ ``tlm::tlm_fifo_debug_if`` — 7 declarations; runtime: 7 untested; mapping: 7 reviewed.
* ☐ ``tlm::tlm_fifo_get_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_fifo_put_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_from_hostendian`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_from_hostendian_aligned`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``tlm::tlm_from_hostendian_generic`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``tlm::tlm_from_hostendian_single`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``tlm::tlm_from_hostendian_word`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``tlm::tlm_fw_direct_mem_if`` — 3 declarations; runtime: 3 untested; mapping: 3 reviewed.
* ☐ ``tlm::tlm_fw_nonblocking_transport_if`` — 3 declarations; runtime: 3 untested; mapping: 3 reviewed.
* ☐ ``tlm::tlm_fw_transport_if`` — 4 declarations; runtime: 4 untested; mapping: 4 reviewed.
* ☐ ``tlm::tlm_generic_payload`` — 60 declarations; runtime: 60 untested; mapping: 1 partial, 59 reviewed.
* ☐ ``tlm::tlm_get_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_get_peek_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_global_quantum`` — 6 declarations; runtime: 6 untested; mapping: 1 partial, 5 reviewed.
* ☐ ``tlm::tlm_gp_option`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::tlm_initiator_socket`` — 5 declarations; runtime: 5 untested; mapping: 5 reviewed.
* ☐ ``tlm::tlm_is_prerelease`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_master_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_mm_interface`` — 3 declarations; runtime: 3 untested; mapping: 1 partial, 2 reviewed.
* ☐ ``tlm::tlm_nonblocking_get_if`` — 5 declarations; runtime: 5 untested; mapping: 5 reviewed.
* ☐ ``tlm::tlm_nonblocking_get_peek_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_nonblocking_get_port`` — 5 declarations; runtime: 5 untested; mapping: 5 partial.
* ☐ ``tlm::tlm_nonblocking_master_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_nonblocking_peek_if`` — 5 declarations; runtime: 5 untested; mapping: 5 reviewed.
* ☐ ``tlm::tlm_nonblocking_peek_port`` — 5 declarations; runtime: 5 untested; mapping: 5 partial.
* ☐ ``tlm::tlm_nonblocking_put_if`` — 5 declarations; runtime: 5 untested; mapping: 5 reviewed.
* ☐ ``tlm::tlm_nonblocking_put_port`` — 5 declarations; runtime: 5 untested; mapping: 5 partial.
* ☐ ``tlm::tlm_nonblocking_slave_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_peek_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_phase`` — 8 declarations; runtime: 8 untested; mapping: 8 partial.
* ☐ ``tlm::tlm_phase_enum`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_put_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_release`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_req_rsp_channel`` — 10 declarations; runtime: 10 untested; mapping: 10 partial.
* ☐ ``tlm::tlm_response_status`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::tlm_slave_if`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_slave_to_transport`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``tlm::tlm_socket_category`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::tlm_sync_enum`` — 1 declarations; runtime: 1 untested; mapping: 1 reviewed.
* ☐ ``tlm::tlm_tag`` — 2 declarations; runtime: 2 untested; mapping: 2 reviewed.
* ☐ ``tlm::tlm_target_socket`` — 5 declarations; runtime: 5 untested; mapping: 5 reviewed.
* ☐ ``tlm::tlm_to_hostendian_aligned`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``tlm::tlm_to_hostendian_generic`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``tlm::tlm_to_hostendian_single`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``tlm::tlm_to_hostendian_word`` — 2 declarations; runtime: 2 untested; mapping: 2 partial.
* ☐ ``tlm::tlm_transport_channel`` — 8 declarations; runtime: 8 untested; mapping: 8 partial.
* ☐ ``tlm::tlm_transport_dbg_if`` — 3 declarations; runtime: 3 untested; mapping: 3 reviewed.
* ☐ ``tlm::tlm_transport_if`` — 4 declarations; runtime: 4 untested; mapping: 4 reviewed.
* ☐ ``tlm::tlm_transport_to_master`` — 7 declarations; runtime: 7 untested; mapping: 7 partial.
* ☐ ``tlm::tlm_version`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_version_major`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_version_minor`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_version_originator`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_version_patch`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_version_prerelease`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_version_release_date`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_version_string`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_version_string_2`` — 1 declarations; runtime: 1 untested; mapping: 1 partial.
* ☐ ``tlm::tlm_write_if`` — 3 declarations; runtime: 3 untested; mapping: 3 reviewed.

tlm_utils public families
~~~~~~~~~~~~~~~~~~~~~~~~~

* ☐ ``tlm_utils::instance_specific_extension`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::instance_specific_extension_accessor`` — 6 declarations; runtime: 6 untested; mapping: 6 partial.
* ☐ ``tlm_utils::instance_specific_extensions_per_accessor`` — 13 declarations; runtime: 13 untested; mapping: 13 partial.
* ☐ ``tlm_utils::ispex_base`` — 3 declarations; runtime: 3 untested; mapping: 3 partial.
* ☐ ``tlm_utils::multi_passthrough_initiator_socket`` — 25 declarations; runtime: 25 untested; mapping: 25 partial.
* ☐ ``tlm_utils::multi_passthrough_initiator_socket_optional`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::multi_passthrough_target_socket`` — 28 declarations; runtime: 28 untested; mapping: 28 partial.
* ☐ ``tlm_utils::multi_passthrough_target_socket_optional`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::passthrough_target_socket`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::passthrough_target_socket_b`` — 14 declarations; runtime: 14 untested; mapping: 14 partial.
* ☐ ``tlm_utils::passthrough_target_socket_optional`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::passthrough_target_socket_tagged`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::passthrough_target_socket_tagged_b`` — 14 declarations; runtime: 14 untested; mapping: 14 partial.
* ☐ ``tlm_utils::passthrough_target_socket_tagged_optional`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::peq_with_cb_and_phase`` — 8 declarations; runtime: 8 untested; mapping: 8 partial.
* ☐ ``tlm_utils::peq_with_get`` — 10 declarations; runtime: 10 untested; mapping: 1 partial, 9 reviewed.
* ☐ ``tlm_utils::simple_initiator_socket`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::simple_initiator_socket_b`` — 12 declarations; runtime: 12 untested; mapping: 12 partial.
* ☐ ``tlm_utils::simple_initiator_socket_optional`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::simple_initiator_socket_tagged`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::simple_initiator_socket_tagged_b`` — 12 declarations; runtime: 12 untested; mapping: 12 partial.
* ☐ ``tlm_utils::simple_initiator_socket_tagged_optional`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::simple_target_socket`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::simple_target_socket_b`` — 16 declarations; runtime: 16 untested; mapping: 16 partial.
* ☐ ``tlm_utils::simple_target_socket_optional`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::simple_target_socket_tagged`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::simple_target_socket_tagged_b`` — 16 declarations; runtime: 16 untested; mapping: 16 partial.
* ☐ ``tlm_utils::simple_target_socket_tagged_optional`` — 4 declarations; runtime: 4 untested; mapping: 4 partial.
* ☐ ``tlm_utils::tlm_quantumkeeper`` — 14 declarations; runtime: 14 untested; mapping: 2 partial, 12 reviewed.

Floating FIFO callback samples
------------------------------

Structural float, double, and long double FIFO reads preserve infinities, NaN,
and signed zero through value-returning reads, caller-output blocking reads,
and successful nonblocking reads. Finite
recovery is checked in the same simulation and compared with independent native
SystemC FIFO runs for each type. Complete conversion, tracing, reset,
and native-reference matrices remain open.

Rejected scalar FIFO reads preserve caller-owned output in both blocking and
nonblocking paths for tested integer widths, bool, and floating types. Callback
errors, including raised owner exceptions, are cleared under the GIL before
checking subsequent recovery.

Additional partial-first TLM get/peek binding orders share the combined native
interface and matching get/peek readiness events while retaining queue state.
Both readiness observers execute exactly once at the scheduled 1 ns notification.
The complete cross-specialization identity matrix remains open.

Stopped callback release counts one shared get/peek adapter across partial and
combined views, retains owners before stop, permits owner collection, and rejects
later get/peek callbacks. Rejected nonblocking peeks preserve caller output.
Native adapter objects remain borrowed by ports; full native teardown is open.

Rejected cleanup during simulation leaves shared TLM callbacks usable, including
final get/peek accesses from end-of-simulation hooks before explicit release.

Blocking TLM get/peek bridges use internal value-pair callbacks to support
multiword native return types while preserving public native signatures. Reset
and kill ordering is checked across int, float, double, long double, and
sc_biguint<137> for FIFO read/write and TLM get/put/peek with nine wait forms
and combined TLM blocking get/peek (315 combinations). Complete channel and TLM matrices remain open.

Multiword standalone and combined TLM blocking get/peek completion checks both returned values
and caller-output overloads for long long, unsigned long long, long double, and sc_biguint<137> with bit136
set.
Each service waits 1 ns; callback counts and total 48 ns completion are asserted.

Multiword standalone/combined blocking TLM outputs remain unchanged when owners
raise or return nonconvertible objects, followed by normal timed completion.

The full blocking/nonblocking get/peek family shares the multiword-safe blocking
callback path, including automatic selection from partial interface bindings.

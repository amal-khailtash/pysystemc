Features
========

This checklist tracks implemented and verified capabilities alongside remaining
work. It is maintained with the development plan as implementation groups land.

* ☑ Delivered and verified for the described scope.
* ☐ Open requirement: incomplete, failing or untested as stated.
* ☐ Deferred roadmap item: planned after the binding and example milestones.

A checked item does not establish every overload, type, lifetime or platform
combination. Open completeness gates remain visible beside delivered increments.
Native symbol availability alone does not establish Python support.
See :doc:`../api/coverage` for verification evidence and
:doc:`../api/bindings` for usage and lifetime rules.

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

The following groups mirror the detailed M6A checklist. They retain open audit
and lifecycle requirements even where representative implementations pass.

Public declaration inventory and Python mapping — incomplete
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☑ Reproducible compiler-derived declaration scaffold across core/datatypes/TLM/utilities.
* ☑ Real-header regression checks overloads/templates/access, source locations and enum spelling.

* ☐ Inventory every public declaration/overload in systemc/sysc.
* ☐ Inventory every public declaration/overload in tlm/tlm_core.
* ☐ Inventory every public declaration/overload in tlm_utils.
* ☐ Map each inventory entry to Python spelling, ownership, GIL, errors and evidence.
* ☐ Classify internal facilities with source-based rationale.
* ☐ Reconcile inventory with examples and identify APIs absent from examples.

Custom interfaces and channels — partially implemented
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☑ Structural port/export binding and native-style call syntax delivered.
* ☑ Annotated Protocol declaration and explicit native signature support delivered.
* ☑ Ordinary Python callback spelling adaptation delivered for tested TLM families.
* ☑ Retain structural callback owners through native process lifetime.
* ☑ Explicit custom lvalue-reference result trampolines preserving const/mutable native identity.
* ☑ Retain distinct returned storage through shutdown; replacement survival and invalid-result rejection verified.
* ☑ Shared event result identity and sensitivity within an explicit custom adapter verified against native reference.
* ☑ Native structural signal input/inout reference adapters; 137-bit read/trace address identity verified.
* ☑ Bool/logic signal edge callbacks and borrowed edge-event identity verified.
* ☑ Fundamental scalar signal read/trace slots; signed/floating/unsigned64-bit conversion and recovery verified.
* ☑ Native structural FIFO input/output adapters; both blocking reads, writes and failed-output preservation verified.
* ☑ Separate blocking/nonblocking FIFO input/output adapters with minimal callback contracts delivered.
* ☑ Native FIFO callback result validation, malformed-output preservation and blocking exception recovery verified.
* ☑ Shared owner event identity across native signal input/inout adapter specializations verified.
* ☐ Complete native signal/FIFO scalar/type/partial-interface/reset/error/reference matrices.
* ☐ Shared interface/event identity across different adapter specializations.
* ☐ Adapter teardown and release of retained callback owners before shutdown.
* ☐ Complete custom-channel error/reset/GIL/access-path matrix.

TLM interface families — implemented increments; full matrix open
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☑ Structural TLM1 blocking put/get/peek/transport/analysis adapters.
* ☑ Structural TLM1 nonblocking put/get/peek and combined get/put/peek/get-peek adapters.
* ☑ Structural TLM1 master/slave families and FIFO debug/config/compound adapters.
* ☑ Structural TLM2 fw/bw, blocking/nonblocking, debug and DMI callback adapters.
* ☑ Selected wide native payload/value identity and failed-output preservation checks.
* ☐ Full public overload/type/event/lifetime and independent native reference matrices.

Callable sockets — delivered API families; completeness open
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☑ Simple target: all four forward registrations; simple initiator: both backward registrations.
* ☑ Tagged target/initiator registrations with independent user IDs.
* ☑ Multi target/initiator registrations with native connection IDs.
* ☑ Ordinary/tagged passthrough target registrations.
* ☑ Direct multi-to-multi connection mapping.
* ☑ Simple parent initiator/target hierarchy resolution probes.
* ☑ Automatic blocking GIL configuration on tested get_interface/indexed access paths.
* ☑ Selected simple/multi/passthrough blocking exception and reset/kill cleanup checks.
* ☑ Custom 137-bit payload probes for simple/multi/passthrough sockets.
* ☑ Independent simple/passthrough custom native references.
* ☑ Multi/passthrough duplicate semantics, debug bounds and registration input checks.
* ☑ Optional policy elaboration and required binding rejection.
* ☐ Independent native multi socket reference matrices.
* ☐ Complete multi/passthrough hierarchy, conversion and all native access paths.
* ☐ Complete tagged custom protocol, callback error, reset and lifetime matrices.
* ☐ Complete socket public overload/template inventory and owner teardown contract.

TLM timing, payloads, extensions and endian utilities — partial
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☑ Quantumkeeper guarded sync/set_and_sync and reset/kill probes.
* ☑ Native PEQ/get and callable PEQ delivery, cancellation and selected custom protocol checks.
* ☑ Independent quantumkeeper/PEQ timing reference.
* ☑ Instance-private accessor isolation/replacement/carrier release/deep-copy omission.
* ☑ Independent instance-private extension lifecycle reference.
* ☑ Generic/word/aligned/single endian request and explicit response probes.
* ☑ Generic streaming/repeating enables, unaligned and 72-case type/offset probes.
* ☑ Word 72-case type/offset native comparison.
* ☑ Generic combined streaming native reference; single 27-case native reference.
* ☐ Complete generic/aligned native type and byte-enable reference matrices.
* ☐ Complete extension clone/copy/free/mm and payload memory-manager ownership matrices.
* ☐ Complete timing/extension/endian public declarations and context/lifetime audit.

Core semantics and final gates — incomplete
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ☐ Full datatype/proxy/concat/implicit-conversion/overload audit.
* ☐ Full writer/port/fixed-point policy audit.
* ☐ Full tracing type/format and vector assembly audit.
* ☐ Full report/process-control/lifecycle error audit.
* ☐ Implement applicable arbitrary throw_it, channel-internal reset and direct-wait behavior.
* ☐ Verify host/kernel lifetime and adapter ownership release.
* ☑ Public socket, instance-extension and endian guides added with precise limits.
* ☑ Strict Sphinx validation of the published API guides.
* ☐ Reconciled full regression and coverage for the final M6A implementation.
* ☐ Review first-party adapters and verify latest installed wheel and sdist.
* ☐ Verify every selected public-contract entry; obtain explicit scope decisions for unrepresentable APIs.

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

Passing selected ports does not complete M6B. The remaining group audit includes:

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
* ☐ Final full regression, measured coverage and installed wheel/sdist gates for M6A.
* ☐ Complete public declaration-to-Python semantic mapping and upstream reconciliation.

Deferred extensions
-------------------

These roadmap capabilities follow M6A/M6B and are not implemented support claims.

* ☐ Deferred: SystemC Verification library integration and transaction recording.
* ☐ Deferred: CRAVE constrained randomization and reproducibility.
* ☐ Deferred: FC4SC functional coverage and report export.
* ☐ Deferred: UVM-SystemC components, sequences and phase/objection behavior.
* ☐ Deferred: Verilog/SystemVerilog simulator integration through Verilator or VPI/DPI.

Maintaining this checklist
--------------------------

Update the affected feature group with each implementation change. Check an item
only when its described behavior has semantic evidence. Split broad requirements
into delivered increments and remaining gates rather than checking a whole family
from one passing example. Mark failures, untested behavior and deferred work
explicitly. Keep detailed usage in API guides and session state outside published
documentation.

# Changelog

<!-- markdownlint-disable MD024 -->

Notable changes follow [Keep a Changelog](https://keepachangelog.com/) and
[Semantic Versioning](https://semver.org/). The project version is maintained in
`pyproject.toml`. Unreleased changes have not been published as a new release.

## [Unreleased]

### Added
- Release the GIL for native sc_initialize and verify pthread initialization/wait, free notify overload timing and pause/resume against C++.
- Support explicit native argc/argv calls to sc_elab_and_sim alongside the default sys.argv helper, with pre-entry count/type/NUL validation and native status preservation.
- Return immutable string snapshots from public Python sc_argv during native entry, reject reads outside entry, and verify original UTF-8 and empty arguments against native C++.
- Support Python string and bytes inputs for native void-pointer string hash/compare/key-duplication helpers while preserving native pointer calls and explicit key allocation ownership.
- Restore the default preserve-first argument for native unique-name generation and verify module, thread and global scope counters and native version function returns against C++.
- Restore default-context pending-activity queries, verify both hierarchical name-registry overloads against C++, and include complete name-generator declarations in source extraction.
- Restore default-context top-level object/event lookups and safe iteration of empty borrowed pointer vectors; expose the complete native name generator.
- Expose native tracing report identifiers omitted by the SystemC umbrella header and verify public sc_core constants, bind proxies and CFT comparator callbacks.
- Support plain/delayed TLM write and delayed analysis structural interfaces, and enforce debug transport payload-length bounds.
- Audit every registered structural adapter family through callback release and native destruction.
- Guard native FIFO, mutex and semaphore blocking calls against reset/kill unwind, including borrowed interfaces and FIFO convenience ports; reject teardown with suspended thread frames.
- Preserve native signal template types in floating waveform registration and validate structural writer-policy enum types.
- Release the GIL for arbitrary declared-interface native method entry and verify reset/kill callback cleanup.
- Preserve multiword TLM1 transport response overloads and share FIFO debug/get/put and compound/custom-protocol interface adapters.
- Reject structural teardown while Python-created traces are open.
- Add stopped-kernel structural adapter teardown guarded against live native ports and exports.
- Share complete standard-protocol TLM2 transport adapters across compatible partial-first bindings using native inheritance checks.
- Share complete TLM put adapters and readiness events across blocking-first, nonblocking-first and combined bindings.

### Tests
- Compare native public time resolution/default-unit configuration and rejection recovery, maximum/simulation time, stop-mode settings and simulator status against C++.
- Compare public process kind/raw/current/last-created queries and reset/kill unwind state against native method and pthread thread behavior, including construction-scope fallback.

- Extend B3 reset/kill teardown to multiword and arbitrary-width types, and add independent native TLM get/peek lifecycle references.
- Extend B3 teardown after reset/kill to nine guarded wait families across six paths, and compare native FIFO cleanup/deletion ordering.
- Verify native teardown follows reset/kill cleanup and process termination across FIFO, TLM get/master/slave and transport callbacks.
- Check master/slave and TLM1/TLM2 transport native teardown after end hooks, preserving callback owners until final reads.
- Verify native teardown across seven scalar/wide signal, FIFO, TLM and analysis adapters, including retained-result collection and VCD closure.
- Verify TLM put identity and copied scalar/wide values with delayed readiness callbacks and an independent native reference.

## [0.13.2](https://github.com/amal-khailtash/pysystemc/compare/v0.13.1...v0.13.2) (2026-10-20)


### Bug Fixes

* preserve compound interface callbacks and validate plain char values ([#12](https://github.com/amal-khailtash/pysystemc/issues/12)) ([29f3874](https://github.com/amal-khailtash/pysystemc/commit/29f38748185a3e8afdaf76bd1716db2b71de65b0))

### Fixed
- Validate plain char signal/FIFO callback integers using native signedness before conversion.
- Preserve master/slave multiword blocking get/peek signatures through native pair-return callbacks.
- Validate compound TLM FIFO integral blocking/nonblocking callback values before native conversion, preserving caller outputs on rejected results.
- Use native pair-return callbacks for compound TLM FIFO blocking get/peek so multiword scalar dispatch preserves public value/output overloads.

### Tests
- Compare plain char signal references and FIFO output overloads with native endpoints; reject invalid samples and preserve prior storage.
- Exercise 720 master/slave reset/kill combinations with a 32-consumer native output-unwind reference.
- Verify compound FIFO/master/slave callback-owner retention, stopped-kernel release and post-release output preservation.
- Compare scalar signal endpoint and floating special-value semantics with native signals, including callback rejection and recovery.
- Compare twelve asymmetric master/slave overload specializations with independent native references and expand signal/FIFO/TLM specialization identity matrices.
- Extend the planned reset matrix to compound FIFO get/peek value/output overloads across 2,268 combinations and native compound output references.
- Extend compound FIFO overload checks to signed/unsigned 128-bit and unsigned 137-bit values, plus integral callback rejection and endpoint recovery.
- Assert interrupted FIFO/TLM blocking caller-output storage stays unchanged before reset/kill unwind is rethrown.
- Compare caller-output reset/kill semantics with an independent native FIFO/TLM reference across fourteen types.
- Cover caller-output FIFO read and TLM get/peek reset unwind across 1,764 existing type/wait/access combinations.
- Broaden existing scalar interface reset coverage to short/unsigned short/unsigned int/long/unsigned long across 1,386 combinations.
- Assert released standalone/full TLM blocking caller-output overloads preserve output and full readiness callbacks reject access.
- Verify standalone and full TLM readiness events remain distinct and preserve native addresses after callback-owner cleanup.
- Check released standalone get/peek readiness callbacks reject access and native nonblocking get preserves caller output after owner cleanup.
- Extend existing interface reset/kill coverage to signed/unsigned 128-bit values, preserving high bits and signed copied arguments across 891 combinations.
- Extend existing standalone TLM native-reference, overload, error, and readiness checks to signed/unsigned 128-bit SystemC values with bit 127 set.
- Compare inherited TLM get/peek readiness-event identity with independent native FIFO specializations across the four multiword/wide types.
- Extend the independent multiword TLM FIFO comparison to nonblocking copies, empty outputs, and readiness before and after consumption.
- Compare standalone multiword/wide TLM get/peek value and caller-output overloads against independent native tlm_fifo references.


## [0.13.1](https://github.com/amal-khailtash/pysystemc/compare/v0.13.0...v0.13.1) (2026-10-20)

### Bug Fixes

* preserve multiword TLM callbacks and expand interface validation ([#7](https://github.com/amal-khailtash/pysystemc/issues/7)) ([58f4c4f](https://github.com/amal-khailtash/pysystemc/commit/58f4c4f19f5c052e18a648189e1e4a6409a3617c))
* synchronize release documentation and dispatch publishing directly ([#9](https://github.com/amal-khailtash/pysystemc/issues/9)) ([83ece5a](https://github.com/amal-khailtash/pysystemc/commit/83ece5ace12c1da5aa3dab295da3beebd41676fe))

### Fixed

- Dispatch publication directly with its configured trusted publisher workflow identity.
- Synchronize Read the Docs release tags, wait for discovery, activate versions, and request builds after publication.
- Preserve standalone, blocking-combined, and full TLM get/peek signatures while avoiding cppyy multiword return dispatcher failures, including long double, and expose concrete adapter result aliases for successful completion.

### Tests
- Verify standalone get/peek callback owners survive end-of-simulation and are collected after explicit release, with post-release callback rejection and preserved peek output.
- Check malformed standalone TLM nonblocking tuple shape, success flags, and values preserve caller output and recover to valid copies.
- Check standalone multiword/wide TLM get/peek inherited readiness-event identity and exactly one delayed observer callback at 1 ns.
- Verify standalone TLM value-returning get/peek propagate owner exceptions and conversion failures before normal timed recovery.
- Cover full blocking/nonblocking TLM get/peek reset and kill cleanup for both blocking operations, bringing the structural matrix to 693 combinations.
- Include signed and unsigned 64-bit types in all structural reset/kill paths, checking 567 type/wait/access combinations.
- Extend reset/kill cleanup to standalone combined TLM get and peek owners without full-interface selection, covering 405 type/wait/access combinations.
- Exercise standalone multiword/wide TLM get and peek owners without full-interface auto-upgrade, checking both blocking overloads, readiness, output preservation, exception recovery, and exact service time.
- Verify malformed nonblocking TLM result tuples, nonconvertible values, and raised exceptions preserve outputs and recover.
- Verify full multiword TLM get/peek both readiness states and successful/failed nonblocking output behavior through partial bindings.
- Extend reset/kill cleanup checks to combined blocking TLM get/peek across scalar and arbitrary-width types and nine wait forms.
- Check multiword and arbitrary-width TLM blocking output preservation after invalid results and owner exceptions, followed by timed recovery.
- Verify stopped callback cleanup releases a shared TLM get/peek owner once across partial and combined interfaces, preserving native nonblocking output after release and callbacks through end-of-simulation.
- Check additional partial-first TLM get/peek binding orders preserve native interface and readiness-event identity with timed get/peek observers.
- Verify rejected scalar FIFO blocking reads preserve caller output and recover after invalid results or raised owner exceptions.
- Validate floating FIFO value-returning, caller-output blocking, and nonblocking callback outputs for infinities, NaN, signed zero, and finite recovery against independent native SystemC FIFOs.

### CI
- Match the verified pyipxact-de uv trusted-publishing and attestation flow while retaining native wheel validation.
- Consolidate numbered PR, CI, release, and publication workflows; reuse validated distributions and trigger release docs after PyPI.

## [0.13.0] - 2026-10-20

### Added
- Release-please automation with reviewed release PRs, Conventional Commit title
  checks and verified native-wheel publication through a reusable workflow.
- Structural interface identity checks across signal, FIFO and TLM inherited
  views, plus scalar FIFO callback validation and recovery tests.
  Inherited get/peek readiness notifications match an independent native model.
- Explicit stopped-kernel structural callback cleanup releases Python owners
  while retaining native adapters borrowed by ports.
- Structural FIFO read/write and TLM get/put/peek reset/kill tests verify nested
  callback cleanup, process restart and simulation times through guarded waits.
  Timed, event, OR/AND, timed-event/list, cycle and counted-cycle waits are
  covered for each path with explicit static clock sensitivity, including
  137-bit values and high-bit-preserving write/put callback arguments.

### Fixed
- Forward native export bindings to their underlying interface, preserving
  structural identity through export chains.
- Reject nonnumeric floating signal callback results before native sampled
  reference storage changes, with recovery through read and trace-reference access.
- Reuse existing compatible structural adapters before validating a narrower
  interface, preserving the shared native interface and event identity.
- Adapt fundamental FIFO read results through native pairs so cppyy can compile
  callbacks returning multiword scalar types such as `long long`.

## [0.12.0] - 2026-10-19

Adds Python ports and native baseline comparisons for all 33 upstream example targets.
Alternate configurations and complete public API validation remain open.

### Added
- Python mixed-endian and temporal-decoupling example ports using native word
  converters and the quantum keeper, with scripted data, memory timestamps and
  synchronization comparisons against independent native executables.
- Python ports of mixed/delayed AT targets, LT, LT DMI and mandatory extensions
  with native memory/timing and extension lifecycle comparisons. Preserve source
  DMI range/invalidation behavior and retain pointer buffers through callbacks.
- Python ports of the performance FIFO and four approximately timed TLM examples
  (one/two/four-phase and optional extension), with native value/time comparisons,
  retained payload buffers and checked traffic completion.
- Python packet-switch, seeded RSA, custom immediate-event FIFO and simple-bus
  stimulus ports with independent native record comparisons.
- Python implementations of all ten upstream RISC CPU components, preserving
  decoder opcodes, register writeback and clocked memory/processor timing, with
  an independent native default-image comparison.
- Python ports of both upstream FFT numeric models, behavioral/RTL FIR and the
  double pipeline. FFT compares all five supplied input pairs per numeric model;
  FIR compares both executables, including value/time and completion records.
- Python ports of the upstream ready/valid and toggle/toggle examples preserve
  native clocked handshake/reset semantics; all 131/132 value/time records match
  independent C++ references, including forty consumed values.

### Changed
- Complete the next five baseline example items: simple_async, fft_flpt,
  fft_fxpt, fir and pipe. Compile unmodified FFT/FIR reference sources in separate
  translation units to accommodate upstream headers without include guards.
- Complete five further baseline example audits: barrier, FIFO mutex policy,
  specialized signals, ready/valid and toggle/toggle. Document unused helpers and
  compiler alternatives separately; restore the mutex port's full permission notice.
- Complete the baseline executable-variant audits for dpipe, forkjoin,
  reset_signal_is, sc_export and sc_report; document inputs, native comparisons,
  Python adaptations and the separate untested platform/build alternatives.
- Strengthen fork/join comparisons to include function invocations and completion,
  reset checks to count reset entries, and report checks to compare scenario headings.

## [0.11.0] - 2026-10-19

Publishes the fixed SystemC public API inventory and remaining binding work.
Inventory completion does not imply complete Python API support.

### Added
- Features lists all 794 public API families and links to 12,701 declaration
  contracts with native signatures, Python mappings, ownership, GIL, errors and statuses.
- Source inventory accounts for 15,778 identities across modeling and compatibility
  configurations, including 3,077 internal declarations and recorded macro/friend/alias sites.
- All 33 upstream executable targets have source, dependency and input/configuration
  records; nine default native/Python comparisons pass and 24 remain unported or uncompared.
- Independent native multi-socket comparisons check two-connection timing, phases,
  debug counts, DMI ranges, invalidation routing and callback error recovery.
- Scoped datatype, numeric protocol and time-constructor audit regressions distinguish
  native semantics from known Python binding failures.

### Fixed
- Preserve private enum access, static method metadata, out-of-class owners,
  specialization context and continued conditional macro directives in extraction.
- Preserve explicit review contracts and distinguish partial mappings from missing fields.
- Correct sc_core reset documentation to describe tested guarded reset/kill behavior.

### Changed
- Document the hand-coded project origins, cppyy selection for templates and
  runtime features, and subsequent use of AI-assisted vibe coding.
- Keep existing M6A requirements stable and publish explicit untested/failing statuses;
  11,642 partial mappings remain unresolved binding verification work.

## [0.10.0] - 2026-10-18

Adds custom quantumkeeper hooks, stricter callback contracts and declaration audit
metadata. M6A remains incomplete; full API, type, reset and teardown audits are open.

### Added
- Custom quantumkeeper checks cover reset/kill cleanup, hook exceptions and invalid
  results after guarded waits, and remaining-global-quantum boundary timing.
- Custom quantumkeeper factory dispatches the protected local-quantum hook to a
  Python callback with copied native time, result validation, exception recovery,
  guarded synchronization and callback ownership checks against native timing.
- Declaration inventories discover all TLM utility headers, attribute active macro
  definitions to source locations and preserve specialization template arguments.
- Public API inventory supports baseline/modeling conditional configurations and
  retains inheritance and overload parameter/default metadata.
- Portable declaration IDs and validated review input link source declarations to
  classification, Python spelling, ownership, GIL, errors and semantic evidence.
  Full public declaration classification and semantic coverage remain incomplete.

### Fixed
- Structural TLM callbacks validate forward/backward synchronization enums and
  unsigned debug counts before conversion; failed nonblocking get/peek accepts
  an absent value while preserving caller output.
- Refresh changelog comparison links through 0.9.0 and keep adjacent bullets together.
- Convenience socket callbacks validate void, DMI bool, unsigned debug count and
  native nonblocking synchronization enum results before cppyy conversion, retaining
  callback ownership and borrowed argument behavior across all factory families.
- Structural FIFO available/free counts enforce signed native integer bounds.
- Structural bool and signed/unsigned integer signal reads validate exact result
  types and native-width bounds, with read/trace accessor recovery checks.
- Structural event queue notify/cancel, TLM blocking transport/DMI invalidation and
  FIFO debug callbacks require None, with overload, argument and recovery checks.
- Structural signal inout/write-only, blocking FIFO write, TLM analysis write and
  blocking put callbacks reject non-None results instead of silently discarding them.
  Recovery checks preserve copied native arguments; callback mutations are not rolled back.
- Custom quantumkeepers reuse one native dispatcher while retaining independent
  callback owners; native hook dispatch rejects null result pointers.
- Event queue overload regression uses structural callbacks and borrowed event
  forwarding, preserving duplicate-delta notification and cancellation checks.
- Inventory friend qualification preserves the enclosing namespace; specialization
  IDs distinguish template arguments. Review batches reject malformed optional fields
  and validate completely before applying updates.

## [0.9.0] - 2026-10-17

Expands structural synchronization interfaces and callback ownership/validation.
M6A remains incomplete; full API, reset/error and teardown audits are open.

### Fixed
- TLM FIFO debug count callbacks reject invalid integer conversions while
  preserving native signed bounds and negative sentinel values.
- TLM analysis, put and debug-poke callbacks receive independent native value
  storage that can be retained after the call; debug indices remain intact.
- Write-only signal and native FIFO output callbacks receive Python-owned native
  copies that remain valid when retained after the native call returns.
- Structural signal inout write callbacks receive independent value copies,
  preventing retained or mutated callback arguments from aliasing caller storage.
- Structural signal inout interfaces forward an optional owner writer-policy
  callback, preserving the native default when absent and validating explicit policies.
- Accept equivalent Clang namespace qualification in the source inventory test.
- Upload generated coverage reports when tests fail while retaining CI failure.

### Added
- TLM FIFO expansion/unbound callbacks require None; reduction, bound and debug
  poke callbacks require bool, with native defaults and recovery checks.
- Structural TLM nonblocking put, readiness and DMI callbacks require bool
  results, with malformed-result rejection and recovery checks.
- Structural callback validation covers mutex/semaphore integer bounds, signal
  event flags, writer policies and live native event returns, with recovery checks.
- Structural mutex, semaphore and event queue interfaces, including blocking GIL
  release, notify overloads, borrowed event identity and cancellation.
- Structural signal-write interfaces with copied arbitrary-width values and
  owner writer-policy callbacks; native reference checks cover timing and values.

## [0.8.0] - 2026-10-16

Adds structural native reference, signal and FIFO adapters. Full public API
coverage, reset/error matrices and adapter teardown remain incomplete.

### Added
- Comprehensive Features checklist covering implemented capabilities, open binding
  requirements and upstream example audits.
- Structural scalar signal interfaces use stable native read/trace storage for
  fundamental C++ types, with signed, floating and 64-bit conversion checks.
- Native FIFO callback validation rejects non-bool write results and malformed
  read pairs; error checks preserve outputs and verify recovery after exceptions.
- Separate blocking/nonblocking native FIFO input/output structural adapters.
  Owners need only their selected interface callbacks; wide read overloads,
  copied writes, borrowed events and failed-output preservation are retained.
- Structural native signal input/inout and FIFO input/output adapters with ordinary
  Python callbacks, borrowed event identity, wide signal reference storage,
  bool/logic edge events, and automatic GIL release for blocking FIFO calls.
- FIFO checks preserve failed nonblocking outputs and compare 137-bit blocking
  transfers at 2 ns and 3 ns against an independent native model.
- Structural custom interfaces now preserve native lvalue reference results
  through pointer trampolines, retaining returned storage and shared event
  identity. Checks cover mutable/const 137-bit values, replacement, invalid
  results, owner retention and event timing with an independent native reference.
- Reproducible M6A source declaration inventory covering the checked-out core,
  datatype, TLM and TLM utility headers, with overloads, template parameters,
  access levels and portable source locations. Entries remain unreviewed until
  matched to semantic evidence; this does not claim complete binding support.
- Header-based audit regression checks for blocking-get and extension overloads,
  protected wait/quantum hooks, specialization retention and enum spelling.

## [0.7.0] - 2026-10-15

This release adds callable socket families and expands native semantic validation.
M6A remains incomplete: the full public declaration audit, general custom-channel
reference returns, adapter teardown and remaining type/lifetime matrices are open.

### Added
- Tagged simple target and initiator sockets with independent callback IDs.
- Multi target and initiator sockets with connection-indexed callbacks, direct
  multi-to-multi binding and automatic GIL release for indexed blocking calls.
- Ordinary and tagged passthrough target sockets accepting Python callbacks.
- Socket checks for hierarchical binding, custom 137-bit payload identity,
  optional policies, registration errors, debug bounds and reset/kill/error cleanup.
- Independent native references for simple and passthrough custom transactions.
- Instance-specific extension isolation, replacement, carrier release, deep-copy
  omission and clone/copy/free checks, with an independent native reference.
- Endian conversion checks for generic, word, aligned and single-word families,
  including unaligned transfers, streaming, byte enables and explicit response APIs.
  Native comparisons cover single-word addresses, the word type/offset matrix and
  generic unaligned streaming buffers.
- Public guides for socket callbacks, private extension lifecycles and endian
  conversion preconditions, ownership and verification limits.

### Fixed
- Preserve original multi and passthrough callbacks after native duplicate
  registration warnings, including native tagged passthrough ID updates.
- Check custom PEQ high-bit values through native bit-proxy `to_bool()`.
- Separate documented C++ functions with blank lines and expand short bodies.
- Refresh the README PyPI badge cache identity for the release.

## [0.6.0] - 2026-10-15

This release expands TLM bindings. M6A remains incomplete; full public API,
custom-type and lifecycle verification continues.

### Added
- Convenience initiator sockets register retained backward transport and DMI
  invalidation Python callbacks with native phase/delay and 64-bit range semantics.
- Convenience target sockets register nonblocking transport, debug transport and
  DMI Python callbacks, preserving native references and unsigned debug bounds.
- Native convenience target sockets accept retained blocking Python callbacks
  through a member-pointer bridge with arbitrary width and protocol parameters.
- Callback payload event queues accept retained Python callables with native
  protocol types, phase copying, delta/timed scheduling and cancellation.
- Independent native reference checks compare quantumkeeper and payload queue
  delivery order, simulation times, cancellation and local-time behavior.
- Native `tlm_utils` namespace exposes convenience socket and utility templates;
  quantumkeeper waits release the GIL and payload queues retain native scheduling semantics.
- Structural TLM-2 combined and standalone transport, debug and DMI interfaces
  accept ordinary Python callbacks with native method names.
- Structural TLM-1 blocking, nonblocking and combined master/slave interfaces
  preserve independent request/response template types and native output overloads.
- Structural FIFO debug and size-configuration interfaces preserve indexed access,
  failed output values and native default arguments.
- Structural FIFO put/get interfaces preserve ordinary and indexed peek overloads,
  shared queue state and blocking process suspension.

### Fixed
- Indexed convenience initiator socket access configures blocking GIL release
  while preserving native interface identity and forward/backward callbacks.
- Convenience initiator forward-interface access configures blocking GIL release;
  socket callback reset/kill preserves Python cleanup and native unwind.
- Callback payload queues convert primitive protocol phases by value while
  preserving native class phase references and callback exception propagation.
- Quantumkeeper synchronization preserves native reset/kill unwind and Python
  cleanup across utility JIT transactions.
- Restore the native default simulation context for Python `sc_is_running()` calls.
- Restore CI formatting and strict documentation builds after structural interface changes.
- Compare installed-wheel versions with distribution metadata rather than a stale release literal.
- Verify repaired release wheels on a separate Ubuntu runner before publication, away from the manylinux compiler environment.

## [0.5.0] - 2026-10-15

### Added
- Structural TLM-2 blocking transport supports ordinary b_transport callbacks,
  borrowed payload/delay mutation and automatic GIL release for process waits.
- Structural TLM regression verifies 73-bit native value copies across blocking
  and nonblocking put/get/peek calls, including preserved failure outputs.
- Native-style port/export call syntax adapts structural Python implementations
  through the same path as bind, including annotated Protocol exports.
- Blocking-only TLM get-peek interfaces accept structural Python implementations
  with automatic GIL handling and native value/output overload forwarding.
- Full TLM get-peek interface supports shared blocking/nonblocking Python state
  and dispatch through native blocking and nonblocking base-interface ports.
- Combined TLM peek interface forwards blocking and nonblocking callbacks while
  preserving nonconsuming value/output overloads and native readiness storage.
- Combined TLM put interface forwards blocking and nonblocking operations to one
  Python implementation with copied values and stable native readiness storage.
- Combined TLM get interface supports shared Python blocking/nonblocking callbacks,
  native output overloads and stable readiness events.
- Structural nonblocking TLM put/get/peek and combined get-peek bindings use
  familiar callback names and Python success/value tuples with native event storage.
- Structural blocking-put binding copies native const-reference values into Python
  put callbacks and automatically releases the GIL for blocking port calls.
- Structural Python implementations bind through native SystemC ports using
  ordinary get, peek, transport and write methods, with retained adapters and
  automatic GIL release for supported blocking calls.
- Annotated Protocol interfaces infer native void, bool, double and cppyy value
  signatures; explicit declarations remain available for reference signatures.
- A Protocol-based sc_export example alongside the retained native implementation,
  with both variants checked against the upstream reference.
- Standalone interface composition experiments preserved outside the library.
- TLM-1 combined nonblocking-get/peek adapter preserves output references and
  supports native combined/base ports with separate channel-owned readiness events.
- TLM-1 nonblocking-peek adapter with stable readiness events, preserved failure
  outputs and repeated nonconsuming reference reads.
- TLM-1 nonblocking-get adapter preserves output references on failure and owns
  stable readiness events for native-port callbacks.
- TLM-1 nonblocking-put adapter with Python acceptance/readiness callbacks and
  channel-owned readiness-event references for native ports.
- TLM-1 blocking-get adapter preserves native value and reference-output
  overloads, including waits inside Python callbacks.
- TLM-1 transport and blocking-peek adapters preserve both value-return and
  reference-output overloads through Python value callbacks.
- Python-subclassable interface and primitive-channel bases with public native
  constructors and update/lifecycle hooks; preserve the native namespace types.
- Upstream asynchronous host-event port using the packaged primitive-channel
  base, without example-local C++ declarations.
- Specialized-signals Python port with all 13 native concatenation records and
  26 mixed-arithmetic checks; explicit template adapters resolve cppyy overload gaps.
- Python ports of the upstream 121-bit delay pipeline and FIFO mutex-policy
  examples, with native comparisons of values, acquisition order and times.
- TLM-1 analysis value-copy channel adapter and blocking put/get dispatch checks.
  Analysis callbacks currently use write_value to avoid cppyy reference conversion.
- Combined TLM forward/backward channel adapters with phase/delay mutation,
  checked debug return ranges, DMI access and invalidation regressions.
- Native initiator/target socket integration and Python memory-manager recycling
  checks. Debug callbacks currently use debug_transport due to cppyy limitations.
- Package custom interface/channel factories and Python-subclassable primitive,
  signal and FIFO channel bases with native-port dispatch checks.
- Callback-sampled signal storage with deferred updates and bool/logic edges.
- Python native-interface checks for mutex, semaphore and event-queue methods.
- Optional `make native-sanitize` ASan/UBSan checks for first-party native
  reference models; the SystemC dependency library is not instrumented.
- Contribution and troubleshooting guidance covering native library ownership,
  blocking-call GIL requirements, installed validation and dependency licenses.
- Native CI caching keyed by compiler, architecture, SystemC revision and build
  configuration, including the C++17/pthreads ABI settings.

### Changed
- Improve namespace declaration ordering and spacing in binding helpers.
- Break up long conditions in core interface adapters for readability.
- Split core interface declarations, structural binding and waveform helpers
  into related modules; simplify initializer imports and document setup order.
- Use ordinary Python methods in TLM transport, peek and blocking-get models.
- Match sc_export example interface/channel class names to upstream C_if and C,
  retaining its native implementation and recording the original source path.
- Record upstream source paths and copyright attribution in converted examples;
  keep license text in dependency notices rather than copying it into ports.
- Organize subprocess regressions into tutorial, upstream, runtime, lifecycle,
  modeling and runner modules with shared simulation helpers.
- Publishing validates installed console arguments, exit status and tutorial
  behavior before uploading distributions.
- Place `native-sanitize` in the Makefile's Build and validation help section;
  describe the startup, console and tutorial checks performed by `test-wheel`.

## [0.4.0] - 2026-10-15

### Fixed
- Store the bundled SystemC shared library once in wheels, omitting duplicate
  versioned aliases that wheel installers cannot portably restore as symlinks.
- Expose native writer-policy enum template arguments and resolved signal reads
  consistently through cppyy.
- Reject missing VCD output directories before native delayed file creation.
- Preserve native reset/kill unwind across Python thread waits and self-control,
  including finally cleanup; support OR/AND event-expression waits.
- Keep native sc_module reflection intact while exposing the Python callback
  adapter locally; reject non-callable process registrations before native entry.
- Reject Python entry statuses outside the native integer range, decode native
  arguments with Python filesystem semantics, and retain spawn callbacks only
  after successful registration.
- Preserve entry cleanup for SystemExit and KeyboardInterrupt, and release
  entry references and GIL state through scoped native ownership.

### Added
- Packaged `pysystemc-cpp` console runner with source/include handling, native
  arguments and exit status, compilation diagnostics and installed tutorial checks.
- Queued/resolved channels, FIFO ports, writer contention, vector/attribute,
  scoped VCD/WIF, dynamic datatype and native reference regressions.
- Controlled runtime dependency/header repair diagnostics and override-preservation
  checks without modifying shared dependency caches.
- Verified API coverage tables, native modeling/VCD comparisons, datatype mutation
  and typed-port checks, binding diagnostics and trace/hierarchy regressions.
- Native thread/reset reference checks, repeated reset/kill wait-family stress,
  self-control and entry pause/resume lifecycle validation.
- Module identity, callback registration and lifecycle isolation checks across
  multiple hosts with garbage collection before simulation.
- Static method sensitivity and entry callable reference cleanup regressions.
- Native reference comparison for method sensitivity, callback lifetime and
  failed-registration cleanup checks, and all four lifecycle exception checks.
- `make test-wheel` checks a local release wheel outside the checkout, verifying
  the installed version, native startup, callback execution and simulation time.

## [0.3.0] - 2026-10-14

### Added
- Native fixed-point templates and policy enums, TLM namespace types, grouped
  help and enum inventories without restricting native template parameters.
- Context-managed VCD cleanup, hierarchical export port, and finite event and
  mixed C++/Python examples with semantic comparisons.
- Semantic test CI with Python 3.14, native dependency setup, Ruff checks,
  XML/HTML coverage artifacts and Codecov uploads; add test and coverage badges.
- Ruff development tooling and modern annotations for callback/trace adapters.
- Original parameterized adder and eight-bit counter examples, shown inline in
  documentation with tutorial references and behavioral assertions.
- Architecture/baseline, supported-behavior and future-roadmap pages covering
  SCV, CRAVE, FC4SC, UVM-SystemC and RTL co-simulation references.
- Tests for missing entry callables, dynamic spawn options, static cycle waits,
  invalid callbacks and idempotent cleanup.

### Changed
- Centralize native discovery/loading in a private runtime module and package
  the licensed SystemC entry adapter to remove startup's external source dependency.
- Format first-party Python with Ruff and retain native object lifetimes in examples.
- Produce Codecov-compatible XML alongside local terminal and HTML coverage reports.
- Support CMake 4 when configuring the pinned SystemC 2.3.4 developer build.

### Fixed
- Avoid cppyy empty-vector iteration and native range shadowing in hierarchy inspection.

## [0.2.3] - 2026-10-14

### Fixed
- Simplify the documentation landing page to show the project introduction once,
  keeping the contents in sidebar navigation and the version in the page title.


### Added
- Publish to production PyPI on release-tag pushes after checking the tag matches
  the package version; retain manual TestPyPI and PyPI publishing.
- HTML coverage report under `build/coverage/html` alongside the terminal summary
  when running `make coverage`.

## [0.2.2] - 2026-10-14

### Fixed
- Install documentation dependencies into the Read the Docs virtual environment
  so hosted Sphinx builds can import MyST and the theme.
- Replace the README badge table's empty header with Category and Status labels
  and refresh the production PyPI version badge.

### Added
- Makefile checks for installed TestPyPI and PyPI releases with selectable package
  versions, isolated from checkout native dependency paths. Checks refresh package
  metadata and run main, verifying callback execution and simulation time.

## [0.2.1] - 2026-10-14

### Fixed
- Discover CPyCppyy API headers in the active environment before importing cppyy,
  preventing API-discovery warnings with uv cached installations.
- Exclude ZIP directory entries from wheel RECORD so auditwheel can inspect and
  repair bundled native libraries.

### Changed
- Update GitHub Actions to Node.js 24 releases for publishing and documentation.

## [0.2.0] - 2026-10-14

### Added
- Read the Docs workflow with documentation checks and hosted build requests for
  main, version tags, and manual runs.
- A circuit-chip logo with a compact Python emblem, SystemC™ name, logic gates,
  clock waveform, PNG variants and favicon.
- README status badges for publishing, documentation, package metadata and standards.

### Changed
- Build fmt, spdlog and pthreads SystemC during distribution builds and bundle
  their native resources in platform wheels, staging builds outside the source tree.
- Validate installed wheels with runtime C++ compilation and pthread waits; build
  publishing wheels in manylinux 2.28 and repair them with auditwheel.
- Name the distribution `pysystemc`, retaining the `amal.eda.systemc` namespace;
  update version lookup, PyPI badges, dependency lock and package artifacts.
- Keep the README concise and move detailed development notes into a Sphinx page.
- Keep personal publishing instructions in ignored local records rather than public docs.

## [0.1.0] - 2026-10-14

### Added
- spdlog 1.15.3 with external fmt for colored C++ runner information notes.
- Manual PyPI/TestPyPI trusted publishing workflow and Read the Docs configuration.
- Separate documentation dependency group for native-free hosted documentation builds.
- Aggregate `make setup`, configurable native build paths, verbose tests, Python
  subprocess coverage, and Sphinx documentation with the Read the Docs theme.
- Header-only fmt dependency and optional colored native/Python diagnostics.
- Python diagnostics use a dedicated Loguru sink with Rich terminal rendering.
- Package `__version__` and a startup banner before SystemC, suppressible with
  `PYSC_COPYRIGHT_MESSAGE=DISABLE`.
- Dynamic Python process spawning, joins, wait adapters and report handlers.
- Python ports of upstream fork/join, barrier and report rules with C++ comparison tests.
- Arbitrary-width integer/vector and wide-signal regression checks.
- Upstream Python example hierarchy mirrors SystemC sysc/tlm directories.
- Shared depot guidance and documented example support.
- Loguru and Rich runtime dependencies; Sphinx, theme and pytest-cov development tools.

### Changed
- Require Python 3.14 and use uv for dependency management and distribution builds.
- Pin both native build paths to SystemC 2.3.4 with pthreads and a shared install prefix.
- Discover checkout native libraries automatically; run FIFO tests without location skips.
- Organize documentation into introduction, release notes, references and API pages.

### Fixed
- C++ runner enters native kernel startup, including its banner, and returns the simulation exit status.
- Explicit `PYSYSTEMC_COLOR=always` enables colors even when `NO_COLOR` is set.
- CPyCppyy header discovery and compatibility alias; suppress its case-path diagnostic.
- Native process hosts, callback dispatch and bound Python callable lifetime.
- Named VCD tracing through native signal storage and four-bit counter overflow.
- Finite native FIFO example, blocking-call GIL release and entry-point C API cleanup.
- Status queries, hierarchy traversal and generated hierarchical port policy resolution.

### Known limitations
- Python reset configuration is rejected: cppyy consumes native SystemC unwind
  exceptions. The upstream reset port remains a failing draft.
- Broad feature/example coverage and TLM support remain incomplete.

[Unreleased]: https://github.com/amal-khailtash/pysystemc/compare/v0.13.1...HEAD
[0.13.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.12.0...v0.13.0
[0.12.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.11.0...v0.12.0
[0.11.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.10.0...v0.11.0
[0.10.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.9.0...v0.10.0
[0.9.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.8.0...v0.9.0
[0.8.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.7.0...v0.8.0
[0.7.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.6.0...v0.7.0
[0.6.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.5.0...v0.6.0
[0.5.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.4.0...v0.5.0
[0.4.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.3...v0.3.0
[0.2.3]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.2...v0.2.3
[0.2.2]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/amal-khailtash/pysystemc/releases/tag/v0.1.0

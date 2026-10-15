# Changelog

Notable changes follow [Keep a Changelog](https://keepachangelog.com/) and
[Semantic Versioning](https://semver.org/). The project version is maintained in
`pyproject.toml`. Unreleased changes have not been published as a new release.

## [Unreleased]

### Added

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

[Unreleased]: https://github.com/amal-khailtash/pysystemc/compare/v0.5.0...HEAD
[0.5.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.4.0...v0.5.0
[0.4.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.3...v0.3.0
[0.2.3]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.2...v0.2.3
[0.2.2]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/amal-khailtash/pysystemc/releases/tag/v0.1.0

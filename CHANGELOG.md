# Changelog

Notable changes follow [Keep a Changelog](https://keepachangelog.com/) and
[Semantic Versioning](https://semver.org/). The project version is maintained in
`pyproject.toml`. Unreleased changes have not been published as a new release.

## [Unreleased]

### Added

- Native fixed-point templates and policy enums, with quantization, saturation,
  wrap and arbitrary-width construction checks.
- A TLM namespace exposing payloads, protocol enums, interfaces and socket
  templates; tests cover payload metadata, not transport callback support.
- Grouped namespace help and enum value inventories, plus missing native class
  and template descriptions that preserve cppyy overload documentation.
- Context-managed VCD files with cleanup on Python exceptions and tests that
  inspect flushed waveform transitions.
- A hierarchical sc_export Python port compared with native C++ at 10, 17 and
  20 ns, using explicit get_interface() bindings.
- Small event-cancellation and mixed C++/Python gate examples with assertions
  for callback execution, truth-table values and simulation times.

### Changed

- Improve native namespace exports and introspection without restricting template
  parameters; keep Python print() available in wildcard imports.
- Centralize C++17 selection in the package initializer and remove the redundant
  submodule setting.
- Expand Python and Doxygen documentation, move Development after Examples,
  and apply the depot clang-format configuration with a 120-column limit.
- Build release documentation after successful production PyPI publication;
  retain main-branch and manual documentation builds.
- Give hosted PyPI badges a separate cache URL for each documentation version.

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

[Unreleased]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.3...HEAD
[0.2.3]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.2...v0.2.3
[0.2.2]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.1...v0.2.2
[0.2.1]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/amal-khailtash/pysystemc/releases/tag/v0.1.0

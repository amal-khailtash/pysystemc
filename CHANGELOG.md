# Changelog

Notable changes follow [Keep a Changelog](https://keepachangelog.com/) and
[Semantic Versioning](https://semver.org/). The project version is maintained in
`pyproject.toml`. Unreleased changes have not been published as a new release.

## [Unreleased]

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

[Unreleased]: https://github.com/amal-khailtash/pysystemc/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/amal-khailtash/pysystemc/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/amal-khailtash/pysystemc/releases/tag/v0.1.0

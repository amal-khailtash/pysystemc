# Changelog

Notable changes follow [Keep a Changelog](https://keepachangelog.com/) and
[Semantic Versioning](https://semver.org/). The project version is maintained in
`pyproject.toml`. Unreleased changes have not been published as a new release.

## [Unreleased]

### Added

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

- Explicit `PYSYSTEMC_COLOR=always` enables colors even when `NO_COLOR` is set.

- CPyCppyy header discovery and compatibility alias; suppress its case-path diagnostic.
- Native process hosts, callback dispatch and bound Python callable lifetime.
- Named VCD tracing through native signal storage and four-bit counter overflow.
- Finite native FIFO example, blocking-call GIL release and entry-point C API cleanup.
- Status queries, hierarchy traversal and generated hierarchical port policy resolution.

### Known limitations

- Python reset configuration is rejected: cppyy consumes native SystemC unwind
  exceptions. The upstream reset port remains a failing draft.
- The entry bridge still reads upstream kernel source; independent installed-artifact
  validation, broad feature/example coverage, TLM and CI remain release gates.

## Initial prototype

The existing project metadata identifies the prototype as version 0.1.0. Its
cppyy bindings and hello/counter/FIFO examples predate this changelog. No release
date or publication status is asserted here.

[Unreleased]: https://github.com/amal-khailtash/pysystemc/commits/main/

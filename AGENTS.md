# Depot development guidance

## Purpose and public API

This project binds native SystemC to Python through cppyy. Keep Python modeling
code close to native SystemC: parameterized templates, process registration,
sensitivity, ports, events, waits, and `sc_trace` should preserve native semantics.
Do not restrict template widths or types to a precompiled list.

## Environment and build

- Use uv for dependency management, locking, environment synchronization, and
  distribution builds. Use the native preparation backend with `uv_build` for packaging; do not introduce pip or Hatch.
- Honor existing `UV_*` environment variables. Do not overwrite interpreter,
  cache, index, or installation settings without user authorization.
- Python 3.14 is the current baseline. If the selected environment disagrees,
  report the mismatch; do not silently replace it.
- Keep committed files portable. Never add workstation-specific absolute paths.
- Preserve existing user changes. Do not edit vendored SystemC implementation
  merely to work around a binding issue; isolate adapters in first-party code.

## Implementation and documentation

- Format first-party C++ with the depot’s `.clang-format` configuration.
- Document C++ bridge classes and functions using Doxygen comments. Describe
  ownership, callback lifetime, exceptions, and GIL/thread requirements.
- Give Python modules and test modules docstrings, and document test functions and helpers.
- Give public Python functions/classes proper docstrings, including parameters,
  return values, errors, and lifecycle constraints where relevant.
- Prefer small native template adapters when cppyy overload resolution silently
  selects an incorrect overload. Preserve the familiar Python-facing signature.
- Retain Python callbacks for their native process lifetime. Release acquired GIL
  state and Python references on every C API exit path.
- Keep local AI settings/session state ignored. `AGENTS.md` is shared guidance and
  must remain trackable.

## Validation

- Run independent simulations in subprocesses with timeouts: the native kernel
  maintains singleton state. Do not reset internal kernel globals between tests.
- Assert callback execution, sensitivity, values, simulation times, and lifecycle
  ordering. A zero exit code alone does not prove an example works.
- Inspect VCD declarations and transitions; an empty trace can look successful.
- Verify Python/C++ equivalence using native examples as reference models.
- Both native build paths default to pthreads and share an automatically discovered
  installation prefix. Do not skip thread tests based on location variables.
- Python 3.14 coroutine-stack compatibility is a feasibility gate. Do not claim
  thread/wait/reset support based only on method-process tests.
- Run targeted tests after changes. Report failures and unsupported features
  clearly; do not hide them behind silent skips.

## Planning

- Delegate simple execution and validation tasks to `gpt-6-luna` sub-agents
  when an agent slot is available. Give each worker a bounded specification,
  explicit file ownership, commands and acceptance criteria. Keep detailed
  execution logs and routine debugging in the worker context; return concise
  results with evidence, failures and remaining limitations.
- The main agent orchestrates independent, non-overlapping groups and owns
  integration and updates to the plan, feature status, coverage and changelog.
  Evaluate worker results through the main validation harness before accepting
  them; worker completion alone does not establish correctness or feature
  completion. Queue tasks when the session's concurrency limit is reached.

Record supported, failing, untested, and deferred features distinctly. Port all
upstream examples incrementally, preserving meaningful native reference checks.

Place upstream Python ports under examples/sysc/ or examples/tlm/, preserving
the upstream example subdirectory hierarchy and source-file stem where practical.

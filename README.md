# pysystemc - SystemC python binding using cppyy

<p align="center">
  <img src="https://raw.githubusercontent.com/amal-khailtash/pysystemc/main/docs/_static/logo-256x256.svg" alt="pysystemc — Python and SystemC with logic gates in a simulation chip" width="256"/>
</p>

| Category | Status |
| --- | --- |
| CI/CD | [![01. PR title](https://github.com/amal-khailtash/pysystemc/actions/workflows/pr-title.yml/badge.svg)](https://github.com/amal-khailtash/pysystemc/actions/workflows/pr-title.yml) [![02. CI](https://github.com/amal-khailtash/pysystemc/actions/workflows/ci.yml/badge.svg)](https://github.com/amal-khailtash/pysystemc/actions/workflows/ci.yml) [![03. Release Please](https://github.com/amal-khailtash/pysystemc/actions/workflows/release-please.yml/badge.svg)](https://github.com/amal-khailtash/pysystemc/actions/workflows/release-please.yml) [![04. Publish](https://github.com/amal-khailtash/pysystemc/actions/workflows/publish.yml/badge.svg)](https://github.com/amal-khailtash/pysystemc/actions/workflows/publish.yml) |
| Project | [![GitHub](https://img.shields.io/badge/pysystemc-29b6f6?logo=github&labelColor=0277bd)](https://github.com/amal-khailtash/pysystemc) [![PyPI](https://img.shields.io/pypi/v/pysystemc?logo=pypi&logoColor=FBE072&cacheSeconds=300&release=0.12.0)](https://pypi.org/project/pysystemc/) [![GitHub tag](https://img.shields.io/github/v/tag/amal-khailtash/pysystemc?logo=github&label=version)](https://github.com/amal-khailtash/pysystemc/tags) |
| Docs | [![Read the Docs](https://readthedocs.org/projects/pysystemc/badge/?version=latest)](https://pysystemc.readthedocs.io/en/latest/) |
| Meta | [![License](https://img.shields.io/github/license/amal-khailtash/pysystemc)](https://github.com/amal-khailtash/pysystemc/blob/main/LICENSE) [![Stars](https://img.shields.io/github/stars/amal-khailtash/pysystemc)](https://github.com/amal-khailtash/pysystemc/stargazers) [![Forks](https://img.shields.io/github/forks/amal-khailtash/pysystemc)](https://github.com/amal-khailtash/pysystemc/forks) [![Contributors](https://img.shields.io/github/contributors/amal-khailtash/pysystemc)](https://github.com/amal-khailtash/pysystemc/graphs/contributors) |
| Standards | [![Python](https://img.shields.io/badge/python-3.14-blue?logo=python)](https://www.python.org/) [![SemVer](https://img.shields.io/badge/SemVer-2.0.0-333333?logo=semver)](https://semver.org/) [![Keep a Changelog](https://img.shields.io/badge/Keep%20a%20Changelog-1.1.0-0c7fb0?logo=keepachangelog&logoColor=white)](https://keepachangelog.com/en/1.1.0/) [![Release Please v4](https://img.shields.io/badge/Release%20Please-v4-blue)](https://github.com/googleapis/release-please-action/tree/v4) |

Python bindings for [SystemC](https://systemc.org/overview/systemc/) using
[cppyy](https://github.com/wlav/cppyy). Write Python testbenches and modules while
using the native C++ simulation kernel and on-demand template instantiation.
The PyPI distribution is `pysystemc`; the Python import namespace is
`amal.eda.systemc`.

## Project origins

The original idea and initial implementation were hand coded. Development later
also began using vibe coding with AI assistance to extend the implementation,
tests, and documentation.

## Why Python and cppyy?

SystemC provides a C++ simulation kernel, typed channels, and a rich modeling
library. Python adds a convenient way to assemble models and experiment with
them. You can change stimulus, testbench logic, logging, waveform selection, and
model connections in Python, then rerun the simulation without rebuilding the
existing C++ model libraries.

cppyy was chosen for its support for C++ templates and runtime features. It makes
native C++ classes and templates accessible from Python and allows C++ bridge
code to be compiled within the running Python environment. For example,
`sc_uint[17]`, `sc_biguint[137]`, and `sc_signal[sc_uint[17]]` can be requested
when a script runs, rather than requiring a binding library with a predefined
list of types and widths. Its Cling interpreter compiles the necessary template
specializations and bridge code on demand. This still involves compilation, but
it happens within the running Python environment instead of a separate C++
build-and-link cycle.

Python modules can register methods and threads with the native SystemC kernel,
while existing C++ models continue to execute as C++. This lets a testbench load
configuration files, generate parameterized stimulus, inspect results, and use
Python analysis tools around the same simulation. SystemC still controls time,
delta cycles, and process scheduling; Python callbacks add overhead when the
simulation crosses the language boundary.

This flexibility is especially useful during exploration and verification:
change a test scenario or a traced signal in a script and rerun it. Changes to
the implementation of a compiled C++ model still require rebuilding that model.
SystemC's elaboration rules also still apply: most structural connections are
made before simulation starts. Dynamic processes can be spawned during a run,
but arbitrary structural changes remain constrained by native elaboration rules.
Python thread reset and kill preserve scheduler unwind through the guarded wait
adapter; see the supported behavior documentation for lifecycle constraints.


## Quickstart

Load Python 3.14 and install [uv](https://docs.astral.sh/uv/), then run:

```bash
make setup
make example-hello-py
make example-counter-py
make test
```

Setup installs dependencies and builds SystemC 2.3.4 with pthreads. Existing
`UV_*` environment settings are honored. To run the C++ hello example, use
`make example-hello-cpp`.

Use `make test-parallel` or `make coverage-parallel` to run tests in two isolated
pytest workers. Override the worker count with `TEST_WORKERS=4`, and select tests
with `ARGS="tests/test_cli.py tests/test_repl.py"`. Each simulation still runs in
a fresh subprocess; coverage combines data from workers and their child processes.

Detailed setup, validation, and logging instructions are in the
[documentation](https://pysystemc.readthedocs.io/en/latest/).

## Examples

Use `uv run pysystemc --version` for the installed version and
`uv run pysystemc --help` for command help. Start an interactive Python
playground with `uv run pysystemc repl`; it preloads `sc_core`, `sc_dt`, TLM,
cppyy and the public kernel names. The prompt supports syntax highlighting,
tab completion, multiline definitions, line editing and persistent history.
Use `pysystemc help repl` for history options.
The Makefile provides `make cli`, `make cli ARGS=version` and `make repl`;
both targets forward `ARGS` for additional command options.

`uv run pysystemc-cpp model.cpp -- --cycles 20` compiles native C++ sources
and forwards arguments after `--` to `sc_main`. The compatibility script
`examples/run_sysc_cpp.py` uses the same packaged runner.

There are a few C++ and Python examples in `examples/` directory.

Some examples adapted from:
* [Learn SystemC with Examples](https://www.learnsystemc.com/)
* [Brief SystemC getting started tutorial](https://github.com/AleksandarKostovic/SystemC-tutorial/)


For build options, header repair, logging controls, and validation, see the
[development guide](https://pysystemc.readthedocs.io/en/latest/intro/development.html).

## References

* [Accellera](https://accellera.org/)
  * [SystemC](https://github.com/accellera-official/systemc)
  * [SystemC Verification (SCV)](https://systemc.org/overview/systemc-verification/)
  * [UVM-SystemC](https://github.com/accellera-official/uvm-systemc)
  * [CRAVE constrained randomization](https://github.com/accellera-official/crave)
  * [FC4SC functional coverage](https://github.com/accellera-official/fc4sc)
  * [PySysC](https://github.com/accellera-official/PySysC) - A Python package to make SystemC usable from Python using conan/cppyy
    * [Python and SystemC](https://workspace.accellera.org/document/dl/10968)
* [systemc_python](https://github.com/YouHui1/systemc_python) - use systemc through cppyy
* [pysc](https://github.com/ethanrobbins/pysc) - Python bindings for SystemC using pure Python API
* [pysc](https://github.com/socrocket/pysc) - Python SystemC implementation of USI (Universal Scripting Interface) using swig
  * [Transparent SystemC Model Factory for
Scripting Languages](https://dvcon-proceedings.org/wp-content/uploads/transparent-systemc-model-factory-for-scripting-languages.pdf)
* [bsmedit](https://github.com/tianzhuqiao/bsmedit) - C/C++/SystemC Visualizer [docs](http://bsmedit.feiyilin.com/)
* [gsysc](https://github.com/werneazc/gsysc) - Graphical environment for SystemC
* Python binding generators
  * [swig](https://www.swig.org/Doc1.3/Python.html) - build Python extension
  * [cppyy](https://github.com/wlav/cppyy) - Python-C++ bindings interface based on Cling/LLVM
  * [pybind11](https://pybind11.readthedocs.io/en/stable)
  * [CFFI](https://cffi.readthedocs.io/en/stable/)
  * [Boost.Python](https://www.boost.org/doc/libs/1_86_0/libs/python/doc/html/index.html)
  * [Shiboken](https://doc.qt.io/qtforpython-6/shiboken6/index.html)

The CLI also runs as `python -m amal.eda.cli`; help and version commands do not load the native kernel.

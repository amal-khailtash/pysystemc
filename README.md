# pysystemc - SystemC python binding using cppyy

First attempt at taking the advantage of Python dymanic feature to bind and use [SystemC](https://systemc.org/overview/systemc/) library in an equivalent Pythonic way.

A few other implementations try to mix and match the C++ features and implement them in Python with various difficulties and success.
[cppyy](https://github.com/wlav/cppyy) provides a clean interface to expose C++ classes, functions, templates.


# Development

To get examples working, run the following commands to install uv, create virtual environment,
fetch and compile SystemC 2.3.4 (compiled with C++17 for now).
The project requires Python 3.14 or newer. Load your Python environment before
running these commands; set `UV_PYTHON` if you need to select a specific interpreter.
Existing `UV_*` environment settings are honored. Dependencies are managed with
uv, and distribution artifacts are built with `uv build` (or `make build`).

```bash
make setup
```

Install uv first if needed (`make uv`). `make setup` synchronizes Python 3.14
dependencies, repairs CPyCppyy headers, fetches fmt, and builds pinned SystemC
2.3.4 with pthreads through CMake. All `UV_*` settings are honored.
Individual setup targets remain available, including the Autotools alternative
`make setup-systemc`.

Run validation with `make test`, detailed results with `make test-verbose`, and
Python coverage with `make coverage`. Coverage includes simulation subprocesses;
native C++ execution is not included in the percentage.

Run a few examples:

```bash
# hello example C++/Python version
$ make example-hello-cpp
$ make example-hello-py

# counter example C++/Python version
$ make example-counter-cpp
$ make example-counter-py
```


# Examples

A convient script `examples/run_sysc_cpp.py` is provided that can run any SystemC C++ code.  It reads the source code provided on the command line and calls `sc_main()`, basically running the SystemC simulation.

There are a few C++ and Python examples in `examples/` directory.

Some examples adapted from:
* [Learn SystemC with Examples](https://www.learnsystemc.com/)
* [Brief SystemC getting started tutorial](https://github.com/AleksandarKostovic/SystemC-tutorial/)


# References

* [Accellera](https://accellera.org/)
  * [SystemC](https://github.com/accellera-official/systemc)
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

## Binding repair and validation

`make fix-cppyy` repairs distributions that install `cpycppyy` headers while
cppyy expects `CPyCppyy/API.h`. Package initialization also discovers the active
Python headers and repairs this alias when the environment is writable.
`make test` checks Python method callbacks, lifecycle hooks, counter sensitivity,
four-bit overflow, and VCD declarations/transitions in separate processes.

Both `make setup-systemc` (Autotools) and `make setup-systemc-cmake` use
pthreads by default and install into `build/deps/systemc-install`. Python 3.14
requires native pthread stacks for the tested Python thread processes.
The bindings and C++ runner discover the installed library automatically:

```bash
make setup-systemc-cmake  # or make setup-systemc
make example-simple_fifo-py
make test
```

All three regression tests run without library-location variables. Advanced
installations may override `SYSTEMC_INSTALL_DIR`; `SYSTEMC_LIBRARY_DIR` remains
available for explicit library selection. The bridge is compiled by cppyy at
runtime and loads `libsystemc.so`; it is not yet a prelinked Python extension.
The current entry bridge still reads kernel entry source from the submodule.

See the documentation examples page for verified behavior and current limitations.

## fmt and binding diagnostics

Run `make setup-fmt` before Python examples to fetch fmt 11.2.0 headers, or set
`FMT_HOME` to an existing fmt installation prefix. The native JIT bridge uses fmt
in header-only mode. A system installation visible to the compiler is also
supported. fmt is distributed under its upstream MIT license; the download
includes its license file. Installed Python packages require accessible fmt
headers too.

Binding debug messages are opt-in and go to stderr:

```bash
PYSYSTEMC_LOG_LEVEL=DEBUG make example-hello-py
PYSYSTEMC_LOG_LEVEL=DEBUG PYSYSTEMC_COLOR=always make example-counter-py
```

`PYSYSTEMC_COLOR=auto` is the default and enables colors only on a terminal.
Set `PYSYSTEMC_COLOR=never` to disable them. `NO_COLOR` disables automatic
colors; explicitly setting `PYSYSTEMC_COLOR=always` takes precedence. Errors remain visible
without debug mode. SystemC reports and example output retain their native form.


Additional upstream ports: `make example-forkjoin-py` exercises dynamic spawning,
joins and waits; `make example-report-py` exercises native report rules with a
Python handler. Tests compare both against their original C++ examples.
`make docs` builds Sphinx/Read the Docs HTML into `build/docs`.
The reset example is a draft: Python reset configuration raises
`NotImplementedError` until native unwind handling is safe across cppyy callbacks.


The binding exposes `amal.eda.systemc.__version__` from installed package metadata
and prints its version before the SystemC kernel banner. Set
`PYSC_COPYRIGHT_MESSAGE=DISABLE` to suppress the binding banner. Debug messages
require `PYSYSTEMC_LOG_LEVEL=DEBUG`; to force colored diagnostics even when your
environment sets `NO_COLOR`, run:

```bash
PYSYSTEMC_LOG_LEVEL=DEBUG PYSYSTEMC_COLOR=always make example-hello-py
```


PyPI/TestPyPI publishing and Read the Docs setup are described in
[the publishing guide](https://pysystemc.readthedocs.io/en/latest/intro/publishing.html). Publication is manual; the
workflow defaults to TestPyPI. Native SystemC and fmt are external dependencies.

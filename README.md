# pysystemc - SystemC python binding using cppyy

First attempt at taking the advantage of Python dymanic feature to bind and use [SystemC](https://systemc.org/overview/systemc/) library in an equivalent Pythonic way.

A few other implementations try to mix and match the C++ features and implement them in Python with various difficulties and success.
[cppyy](https://github.com/wlav/cppyy) provides a clean interface to expose C++ classes, functions, templates.


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

"""Compile kernel bridges before importing adapters that reflect their types.

The parent package prepares SystemC and cppyy. Hash lifetime Pythonizations
must be registered before bridge compilation reflects hash specializations.
"""

from pathlib import Path

import cppyy

from ._hashes import install as _install_hashes

_install_hashes()
_directory = Path(__file__).parent
for _bridge in (
    "sc_main.cpp",
    "native_entry.cpp",
    "sc_module.cpp",
    "interface_teardown.cpp",
    "hash_helpers.cpp",
    "report_legacy.cpp",
    "report_handler.cpp",
    "event_lists.cpp",
    "event_finders.cpp",
    "trace_printf.cpp",
    "trace_values.cpp",
    "signal_values.cpp",
    "time_operations.cpp",
    "byte_heap.cpp",
    "process_utilities.cpp",
    "event_expressions.cpp",
    "pvector_access.cpp",
):
    with (_directory / _bridge).open() as _source:
        cppyy.cppdef(_source.read())

cppyy.include(str(_directory / "vector_creator.cpp"))
cppyy.include(str(_directory / "vector_policies.cpp"))

from .._logging import log
import atexit
from enum import Enum
import pathlib
import os
import sys

cppv = 17
# cppv = 14
# os.environ["CLING_STANDARD_PCH"] = os.path.expanduser(os.path.join("~", f"cling_standard_native_{cppv}.pch"))
os.environ.setdefault("EXTRA_CLING_ARGS", f"-std=c++{cppv}")
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -D_LIBCPP_LFTS_STRING_VIEW"
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -DRWrap_libcpp_string_view_h"
# os.environ["EXTRA_CLING_ARGS"] = f"-O2 -march=native -std=c++{cppv} -DR__HAS_STD_EXPERIMENTAL_STRING_VIEW"

# os.environ["CLING_REBUILD_PCH"] = "1"

import cppyy

# from .. import SYSTEMC_HOME
from .. import SYSTEMC_HOME


SCRIPT_PATH = pathlib.Path(__file__).parent.absolute()


with open(SCRIPT_PATH / "sc_main.cpp", "r") as fd:
    cppyy.cppdef(fd.read())

# Reread the sc_main_main.cpp file to link out new sc_main to sc_elan_and_sim function
with open(f"{SYSTEMC_HOME}/src/sysc/kernel/sc_main_main.cpp", "r") as fd:
    cppyy.cppdef(fd.read())

# Store the sc_core.sc_module and its metaclass
# sc_module           = cppyy.gbl.sc_core.sc_module
# sc_module_metaclass = type(cppyy.gbl.sc_core.sc_module)

with open(SCRIPT_PATH / "sc_module_new.cpp", "r") as fd:
    cppyy.cppdef(fd.read())




# print(f"type(sc_core.sc_module): {type(cppyy.gbl.sc_core.sc_module)}")

# # ScModule           = cppyy.gbl.sc_core.ScModule
# # ScModule.__class__ = sc_module_metaclass

# # Replace sc_core.sc_module with the newly defined ScModule
# # Replace the C++ sc_module with the C++ wrapper ScModule

# # cppyy.gbl.sc_core.sc_module = ScModule

# cppyy.gbl.sc_core.ScModule.__release_gil__ = True
# cppyy.gbl.sc_core.ScModule.call_process.__release_gil__ = True
cppyy.gbl.sc_core.sc_start.__release_gil__ = True
cppyy.gbl.sc_core.sc_elab_and_sim.__release_gil__ = True
cppyy.gbl.sc_main.__release_gil__ = True

# cppyy's std::function conversion does not retain bound Python callables.
# Keep them alive on their owning module for the native process lifetime.
def _retain_process_callback(register):
    """Wrap registration to retain bound callables on their owning module."""
    def wrapper(self, name, callback, *args):
        if not hasattr(self, "_process_callbacks"):
            self._process_callbacks = []
        self._process_callbacks.append(callback)
        return register(self, name, callback, *args)
    return wrapper

for _name in ("sc_method", "sc_thread", "sc_cthread"):
    setattr(cppyy.gbl.sc_core.ScModule, _name,
            _retain_process_callback(getattr(cppyy.gbl.sc_core.ScModule, _name)))

# Replace with custom sc_module
cppyy.gbl.sc_core.sc_module = cppyy.gbl.sc_core.ScModule
# # cppyy.gbl.sc_core.sc_module.__class__ = sc_module_metaclass
# # globals()["sc_module"] = sc_core.sc_module

# print(f"type(sc_core.sc_module): {type(cppyy.gbl.sc_core.sc_module)}")




# Access the C++ sc_core namespace
sc_core_namespace = cppyy.gbl.sc_core

def add_sc_core_members():
    module_globals = globals()
    for name in dir(sc_core_namespace):
        if name == "print":
            continue
        try:
            member = getattr(sc_core_namespace, name)
        except AttributeError:
            continue
        module_globals[name] = member

# Add all sc_core members to this module
add_sc_core_members()


# Manually add each public member of the `sc_core` namespace to the module"s globals
for name, member in sc_core_namespace.__dict__.items():
    # Filter out internal or irrelevant attributes
    if not name.startswith("__"):
        globals()[name] = member


# List of classes, enums, and functions to expose from `sc_core`
sc_core_classes = ["sc_module", "sc_module_name", "sc_clock", "sc_event", "sc_signal", "sc_in", "sc_in", "sc_out", "sc_in_clk", "sc_inout_clk", "sc_out_clk"]
sc_core_functions = ["sc_elab_and_sim"]  # Add other functions if needed


# Dynamically add classes and functions from the C++ `sc_core` namespace to this module
for name in sc_core_classes + sc_core_functions:
    try:
        globals()[name] = getattr(sc_core_namespace, name)
    except AttributeError:
        print(f"Warning: {name} not found in sc_core namespace.")





def __getattr__(name):
    mod = getattr(cppyy.gbl.sc_core, name)
    return mod


# Define a custom __dir__ for listing available attributes
def __dir__():
    return list(globals().keys())


def sc_elab_and_sim():
    log(f"sc_elab_and_sim({sys.argv})")

    argc = len(sys.argv)
    argv = sys.argv

    log(f"Entering native simulation with {argc} arguments")


    return cppyy.gbl.sc_core.sc_elab_and_sim(argc, argv)


def sc_stop():
    if cppyy.gbl.sc_core.sc_get_status() != cppyy.gbl.sc_core.SC_STOPPED:
        log("Stopping simulation")
        cppyy.gbl.sc_core.sc_stop()


# Register the cleanup function to be called at exit
atexit.register(sc_stop)


class ScStatus(Enum):
    SC_UNITIALIZED               = cppyy.gbl.sc_core.SC_UNITIALIZED
    SC_ELABORATION               = cppyy.gbl.sc_core.SC_ELABORATION
    SC_BEFORE_END_OF_ELABORATION = cppyy.gbl.sc_core.SC_BEFORE_END_OF_ELABORATION
    SC_END_OF_ELABORATION        = cppyy.gbl.sc_core.SC_END_OF_ELABORATION
    SC_START_OF_SIMULATION       = cppyy.gbl.sc_core.SC_START_OF_SIMULATION
    SC_RUNNING                   = cppyy.gbl.sc_core.SC_RUNNING
    SC_PAUSED                    = cppyy.gbl.sc_core.SC_PAUSED
    SC_STOPPED                   = cppyy.gbl.sc_core.SC_STOPPED
    SC_END_OF_SIMULATION         = cppyy.gbl.sc_core.SC_END_OF_SIMULATION
    SC_END_OF_INITIALIZATION     = cppyy.gbl.sc_core.SC_END_OF_INITIALIZATION
  # SC_END_OF_EVALUATION         = cppyy.gbl.sc_core.SC_END_OF_EVALUATION
    SC_END_OF_UPDATE             = cppyy.gbl.sc_core.SC_END_OF_UPDATE
    SC_BEFORE_TIMESTEP           = cppyy.gbl.sc_core.SC_BEFORE_TIMESTEP
    SC_STATUS_LAST               = cppyy.gbl.sc_core.SC_STATUS_LAST
    SC_STATUS_ANY                = cppyy.gbl.sc_core.SC_STATUS_ANY

    @classmethod
    def from_cpp_enum(cls, cpp_enum_value):
        """Convert a SystemC sc_status int value to the corresponding ScStatus enum."""
        return ScStatus(cpp_enum_value)
        # try:
        #     return ScStatus(cpp_enum_value)
        # except ValueError:
        #     return ScStatus.SC_INVALID_STATUS  # Fallback if the value is invalid

    def __str__(self):
        """Return the string representation of the ScStatus enum."""
        return self.name


def sc_status():
    status = cppyy.gbl.sc_core.sc_get_status()
    return ScStatus(status)  # Convert to Python enum


_native_sc_trace = cppyy.gbl.sc_core.sc_trace

def sc_trace(file, value, name, *args):
    """Register a named waveform using stable native signal storage.

    Args:
        file: Open SystemC trace file.
        value: Native signal/interface or scalar accepted by SystemC.
        name: Name to record in the waveform.
        *args: Additional native scalar tracing arguments.

    Signal template types are instantiated on demand. Keep the traced object
    alive until the trace file closes; Python scalar temporaries are unsuitable
    for tracking mutable native state.
    """
    if hasattr(value, "read"):
        sample = value.read()
        value_type = bool if isinstance(sample, bool) else type(sample)
        return cppyy.gbl.sc_core.trace_signal[value_type](file, value, name)
    return _native_sc_trace(file, value, name, *args)


# Native waits suspend an OS thread. Release the GIL for every overload so
# other Python SystemC processes can acquire it while this process waits.
cppyy.gbl.sc_core.wait.__release_gil__ = True
cppyy.gbl.sc_core.sc_join.wait.__release_gil__ = True
_spawn_callbacks = []

def sc_spawn(callback, name="", options=None):
    """Spawn a Python callable as a native dynamic SystemC process.

    Args:
        callback: No-argument callable. Return values are ignored; use a Python
            closure to retain results when joining processes.
        name: Optional native process name.
        options: Optional native sc_spawn_options object.

    Returns:
        Native sc_process_handle with termination event and process controls.

    Callbacks remain retained for this interpreter's single simulation lifetime.
    Blocking native calls within callbacks must release the GIL.
    """
    if not callable(callback):
        raise TypeError("sc_spawn callback must be callable")
    _spawn_callbacks.append(callback)
    if options is None:
        return cppyy.gbl.sc_core.spawn_callback(callback, name)
    return cppyy.gbl.sc_core.spawn_callback(callback, name, options)


_report_callback = None

def set_report_handler(callback=None):
    """Install a retained Python report handler, or restore the native default.

    The callback receives a native sc_report and an integer action bitmask.
    This adapter avoids cppyy's const-reference scalar callback conversion.
    """
    global _report_callback
    if callback is None:
        cppyy.gbl.sc_core.restore_report_handler()
        _report_callback = None
    else:
        if not callable(callback):
            raise TypeError("report handler must be callable")
        _report_callback = callback
        cppyy.gbl.sc_core.set_python_report_handler(callback)


_native_wait = cppyy.gbl.sc_core.wait
cppyy.gbl.sc_core.wait_cycle.__release_gil__ = True

def wait(*args):
    """Suspend the current native thread, preserving SystemC wait overloads.

    A zero-argument call selects the cycle wait explicitly. Timed, event and
    event-list calls retain native overload resolution and release the GIL.
    Native reset/unwind behavior remains experimental for Python callbacks.
    """
    if not args:
        return cppyy.gbl.sc_core.wait_cycle()
    return _native_wait(*args)


def _unsupported_reset(self, *args):
    """Reject native reset until cppyy preserves SystemC unwind exceptions."""
    raise NotImplementedError(
        "Python process reset is not supported: cppyy consumes the native "
        "sc_unwind_exception; native reset/unwind support is pending."
    )

# Native unwind must cross a Python callback during process reset. Failing early
# avoids SystemC's fatal F539 abort when cppyy translates that control exception.
cppyy.gbl.sc_core.ScModule.reset_signal_is = _unsupported_reset
cppyy.gbl.sc_core.ScModule.async_reset_signal_is = _unsupported_reset

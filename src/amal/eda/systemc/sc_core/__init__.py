"""Native kernel, channels, processes and Python callback adapters.

Callbacks are retained for their native process lifetime. Blocking scheduler
operations release the GIL. Guarded Python waits preserve native reset/kill
unwind in tested cases; direct native waits and arbitrary throw_it require
further compatibility validation.
"""

import atexit
from collections.abc import Callable
from typing import Any
from enum import Enum
import pathlib
import sys

# The parent package selects the JIT C++ standard before importing cppyy.

import cppyy

from .._logging import log
from .._namespace import (
    public_names as _public_names,
    document_member as _document_member,
    namespace_help as _namespace_help,
)
from ._channel_interfaces import channel_interface as channel_interface
from ._channel_interfaces import declare_interface as declare_interface
from ._interfaces import PortTemplate as _PortTemplate
from ._interfaces import interface as interface
from ._interfaces import release_interface_callbacks as release_interface_callbacks
from ._tracing import sc_trace as _python_sc_trace
from ._tracing import vcd_trace as _python_vcd_trace

# Compile first-party bridges before publishing native aliases or configuring
# their methods. The parent package has already loaded SystemC and selected C++17.
SCRIPT_PATH = pathlib.Path(__file__).parent.absolute()
for _bridge in ("sc_main.cpp", "native_entry.cpp", "sc_module_new.cpp"):
    with (SCRIPT_PATH / _bridge).open() as _source:
        cppyy.cppdef(_source.read())

cppyy.gbl.sc_core.sc_start.__release_gil__ = True
cppyy.gbl.sc_core.sc_elab_and_sim.__release_gil__ = True
cppyy.gbl.sc_main.__release_gil__ = True


# cppyy's std::function conversion does not retain bound Python callables.
# Keep them alive on their owning module for the native process lifetime.
def _retain_process_callback(register):
    """Wrap registration to retain bound callables on their owning module."""

    def wrapper(self, name, callback, *args):
        """Retain callable ownership after successful native registration."""
        if not callable(callback):
            raise TypeError("process callback must be callable")
        if not hasattr(self, "_process_callbacks"):
            self._process_callbacks = []
        result = register(self, name, callback, *args)
        self._process_callbacks.append(callback)
        return result

    return wrapper


for _name in ("sc_method", "sc_thread", "sc_cthread"):
    setattr(cppyy.gbl.sc_core.ScModule, _name, _retain_process_callback(getattr(cppyy.gbl.sc_core.ScModule, _name)))

# Access the C++ sc_core namespace
sc_core_namespace = cppyy.gbl.sc_core


def add_sc_core_members():
    """Populate resolvable native names while preserving Python print()."""
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


# Publish Python helpers after namespace copying so native overloads cannot
# overwrite the adapters. Native C++ sc_module remains intact.
sc_trace = _python_sc_trace
vcd_trace = _python_vcd_trace
sc_module = cppyy.gbl.sc_core.ScModule
sc_channel = sc_module
sc_interface = cppyy.gbl.sc_core.PythonInterface
sc_prim_channel = cppyy.gbl.sc_core.PythonPrimChannel
signal_channel = cppyy.gbl.sc_core.PythonSignalChannel
fifo_channel = cppyy.gbl.sc_core.PythonFifoChannel
# Keep the public port specialization native; only Python bind gains adaptation.
sc_port = _PortTemplate(cppyy.gbl.sc_core.sc_port, channel_interface)
sc_export = _PortTemplate(cppyy.gbl.sc_core.sc_export, channel_interface)
_read_interface_callbacks = []


def sc_is_running(context=None) -> bool:
    """Return whether the native simulation is running in the selected context.

    Args:
        context: Borrowed native simulation context; defaults to the current context.

    Returns:
        Native running state. The context must remain alive during this call.
    """
    if context is None:
        context = cppyy.gbl.sc_core.sc_get_curr_simcontext()
    return cppyy.gbl.sc_core.sc_is_running(context)


def read_interface(value_type: Any, name: str, callback: Callable[[], Any]) -> Any:
    """Create a native signal read interface backed by a Python value callback.

    Args:
        value_type: Native scalar/datatype used as the interface template argument.
        name: Channel name within the current module construction scope.
        callback: Nonblocking callable returning a value convertible to value_type.

    Returns:
        Native channel with stable read/event references and notify_changed().

    Raises:
        TypeError: Callback is not callable or native value conversion fails.

    Callback and channel must survive simulation. Callback retention lasts through
    interpreter shutdown. notify_changed samples the callback and schedules a native signal update; reads
    never invoke Python. Call it from the simulation thread after changing state.
    Native bool/logic edge interfaces use the same stable signal storage.
    """
    if not callable(callback):
        raise TypeError("read interface callback must be callable")
    channel = cppyy.gbl.sc_core.PythonReadInterface[value_type](name, callback)
    _read_interface_callbacks.append(callback)
    return channel


def _read_resolved(self: Any) -> Any:
    """Return a copy of native resolved logic after the channel update phase."""
    return cppyy.gbl.sc_core.python_read_resolved(self)


if not hasattr(cppyy.gbl.sc_core.sc_signal_resolved, "read"):
    cppyy.gbl.sc_core.sc_signal_resolved.read = _read_resolved


def _time_modulo(self: Any, other: Any) -> Any:
    """Return native time remainder; reject a zero divisor with ValueError."""
    if other == cppyy.gbl.sc_core.SC_ZERO_TIME:
        raise ValueError("time modulo divisor must be nonzero")
    return cppyy.gbl.sc_core.python_time_modulo(self, other)


cppyy.gbl.sc_core.sc_time.__mod__ = _time_modulo


def __getattr__(name):
    """Resolve a native kernel name or raise AttributeError."""
    mod = _document_member(getattr(cppyy.gbl.sc_core, name))
    return mod


# Define a custom __dir__ for listing available attributes
def __dir__():
    """List Python adapters and resolvable native kernel API names."""
    return sorted(set(globals()) | set(_public_names(cppyy.gbl.sc_core)))


def sc_elab_and_sim() -> int:
    """Enter native sc_main with sys.argv and return its integer exit status.

    This releases the GIL while native code runs and reacquires it for Python
    callbacks. Invoke once per independent simulation process.
    """
    log(f"sc_elab_and_sim({sys.argv})")

    argc = len(sys.argv)
    argv = sys.argv

    log(f"Entering native simulation with {argc} arguments")

    return cppyy.gbl.sc_core.sc_elab_and_sim(argc, argv)


def sc_stop() -> None:
    """Request kernel stop unless already stopped; also used at interpreter exit."""
    if cppyy.gbl.sc_core.sc_get_status() != cppyy.gbl.sc_core.SC_STOPPED:
        log("Stopping simulation")
        cppyy.gbl.sc_core.sc_stop()


# Register the cleanup function to be called at exit
atexit.register(sc_stop)


class ScStatus(Enum):
    """Python names for native simulation lifecycle status values."""

    SC_UNITIALIZED = cppyy.gbl.sc_core.SC_UNITIALIZED
    SC_ELABORATION = cppyy.gbl.sc_core.SC_ELABORATION
    SC_BEFORE_END_OF_ELABORATION = cppyy.gbl.sc_core.SC_BEFORE_END_OF_ELABORATION
    SC_END_OF_ELABORATION = cppyy.gbl.sc_core.SC_END_OF_ELABORATION
    SC_START_OF_SIMULATION = cppyy.gbl.sc_core.SC_START_OF_SIMULATION
    SC_RUNNING = cppyy.gbl.sc_core.SC_RUNNING
    SC_PAUSED = cppyy.gbl.sc_core.SC_PAUSED
    SC_STOPPED = cppyy.gbl.sc_core.SC_STOPPED
    SC_END_OF_SIMULATION = cppyy.gbl.sc_core.SC_END_OF_SIMULATION
    SC_END_OF_INITIALIZATION = cppyy.gbl.sc_core.SC_END_OF_INITIALIZATION
    # SC_END_OF_EVALUATION         = cppyy.gbl.sc_core.SC_END_OF_EVALUATION
    SC_END_OF_UPDATE = cppyy.gbl.sc_core.SC_END_OF_UPDATE
    SC_BEFORE_TIMESTEP = cppyy.gbl.sc_core.SC_BEFORE_TIMESTEP
    SC_STATUS_LAST = cppyy.gbl.sc_core.SC_STATUS_LAST
    SC_STATUS_ANY = cppyy.gbl.sc_core.SC_STATUS_ANY

    @classmethod
    def from_cpp_enum(cls, cpp_enum_value):
        """Convert a SystemC sc_status int value to the corresponding ScStatus enum."""
        return ScStatus(cpp_enum_value)

    def __str__(self):
        """Return the string representation of the ScStatus enum."""
        return self.name


def sc_status() -> ScStatus:
    """Return the current kernel lifecycle state as ScStatus."""
    status = cppyy.gbl.sc_core.sc_get_status()
    return ScStatus(status)  # Convert to Python enum


# Native waits suspend an OS thread. Release the GIL for every overload so
# other Python SystemC processes can acquire it while this process waits.
cppyy.gbl.sc_core.wait.__release_gil__ = True
cppyy.gbl.sc_core.sc_join.wait.__release_gil__ = True
_spawn_callbacks = []


def sc_spawn(callback: Callable[[], Any], name: str = "", options: Any = None) -> Any:
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
    if options is None:
        handle = cppyy.gbl.sc_core.spawn_callback(callback, name)
    else:
        handle = cppyy.gbl.sc_core.spawn_callback(callback, name, options)
    _spawn_callbacks.append(callback)
    return handle


_report_callback = None


def set_report_handler(callback: Callable[[Any, int], None] | None = None) -> None:
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


_native_next_trigger = cppyy.gbl.sc_core.next_trigger


def next_trigger(*args: Any) -> Any:
    """Set dynamic method sensitivity using native arguments or event expressions.

    Args:
        *args: Native next_trigger time/event arguments. Expressions are consumed
            once; referenced events must outlive the scheduled invocation.
    """
    if len(args) == 1 and any(
        isinstance(args[0], cppyy.gbl.sc_core.sc_event_expr[list_type])
        for list_type in (cppyy.gbl.sc_core.sc_event_or_list, cppyy.gbl.sc_core.sc_event_and_list)
    ):
        return cppyy.gbl.sc_core.python_next_trigger(args[0])
    return _native_next_trigger(*args)


def _process_control(adapter):
    """Bind an explicit native control adapter with GIL released."""
    adapter.__release_gil__ = True

    def control(self, *args):
        """Control a target from a running SystemC process; paused calls raise RuntimeError."""
        if cppyy.gbl.sc_core.sc_get_status() != cppyy.gbl.sc_core.SC_RUNNING:
            raise RuntimeError("process reset/kill requires a running SystemC process")
        if not adapter(self, *args):
            raise _ProcessUnwind()

    return control


cppyy.gbl.sc_core.sc_process_handle.kill = _process_control(cppyy.gbl.sc_core.python_kill)
cppyy.gbl.sc_core.sc_process_handle.reset = _process_control(cppyy.gbl.sc_core.python_reset)


cppyy.gbl.sc_core.python_wait.__release_gil__ = True


class _ProcessUnwind(BaseException):
    """Private signal that unwinds Python frames before native reset or kill."""


def wait(*args: Any) -> None:
    """Suspend a native thread with cycle, time, event or event-list sensitivity.

    Args:
        *args: Native wait arguments, including event OR/AND expressions.

    Reset and kill unwind Python frames, execute finally blocks, and then restore
    the original native scheduler exception. Do not suppress BaseException or
    wait during unwind cleanup. Events and sensitivity targets must outlive waits.
    Blocking scheduler calls release the GIL.
    """
    if not cppyy.gbl.sc_core.python_wait(*args):
        raise _ProcessUnwind()


# Publish native classes/enums and Python adapters for wildcard imports.

__all__ = sorted(
    set(_public_names(cppyy.gbl.sc_core))
    | {
        "ScStatus",
        "sc_status",
        "sc_trace",
        "sc_spawn",
        "wait",
        "set_report_handler",
        "sc_elab_and_sim",
        "sc_stop",
        "vcd_trace",
        "interface",
        "release_interface_callbacks",
        "declare_interface",
        "channel_interface",
        "read_interface",
        "signal_channel",
        "fifo_channel",
    }
)

__doc__ = _namespace_help(globals(), cppyy.gbl.sc_core)
